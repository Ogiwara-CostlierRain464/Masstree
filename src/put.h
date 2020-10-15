#ifndef MASSTREE_PUT_H
#define MASSTREE_PUT_H

#include "tree.h"
#include "alloc.h"
#include "gc.h"
#include "debug_helper.h"
#include <algorithm>

namespace masstree{

#ifndef NDEBUG
extern Sleeper put_mark_unstable;
#endif

enum PutResult : uint8_t{
  Done,
  Retry
};

/**
 * Masstreeが空の状態の時、新しいMasstreeを作る。
 * @param key
 * @param value
 * @return
 */
static BorderNode *start_new_tree(const Key &key, Value *value){
  auto root = new BorderNode{};
#ifndef NDEBUG
  Alloc::incBorder();
#endif
  root->setIsRoot(true);

  auto cursor = key.getCurrentSlice();
  if(1 <= cursor.size and cursor.size <= 7){
    root->setKeyLen(0, cursor.size);
    root->setKeySlice(0, cursor.slice);
    root->setLV(0, LinkOrValue(value));
  }else{
    assert(cursor.size == 8);
    if(key.hasNext()){
      // next layerは作らない
      root->setKeySlice(0, cursor.slice);
      root->setKeyLen(0, BorderNode::key_len_has_suffix);
      root->setLV(0, LinkOrValue(value));
      root->getKeySuffixes().set(0, key, 1);
    }else{
      root->setKeyLen(0, 8);
      root->setKeySlice(0, cursor.slice);
      root->setLV(0, LinkOrValue(value));
    }
  }

  return root;
}

/**
 * borderNodeにkeyを挿入した場合、invariantに違反するようになるか確認する。
 * @param borderNode
 * @param key
 * @return 違反の原因となる、現在borderNodeに挿入されているkey sliceのインデックス、もしくはnull。
 */
static std::optional<size_t> check_break_invariant(BorderNode *borderNode, const Key &key) {
  assert(borderNode->getLocked());
  auto p = borderNode->getPermutation();
  if (key.hasNext()) {
    auto cursor = key.getCurrentSlice();
    for (size_t i = 0; i < p.getNumKeys(); ++i) {
      auto true_index = p(i);
      if ((borderNode->getKeyLen(true_index) == BorderNode::key_len_has_suffix
           || borderNode->getKeyLen(true_index) == BorderNode::key_len_layer)
          && borderNode->getKeySlice(true_index) == cursor.slice
        ) {
        return true_index;
      }
    }
  }
  return std::nullopt;
}

/**
 * invariantを違反した場合の処理。§4.6.3を参考。
 * @param border
 * @param key
 * @param value
 * @param old_index
 */
static void handle_break_invariant(BorderNode *n, Key &key, Value *value, size_t old_index, GC &gc){
  assert(n->getLocked());
  if(n->getKeyLen(old_index) == BorderNode::key_len_has_suffix){
    /**
    * """
    * Masstree creates a new layer when inserting a key k1 into
    * a border node that contains a conflicting key k2. It allocates
    * a new empty border node n′, inserts k2’s current value into
    * it under the appropriate key slice, and then replaces k2’s
    * value in n with the next_layer pointer n . Finally, it unlocks
    * n and continues the attempt to insert k1, now using the newly
    * created layer n'.
    * """
    *
    * @see §4.6.3
    */
    auto n1 = new BorderNode{};
#ifndef NDEBUG
    Alloc::incBorder();
#endif
    n1->setIsRoot(true);
    auto k2_val = n->getLV(old_index).value;
    auto k2_suffix_copy = new BigSuffix(*n->getKeySuffixes().get(old_index));
#ifndef NDEBUG
    Alloc::incSuffix();
#endif
    if(k2_suffix_copy->hasNext()){
      n1->setKeyLen(0, BorderNode::key_len_has_suffix);
      n1->setKeySlice(0, k2_suffix_copy->getCurrentSlice().slice);
      k2_suffix_copy->next();
      n1->getKeySuffixes().set(0, k2_suffix_copy);
      n1->setLV(0, LinkOrValue(k2_val));
    }else{
      n1->setKeyLen(0, k2_suffix_copy->getCurrentSlice().size);
      n1->setKeySlice(0, k2_suffix_copy->getCurrentSlice().slice);
      n1->setLV(0, LinkOrValue(k2_val));
      delete k2_suffix_copy;
#ifndef NDEBUG
      Alloc::decSuffix();
#endif
    }

    /**
     * まず、UNSTABLEとマークする
     * 次に、lvを書き換える
     * そして、NEXT_LAYERに書き換える
     * そして、KeySuffixをGCに追加し、このBorderNodeからリンクを外す
     * 最後に、このNodeをunlockする
     */
    n->setKeyLen(old_index, BorderNode::key_len_unstable);
    n->setLV(old_index, LinkOrValue(n1));
#ifndef NDEBUG
    put_mark_unstable.sleepIfUsed();
#endif
    n->setKeyLen(old_index, BorderNode::key_len_layer);
    gc.add(n->getKeySuffixes().get(old_index));
    n->getKeySuffixes().unref(old_index);
  }else{
    assert(n->getKeyLen(old_index) == BorderNode::key_len_layer);
  }
  n->unlock();
}


/**
 * Corresponds to insert_into_leaf in B+tree.
 * 前提: borderに空きがある。
 * @param border
 * @param key
 * @param value
 */
static void insert_into_border(BorderNode *border, const Key &key, Value *value, GC &gc){
  assert(border->getLocked());
  assert(!border->getSplitting());
  assert(!border->getInserting());
  auto p = border->getPermutation();
  assert(p.isNotFull());

  size_t insertion_point_ps = 0;
  size_t num_keys = p.getNumKeys();
  auto cursor = key.getCurrentSlice();
  while (insertion_point_ps < num_keys
         && border->getKeySlice(p(insertion_point_ps)) < cursor.slice){
    ++insertion_point_ps;
  }

  auto pair = border->insertPoint();
  auto insertion_point_ts = pair.first;
  auto reuse = pair.second;

  if(reuse){
    // key len = 0のslotと違い、ここには古いValueとSuffixが残る。
    // これをどのタイミングでdeleteするのか、どのタイミングで参照を外すのか、また何で初期化するかが非常にキモになる。
    // 下手に0で初期化しても、それは「全てが0のバイナリ列」と区別がつかないので、新しい値と古い値の間にそういった状態を発生させるべきではない。
    // writer-writer conflictはlockで対処、readerはpermutationしか見ないので問題ないのでは？
    border->setInserting(true);

    auto suffix = border->getKeySuffixes().get(insertion_point_ts);
    if(suffix != nullptr){
      gc.add(suffix);
    }
    gc.add(border->getLV(insertion_point_ts).value);
  }

  // クリアしておく。ここでクリアしないと、Suffixを上書きし損ねる
  border->getKeySuffixes().set(insertion_point_ts, nullptr);

  if(1 <= cursor.size and cursor.size <= 7){
    border->setKeyLen(insertion_point_ts, cursor.size);
    border->setKeySlice(insertion_point_ts, cursor.slice);
    border->setLV(insertion_point_ts, LinkOrValue(value));
  }else{
    assert(cursor.size == 8);
    if(key.hasNext()){
      border->setKeySlice(insertion_point_ts, cursor.slice);
      border->setKeyLen(insertion_point_ts, BorderNode::key_len_has_suffix);
      border->getKeySuffixes().set(insertion_point_ts, key, key.cursor + 1);
      border->setLV(insertion_point_ts, LinkOrValue(value));
    }else{
      border->setKeyLen(insertion_point_ts, 8);
      border->setKeySlice(insertion_point_ts, cursor.slice);
      border->setLV(insertion_point_ts, LinkOrValue(value));
    }
  }

  p.insert(insertion_point_ps, insertion_point_ts);
  border->setPermutation(p);
}



/**
 * Finds the appropriate place to
 * split a node that is too big into two.
 *
 * BorderNodeでは使えない事に注意！
 * @return
 */
static size_t cut(size_t len){
  if(len % 2 == 0)
    return len/2;
  else
    return len/2+1;
}

/**
 * Corresponds to insert_into_node_after_splitting
 * @param p
 * @param p1
 * @param slice
 * @param n1
 * @param n_index
 * @param[out] k_prime parentにinsertされるkey sliceを返す。通常、これはtemp_key_sliceのうちp1の最初のkey sliceの前のものである。
 */
static void split_keys_among(InteriorNode *p, InteriorNode *p1, KeySlice slice, Node *n1, size_t n_index, std::optional<KeySlice> &k_prime){
  assert(!p->isNotFull());
  assert(!p->getIsRoot());
  assert(!p1->getIsRoot());

  uint64_t temp_key_slice[Node::ORDER] = {};
  Node* temp_child[Node::ORDER + 1] = {};

  for(size_t i = 0, j = 0; i < p->getNumKeys() + 1; ++i, ++j){
    if(j == n_index + 1) ++j;
    temp_child[j] = p->getChild(i);
  }
  for(size_t i = 0, j = 0; i < p->getNumKeys(); ++i, ++j){
    if(j == n_index) ++j;
    temp_key_slice[j] = p->getKeySlice(i);
  }
  temp_child[n_index + 1] = n1;
  temp_key_slice[n_index] = slice;

  // clean
  // InteriorNodeの場合はn_keysを0にするだけで十分だが
  // 予期せぬバグを防ぐためにクリーンにしておく。
  p->resetKeySlices();
  p->resetChildren();
  p->setNumKeys(0);
  size_t split = cut(Node::ORDER);
  size_t i, j;
  for(i = 0; i < split - 1; ++i){
    p->setChild(i, temp_child[i]);
    p->setKeySlice(i, temp_key_slice[i]);
    p->incNumKeys();
  }
  p->setChild(i, temp_child[i]);
  k_prime = temp_key_slice[split - 1];
  for(++i, j = 0; i < Node::ORDER; ++i, ++j){
    p1->setChild(j, temp_child[i]);
    p1->setKeySlice(j, temp_key_slice[i]);
    p1->incNumKeys();
  }
  p1->setChild(j, temp_child[i]);
  p1->setParent(p->getParent());
  for(i = 0; i <= p1->getNumKeys(); ++i){
    p1->getChild(i)->setParent(p1);
  }
}

/**
 * BorderNode中のkey sliceのマップを作る。
 * @param n
 * @param[out] table
 * @param[out] found
 */
static void create_slice_table(BorderNode *n, std::vector<std::pair<KeySlice, size_t>> &table, std::vector<KeySlice> &found){
  auto p = n->getPermutation();
  assert(p.isFull());
  for(size_t i = 0; i < Node::ORDER - 1; ++i){
    if(!std::count(found.begin(), found.end(), n->getKeySlice(i))){ // NOT FOUND
      table.emplace_back(n->getKeySlice(i), i);
      found.push_back(n->getKeySlice(i));
    }
  }
  // already sorted.
}

/**
 * BorderNodeのsplitにおいて、splitする位置を決める。
 * @param new_slice
 * @param table
 * @param found
 * @return
 */
static size_t split_point(KeySlice new_slice, const std::vector<std::pair<KeySlice, size_t>> &table, const std::vector<KeySlice> &found){
  auto min_slice = *std::min_element(found.begin(), found.end());
  auto max_slice = *std::max_element(found.begin(), found.end());
  if(new_slice < min_slice){
    return 1;
  } else if(new_slice == min_slice){
    return table[1].second + 1;
  } else if(min_slice < new_slice and new_slice < max_slice){
    if(std::count(found.begin(), found.end(), new_slice)){ // new_sliceが存在していたなら
      for(size_t i = 0; i < table.size(); ++i){
        if(table[i].first == new_slice){
          return table[i].second;
        }
      }
    }else{
      for(size_t i = 0; i < table.size(); ++i){
        if(table[i].first > new_slice){
          return table[i].second + 1;
        }
      }
    }
  } else if(new_slice == max_slice){
    for(size_t i = 0; i < table.size(); ++i){
      if(table[i].first == new_slice){
        return table[i].second;
      }
    }
  } // else
  assert(new_slice > max_slice);
  return 15;
}

/**
 * Corresponds to insert_into_leaf_after_splitting in B+ tree.
 * @param n
 * @param n1
 * @param k
 * @param value
 */
static void split_keys_among(BorderNode *n, BorderNode *n1, const Key &k, Value *value){
  auto p = n->getPermutation();
  assert(p.isFull());
  assert(n->getSplitting());

  // 簡単のため、splitされるnをソートしておく。
  n->sort();

  uint8_t temp_key_len[Node::ORDER] = {};
  uint64_t temp_key_slice[Node::ORDER] = {};
  LinkOrValue temp_lv[Node::ORDER] = {};
  BigSuffix* temp_suffix[Node::ORDER] = {};

  size_t insertion_index = 0;
  while (insertion_index < Node::ORDER - 1
         && n->getKeySlice(insertion_index) < k.getCurrentSlice().slice){
    ++insertion_index;
  }

  // 16個分、tempにコピー
  for(size_t i = 0, j = 0; i < p.getNumKeys(); ++i, ++j){
    if(j == insertion_index) ++j;
    temp_key_len[j] = n->getKeyLen(i);
    temp_key_slice[j] = n->getKeySlice(i);
    temp_lv[j] = n->getLV(i);
    temp_suffix[j] = n->getKeySuffixes().get(i);
  }

  // ここでも、insert_into_borderと同じように
  // invariantを満たすまで
  auto cursor = k.getCurrentSlice();
  if(1 <= cursor.size and cursor.size <= 7){
    temp_key_len[insertion_index] = cursor.size;
    temp_key_slice[insertion_index] = cursor.slice;
    temp_lv[insertion_index].value = value;
  }else{
    assert(cursor.size == 8);
    if(k.hasNext()){
      // invariantを満たさない場合は、splitは発生しない。
      // よって、チェックする必要がない。
      temp_key_slice[insertion_index] = cursor.slice;
      temp_key_len[insertion_index] = BorderNode::key_len_has_suffix;
      temp_suffix[insertion_index] = BigSuffix::from(k, k.cursor + 1);
      temp_lv[insertion_index].value = value;
    }else{
      temp_key_len[insertion_index] = 8;
      temp_key_slice[insertion_index] = cursor.slice;
      temp_lv[insertion_index].value = value;
    }
  }

  // ここの決定がキモ！
  std::vector<std::pair<KeySlice, size_t>> table{};
  std::vector<KeySlice> found{};
  create_slice_table(n, table, found);
  size_t split = split_point(cursor.slice, table, found);
  // clear both nodes.
  n->resetKeyLen();
  n->resetKeySlice();
  n->resetLVs();
  n->getKeySuffixes().reset();

  n1->resetKeyLen();
  n1->resetKeySlice();
  n1->resetLVs();
  n1->getKeySuffixes().reset();

  for(size_t i = 0; i < split; ++i){
    n->setKeyLen(i, temp_key_len[i]);
    n->setKeySlice(i, temp_key_slice[i]);
    n->setLV(i, temp_lv[i]);
    n->getKeySuffixes().set(i, temp_suffix[i]);
  }
  n->setPermutation(Permutation::fromSorted(split));

  for(size_t i = split, j = 0; i < Node::ORDER; ++i, ++j){
    n1->setKeyLen(j, temp_key_len[i]);
    n1->setKeySlice(j, temp_key_slice[i]);
    n1->setLV(j, temp_lv[i]);
    n1->getKeySuffixes().set(j, temp_suffix[i]);
  }
  n1->setPermutation(Permutation::fromSorted(Node::ORDER - split));

  // TODO: 繋ぎ直しを並行性制御で対応できるように　
  n1->setNext(n->getNext());
  n1->setPrev(n);
  n->setNext(n1);
  if(n1->getNext() != nullptr){
    n1->getNext()->setPrev(n1);
  }
  n1->setParent(n->getParent());
}



/**
 * Corresponds to insert_into_new_root in B+tree.
 * @param left
 * @param slice
 * @param right
 * @return
 */
static InteriorNode *create_root_with_children(Node *left, KeySlice slice, Node *right){
  assert(left->getIsRoot());
  assert(left->getParent() == nullptr);
  auto root = new InteriorNode{};
#ifndef NDEBUG
  Alloc::incInterior();
#endif
  root->setIsRoot(true);
  root->setNumKeys(1);
  root->setKeySlice(0, slice);
  root->setChild(0, left);
  root->setChild(1, right);
  // 親をセットしてから、is_rootをfalseにする
  left->setParent(root);
  right->setParent(root);
  left->setIsRoot(false);
  right->setIsRoot(false);
  return root;
}


/**
 * Corresponds to insert_into_node in B+tree.
 * @param p
 * @param n1
 * @param slice
 */
static void insert_into_parent(InteriorNode *p, Node *n1, KeySlice slice, size_t n_index){
  assert(p->isNotFull());

  // move to right
  for(size_t i = p->getNumKeys(); i > n_index; --i){
    p->setChild(i + 1, p->getChild(i));
    p->setKeySlice(i, p->getKeySlice(i - 1));
  }
  p->setChild(n_index + 1, n1);
  p->setKeySlice(n_index, slice);
  p->incNumKeys();
}

/**
 *
 * @param n
 * @param k
 * @param value
 * @return new root if not nullptr.
 */
static Node *split(Node *n, const Key &k, Value *value){
  // precondition: n locked.
  assert(n->getLocked());
  Node *n1 = new BorderNode{};
#ifndef NDEBUG
    Alloc::incBorder();
#endif
  n->setSplitting(true);
  // n1 is initially locked
  n1->setVersion(n->getVersion());
  split_keys_among(
    reinterpret_cast<BorderNode *>(n),
    reinterpret_cast<BorderNode *>(n1), k, value);
  std::optional<KeySlice> pull_up = std::nullopt;
ascend:
  InteriorNode *p = n->lockedParent();
  if(p == nullptr){
    auto up = pull_up ? pull_up.value() : reinterpret_cast<BorderNode*>(n1)->getKeySlice(0);
    p = create_root_with_children(n, up, n1);
    n->unlock();
    std::atomic_thread_fence(std::memory_order_acq_rel);
    n1->unlock();
    return p;
  }else if(p->isNotFull()){
    p->setInserting(true);
    size_t n_index = p->findChildIndex(n);
    auto up = pull_up ? pull_up.value() : reinterpret_cast<BorderNode*>(n1)->getKeySlice(0);
    insert_into_parent(p, n1, up ,n_index);
    // ここはreorderできないことに注意する
    n->unlock();
    n1->unlock();
    p->unlock();
    return nullptr;
  }else{ // pはfull
    p->setSplitting(true);
    size_t n_index = p->findChildIndex(n);
    n->unlock();
    Node *p1 = new InteriorNode{};
#ifndef NDEBUG
    Alloc::incInterior();
#endif
    p1->setVersion(p->getVersion());
    auto up = pull_up ? pull_up.value() : reinterpret_cast<BorderNode*>(n1)->getKeySlice(0);
    split_keys_among(
      reinterpret_cast<InteriorNode *>(p),
      reinterpret_cast<InteriorNode *>(p1), up, n1, n_index, pull_up);
    n1->unlock(); n = p; n1 = p1; goto ascend;
  }
}




/**
 * treeにkey-valueを配置する。
 * @param root 各layerのroot
 * @param key
 * @param value
 * @param upper_layer rootをnext_layerとして持つnode
 * @param upper_index rootをnext_layerとして持つnode中のindex
 * @return
 */
[[maybe_unused]]
static std::pair<PutResult,Node*> put(Node *root, Key &k, Value *value, BorderNode *upper_layer, size_t upper_index, GC &gc){
  if(root == nullptr){
    // Layer0が空の時のみここに来る
    assert(upper_layer == nullptr);
    return std::make_pair(Done,start_new_tree(k, value));
  }
retry:
  auto n_v = findBorder(root, k); auto n = n_v.first; auto v = n_v.second;
  n->lock();
  /**
   * putの場合はfindBorderでnをゲットしたら、すぐにlockをする
   * lockをする直前にそのnodeがdeletedになるかもしれないし、
   * 値を挿入すべきBorderNodeがsplitによって移動されるかもしれないし、
   * 他のputがすでに値を追加しているかもしれない(あるいは、同じkeyのputが二度呼ばれるかもしれない)
   * splitされた場合は、nextを辿ってputすべきborder nodeを探し、もう一度やり直す。
   * この時、hand over hand lockingが必要となるであろう。
   */
forward:
  auto p = n->getPermutation();
  assert(n->getLocked());
  if(v.deleted){
    n->unlock();
    if(v.is_root){
      // 探していたKeyが上のLayerに行ってしまった時、あるいはLayer0が消えた時
      // getの時と同じように、putは途中まではただのreaderなのでこのような状況は
      // 発生しうる。
      // 最も簡単なのは、treeのrootから処理をやり直すことである。
      return std::make_pair(Retry, nullptr);
    }else{
      goto retry;
    }
  }
  auto t_lv_i = n->extractLinkOrValueWithIndexFor(k);
  auto t = std::get<0>(t_lv_i);
  auto lv = std::get<1>(t_lv_i);
  auto index = std::get<2>(t_lv_i);
  if(Version::splitHappened(v, n->getVersion())){
    // findBorderとlockの間でsplit処理が起きたら
    v = n->getVersion();
    auto next = n->getNext();

    while (!v.deleted and next != nullptr){
      next->lock();
      n->unlock(); // Hand-over-Hand locking
      if(k.getCurrentSlice().slice >= next->lowestKey()){
        n = next; v = n->getVersion(); next = n->getNext();
      }else{
        break;
      }
    }
    goto forward;
  }else if(t == NOTFOUND){
    // insertをする
    auto check = check_break_invariant(n, k);
    if(check){
      auto old_index = check.value();
      handle_break_invariant(n, k, value, old_index, gc);
      k.next();
      auto pair = put(n->getLV(old_index).next_layer, k, value, n, old_index, gc);
      if(pair.first == Retry){
        return std::make_pair(Retry, nullptr);
      }
    }else{
      if(p.isNotFull()){
        insert_into_border(n, k, value, gc);
        n->unlock();
      }else{
        auto may_new_root = split(n, k, value);
        if(may_new_root != nullptr){
          // rootがsplitによって新しくなったので、Layer0以外においては
          // 上のlayerのlv.next_layerを更新する必要がある
          if(upper_layer != nullptr){
            upper_layer->setLV(upper_index, LinkOrValue(may_new_root));
          }

          return std::make_pair(Done, may_new_root);
        }
      }
    }
  }else if(t == VALUE){
    // 上書き
    gc.add(n->getLV(index).value);
    n->setLV(index, LinkOrValue(value));
    n->unlock();
  }else if(t == LAYER){
    n->unlock();
    k.next();
    auto pair = put(lv.next_layer, k, value, n, index, gc);
    if(pair.first == Retry){
      return std::make_pair(Retry, nullptr);
    }
  }else {
    // t == UNSTABLE
    assert(false);
  }

  return std::make_pair(Done, root);
}

[[nodiscard]]
static std::pair<PutResult, Node*>put_at_layer0(Node *root, Key &k, Value *value, GC &gc){
  return put(root, k, value, nullptr, 0, gc);
}



}

#endif //MASSTREE_PUT_H
