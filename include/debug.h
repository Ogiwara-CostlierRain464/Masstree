#ifndef MASSTREE_DEBUG_H
#define MASSTREE_DEBUG_H

#include <cstddef>

namespace masstree{

struct Alloc{
  static size_t borderNode;
  static size_t interiorNode;
  static size_t bigSuffix;

  static void init(){
    borderNode = 0;
    interiorNode = 0;
    bigSuffix = 0;
  }
};

}

#endif //MASSTREE_DEBUG_H
