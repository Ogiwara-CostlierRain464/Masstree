#ifndef MASSTREE_TREE_H
#define MASSTREE_TREE_H

#include "version.h"
#include "key.h"
#include "permutation.h"
#include "alloc.h"
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
#include <atomic>
#include <mutex>

namespace masstree{

struct InteriorNode;
struct BorderNode;

class Node{
public:
  static constexpr size_t ORDER = 16;

  Node() = default;
  Node(const Node& other) = delete;
  Node &operator=(const Node& other) = delete;
  Node(Node&& other) = delete;
  Node &operator=(Node&& other) = delete;

  [[nodiscard]]
  Version stableVersion() const{
    auto v = getVersion();
    while(v.inserting or v.splitting){
      v = getVersion();
    }
    return v;
  }

  void lock(){
    // NOTE: 論文通りの記述にとりあえず従った
    if(this == nullptr)
      return;

    for(;;){
      auto expected = getVersion();
      auto desired = expected;
      desired.locked = true;
      if(version.compare_exchange_weak(expected, desired)){
        break;
      }
    }
  }


  void unlock(){
    auto copy_v = getVersion();
    if(copy_v.inserting){
      ++copy_v.v_insert;
    }else if(copy_v.splitting){
      ++copy_v.v_split;
    }
    copy_v.locked = false;
    copy_v.inserting = false;
    copy_v.splitting = false;

    setVersion(copy_v);
  }

  InteriorNode* lockedParent() const{
  retry:
    auto p = reinterpret_cast<Node *>(getParent()); p->lock();
    if(p != reinterpret_cast<Node *>(getParent())){
      p->unlock(); goto retry;
    }
    return reinterpret_cast<InteriorNode *>(p);
  }

  [[nodiscard]]
  inline InteriorNode* getParent() const{
    return parent.load(std::memory_order_acquire);
  }

  inline void setVersion(Version const &v) {
    version.store(v, std::memory_order_release);
  }

  inline void setParent(InteriorNode* const &p) {
    parent.store(p, std::memory_order_release);
  }

  inline void setIsBorder(bool is_border){
    auto v = getVersion();
    v.is_border = is_border;
    setVersion(v);
  }

  [[nodiscard]]
  inline bool getIsBorder() const{
    auto v = getVersion();
    return v.is_border;
  }

  inline void setIsRoot(bool is_root){
    auto v = getVersion();
    v.is_root = is_root;
    setVersion(v);
  }

  [[nodiscard]]
  inline bool getIsRoot() const{
    auto v = getVersion();
    return v.is_root;
  }

  [[nodiscard]]
  inline Version getVersion() const{
    return version.load(std::memory_order_acquire);
  }

  [[nodiscard]]
  inline bool getLocked() const{
    auto v = getVersion();
    return v.locked;
  }

  inline void setSplitting(bool splitting){
    auto v = getVersion();
    v.splitting = splitting;
    setVersion(v);
  }

  inline void setInserting(bool inserting){
    auto v = getVersion();
    v.inserting = inserting;
    setVersion(v);
  }


private:
  std::atomic<Version> version = {};
  std::atomic<InteriorNode*> parent = nullptr;
};

class InteriorNode: public Node{
public:
  Node *findChild(KeySlice slice){
    for(size_t i = 0; i < getNumKeys(); ++i){
      if(slice < key_slice[i]){
        return getChild(i);
      }
    }

    return getChild(getNumKeys());
  }

  [[nodiscard]]
  bool isNotFull() const{
    return getNumKeys() != ORDER - 1;
  }

  void printNode() const{
    printf("/");
    for(size_t i = 0; i < ORDER - 1; ++i){
      printf("%llu/", getKeySlice(i));
    }
    printf("\\\n");
  }

  size_t findChildIndex(Node *a_child) const{
    size_t index = 0;
    while (index <= getNumKeys()
           && getChild(index) != a_child){
      ++index;
    }
    return index;
  }

  [[nodiscard]]
  inline uint8_t getNumKeys() const {
    return n_keys.load(std::memory_order_acquire);
  }

  inline void setNumKeys(const uint8_t &nKeys) {
    n_keys.store(nKeys, std::memory_order_release);
  }

  inline void incNumKeys(){
    n_keys.fetch_add(1);
  }

  inline void decNumKeys(){
    n_keys.fetch_sub(1);
  }

  [[nodiscard]]
  inline KeySlice getKeySlice(size_t index) const {
    return key_slice[index].load(std::memory_order_acquire);
  }

  void resetKeySlices(){
    for(size_t i = 0; i < ORDER - 1; ++i){
      setKeySlice(i, 0);
    }
  }

  inline void setKeySlice(size_t index, const KeySlice &slice) {
    key_slice[index].store(slice, std::memory_order_release);
  }

  [[nodiscard]]
  inline Node *getChild(size_t index) const {
    return child[index].load(std::memory_order_acquire);
  }

  inline void setChild(size_t index, Node* const &c) {
    child[index].store(c, std::memory_order_release);
  }

