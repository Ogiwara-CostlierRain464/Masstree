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
    assert(b->getDeleted());
    borders.push_back(b);
  }

  void add(InteriorNode* i){
    assert(!contain(i));
    assert(i->getDeleted());
    interiors.push_back(i);
  }

  void add(Value* v){
    assert(!contain(v));
    values.push_back(v);
  }

  void add(BigSuffix* suffix){
    assert(!contain(suffix));
    suffixes.push_back(suffix);
  }

  bool contain(BorderNode const *n) const{
    return std::find(borders.begin(), borders.end(), n) != borders.end();
  }

  bool contain(InteriorNode const *n) const{
    return std::find(interiors.begin(), interiors.end(), n) != interiors.end();
  }

  [[nodiscard]]
  bool contain(Value const *n) const{
    return std::find(values.begin(), values.end(), n) != values.end();
  }

  bool contain(BigSuffix const *suffix) const{
    return std::find(suffixes.begin(), suffixes.end(), suffix) != suffixes.end();
  }


  void run() noexcept{
    for(auto &b: borders){
      // destructorでvalueとsuffixも解放される
      // TODO: BorderNodeの削除と同時にデストラクタでその子要素を削除するのは不適切であろうか？
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

    for(auto &v: values){
      delete v;
#ifndef NDEBUG
      Alloc::decValue();
#endif
    }
    values.clear();

    for(auto &s: suffixes){
      delete s;
#ifndef NDEBUG
      Alloc::decSuffix();
#endif
    }
    suffixes.clear();
  }

private:
  std::vector<BorderNode*> borders{};
  std::vector<InteriorNode*> interiors{};
  std::vector<Value*> values{};
  std::vector<BigSuffix*> suffixes{};
};

using GC = GarbageCollector;

}

#endif //MASSTREE_GC_H
