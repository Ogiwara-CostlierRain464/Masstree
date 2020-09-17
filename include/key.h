#ifndef MASSTREE_KEY_H
#define MASSTREE_KEY_H

#include <cstdint>
#include <vector>
#include <tuple>
#include <cassert>

namespace masstree {

typedef uint64_t KeySlice;

struct Key {
  std::vector<KeySlice> slices;
  size_t length;
  size_t cursor = 0;

  Key(std::vector<KeySlice> slices_, size_t len)
    : slices(std::move(slices_)), length(len) {}

  [[nodiscard]]
  bool hasNext() const {
    if (slices.size() == cursor + 1)
      return false;

    return true;
  }

  [[nodiscard]]
  size_t getCurrentSliceSize() const {
    if (hasNext()) {
      return 8;
    }else{
      return length - (slices.size() - 1) * 8;
    }
  }

  std::pair<KeySlice, size_t> getCurrentSlice(){
    return std::pair(slices[cursor], getCurrentSliceSize());
  }

  void next(){
    assert(hasNext());
    ++cursor;
  }

  bool operator==(const Key &rhs) const{
    if(length != rhs.length){
      return false;
    }
    return slices == rhs.slices;
  }

  bool operator!=(const Key& rhs) const{
    return !(*this == rhs);
  }
};

}

#endif //MASSTREE_KEY_H
