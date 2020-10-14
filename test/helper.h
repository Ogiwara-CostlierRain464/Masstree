#ifndef MASSTREE_HELPER_H
#define MASSTREE_HELPER_H
#include "../src/tree.h"

using namespace masstree;

struct KeySuffixSnap{
  std::array<BigSuffix*, 15> suffixes{};

  static KeySuffixSnap from(const KeySuffix &keySuffix){
    KeySuffixSnap snap;
    for(size_t i = 0; i < 15; ++i){
      snap.suffixes[i] = keySuffix.get(i);
    }
    return snap;
  }
};

struct BorderNodeSnap{
  std::array<uint8_t, 15> key_len{};
  Permutation permutation{};
  std::array<uint64_t, 15> key_slice{};
  std::array<LinkOrValue, 15> lv{};
  BorderNode * next{};
  BorderNode* prev{};
  KeySuffixSnap key_suffixes{};

  static BorderNodeSnap from(const BorderNode * const border){
    BorderNodeSnap snap;
    for(size_t i = 0; i < 15; ++i){
      snap.key_len[i] = border->getKeyLen(i);
    }
    snap.permutation = border->getPermutation();
    for(size_t i = 0; i < 15; ++i){
      snap.key_slice[i] = border->getKeySlice(i);
    }
    for(size_t i = 0; i < 15; ++i){
      snap.lv[i] = border->getLV(i);
    }
    snap.next = border->getNext();
    snap.prev = border->getPrev();
    snap.key_suffixes = KeySuffixSnap::from(border->getKeySuffixes());
    return snap;
  }
};


#endif //MASSTREE_HELPER_H
