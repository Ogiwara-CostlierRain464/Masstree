#ifndef MASSTREE_KEY_H
#define MASSTREE_KEY_H

#include <cstdint>
#include <vector>
#include <tuple>
#include <cassert>

namespace masstree {

typedef uint64_t KeySlice;

struct SliceWithSize{
  KeySlice slice;
  uint8_t size;

  SliceWithSize(KeySlice slice_, uint8_t size_)
  : slice(slice_)
  , size(size_)
  {
    assert(1 <= size and size <= 8);
  }

  bool operator==(const SliceWithSize &rhs) const{
    return slice == rhs.slice and size == rhs.size;
  }

  bool operator!=(const SliceWithSize &rhs) const{
    return !(*this == rhs);
  }
};

/**
 * Keyを表す。
 * 状態をもち、Iteratorとしての機能も持たせる。
 */
struct Key {
  std::vector<KeySlice> slices;
  size_t length;
  size_t cursor = 0;

  Key() = default;
  Key(const Key& other) = default;
  Key &operator=(const Key& other) = default;

  Key(std::vector<KeySlice> slices_, size_t len)
    : slices(std::move(slices_)), length(len) {
    assert((slices.size() - 1) * 8 <= length
    and length <= slices.size() * 8);
  }

  [[nodiscard]]
  bool hasNext() const {
    if (slices.size() == cursor + 1)
      return false;

    return true;
  }

  /**
   * Keyの残りの長さ
   * @param from from以降のカーソルからの長さを返す
   * @return
   */
  size_t remainLength(size_t from) const{
    assert(from <= slices.size() - 1);

    return length - from * 8;
  }

  size_t lastSliceSize() const{
    if(length % 8 == 0){
      return 8;
    }else{
      return length % 8;
    }
  }

  [[nodiscard]]
  size_t getCurrentSliceSize() const {
    if (hasNext()) {
      return 8;
    }else{
      return length - (slices.size() - 1) * 8;
    }
  }

  SliceWithSize getCurrentSlice() const{
    return SliceWithSize(slices[cursor], getCurrentSliceSize());
  }

  void next(){
    assert(hasNext());
    ++cursor;
  }

  bool operator==(const Key &rhs) const{
    return length == rhs.length
    && cursor == rhs.cursor
    && slices == rhs.slices;
  }

  bool operator!=(const Key& rhs) const{
    return !(*this == rhs);
  }
};

}

#endif //MASSTREE_KEY_H