  void resetChildren(){
    for(size_t i = 0; i < ORDER; ++i){
      setChild(i, nullptr);
    }
  }

private:
  /* 0~15 */
  std::atomic<uint8_t> n_keys = 0;
  std::array<std::atomic<uint64_t>, ORDER - 1> key_slice = {};
  std::array<std::atomic<Node*>, ORDER> child = {};

};

union LinkOrValue{

  LinkOrValue() = default;

  explicit LinkOrValue(Node *next)
  : next_layer(next){}
  explicit LinkOrValue(void *value_)
    : value(value_){}

  LinkOrValue(const LinkOrValue &other) = default;


  Node *next_layer = nullptr;
  void *value;
};


template<typename T>
static void pop_front(std::vector<T> &v)
{
  if (!v.empty()) {
    v.erase(v.begin());
  }
}


class BigSuffix{
public:
  BigSuffix() = default;
  BigSuffix(const BigSuffix& other)
  : slices(other.slices)
  , lastSliceSize(other.lastSliceSize)
  {}

  BigSuffix &operator=(const BigSuffix& other) = delete;
  BigSuffix(BigSuffix&& other) = delete;
  BigSuffix &operator=(BigSuffix&& other) = delete;

  BigSuffix(
    std::vector<KeySlice> &&slices_,
    size_t lastSliceSize_)
    : slices(std::move(slices_))
    , lastSliceSize(lastSliceSize_)
  {
    assert(1 <= lastSliceSize and lastSliceSize <= 8);
  }

  SliceWithSize getCurrentSlice(){
    if(hasNext()){
      std::lock_guard<std::mutex> lock(suffixMutex);
      return SliceWithSize(slices[0], 8);
    }else{
      std::lock_guard<std::mutex> lock(suffixMutex);
      return SliceWithSize(slices[0], lastSliceSize);
    }
  }

  size_t remainLength(){
    std::lock_guard<std::mutex> lock(suffixMutex);
    return (slices.size() - 1) * 8 + lastSliceSize;
  }

  bool hasNext(){
    std::lock_guard<std::mutex> lock(suffixMutex);
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
    std::lock_guard<std::mutex> lock(suffixMutex);
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

    std::lock_guard<std::mutex> lock(suffixMutex);
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

private:
  std::mutex suffixMutex{};
  std::vector<KeySlice> slices = {};
  const size_t lastSliceSize = 0;

};

/**
 * Border nodes store the suffixes of their keys in key-suffixes
 * data structure.
 *
 */
class KeySuffix{
public:
  KeySuffix() = default;

  /**
   * from以降のkey sliceをBigSuffixにコピーする
   * @param i
   * @param key
   * @param from
   */
  void set(size_t i, const Key &key, size_t from){
    suffixes[i].store(BigSuffix::from(key, from), std::memory_order_release);
  }

  /**
   * NOTE: ptrはdeleteされる可能性があるため、Stackではなく
   * Heapに設置する事に注意
   * @param i
   * @param ptr
   */
  void set(size_t i, BigSuffix* const &ptr){
    suffixes[i].store(ptr, std::memory_order_release);
  }

  BigSuffix* get(size_t i){
    return suffixes[i].load(std::memory_order_acquire);
  }

  void delete_ptr(size_t i){
    auto ptr = get(i);
    delete ptr;
#ifndef NDEBUG
    Alloc::decSuffix();
#endif
    set(i, nullptr);
  }

  /**
   * from以降で、keyがsuffixと一致するか
   * @param i
   * @param key
   * @param from
   * @return
   */
  bool isSame(size_t i, const Key &key, size_t from){
    return get(i)->isSame(key, from);
  }

  /**
   * suffixesをリセットする。
   * splitの時などに使う。
   */
  void reset(){
    std::fill(suffixes.begin(), suffixes.end(), nullptr);
  }

private:
  std::array<std::atomic<BigSuffix*>,Node::ORDER - 1> suffixes = {};
};

enum ExtractResult: uint8_t {
  NOTFOUND,
  VALUE,
  LAYER,
  UNSTABLE
};

class BorderNode: public Node{
public:
  static constexpr uint8_t key_len_layer = 255;
  static constexpr uint8_t key_len_unstable = 254;
  static constexpr uint8_t key_len_has_suffix = 9;

  explicit BorderNode(){
    setIsBorder(true);
  }

