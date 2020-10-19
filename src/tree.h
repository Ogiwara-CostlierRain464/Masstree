#ifndef MASSTREE_TREE_H
#define MASSTREE_TREE_H

#include "version.h"
#include "key.h"
#include "permutation.h"
#include "alloc.h"
#include "value.h"
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

static constexpr std::memory_order READ_MEMORY_ORDER = std::memory_order_seq_cst;
static constexpr std::memory_order WRITE_MEMORY_ORDER = std::memory_order_seq_cst;

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
    assert(this != nullptr);
    for(;;){
      auto expected = getVersion();
      if(expected.locked){
        continue;
      }else{
        // lockが外された！
        auto desired = expected;
        expected.locked = false;
        desired.locked = true;
        if(version.compare_exchange_weak(expected, desired)){
          break;
        }
      }
    }
  }


  void unlock(){
    auto copy_v = getVersion();
    assert(copy_v.locked);
    assert(!(copy_v.inserting and copy_v.splitting));
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

  [[nodiscard]]
  InteriorNode* lockedParent() const{
  retry:
    auto p = reinterpret_cast<Node *>(getParent());
    if(p != nullptr){
      p->lock();
    }
    // parent changed underneath us
    if(p != reinterpret_cast<Node *>(getParent())){
      assert(p != nullptr);
      p->unlock();
      goto retry;
    }
    return reinterpret_cast<InteriorNode *>(p);
  }

  [[nodiscard]]
  inline InteriorNode* getParent() const{
    return parent.load(READ_MEMORY_ORDER);
  }

  inline void setVersion(Version const &v) {
    version.store(v, WRITE_MEMORY_ORDER);
  }

  inline void setParent(InteriorNode* p) {
    // setParentをする時には、その親はlockされていなければならない。
    // 逆に、このNode自身はlockされている必要がない。
    if(p != nullptr){
      assert(reinterpret_cast<Node *>(p)->isLocked());
    }
    parent.store(p, WRITE_MEMORY_ORDER);
  }

  [[nodiscard]]
  inline BorderNode* getUpperLayer() const{
    return upperLayer.load(READ_MEMORY_ORDER);
  }

  inline void setUpperLayer(BorderNode* p){
    // setParentをする時には、その親はlockされていなければならない。
    // 逆に、このNode自身はlockされている必要はない。
    if(p != nullptr){
      assert(reinterpret_cast<Node *>(p)->isLocked());
    }
    upperLayer.store(p, WRITE_MEMORY_ORDER);
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
    return version.load(READ_MEMORY_ORDER);
  }

  [[nodiscard]]
  inline bool isLocked() const{
    auto v = getVersion();
    return v.locked;
  }

  /**
   * NOTE: assert(isUnlocked())は無効であることに注意
   * @return
   */
  [[nodiscard]]
  inline bool isUnlocked() const{
    return !isLocked();
  }

  [[nodiscard]]
  inline bool getSplitting() const{
    auto v = getVersion();
    return v.splitting;
  }

  inline void setSplitting(bool splitting){
    auto v = getVersion();
    assert(v.locked);
    v.splitting = splitting;
    setVersion(v);
  }

  [[nodiscard]]
  inline bool getInserting() const{
    auto v = getVersion();
    return v.inserting;
  }

  inline void setInserting(bool inserting){
    auto v = getVersion();
    assert(v.locked);
    v.inserting = inserting;
    setVersion(v);
  }

  [[nodiscard]]
  inline bool getDeleted() const{
    auto v = getVersion();
    return v.deleted;
  }

  inline void setDeleted(bool deleted){
    auto v = getVersion();
    v.deleted = deleted;
    setVersion(v);
  }


private:
  std::atomic<Version> version = {};
  std::atomic<InteriorNode*> parent = nullptr;
  // 上のlayerのBorderNodeを指す。
  std::atomic<BorderNode*> upperLayer = nullptr;
};

class InteriorNode: public Node{
public:
  Node *findChild(KeySlice slice){
    auto num_keys = getNumKeys();
    for(size_t i = 0; i < num_keys; ++i){
      if(slice < key_slice[i]){
        return getChild(i);
      }
    }

    return getChild(num_keys);
  }

