#ifndef MASSTREE_TREE_H
#define MASSTREE_TREE_H

#include <cstdint>
#include <cstddef>
#include <cassert>
#include <tuple>
#include <utility>
#include <vector>
#include <algorithm>
#include <optional>
#include <tuple>
#include <iostream>
#include <memory>
#include "version.h"
#include "key.h"
#include "permutation.h"
#include "alloc.h"

namespace masstree{

struct InteriorNode;
struct BorderNode;

struct Node{
  static constexpr size_t ORDER = 16;
  /* alignas(?) */ Version version = {};
  InteriorNode *parent = nullptr;

//  BorderNode *b(){
//    assert(version.is_border);
//    return reinterpret_cast<BorderNode *>(this);
//  }
//
//  InteriorNode *i(){
//    assert(!version.is_border);
//    return reinterpret_cast<InteriorNode *>(this);
//  }
};

struct InteriorNode: Node{
  /* 0~15 */
  uint8_t n_keys = 0;
  std::array<uint64_t, ORDER - 1> key_slice = {};
  std::array<Node*, ORDER> child = {};

  Node *findChild(KeySlice slice){
    /**
     * B+ treeのinterior nodeと同じ走査方法
     * 左側：未満
     * 右側：以上
     */
    for(size_t i = 0; i < n_keys; ++i){
      if(slice < key_slice[i]){
        return child[i];
      }
    }

    return child[n_keys];
  }

  size_t findChildIndex(Node *a_child){
    size_t index = 0;
    while (index <= n_keys
    && child[index] != a_child){
      ++index;
    }
    return index;
  }

  [[nodiscard]]
  bool isNotFull() const{
    return n_keys != ORDER - 1;
  }

  void printNode(){
    printf("/");
    for(size_t i = 0; i < ORDER - 1; ++i){
      printf("%llu/", key_slice[i]);
    }
    printf("\\\n");
  }
};

union LinkOrValue{
  /*alignas*/ Node *next_layer = nullptr;
  /*alignas*/ void *value;
};


template<typename T>
static void pop_front(std::vector<T> &v)
{
  if (!v.empty()) {
    v.erase(v.begin());
  }
}


struct BigSuffix{
  std::vector<KeySlice> slices = {};
  size_t lastSliceSize = 0;

  BigSuffix() = default;
  BigSuffix(const BigSuffix& other) = default;

  BigSuffix(std::vector<KeySlice> &&slices_, size_t lastSliceSize_)
  : slices(std::move(slices_))
  , lastSliceSize(lastSliceSize_)
  {
    assert(1 <= lastSliceSize and lastSliceSize <= 8);
  }

  SliceWithSize getCurrentSlice() const{
    if(hasNext()){
      return SliceWithSize(slices[0], 8);
    }else{
      return SliceWithSize(slices[0], lastSliceSize);
    }
  }

  size_t remainLength(){
    return (slices.size() - 1) * 8 + lastSliceSize;
  }

  bool hasNext() const{
    return slices.size() >= 2;
  }

  void next(){
    assert(hasNext());
    pop_front(slices);
  }

  /**
   * Stackとしてのslicesのトップに要素を乗っける
   * @param slice
   */
  void insertTop(KeySlice slice){
    slices.insert(slices.begin(), slice);
  }

  /**
   * from以降で、keyがsuffixと一致するか
   * @param key
   * @param from
   * @return
   */
  bool isSame(const Key &key, size_t from){
    // keyのサイズと、suffixのサイズの比較
    if(key.remainLength(from) != this->remainLength()){
      return false;
    }

    for(size_t i = 0; i < slices.size(); ++i){
      if(key.slices[i + from] != slices[i]){
        return false;
      }
    }

    return true;
  }

