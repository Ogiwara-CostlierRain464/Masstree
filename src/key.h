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
 * 一つのthreadのみが扱うので、thread-safeである必要はない。
 */
struct Key {
  std::vector<KeySlice> slices;
  size_t lastSliceSize = 0;
  size_t cursor = 0;

  Key() = default;
  Key(Key&& other) = default;
  Key(const Key& other) = default;
  Key &operator=(const Key& other) = default;
  Key &operator=(Key&& other) = default;

  Key(std::vector<KeySlice> slices_, size_t lastSliceSize_) noexcept
    : slices(std::move(slices_)), lastSliceSize(lastSliceSize_) {
    assert(1 <= lastSliceSize and lastSliceSize <= 8);
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
  [[nodiscard]]
  size_t remainLength(size_t from) const{
    assert(from <= slices.size() - 1);

    return (slices.size() - from - 1)*8 + lastSliceSize;
  }

  [[nodiscard]]
  size_t getCurrentSliceSize() const {
    if (hasNext()) {
      return 8;
    }else{
      return lastSliceSize;
    }
  }

  [[nodiscard]]
  SliceWithSize getCurrentSlice() const{
    return SliceWithSize(slices[cursor], getCurrentSliceSize());
  }

  void next(){
    assert(hasNext());
    ++cursor;
  }

  void back(){
    assert(cursor != 0);
    --cursor;
  }

  void reset(){
    cursor = 0;
  }

  bool operator==(const Key &rhs) const{
    return lastSliceSize == rhs.lastSliceSize
    && cursor == rhs.cursor
    && slices == rhs.slices;
  }

  bool operator!=(const Key& rhs) const{
    return !(*this == rhs);
  }
};

}

#endif //MASSTREE_KEY_H