  [[nodiscard]]
  inline bool isNotFull() const{
    return getNumKeys() != ORDER - 1;
  }

  [[nodiscard]]
  inline bool isFull() const{
    return !isNotFull();
  }

  /**
   * Interior Nodeのkey sliceにスキップがあるか
   * @return
   */
  bool debug_has_skip() const{
    for(size_t i = 1; i < ORDER - 2; ++i){
      auto now = getKeySlice(i);
      if(getKeySlice(i-1) and now == 0 and getKeySlice(i+1) != 0){
        return true;
      }
    }
    return false;
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
    assert(getChild(index) == a_child);
    return index;
  }

  [[nodiscard]]
  inline uint8_t getNumKeys() const {
    return n_keys.load(READ_MEMORY_ORDER);
  }

  inline void setNumKeys(uint8_t nKeys) {
    n_keys.store(nKeys, WRITE_MEMORY_ORDER);
  }

  inline void incNumKeys(){
    n_keys.fetch_add(1);
  }

  inline void decNumKeys(){
    n_keys.fetch_sub(1);
  }

  [[nodiscard]]
  inline KeySlice getKeySlice(size_t index) const {
    return key_slice[index].load(READ_MEMORY_ORDER);
  }

  void resetKeySlices(){
    for(size_t i = 0; i < ORDER - 1; ++i){
      setKeySlice(i, 0);
    }
  }

  inline void setKeySlice(size_t index, const KeySlice &slice) {
    key_slice[index].store(slice, WRITE_MEMORY_ORDER);
  }

  [[nodiscard]]
  inline Node *getChild(size_t index) const {
    return child[index].load(READ_MEMORY_ORDER);
  }

  inline void setChild(size_t index, Node* c) {
    assert(0 <= index and index <= 15);
    child[index].store(c, WRITE_MEMORY_ORDER);
  }

