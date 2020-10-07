#ifndef MASSTREE_PERMUTATION_H
#define MASSTREE_PERMUTATION_H

#include <cstdint>

namespace masstree{

struct Permutation{
  uint64_t body = 0;

  Permutation() = default;
  Permutation(const Permutation &other) = default;
  Permutation &operator=(const Permutation &other) = default;


  inline uint8_t getNumKeys() const{
    return body & 0b1111LLU;
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
    auto result = rshift & 0b1111LL;
    assert(0 <= result and result <= 14);
    return result;
  }

  inline void setKeyIndex(size_t i, uint8_t true_index){
    assert(0 <= i and i <= 14);
    assert(0 <= true_index and true_index <= 14);

    auto left = i == 0 ? 0 : (body >> (16-i)*4) << (16-i)*4;
    auto right = body & ((1LLU << ((15-i)*4))-1);
    auto middle = true_index * (1LLU << (15-i)*4);
    body =  left | middle | right;

    auto check = (body >> (15-i)*4) & 0b1111LLU;
    assert(0 <= check and check <= 14);
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

  bool isFull() const{
    return !isNotFull();
  }

  void insert(size_t insertion_point_ps, size_t index_ts){
    for(size_t i = getNumKeys(); i > insertion_point_ps; --i){ // 右シフト
      setKeyIndex(i, getKeyIndex(i - 1));
    }
    setKeyIndex(insertion_point_ps, index_ts);
    incNumKeys();
  }

  /**
   * Returns oen element Permutation
   * Used at BorderNode init.
   * @return
   */
  static Permutation sizeOne(){
    Permutation p{};
    p.setNumKeys(1);
    p.setKeyIndex(0,0);
    return p;
  }

  static Permutation fromSorted(size_t n_keys){
    Permutation p{};
    for(size_t i = 0; i < n_keys; ++i){
      p.setKeyIndex(i,i);
    }
    p.setNumKeys(n_keys);
    return p;
  }

  static Permutation from(const std::vector<size_t> &vec){
    Permutation p{};
    for(size_t i = 0; i < vec.size(); ++i){
      p.setKeyIndex(i, vec[i]);
    }
    p.setNumKeys(vec.size());
    return p;
  }
};

}

#endif //MASSTREE_PERMUTATION_H