  static BigSuffix *from(const Key &key, size_t from){
    std::vector<KeySlice> tmp{};
    for(size_t j = from; j < key.slices.size(); ++j){
      tmp.push_back(key.slices[j]);
    }
#ifndef NDEBUG
    Alloc::incSuffix();
#endif
    return new BigSuffix(std::move(tmp), key.lastSliceSize());
  }
};

/**
 * Border nodes store the suffixes of their keys in key-suffixes
 * data structure.
 *
 */
struct KeySuffix{
  std::array<BigSuffix*,Node::ORDER - 1> suffixes = {};

  KeySuffix() = default;

  /**
   * from以降のkey sliceをBigSuffixにコピーする
   * @param i
   * @param key
   * @param from
   */
  void set(size_t i, const Key &key, size_t from){
    suffixes[i] = BigSuffix::from(key, from);
  }

  /**
   * NOTE: ptrはdeleteされる可能性があるため、Stackではなく
   * Heapに設置する事に注意
   * @param i
   * @param ptr
   */
  void set(size_t i, BigSuffix* ptr){
    suffixes[i] = ptr;
  }

  BigSuffix* get(size_t i){
    return suffixes[i];
  }

  void delete_ptr(size_t i){
    auto ptr = suffixes[i];
    delete ptr;
#ifndef NDEBUG
    Alloc::decSuffix();
#endif
    suffixes[i] = nullptr;
  }

  /**
   * from以降で、keyがsuffixと一致するか
   * @param i
   * @param key
   * @param from
   * @return
   */
  bool isSame(size_t i, const Key &key, size_t from){
    return suffixes[i]->isSame(key, from);
  }

  /**
   * suffixesをリセットする。
   * splitの時などに使う。
   */
  void reset(){
    std::fill(suffixes.begin(), suffixes.end(), nullptr);
  }
};

enum ExtractResult: uint8_t {
  NOTFOUND,
  VALUE,
  LAYER,
  UNSTABLE
};

struct BorderNode: Node{
  static constexpr uint8_t key_len_layer = 255;
  static constexpr uint8_t key_len_unstable = 254;
  static constexpr uint8_t key_len_has_suffix = 9;

  uint8_t n_removed = 0;
  /**
   * border nodeの各key_sliceの中の
   * sliceの長さ(byte数)を表す
   *
   * 0の時:
   * 該当するKeyは存在しない
   *
   * 1~8の時:
   * LinkOrValueにはvalueがある
   *
   * 9の時:
   * Suffixesに残りがあって、LinkOrValueにはvalueがある。
   * 残りの長さはSuffixesにある。
   *
   * 254の時:
   * LinkOrValueの状態が不安定、readerは最初からやり直し
   *
   * 255の時
   * LinkOrValueにはnext_layerがある
   */
  std::array<uint8_t, ORDER - 1> key_len = {};
  Permutation permutation = {};
  std::array<uint64_t, ORDER - 1> key_slice = {};
  std::array<LinkOrValue, ORDER - 1> lv = {};
  BorderNode *next = nullptr;
  BorderNode *prev = nullptr;
  KeySuffix key_suffixes = {};

  explicit BorderNode(){
    version.is_border = true;
  }

  /**
 * BorderNode内で、keyに該当するLinkOrValueを取得する。
 * @param key
 * @return
 */
  std::pair<ExtractResult, LinkOrValue> extractLinkOrValueFor(const Key &key){
    auto triple = extractLinkOrValueWithIndexFor(key);
    return std::pair(std::get<0>(triple), std::get<1>(triple));
  }

