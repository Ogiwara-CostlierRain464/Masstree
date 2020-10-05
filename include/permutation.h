#ifndef MASSTREE_PERMUTATION_H
#define MASSTREE_PERMUTATION_H

#include <cstdint>

namespace masstree{

struct Permutation{
  uint64_t body;

  inline uint8_t getNumKeys() const{
    return body & 0b1111;
  }

  inline void setNumKeys(size_t num){
    assert(0 <= num and num <= 15);
    auto rm_num = body >> 4;
    rm_num = rm_num << 4;
    body = rm_num | num;
  }

  inline uint8_t getKeyIndex(size_t i){
    auto rshift = body >> (15-i)*4;
    return rshift & 0b1111;
  }

  inline void setKeyIndex(size_t i, uint8_t index){
    assert(0 <= index and index <= 15);

    auto left = (body >> (16-i)*4) << (16-i)*4;
    auto right = body & ((1LL << ((15-i)*4))-1);
    auto middle = index * (1LL << (15-i)*4);
    body =  left | middle | right;
  }
};

}

#endif //MASSTREE_PERMUTATION_H
