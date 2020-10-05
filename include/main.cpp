#include "put.h"
#include "get.h"
#include "remove.h"

using namespace masstree;

Key *make_key(){
  std::vector<KeySlice> vec{};
  size_t slices_len = 1;
  for(size_t i = 1; i <= slices_len; ++i){
    auto slice = rand() % 10;
    vec.push_back(slice);
  }
  auto lastSize = (rand() % 8) + 1;
  return new Key(vec, lastSize);
}

int main(){
  for(;;){
    auto seed = time(0);
    srand(seed);

    constexpr size_t COUNT = 100000;

    Node *root = nullptr;
    std::array<Key*, COUNT> inserted_keys{};
    for(size_t i = 0; i < COUNT; ++i){
      auto k = make_key();
      root = put_at_layer0(root, *k, new Value(k->remainLength(0)));

      k->reset();
      inserted_keys[i] = k;
    }

    for(int i = COUNT - 1; i >= 0; --i){
      auto k = inserted_keys[i];
      auto p = get(root, *k);
//      assert(*reinterpret_cast<int *>(p) == k->length);
      if(*p != k->remainLength(0)){
        exit(-1);
      }

      k->reset();
    }

    for(size_t i = 0; i < COUNT; ++i){
      auto k = inserted_keys[i];
      root = remove_at_layer0(root, *k);
      delete k;
    }

    Alloc::print();
    Alloc::reset();
  }

  return 0;
}

