#ifndef MASSTREE_BPTREE_H
#define MASSTREE_BPTREE_H

#include "tree.h"

namespace masstree{

Node *insert(Node *root, Key &key, void* value);

Node *start_new_tree(const Key &key, void *value){
  auto root = new BorderNode;
  root->version.is_root = true;

  auto cursor = key.getCurrentSlice();
  if(1 <= cursor.size and cursor.size <= 7){
    root->key_len[0] = cursor.size;
    root->key_slice[0] = cursor.slice;
    root->lv[0].value = value;
  }else{
    assert(cursor.size == 8);
    if(key.hasNext()){
      // next layerは作らない
      root->key_slice[0] = cursor.slice;
      root->key_len[0] = BorderNode::key_ken_has_suffix;
      root->lv[0].value = value;
      root->key_suffixes.set(0, key, 1);
    }else{
      root->key_len[0] = 8;
      root->key_slice[0] = cursor.slice;
      root->lv[0].value = value;
    }
  }

  return root;
}

std::optional<size_t> check_break_invariant(BorderNode *borderNode, const Key &key){
  if(key.hasNext()){
    auto cursor = key.getCurrentSlice();
    for(size_t i = 0; i < borderNode->numberOfKeys(); ++i){
      if(borderNode->key_len[i] >= BorderNode::key_ken_has_suffix
        && borderNode->key_slice[i] == cursor.slice
      ){
        return i;
      }
    }
  }
  return std::nullopt;
}

bool check_break_invariant(const Key &key, const BigSuffix* bigSuffix){
  if(key.hasNext() and bigSuffix->hasNext()){
    auto key_cursor = key.getCurrentSlice();
    auto suffix_cursor = bigSuffix->getCurrentSlice();
    if(key_cursor.slice == suffix_cursor.slice){
      return true;
    }
  }
  return false;
}

/**
 * Corresponds to insert_into_leaf in B+tree.
 * 前提: borderに空きがある。
 * @param border
 * @param key
 * @param value
 */
void insert_into_border(BorderNode *border, Key &key, void *value){
  // 簡単のため、先にnew layerを作る場合の処理について扱います
  auto check = check_break_invariant(border, key);
  if(check){
    auto index = check.value();
    if(border->key_len[index] == BorderNode::key_ken_has_suffix){
      auto old_val = border->lv[index].value;
      auto old_key_suffix_copy = new BigSuffix(*border->key_suffixes.get(index));
      BorderNode *old_layer;
      BorderNode *first_new_layer;
      bool first = false;
    next_layer:
      auto new_layer = new BorderNode;

      if(!first){
        first_new_layer = new_layer;
        first = true;
      }else{
        old_layer->lv[0].next_layer = new_layer;
      }

      new_layer->version.is_root = true;

      key.next();
      // 新しいlayerに移動
      // ここでもinvariateするかどうかで変わる
      if(check_break_invariant(key, old_key_suffix_copy)){
        new_layer->key_len[0] = BorderNode::key_len_layer;
        new_layer->key_slice[0] = old_key_suffix_copy->getCurrentSlice().slice;
        old_layer = new_layer;
        goto next_layer;
      }else{
        // 2つのkeyをそれぞれinsertする
        // newの方は、普通にinsert_into_borderを呼び出す
        // oldの方は、suffixか、sliceに入る事に
        insert_into_border(new_layer, key, value);
        if(old_key_suffix_copy->hasNext()){
          new_layer->key_slice[1] = old_key_suffix_copy->getCurrentSlice().slice;
          new_layer->key_len[1] = BorderNode::key_ken_has_suffix;
          old_key_suffix_copy->next();
          new_layer->key_suffixes.set(1, old_key_suffix_copy);
          new_layer->lv[1].value = old_val;
        }else{
          new_layer->key_slice[1] = old_key_suffix_copy->getCurrentSlice().slice;
          new_layer->key_len[1] = old_key_suffix_copy->getCurrentSlice().size;
          new_layer->lv[1].value = old_val;
          delete old_key_suffix_copy;
        }
      }
      border->key_len[index] = BorderNode::key_len_layer;
      delete border->key_suffixes.get(index);
      border->lv[index].next_layer = first_new_layer;
    }else{
      assert(border->key_len[index] == BorderNode::key_len_layer);
      key.next();
      insert(border->lv[index].next_layer, key, value);
    }
    return;
  }

  size_t insertion_point = 0;
  size_t num_keys = border->numberOfKeys();
  auto cursor = key.getCurrentSlice();
  while (insertion_point < num_keys
  && border->key_slice[insertion_point] < cursor.slice){
    ++insertion_point;
  }

  for(size_t i = num_keys; i > insertion_point; --i){
    border->key_len[i] = border->key_len[i - 1];
    border->key_slice[i] = border->key_slice[i - 1];
    border->lv[i] = border->lv[i - 1];
  }

  // new treeの時と同じように、invariantを満たす
  // まで、layerを作り続ける
  if(1 <= cursor.size and cursor.size <= 7){
    border->key_len[insertion_point] = cursor.size;
    border->key_slice[insertion_point] = cursor.slice;
    border->lv[insertion_point].value = value;
  }else{
    assert(cursor.size == 8);
    if(key.hasNext()){
      border->key_slice[insertion_point] = cursor.slice;
      // invariantを満たさない場合がある。
      // 満たさない間は、new layerを作り続ける
      // 満たしたら、sliceに入れるか、suffixに入れる
      // 面倒なので、とりあえずslicesに保存します
      border->key_len[insertion_point] = BorderNode::key_ken_has_suffix;
      border->key_suffixes.set(insertion_point, key, key.cursor + 1);
      border->lv[insertion_point].value = value;
    }else{
      border->key_len[insertion_point] = 8;
      border->key_slice[insertion_point] = cursor.slice;
      border->lv[insertion_point].value = value;
    }
  }
}

/**
 * Finds the appropriate place to
 * split a node that is too big into two.
 * @return
 */
size_t cut(size_t len){
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
 */
void split_keys_among(InteriorNode *p, InteriorNode *p1, KeySlice slice, Node *n1, size_t n_index){
  uint64_t temp_key_slice[Node::ORDER] = {};
  Node* temp_child[Node::ORDER + 1] = {};

  for(size_t i = 0, j = 0; i < p->n_keys + 1; ++i, ++j){
    if(j == n_index + 1) ++j;
    temp_child[j] = p->child[i];
  }
  for(size_t i = 0, j = 0; i < p->n_keys; ++i, ++j){
    if(j == n_index) ++j;
    temp_key_slice[j] = p->key_slice[i];
  }
  temp_child[n_index + 1] = n1;
  temp_key_slice[n_index] = slice;

  // clean
  // 実は、InteriorNodeの場合はn_keysを0にするだけで十分
  //std::fill(p->key_slice, p->key_slice + Node::ORDER + 1, 0);
  //std::fill(p->child, p->child + Node::ORDER, nullptr);
  p->n_keys = 0;
  size_t split = cut(Node::ORDER);
  size_t i, j;
  for(i = 0; i < split - 1; ++i){
    p->child[i] = temp_child[i];
    p->key_slice[i] = temp_key_slice[i];
    ++p->n_keys;
  }
  p->child[i] = temp_child[i];
  for(++i, j = 0; i < Node::ORDER; ++i, ++j){
    p1->child[j] = temp_child[i];
    p1->key_slice[j] = temp_key_slice[i];
    ++p1->n_keys;
  }
  p1->child[j] = temp_child[i];
  p1->parent = p->parent;
  for(i = 0; i <= p1->n_keys; ++i){
    p1->child[i]->parent = p1;
  }
}

/**
 * Corresponds to insert_into_leaf_after_splitting in B+ tree.
 * @param n
 * @param n1
 * @param k
 * @param value
 */
void split_keys_among(BorderNode *n, BorderNode *n1, const Key &k, void *value){
  uint8_t temp_key_len[Node::ORDER] = {};
  uint64_t temp_key_slice[Node::ORDER] = {};
  LinkOrValue temp_lv[Node::ORDER] = {};
  BigSuffix* temp_suffix[Node::ORDER] = {};

  size_t insertion_index = 0;
  while (insertion_index < Node::ORDER - 1
  && n->key_slice[insertion_index] < k.getCurrentSlice().slice){
    ++insertion_index;
  }

  // 16個分、tempにコピー
  for(size_t i = 0, j = 0; i < n->numberOfKeys(); ++i, ++j){
    if(j == insertion_index) ++j;
    temp_key_len[j] = n->key_len[i];
    temp_key_slice[j] = n->key_slice[i];
    temp_lv[j] = n->lv[i];
    temp_suffix[j] = n->key_suffixes.get(i);
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
      temp_key_slice[insertion_index] = cursor.slice;
      // invariantを満たさない場合がある。
      // 満たさない間は、new layerを作り続ける
      // 満たしたら、sliceに入れるか、suffixに入れる
      // 面倒なので、とりあえずslicesに保存します
      temp_key_len[insertion_index] = BorderNode::key_ken_has_suffix;
      temp_suffix[insertion_index] = BigSuffix::from(k, k.cursor + 1);
      temp_lv[insertion_index].value = value;
    }else{
      temp_key_len[insertion_index] = 8;
      temp_key_slice[insertion_index] = cursor.slice;
      temp_lv[insertion_index].value = value;
    }
  }

  // clear both nodes.
  std::fill(n->key_len.begin(), n->key_len.end(), 0);
  std::fill(n->key_slice.begin(), n->key_slice.end(), 0);
  std::fill(n->lv.begin(), n->lv.end(), LinkOrValue{});
  n->key_suffixes.reset();

  std::fill(n1->key_len.begin(), n1->key_len.end(), 0);
  std::fill(n1->key_slice.begin(), n1->key_slice.end(), 0);
  std::fill(n1->lv.begin(), n1->lv.end(), LinkOrValue{});
  n1->key_suffixes.reset();

  size_t split = cut(Node::ORDER - 1);

  for(size_t i = 0; i < split; ++i){
    n->key_len[i] = temp_key_len[i];
    n->key_slice[i] = temp_key_slice[i];
    n->lv[i] = temp_lv[i];
    n->key_suffixes.set(i, temp_suffix[i]);
  }

  for(size_t i = split, j = 0; i < Node::ORDER; ++i, ++j){
    n1->key_len[j] = temp_key_len[i];
    n1->key_slice[j] = temp_key_slice[i];
    n1->lv[j] = temp_lv[i];
    n1->key_suffixes.set(j, temp_suffix[i]);
  }

  n1->next = n->next;
  n->next = n1;
  n1->prev = n;
  n1->parent = n->parent;
}

/**
 * Corresponds to insert_into_new_root in B+tree.
 * @param left
 * @param slice
 * @param right
 * @return
 */
InteriorNode *create_root_with_children(Node *left, KeySlice slice, Node *right){
  auto root = new InteriorNode;
  root->version.is_root = true;
  root->n_keys = 1;
  root->key_slice[0] = slice;
  root->child[0] = left;
  root->child[1] = right;
  left->parent = root;
  right->parent = root;
  return root;
}

/**
 * Corresponds to insert_into_node in B+tree.
 * @param p
 * @param n1
 * @param slice
 */
void insert_into_parent(InteriorNode *p, Node *n1, KeySlice slice, size_t n_index){
  // move to right
  for(size_t i = p->n_keys; i > n_index; --i){
    p->child[i + 1] = p->child[i];
    p->key_slice[i] = p->key_slice[i - 1];
  }
  p->child[n_index + 1] = n1;
  p->key_slice[n_index] = slice;
  ++p->n_keys;
}

size_t get_n_index(InteriorNode *parent, Node* n){
  size_t n_index = 0;
  while (n_index <= parent->n_keys
  && parent->child[n_index] != n){
    ++n_index;
  }
  return n_index;
}

KeySlice getMostLeftSlice(Node *n){
  if(n->version.is_border){
    auto border = reinterpret_cast<BorderNode*>(n);
    return border->key_slice[0];
  }else{
    auto interior = reinterpret_cast<InteriorNode*>(n);
    return interior->key_slice[0];
  }
}

/**
 *
 * @param n
 * @param k
 * @param value
 * @return new root if not nullptr.
 */
Node *split(Node *n, const Key &k, void *value){
  // precondition: n locked.
  assert(n->version.locked);
  Node *n1 = new BorderNode;
  n->version.is_root = false;
  n->version.splitting = true;
  // n1 is initially locked
  n1->version = n->version;
  auto cursor = k.getCurrentSlice();
  split_keys_among(
    reinterpret_cast<BorderNode *>(n),
    reinterpret_cast<BorderNode *>(n1), k, value);
ascend:
  InteriorNode *p = lockedParent(n);
  auto mostLeft = getMostLeftSlice(n1);
  if(p == nullptr){
    p = create_root_with_children(n, mostLeft, n1);
    unlock(n);
    std::atomic_thread_fence(std::memory_order_acq_rel);
    unlock(n1);
    return p;
  }else if(p->isNotFull()){
    p->version.inserting = true;
    size_t n_index = get_n_index(p, n);
    insert_into_parent(p, n1, mostLeft ,n_index);
    unlock(n);
    std::atomic_thread_fence(std::memory_order_acq_rel);
    unlock(n1);
    std::atomic_thread_fence(std::memory_order_acq_rel);
    unlock(p);
    return nullptr;
  }else{
    p->version.splitting = true;
    size_t n_index = get_n_index(p, n);
    std::atomic_thread_fence(std::memory_order_acq_rel);
    unlock(n);
    std::atomic_thread_fence(std::memory_order_acq_rel);
    Node *p1 = new InteriorNode;
    p1->version = p->version;
    split_keys_among(
      reinterpret_cast<InteriorNode *>(p),
      reinterpret_cast<InteriorNode *>(p1), mostLeft, n1, n_index);
    unlock(n1); n = p; n1 = p1; goto ascend;
  }
}


/**
 * NormalなB+ treeを参考にして作る
 * rootが更新される場合があるので、rootへのポインタを返す
 */
Node *insert(Node *root, Key &key, void* value){
  /**
    * Case 1: Treeのrootが空の場合
    */
  if(root == nullptr){
    return start_new_tree(key, value);
  }

  /**
   * Case 2: すでにKeyが存在している場合
   */
  if(write(root, key, value)){
    return root;
  }

  auto border_v = findBorder(root, key);
  auto border = border_v.first;

  /**
   * Case 3: border nodeに空きがある場合
   */
  if(border->isNotFull()){
    insert_into_border(border, key, value);
    return root;
  }

  /**
   * Case 4: border nodeに空きが無い場合
   */
  lock(border);
  auto new_root = split(border, key, value);
  return new_root ? new_root : root;
}


void print_sub_tree(Node *root){
  if(root->version.is_border){
    auto border = reinterpret_cast<BorderNode *>(root);
    border->printNode();
  }else{
    auto interior = reinterpret_cast<InteriorNode *>(root);
    interior->printNode();
  }
}

}

#endif //MASSTREE_BPTREE_H
