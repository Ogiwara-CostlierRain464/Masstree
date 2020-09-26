#ifndef MASSTREE_OPERATION_H
#define MASSTREE_OPERATION_H

#include "tree.h"
#include <algorithm>

namespace masstree{

static std::pair<BorderNode *, Version> findBorder(Node *root, const Key &key){
retry:
  auto n = root; auto v = stableVersion(n);

  if(!v.is_root){
    root = root->parent; goto retry;
  }
descend:
  if(n->version.is_border){
    return std::pair(reinterpret_cast<BorderNode *>(n), v);
  }
  auto interior_n = reinterpret_cast<InteriorNode *>(n);
  auto n1 = interior_n->findChild(key.getCurrentSlice().slice);
  Version v1 = stableVersion(n1);
  if((n->version ^ v) <= Version::lock){
    n = n1; v = v1; goto descend;
  }
  auto v2 = stableVersion(n);
  if(v2.v_split != v.v_split){
    goto retry;
  }
  v = v2; goto descend;
}

static void *get(Node *root, Key &k){
retry:
  auto n_v = findBorder(root, k); auto n = n_v.first; auto v = n_v.second;
forward:
  if(v.deleted)
    goto retry;
  auto t_lv = n->extractLinkOrValueFor(k); auto t = t_lv.first; auto lv = t_lv.second;
  if((n->version ^ v) > Version::lock){
    v = stableVersion(n); auto next = n->next;
    while(!v.deleted and next != nullptr and k.getCurrentSlice().slice >= next->lowestKey()){
      n = next; v = stableVersion(n); next = n->next;
    }
    goto forward;
  }else if(t == NOTFOUND){
    return nullptr;
  }else if(t == VALUE){
    return lv.value;
  }else if(t == LAYER){
    root = lv.next_layer;
    // advance k to next slice
    k.next();
    goto retry;
  }else{ // t == UNSTABLE
    goto forward;
  }
}

static BorderNode *start_new_tree(const Key &key, void *value){
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
      root->key_len[0] = BorderNode::key_len_has_suffix;
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

static std::optional<size_t> check_break_invariant(BorderNode *borderNode, const Key &key) {
  if (key.hasNext()) {
    auto cursor = key.getCurrentSlice();
    for (size_t i = 0; i < borderNode->numberOfKeys(); ++i) {
      if ((borderNode->key_len[i] == BorderNode::key_len_has_suffix
           || borderNode->key_len[i] == BorderNode::key_len_layer)
          && borderNode->key_slice[i] == cursor.slice
        ) {
        return i;
      }
    }
  }
  return std::nullopt;
}

static Node *put(Node *root, Key &k, void *value, BorderNode *upper_layer, size_t upper_index);

static void handle_break_invariant(BorderNode *border, Key &key, void *value, size_t old_index){
  if(border->key_len[old_index] == BorderNode::key_len_has_suffix){
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
    lock(border);
    auto n1 = new BorderNode;
    n1->version.is_root = true;
    auto k2_val = border->lv[old_index].value;
    auto k2_suffix_copy = new BigSuffix(*border->key_suffixes.get(old_index));
    if(k2_suffix_copy->hasNext()){
      n1->key_len[0] = BorderNode::key_len_has_suffix;
      n1->key_slice[0] = k2_suffix_copy->getCurrentSlice().slice;
      k2_suffix_copy->next();
      n1->key_suffixes.set(0, k2_suffix_copy);
      n1->lv[0].value = k2_val;
    }else{
      n1->key_len[0] = k2_suffix_copy->getCurrentSlice().size;
      n1->key_slice[0] = k2_suffix_copy->getCurrentSlice().slice;
      n1->lv[0].value = k2_val;
      delete k2_suffix_copy;
    }

    border->key_len[old_index] = BorderNode::key_len_layer;
    border->key_suffixes.delete_ptr(old_index);
    border->lv[old_index].next_layer = n1;

    unlock(border);
    key.next();
    put(border->lv[old_index].next_layer, key, value, border, old_index);
  }else{
    assert(border->key_len[old_index] == BorderNode::key_len_layer);
    key.next();
    put(border->lv[old_index].next_layer, key, value, border, old_index);
  }
}

/**
 * Corresponds to insert_into_leaf in B+tree.
 * 前提: borderに空きがある。
 * @param border
 * @param key
 * @param value
 */
static void insert_into_border(BorderNode *border, Key &key, void *value){
  assert(border->isNotFull());

  size_t insertion_point = 0;
  size_t num_keys = border->numberOfKeys();
  auto cursor = key.getCurrentSlice();
  while (insertion_point < num_keys
         && border->key_slice[insertion_point] < cursor.slice){ // NOTE: ここで、size見ないの？
    ++insertion_point;
  }

  for(size_t i = num_keys; i > insertion_point; --i){
    border->key_len[i] = border->key_len[i - 1];
    border->key_slice[i] = border->key_slice[i - 1];
    border->key_suffixes.set(i, border->key_suffixes.get(i - 1));
    border->lv[i] = border->lv[i - 1];
  }

  if(1 <= cursor.size and cursor.size <= 7){
    border->key_len[insertion_point] = cursor.size;
    border->key_slice[insertion_point] = cursor.slice;
    border->lv[insertion_point].value = value;
  }else{
    assert(cursor.size == 8);
    if(key.hasNext()){
      // invariantを満たさない場合の処理は完了したので、ここでは
      // チェック不要
      border->key_slice[insertion_point] = cursor.slice;
      border->key_len[insertion_point] = BorderNode::key_len_has_suffix;
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
 */
static void split_keys_among(InteriorNode *p, InteriorNode *p1, KeySlice slice, Node *n1, size_t n_index){
  assert(!p->isNotFull());

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
  std::fill(p->key_slice.begin(), p->key_slice.end(), 0);
  std::fill(p->child.begin(), p->child.end(), nullptr);
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


static void create_slice_table(BorderNode *n, std::vector<std::pair<KeySlice, size_t>> &table, std::vector<KeySlice> &found){
  assert(!n->isNotFull());
  for(size_t i = 0; i < Node::ORDER - 1; ++i){
    if(!std::count(found.begin(), found.end(), n->key_slice[i])){ // NOT FOUND
      table.emplace_back(n->key_slice[i], i);
      found.push_back(n->key_slice[i]);
    }
  }
  // already sorted.
}

static size_t split_point(KeySlice new_slice, const std::vector<std::pair<KeySlice, size_t>> &table, const std::vector<KeySlice> &found){
  auto min_slice = *std::min_element(found.begin(), found.end());
  auto max_slice = *std::max_element(found.begin(), found.end());
  if(new_slice < min_slice){
    return 1;
  } else if(new_slice == min_slice){
    return table[1].second;
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
          return table[i].second;
        }
      }
    }
  } else if(new_slice == max_slice){
    for(size_t i = 0; i < table.size(); ++i){
      if(table[i].first == new_slice){
        return table[i].second;
      }
    }
  }else{
    assert(new_slice > max_slice);
    return 15;
  }
  // should not reach here.
  assert(false);
}

/**
 * Corresponds to insert_into_leaf_after_splitting in B+ tree.
 * @param n
 * @param n1
 * @param k
 * @param value
 */
static void split_keys_among(BorderNode *n, BorderNode *n1, const Key &k, void *value){
  assert(!n->isNotFull());

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
  std::fill(n->key_len.begin(), n->key_len.end(), 0);
  std::fill(n->key_slice.begin(), n->key_slice.end(), 0);
  std::fill(n->lv.begin(), n->lv.end(), LinkOrValue{});
  n->key_suffixes.reset();

  std::fill(n1->key_len.begin(), n1->key_len.end(), 0);
  std::fill(n1->key_slice.begin(), n1->key_slice.end(), 0);
  std::fill(n1->lv.begin(), n1->lv.end(), LinkOrValue{});
  n1->key_suffixes.reset();

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
static InteriorNode *create_root_with_children(Node *left, KeySlice slice, Node *right){
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
static void insert_into_parent(InteriorNode *p, Node *n1, KeySlice slice, size_t n_index){
  assert(p->isNotFull());

  // move to right
  for(size_t i = p->n_keys; i > n_index; --i){
    p->child[i + 1] = p->child[i];
    p->key_slice[i] = p->key_slice[i - 1];
  }
  p->child[n_index + 1] = n1;
  p->key_slice[n_index] = slice;
  ++p->n_keys;
}

static KeySlice getMostLeftSlice(Node *n){
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
static Node *split(Node *n, const Key &k, void *value){
  // precondition: n locked.
  assert(n->version.locked);
  Node *n1 = new BorderNode;
  // splitする時点で、nはrootにはなり得ない
  n->version.is_root = false;
  n->version.splitting = true;
  // n1 is initially locked
  n1->version = n->version;
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
    size_t n_index = p->findChildIndex(n);
    insert_into_parent(p, n1, mostLeft ,n_index);
    unlock(n);
    std::atomic_thread_fence(std::memory_order_acq_rel);
    unlock(n1);
    std::atomic_thread_fence(std::memory_order_acq_rel);
    unlock(p);
    return nullptr;
  }else{
    p->version.splitting = true;
    size_t n_index = p->findChildIndex(n);
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

static void print_sub_tree(Node *root){
  if(root->version.is_border){
    auto border = reinterpret_cast<BorderNode *>(root);
    border->printNode();
  }else{
    auto interior = reinterpret_cast<InteriorNode *>(root);
    interior->printNode();
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
static Node *put(Node *root, Key &k, void *value, BorderNode *upper_layer, size_t upper_index){
  if(root == nullptr){
    // Layer0が空の時のみここに来る
    assert(upper_layer == nullptr);
    return start_new_tree(k, value);
  }
retry:
  auto n_v = findBorder(root, k); auto n = n_v.first; auto v = n_v.second;
forward:
  if(v.deleted)
    goto retry;
  auto t_lv_i = n->extractLinkOrValueWithIndexFor(k);
  auto t = std::get<0>(t_lv_i);
  auto lv = std::get<1>(t_lv_i);
  auto index = std::get<2>(t_lv_i);
  if((n->version ^ v) > Version::lock){
    v = stableVersion(n); auto next = n->next;
    while (!v.deleted and next != nullptr and k.getCurrentSlice().slice >= next->lowestKey()){
      n = next; v = stableVersion(n); next = n->next;
    }
    goto forward;
  }else if(t == NOTFOUND){
    // insertをする
    auto check = check_break_invariant(n, k);
    if(check){
      handle_break_invariant(n, k, value, check.value());
    }else{
      if(n->isNotFull()){
        insert_into_border(n, k, value);
      }else{
        lock(n);
        auto may_new_root = split(n, k, value);
        if(may_new_root != nullptr){
          // rootがsplitによって新しくなったので、Layer0以外においては
          // 上のlayerのlv.next_layerを更新する必要がある
          if(upper_layer != nullptr){
            upper_layer->lv[upper_index].next_layer = may_new_root;
          }

          return may_new_root;
        }
      }
    }
  }else if(t == VALUE){
    // 上書きする
    // NOTE: 並行時には、古い値をGCする時に注意
    delete n->lv[index].value;
    n->lv[index].value = value;
  }else if(t == LAYER){
    k.next();
    put(lv.next_layer, k, value, n, index);
  }else {
    assert(t == UNSTABLE);
    goto forward;
  }

  return root;
}

static Node *put_at_layer0(Node *root, Key &k, void *value){
  return put(root, k, value, nullptr, 0);
}


enum RootChange : uint8_t {
  NotChange,
  NewRoot,
  LayerDeleted
};

static void handle_delete_layer_in_remove(BorderNode *n, BorderNode *upper_layer, size_t upper_index){
  assert(n->parent == nullptr);
  assert(n->numberOfKeys() == 1);
  assert(n->version.is_root);
  assert(upper_layer != nullptr);

  BigSuffix *upper_suffix;
  if(n->key_len[0] == BorderNode::key_len_has_suffix){
    auto old_suffix = n->key_suffixes.get(0);
    old_suffix->insertTop(n->key_slice[0]);
    // suffixをそのまま再利用
    upper_suffix = old_suffix;
  }else{
    // Key TerminalとなるBorderに対する処理なので、BorderNode::key_len_layerにはなりえない。
    assert(1 <= n->key_len[0] and n->key_len[0] <= 8);
    upper_suffix = new BigSuffix({n->key_slice[0]}, n->key_len[0]);
  }

  upper_layer->key_len[upper_index] = BorderNode::key_len_has_suffix;
  assert(upper_layer->key_suffixes.get(upper_index) == nullptr);
  upper_layer->key_suffixes.set(upper_index, upper_suffix);
  upper_layer->lv[upper_index].value = n->lv[0].value;

  delete n;
}


/**
 * removeの処理で、Borderがdeleteされ、parentの配置が変わる時の
 * 処理。
 * Borderがdeleteされるのは、その中の要素数が0の時のみ。
 *
 * @param n
 * @return new root
 */
static std::pair<RootChange, Node*> delete_border_node_in_remove(BorderNode *n, BorderNode *upper_layer, size_t upper_index){
  assert(n->numberOfKeys() == 0);

  if(n->version.is_root){
    // Layer 0以外ではここには到達しない
    assert(n->parent == nullptr);
    assert(upper_layer == nullptr);
    return std::make_pair(LayerDeleted, nullptr);
  }

  auto p = n->parent;
  auto n_index = p->findChildIndex(n);

  if(p->n_keys >= 2){
    if(n_index == 0){
      for(size_t i = 0; i <= 13; ++i){
        p->key_slice[i] = p->key_slice[i+1];
      }
      for(size_t i = 0; i <= 14; ++i){
        p->child[i] = p->child[i+1];
      }
    }else{
      for(size_t i = n_index-1; i <= 13; ++i){
        p->key_slice[i] = p->key_slice[i+1];
      }
      for(size_t i = n_index; i <= 14; ++i){
        p->child[i] = p->child[i+1];
      }
    }
    --p->n_keys;

    n->connectPrevAndNext();
    delete n;
    // rootの変更無し
  }else{
    assert(p->n_keys == 1);
    auto pull_up_index = n_index == 1 ? 0 : 1;
    auto pull_up_node = p->child[pull_up_index];
    if(p->version.is_root){
      assert(p->parent == nullptr);
      // rootが変わり、upper_layerからの付け替えが必要になる
      if(upper_layer == nullptr){
        // Layer0の時
        pull_up_node->version.is_root = true;
        pull_up_node->parent = nullptr;
        n->connectPrevAndNext();
        delete p;
        delete n;
        return std::make_pair(NewRoot, pull_up_node);
      }else{
        // upper layerの更新
        pull_up_node->version.is_root = true;
        pull_up_node->parent = nullptr;
        upper_layer->lv[upper_index].next_layer = pull_up_node;

        n->connectPrevAndNext();
        delete p;
        delete n;
        return std::make_pair(NewRoot, pull_up_node);
      }
    }else{
      auto pp = p->parent;
      auto p_index = pp->findChildIndex(p);
      pp->child[p_index] = pull_up_node;
      pull_up_node->parent = pp;

      n->connectPrevAndNext();
      delete p;
      delete n;
    }
  }

  return std::make_pair(NotChange, nullptr);
}

/**
 * treeから該当するkey-valueを削除する。
 * @param root
 * @param k
 * @param upper_layer rootをnext_layerとして持つnode
 * @param upper_index rootをnext_layerとして持つnode中のindex
 * @return 新しいroot
 */
static std::pair<RootChange, Node*> remove(Node *root, Key &k, BorderNode *upper_layer, size_t upper_index){
  if(root == nullptr){
    // Layer0以外では起きえない
    assert(upper_layer == nullptr);
    return std::make_pair(NotChange, nullptr);
  }

retry:
  auto n_v = findBorder(root, k); auto n = n_v.first; auto v = n_v.second;
forward:
  if(v.deleted)
    goto retry;
  auto t_lv_i = n->extractLinkOrValueWithIndexFor(k);
  auto t = std::get<0>(t_lv_i);
  auto lv = std::get<1>(t_lv_i);
  auto index = std::get<2>(t_lv_i);
  if((n->version ^ v) > Version::lock){
    v = stableVersion(n); auto next = n->next;
    while (!v.deleted and next != nullptr and k.getCurrentSlice().slice >= next->lowestKey()){
      n = next; v = stableVersion(n); next = n->next;
    }
    goto forward;
  }else if(t == NOTFOUND){
    // 何もしない?
    // 何らかの形でユーザに通知を行うべきだろうか？
    ;
  }else if(t == VALUE){
    /**
     * 削除
     *
     * 要素が一個しかないBorderNodeがrootならそのLayerは消えて、
     * 上のLayerのsuffixにセットする。
     * そして、上のLayerからそのKeyのremoveをやり直す。(そのLayerのrootから)
     *
     * そうでないなら、
     * まずborder nodeから消す
     * border node内で左シフト
     * 次にborder node内の個数を調べる
     * border nodeにまだあるならそれで終わり

     * border nodeが空になるなら、parentとしてのinteriorの変更が発生
     *
     * parentが残り二つ以上の時
     * 左シフト
     *
     * parentが一つの時
     * - root 付近の時
     * upper_layerからの繋ぎ直し、is_rootの更新
     *
     * - そうでない時
     * grand parentからリンクを伸ばす
     *
     * NOTE: どのケースにおいても、先に親としてのinteriorに先に
     * ロックをかける必要がありそうだ
     */
    if(n->version.is_root and n->numberOfKeys() == 1 and upper_layer != nullptr){
      handle_delete_layer_in_remove(n, upper_layer, upper_index);
      return std::make_pair(LayerDeleted, nullptr);
    }

    n->key_len[index] = 0;
    n->key_slice[index] = 0;
    auto suffix = n->key_suffixes.get(index);
    if(suffix != nullptr){
      n->key_suffixes.delete_ptr(index);
    }
    delete n->lv[index].value;

    for(size_t i = index; i < Node::ORDER - 2; ++i){ // i=0~13
      n->key_len[i] = n->key_len[i+1];
      n->key_slice[i] = n->key_slice[i+1];
      n->key_suffixes.set(i, n->key_suffixes.get(i+1));
      n->lv[i] = n->lv[i+1];
    }
    auto current_num_keys = n->numberOfKeys();

    if(current_num_keys == 0){
      auto pair = delete_border_node_in_remove(n, upper_layer, upper_index);
      if(pair.first != NotChange){
        return pair;
      }
    }
  }else if(t == LAYER){
    k.next();
    auto pair = remove(lv.next_layer, k, n, index);
    if(pair.first == LayerDeleted){
      k.back();
      goto retry;
    }
  }else{
    assert(t == UNSTABLE);
    goto forward;
  }

  return std::make_pair(NotChange, root);
}

static Node *remove_at_layer0(Node *root, Key &k){
  return remove(root, k, nullptr, 0).second;
}

}

#endif //MASSTREE_OPERATION_H