  /**
   * Interior Nodeがcを含んでいるか確認
   * @param c
   * @return
   */
  bool debug_contain_child(Node* c) const{
    for(size_t i = 0; i < ORDER; ++i){
      if(getChild(i) == c)
        return true;
    }
    return false;
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

/**
 * deleteされるので、stack上ではなくheap上にアロケートする
 */
union LinkOrValue{

  LinkOrValue() = default;

  explicit LinkOrValue(Node *next)
  : next_layer(next){}
  explicit LinkOrValue(Value *value_)
    : value(value_){}

  LinkOrValue(const LinkOrValue &other) = default;


  Node *next_layer = nullptr;
  Value *value;
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
    return new BigSuffix(std::move(tmp), key.lastSliceSize);
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
    suffixes[i].store(BigSuffix::from(key, from), WRITE_MEMORY_ORDER);
  }

  /**
   * NOTE: ptrはdeleteされる可能性があるため、Stackではなく
   * Heapに設置する事に注意
   * @param i
   * @param ptr
   */
  inline void set(size_t i, BigSuffix* const &ptr){
    suffixes[i].store(ptr, WRITE_MEMORY_ORDER);
  }

  [[nodiscard]]
  inline BigSuffix* get(size_t i) const{
    return suffixes[i].load(READ_MEMORY_ORDER);
  }

  /**
   * リファレンスを外す
   */
  void unref(size_t i){
    assert(get(i) != nullptr);
    set(i, nullptr);
  }

  void delete_ptr(size_t i){
    auto ptr = get(i);
    assert(ptr != nullptr);
    delete ptr;
#ifndef NDEBUG
    Alloc::decSuffix();
#endif
    set(i, nullptr);
  }

  void deleteAll(){
    for(size_t i = 0; i < Node::ORDER - 1; ++i){
      auto p = get(i);
      if(p != nullptr){
        delete_ptr(i);
      }
    }
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
    auto p = getPermutation();

    if(!key.hasNext()){ // next key sliceがない場合
      for(size_t i = 0; i < p.getNumKeys(); ++i){
        auto true_index = p(i);

        if(getKeySlice(true_index) == current.slice
          and getKeyLen(true_index) == current.size){
          return std::tuple(VALUE, getLV(true_index), true_index);
        }
      }
    }else{ // next key sliceがある場合
      for(size_t i = 0; i < p.getNumKeys(); ++i){
        auto true_index = p(i);

        if(getKeySlice(true_index) == current.slice){
          if(getKeyLen(true_index) == BorderNode::key_len_has_suffix){
            // suffixの中を見る
            // この処理中に、BorderNodeからunrefされているかもしれない。
            auto suffix = getKeySuffixes().get(true_index);
            if(suffix != nullptr and suffix->isSame(key, key.cursor + 1)){
              return std::tuple(VALUE, getLV(true_index), true_index);
            }
          }

          if(getKeyLen(true_index) == BorderNode::key_len_layer){
            return std::tuple(LAYER, getLV(true_index), true_index);
          }
          if(getKeyLen(true_index) == BorderNode::key_len_unstable){
            return std::tuple(UNSTABLE, LinkOrValue{}, 0);
          }
        }
      }
    }

    return std::tuple(NOTFOUND, LinkOrValue{}, 0);
  }

  [[nodiscard]]
  KeySlice lowestKey() const{
    auto p = getPermutation();
    return getKeySlice(p(0));
  }

  /**
   * このBorderNodeをdeleteする前に呼ばれる。
   */
  void connectPrevAndNext() const{
    // prevをlockすることにより、Masstreeはsplit "to the right"の制約があるため、
    // splitについて考える必要がなくなった。
    // また、nextのremoveについてもthisをlockする必要があるため、nextがdeletedになることについて考える必要は
    // ない。
retry_prev_lock:
    auto prev_ = getPrev();
    if(prev_ != nullptr){
      prev_->lock();
      if(prev_->getDeleted() or prev_ != getPrev()){
        prev_->unlock();
        goto retry_prev_lock;
      }else{
        auto next_ = getNext();
        prev_->setNext(next_);
        if(next_ != nullptr){
          assert(!next_->getDeleted());
          next_->setPrev(prev_);
        }
        prev_->unlock();
      }
    }else{
      if(getNext() != nullptr){
        getNext()->setPrev(nullptr);
      }
    }
  }

  /**
   * keyをinsertすべきpointを返す。
   * もしremoved slotがreuseされることになるならば、二番目の要素がtrueになる。
   * その時にはinsert側はv_insertを更新する必要がある
   * @return
   */
  [[nodiscard]]
  std::pair<size_t, bool> insertPoint() const{
    // NOTE: ここでなるべくlen=0のslotを使うことにより、性能をあげる事ができる
    assert(getPermutation().isNotFull());
    for(size_t i = 0; i < ORDER - 1; ++i){
      auto len = getKeyLen(i);
      if(len == 0){
        return std::make_pair(i, false);
      }
      if(10 <= len and len <= 18){
        return std::make_pair(i, true);
      }
    }
    assert(false);
  }

  void printNode() const{
    printf("|");
    for(size_t i = 0; i < ORDER - 1; ++i){
      printf("%llu|", getKeySlice(i));
    }
    printf("\n");
  }

  /**
   * Sort unordered BorderNode, including Permutation.
   * This should be called only at BorderNode splitting.
   */
  void sort(){
    auto p = getPermutation();
    assert(p.isFull());
    assert(isLocked());
    assert(getSplitting());

    uint8_t temp_key_len[Node::ORDER - 1] = {};
    uint64_t temp_key_slice[Node::ORDER - 1] = {};
    LinkOrValue temp_lv[Node::ORDER - 1] = {};
    BigSuffix* temp_suffix[Node::ORDER - 1] = {};

    for(size_t i = 0; i < ORDER - 1; ++i){
      auto index_ts = p(i);
      temp_key_len[i] = getKeyLen(index_ts);
      temp_key_slice[i] = getKeySlice(index_ts);
      temp_lv[i] = getLV(index_ts);
      temp_suffix[i] = getKeySuffixes().get(index_ts);
    }

    for(size_t i = 0; i < ORDER - 1; ++i){
      setKeyLen(i, temp_key_len[i]);
      setKeySlice(i, temp_key_slice[i]);
      setLV(i, temp_lv[i]);
      getKeySuffixes().set(i, temp_suffix[i]);
    }

    setPermutation(Permutation::fromSorted(ORDER - 1));
  }

  /**
   * Mark that key has removed.
   * Indicate by key length.
   * @param i
   */
  void markKeyRemoved(uint8_t i){
    auto len = getKeyLen(i);
    assert(1 <= len and len <= key_len_has_suffix);
    setKeyLen(i, len + 9);
  }

  /**
   * Check whether corresponding index of key has removed.
   * @param i
   * @return
   */
  [[nodiscard]]
  bool isKeyRemoved(uint8_t i) const{
    auto len = getKeyLen(i);
    return (10 <= len and len <= 18);
  }

  [[nodiscard]]
  inline uint8_t getKeyLen(size_t i) const{
    return key_len[i].load(READ_MEMORY_ORDER);
  }

  inline void setKeyLen(size_t i, const uint8_t &len){
    key_len[i].store(len, WRITE_MEMORY_ORDER);
  }

  void resetKeyLen(){
    for(size_t i = 0; i < ORDER - 1; ++i){
      setKeyLen(i, 0);
    }
  }

  [[nodiscard]]
  inline KeySlice getKeySlice(size_t i) const{
    return key_slice[i].load(READ_MEMORY_ORDER);
  }

  inline void setKeySlice(size_t i, const KeySlice &slice){
    key_slice[i].store(slice, WRITE_MEMORY_ORDER);
  }

  void resetKeySlice(){
    for(size_t i = 0; i < ORDER - 1; ++i){
      setKeySlice(i, 0);
    }
  }

  [[nodiscard]]
  inline LinkOrValue getLV(size_t i) const{
    return lv[i].load(READ_MEMORY_ORDER);
  }

  inline void setLV(size_t i, const LinkOrValue& lv_){
    lv[i].store(lv_, WRITE_MEMORY_ORDER);
  }

  void resetLVs(){
    for(size_t i = 0; i < ORDER - 1; ++i){
      setLV(i, LinkOrValue{});
    }
  }

  [[nodiscard]]
  inline BorderNode* getNext() const{
    return next.load(READ_MEMORY_ORDER);
  }

  inline void setNext(BorderNode* next_){
    next.store(next_, WRITE_MEMORY_ORDER);
  }

  [[nodiscard]]
  inline BorderNode* getPrev() const{
    return prev.load(READ_MEMORY_ORDER);
  }

  inline void setPrev(BorderNode* prev_){
    prev.store(prev_, WRITE_MEMORY_ORDER);
  }

  [[nodiscard]]
  inline bool CASNext(BorderNode *expected, BorderNode *desired){
    return next.compare_exchange_weak(expected, desired);
  }

  inline KeySuffix& getKeySuffixes(){
    return key_suffixes;
  }

  [[nodiscard]]
  inline const KeySuffix& getKeySuffixes() const{
    return key_suffixes;
  }

  [[nodiscard]]
  inline Permutation getPermutation() const{
    return permutation.load(READ_MEMORY_ORDER);
  }

  inline void setPermutation(const Permutation &p){
    permutation.store(p, WRITE_MEMORY_ORDER);
  }

  ~BorderNode(){
    for(size_t i = 0; i < ORDER - 1; ++i){
      assert(getKeyLen(i) != key_len_layer);
      auto value = getLV(i).value;
      if(value != nullptr){
        delete value;
        setLV(i, LinkOrValue{});
#ifndef NDEBUG
        Alloc::decValue();
#endif
      }
    }

    getKeySuffixes().deleteAll();
  }

private:
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
  std::atomic<Permutation> permutation = Permutation::sizeOne();
  std::array<std::atomic<uint64_t>, ORDER - 1> key_slice = {};
  std::array<std::atomic<LinkOrValue>, ORDER - 1> lv = {};
  std::atomic<BorderNode*> next{nullptr};
  std::atomic<BorderNode*> prev{nullptr};
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
  // 当然、ここでconcurrent splitによってnの構造がグチャグチャになり、n1 == nullptrとなる可能性がある
  auto n1 = interior_n->findChild(key.getCurrentSlice().slice);
  Version v1 = n1 != nullptr ? n1->stableVersion() : Version();
  if((n->getVersion() ^ v) <= Version::has_locked){
    assert(n1 != nullptr);
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