  /**
 * BorderNode内で、keyに該当するLinkOrValueを取得する
 * また、見つけた位置のindexも返す
 * @param key
 * @return
 */
  std::tuple<ExtractResult, LinkOrValue, size_t> extractLinkOrValueWithIndexFor(const Key &key){
    /**
     * 現在のkey sliceの次のkey sliceがあるかどうかで場合わけ
     *
     * 次のkey sliceがない場合は、現在のkey sliceの長さは1~8
     * 各スロットについて、現在のkey sliceと一致するかどうか確認し、valueを返す
     *
     * 次のkey sliceがある場合は、現在のkey sliceの長さは8
     * 現在のkey sliceとnode中のkey_sliceが一致する場所を見つけ、そこでさらに場合わけ
     *
     * もし、key_lenが8の場合は、残りのkey sliceは一個のみであり、次のkey sliceはKey Suffixに保存されている
     * のでそこからvalueを返す
     * key_lenがLAYERを表す場合は、次のlayerを返す
     * key_lenがUNSTABLEの場合は、UNSTABLEを返す
     *
     * どれにも該当しない場合、NOTFOUNDを返す
     *
     */
    auto current = key.getCurrentSlice();

    if(!key.hasNext()){ // next key sliceがない場合
      for(size_t i = 0; i < numberOfKeys(); ++i){
        if(key_slice[i] == current.slice and key_len[i] == current.size){
          return std::tuple(VALUE, lv[i], i);
        }
      }
    }else{ // next key sliceがある場合
      for(size_t i = 0; i < numberOfKeys(); ++i){
        if(key_slice[i] == current.slice){
          if(key_len[i] == BorderNode::key_len_has_suffix){
            // suffixの中を見る
            if(key_suffixes.isSame(i, key, key.cursor + 1)){
              return std::tuple(VALUE, lv[i], i);
            }
          }

          if(key_len[i] == BorderNode::key_len_layer){
            return std::tuple(LAYER, lv[i], i);
          }
          if(key_len[i] == BorderNode::key_len_unstable){
            return std::tuple(UNSTABLE, LinkOrValue{}, 0);
          }
        }
      }
    }

    return std::tuple(NOTFOUND, LinkOrValue{}, 0);
  }

  /**
   * 新たにkey sliceを挿入する余裕があるかどうか
   */
  [[nodiscard]]
  bool isNotFull() const{
    // key_lenが0 ⇒ その要素は空
    return key_len[ORDER - 2] == 0;
  }

  /**
   * 有効なkeyの数
   * @return
   */
  size_t numberOfKeys() const{
    // key_lenが0 ⇒ その要素は空
    size_t result = 0;
    for(size_t i = 0; i < ORDER - 1; ++i){
      if(key_len[i] > 0){
        ++result;
      }else{
        break;
      }
    }
    return result;
  }

  KeySlice lowestKey(){
    return key_slice[0];
  }

  /**
   * このBorderNodeをdeleteする前に呼ばれる。
   */
  void connectPrevAndNext(){
    if(next != nullptr){
      next->prev = this->prev;
    }
    if(prev != nullptr){
      prev->next = this->next;
    }
  }

  void printNode(){
    printf("|");
    for(size_t i = 0; i < ORDER - 1; ++i){
      printf("%llu|", key_slice[i]);
    }
    printf("\n");
  }
};

static Version stableVersion(Node *n){
  Version v = n->version;
  while(v.inserting or v.splitting)
    v = n->version;
  return v;
}

static void lock(Node *n){
  if(n == nullptr){
    return;
  }

  n->version.locked = true;

//  for(;;){
//    Version expected = n->version;
//    Version desired = expected;
//    desired.locked = true;
//
//    if(__atomic_compare_exchange_n(&n->version.body,
//                                &expected.body,
//                                desired.body,
//                                false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)){
//      break;
//    }
//  }
}

static void unlock(Node *n){
  auto copy_v = n->version;

  if(copy_v.inserting){
    ++copy_v.v_insert;
  } else if(copy_v.splitting){
    ++copy_v.v_split;
  }
  copy_v.locked = false;
  copy_v.inserting = false;
  copy_v.splitting = false;

  n->version = copy_v;
}

static InteriorNode *lockedParent(Node *n){
  retry: InteriorNode *p = n->parent; lock(p);
  if(p != n->parent){
    unlock(p); goto retry;
  }

  return p;
}

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


static void print_sub_tree(Node *root){
  if(root->version.is_border){
    auto border = reinterpret_cast<BorderNode *>(root);
    border->printNode();
  }else{
    auto interior = reinterpret_cast<InteriorNode *>(root);
    interior->printNode();
  }
}

}

#endif //MASSTREE_TREE_H