  /**
    * BorderNode内で、keyに該当するLinkOrValueを取得する。
    * @param key
    * @return
    */
  inline std::pair<ExtractResult, LinkOrValue> extractLinkOrValueFor(const Key &key){
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
        if(getKeySlice(i) == current.slice and getKeyLen(i) == current.size){
          return std::tuple(VALUE, getLV(i), i);
        }
      }
    }else{ // next key sliceがある場合
      for(size_t i = 0; i < numberOfKeys(); ++i){
        if(getKeySlice(i) == current.slice){
          if(getKeyLen(i) == BorderNode::key_len_has_suffix){
            // suffixの中を見る
            if(getKeySuffixes().isSame(i, key, key.cursor + 1)){
              return std::tuple(VALUE, getLV(i), i);
            }
          }

          if(getKeyLen(i) == BorderNode::key_len_layer){
            return std::tuple(LAYER, getLV(i), i);
          }
          if(getKeyLen(i) == BorderNode::key_len_unstable){
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
    return getKeyLen(ORDER - 2) == 0;
  }

  /**
   * 有効なkeyの数
   * @return
   */
  [[nodiscard]]
  size_t numberOfKeys() const{
    // key_lenが0 ⇒ その要素は空
    size_t result = 0;
    for(size_t i = 0; i < ORDER - 1; ++i){
      if(getKeyLen(i) > 0){
        ++result;
      }else{
        break;
      }
    }
    return result;
  }

  [[nodiscard]]
  KeySlice lowestKey() const{
    return getKeySlice(0);
  }

  /**
   * このBorderNodeをdeleteする前に呼ばれる。
   */
  void connectPrevAndNext() const{
    auto next_ = getNext();
    auto prev_ = getPrev();

    if(next_ != nullptr){
      next_->setPrev(prev_);
    }
    if(prev_ != nullptr){
      prev_->setNext(next_);
    }
  }

  void printNode() const{
    printf("|");
    for(size_t i = 0; i < ORDER - 1; ++i){
      printf("%llu|", getKeySlice(i));
    }
    printf("\n");
  }

  [[nodiscard]]
  inline uint8_t getKeyLen(size_t i) const{
    return key_len[i].load(std::memory_order_acquire);
  }

  inline void setKeyLen(size_t i, const uint8_t &len){
    key_len[i].store(len, std::memory_order_release);
  }

  void resetKeyLen(){
    for(size_t i = 0; i < ORDER - 1; ++i){
      setKeyLen(i, 0);
    }
  }



  [[nodiscard]]
  inline KeySlice getKeySlice(size_t i) const{
    return key_slice[i].load(std::memory_order_acquire);
  }

  inline void setKeySlice(size_t i, const KeySlice &slice){
    key_slice[i].store(slice, std::memory_order_release);
  }

  void resetKeySlice(){
    for(size_t i = 0; i < ORDER - 1; ++i){
      setKeySlice(i, 0);
    }
  }

  [[nodiscard]]
  inline LinkOrValue getLV(size_t i) const{
    return lv[i].load(std::memory_order_acquire);
  }

  inline void setLV(size_t i, const LinkOrValue& lv_){
    lv[i].store(lv_, std::memory_order_release);
  }

  void resetLVs(){
    for(size_t i = 0; i < ORDER - 1; ++i){
      setLV(i, LinkOrValue{});
    }
  }

  [[nodiscard]]
  inline BorderNode* getNext() const{
    return next.load(std::memory_order_acquire);
  }

  inline void setNext(BorderNode* const &next_){
    next.store(next_, std::memory_order_release);
  }

  [[nodiscard]]
  inline BorderNode* getPrev() const{
    return prev.load(std::memory_order_acquire);
  }

  inline void setPrev(BorderNode* const &prev_){
    prev.store(prev_, std::memory_order_release);
  }

  inline KeySuffix& getKeySuffixes(){
    return key_suffixes;
  }

private:

  std::atomic<uint8_t> n_removed = 0;
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
  std::array<std::atomic<uint8_t>, ORDER - 1> key_len = {};
  std::atomic<Permutation> permutation = {};
  std::array<std::atomic<uint64_t>, ORDER - 1> key_slice = {};
  std::array<std::atomic<LinkOrValue>, ORDER - 1> lv = {};
  std::atomic<BorderNode*> next = nullptr;
  std::atomic<BorderNode*> prev = nullptr;
  KeySuffix key_suffixes = {};
};


static std::pair<BorderNode *, Version> findBorder(Node *root, const Key &key){
  retry:
  auto n = root; auto v = n->stableVersion();

  if(!v.is_root){
    root = root->getParent(); goto retry;
  }
  descend:
  if(n->getIsBorder()){
    return std::pair(reinterpret_cast<BorderNode *>(n), v);
  }
  auto interior_n = reinterpret_cast<InteriorNode *>(n);
  auto n1 = interior_n->findChild(key.getCurrentSlice().slice);
  Version v1 = n1->stableVersion();
  if((n->getVersion() ^ v) <= Version::lock){
    n = n1; v = v1; goto descend;
  }
  auto v2 = n->stableVersion();
  if(v2.v_split != v.v_split){
    goto retry;
  }
  v = v2; goto descend;
}


static void print_sub_tree(Node *root){
  if(root->getIsBorder()){
    auto border = reinterpret_cast<BorderNode *>(root);
    border->printNode();
  }else{
    auto interior = reinterpret_cast<InteriorNode *>(root);
    interior->printNode();
  }
}

}

#endif //MASSTREE_TREE_H
