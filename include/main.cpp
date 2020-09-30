#include "put.h"
#include "get.h"
#include "remove.h"

using namespace masstree;

KeySlice arr[] = {0x01, 0x02, 0x03, 0x04, 0x05};

void make_key(Key *k){
  size_t slices_len = (rand() % 100)+10;
  for(size_t i = 1; i <= slices_len; ++i){
    auto slice = arr[rand() % 5];
    k->slices.push_back(slice);
  }
  k->length = slices_len * 8;
}


int main(){
  auto seed = time(0);
  srand(seed);

  constexpr size_t COUNT = 10000;

  Node *root = nullptr;
  std::array<Key*, COUNT> inserted_keys{};
  for(size_t i = 0; i < COUNT; ++i){
    auto k = new Key;
    make_key(k);
    root = put_at_layer0(root, *k, new int(9));

    k->reset();
    inserted_keys[i] = k;
  }

  for(int i = COUNT - 1; i >= 0; --i){
    auto k = inserted_keys[i];
    auto p = get(root, *k);
    assert(*reinterpret_cast<int *>(p) == 9);
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

