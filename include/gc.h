#ifndef MASSTREE_GC_H
#define MASSTREE_GC_H

#include "tree.h"

namespace masstree{

/**
 * GCを表す。一つのthreadにつき一つこれを持つ。
 */
class GarbageCollector{
public:
  GarbageCollector() = default;
  GarbageCollector(GarbageCollector &&other) = delete;
  GarbageCollector(const GarbageCollector &other) = delete;
  GarbageCollector &operator=(GarbageCollector &&other) = delete;
  GarbageCollector &operator=(const GarbageCollector &other) = delete;

  void add(BorderNode* b){
    assert(!contain(b));
    borders.push_back(b);
  }

  void add(InteriorNode* i){
    assert(!contain(i));
    interiors.push_back(i);
  }

  bool contain(BorderNode const *n) const{
    return std::find(borders.begin(), borders.end(), n) != borders.end();
  }

  bool contain(InteriorNode const *n) const{
    return std::find(interiors.begin(), interiors.end(), n) != interiors.end();
  }


  void run(){
    for(auto &b: borders){
      // destructorでvalueとsuffixも解放される
      // TODO: BorderNodeの削除と同時にその子要素を削除するのは不適切であろう。
      delete b;
#ifndef NDEBUG
      Alloc::decBorder();
#endif
    }
    borders.clear();

    for(auto &i: interiors){
      delete i;
#ifndef NDEBUG
      Alloc::decInterior();
#endif
    }
    interiors.clear();
  }

private:
  std::vector<BorderNode*> borders{};
  std::vector<InteriorNode*> interiors{};
};

using GC = GarbageCollector;

}

#endif //MASSTREE_GC_H
