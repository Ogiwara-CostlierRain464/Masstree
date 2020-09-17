#ifndef MASSTREE_TREE_H
#define MASSTREE_TREE_H

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
#include "version.h"
#include "key.h"
#include "permutation.h"

namespace masstree{

struct InteriorNode;
struct BorderNode;

struct Node{
  static constexpr size_t ORDER = 16;
  /* alignas(?) */ Version version = {};
  InteriorNode *parent = nullptr;
};

struct InteriorNode: Node{
  /* 0~15 */

};


}

#endif //MASSTREE_TREE_H
