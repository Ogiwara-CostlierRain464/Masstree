#include "put.h"
#include "get.h"
#include "remove.h"
#include "masstree.h"

using namespace masstree;

Key *make_some_key(){
  std::vector<KeySlice> vec{};
  size_t slices_len = (rand() % 10) + 1;
  for(size_t i = 1; i <= slices_len; ++i){
    auto slice = rand() % 50;
    vec.push_back(slice);
  }
  auto lastSize = (rand() % 8) + 1;
  return new Key(vec, lastSize);
}

int main(){
  auto seed = time(nullptr);
  srand(seed);
  for(size_t i = 0; i < 10000; ++i){

    Masstree tree{};
    Key k0({0}, 1);
    GC _{};
    tree.put(k0, new Value(0), _);
    std::atomic_bool ready{false};

    constexpr size_t COUNT = 100;

    std::array<Key*, COUNT * 2> inserts{};

    auto w1 = [&tree, &ready, &inserts](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t j = 0; j < COUNT; ++j){
        auto k = make_some_key();
        tree.put(*k, new Value(k->lastSliceSize) ,gc);
        inserts[j] = k;
      }
    };

    auto w0 = [&tree, &ready, &inserts](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t j = COUNT; j < COUNT*2; ++j){
        auto k = make_some_key();
        tree.put(*k, new Value(k->lastSliceSize) ,gc);
        inserts[j] = k;
      }
    };

    auto w2 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t j = 0; j < COUNT; ++j){
        auto k = make_some_key();
        tree.remove(*k, gc);
      }
    };

    auto w3 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t j = 0; j < COUNT; ++j){
        auto k = make_some_key();
        tree.remove(*k, gc);
      }
    };

    auto w4 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t j = 0; j < COUNT; ++j){
        auto k = make_some_key();
        tree.get(*k);
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    std::thread c(w3);
    std::thread d(w4);
    std::thread e(w0);
    ready = true;
    a.join();
    b.join();
    c.join();
    d.join();
    e.join();

    for(size_t j = 0; j < COUNT * 2; ++j){
      auto k = inserts[j];
      auto p = tree.get(*k);
      if(p != nullptr){
        if(p->getBody() != k->lastSliceSize){
          exit(-1);
        }
      }
    }
  }

  return 0;
}

