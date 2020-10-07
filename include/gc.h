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
    borders.push_back(b);
  }

  void add(InteriorNode* i){
    interiors.push_back(i);
  }

  void run(){
    for(auto &b: borders){
      // TODO: Destructorでvalueとsuffixも解放する
      delete b;
    }
    borders.clear();

    for(auto &i: interiors){
      delete i;
    }
    interiors.clear();
  }

private:
  std::vector<BorderNode*> borders{};
  std::vector<InteriorNode*> interiors{};
};

}

#endif //MASSTREE_GC_H
