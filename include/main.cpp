#include "put.h"
#include "get.h"
#include "remove.h"

using namespace masstree;

Key *make_1layer_key(){
  KeySlice slice = rand() % 3;
  auto size = (rand() % 8) + 1;
  return new Key({slice}, size);
}


int main(){
  auto seed = time(0);
  srand(seed);

  for(size_t i = 0; i < 100000; ++i){

    std::atomic<Node*> root = nullptr;
    std::atomic_bool ready{false};

    auto w1 = [&root, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 15; ++i){
        auto k = make_1layer_key();
        root = put_at_layer0(root, *k, new Value(k->remainLength(0)), gc);
      }
    };

    auto w2 = [&root, &ready](){
      while (!ready){ _mm_pause(); }

      for(size_t i = 0; i < 15; ++i){
        auto k = make_1layer_key();
        auto p = get(root, *k);
        if(p != nullptr){
          auto body = p->getBody();
          assert(1 <= body and body <= 8);
        }
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    ready = true;
    a.join();
    b.join();
  }

  return 0;
}

