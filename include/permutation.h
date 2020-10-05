#ifndef MASSTREE_PERMUTATION_H
#define MASSTREE_PERMUTATION_H

#include <cstdint>

namespace masstree{

struct Permutation{
  uint64_t body = 0;

  Permutation(){
    setNumKeys(1);
    setKeyIndex(0,0);
  }
  Permutation(const Permutation &other) = default;
  Permutation &operator=(const Permutation &other) = default;


  inline uint8_t getNumKeys() const{
    return body & 0b1111;
  }

  inline void setNumKeys(size_t num){
    assert(0 <= num and num <= 15);
    auto rm_num = body >> 4;
    rm_num = rm_num << 4;
    body = rm_num | num;
  }

  inline void incNumKeys(){
    assert(getNumKeys() <= 14);
    setNumKeys(getNumKeys() + 1);
  }

  inline void decNumKeys(){
    assert(1 <= getNumKeys());
    setNumKeys(getNumKeys() - 1);
  }

  inline uint8_t getKeyIndex(size_t i) const{
    assert(0 <= i and i < getNumKeys());

    auto rshift = body >> (15-i)*4;
    return rshift & 0b1111;
  }

  inline void setKeyIndex(size_t i, uint8_t true_index){
    assert(0 <= i and i <= 14);
    assert(0 <= true_index and true_index <= 14);

    auto left = (body >> (16-i)*4) << (16-i)*4;
    auto right = body & ((1LL << ((15-i)*4))-1);
    auto middle = true_index * (1LL << (15-i)*4);
    body =  left | middle | right;
  }

  /**
   * getKeyIndexのsyntax sugar.
   * @param i
   * @return
   */
  uint8_t operator()(size_t i) const{
    return getKeyIndex(i);
  }

  bool isNotFull() const{
    auto num = getNumKeys();
    return num != 15;
  }

  static Permutation fromSorted(size_t n_keys){
    Permutation p{};
    for(size_t i = 0; i < n_keys; ++i){
      p.setKeyIndex(i,i);
    }
    p.setNumKeys(n_keys);
    return p;
  }
};

}

#endif //MASSTREE_PERMUTATION_H
