#include "put.h"
#include "get.h"
#include "remove.h"
#include <thread>

using namespace masstree;

KeySlice arr[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12};

Key *make_key(){
  std::vector<KeySlice> vec{};
  size_t slices_len = (rand() % 1)+3;
  for(size_t i = 1; i <= slices_len; ++i){
    auto slice = arr[rand() % 2];
    vec.push_back(slice);
  }
  auto length = (slices_len-1) * 8 + (rand() % 8) + 1;
  return new Key(vec, length);
}

void w1(){
  auto seed = time(0);
  srand(seed);

  constexpr size_t COUNT = 50000;

  Node *root = nullptr;
  std::array<Key*, COUNT> inserted_keys{};
  for(size_t i = 0; i < COUNT; ++i){
    auto k = make_key();
    root = put_at_layer0(root, *k, new int(k->length));

    k->reset();
    inserted_keys[i] = k;
  }

  for(int i = COUNT - 1; i >= 0; --i){
    auto k = inserted_keys[i];
    auto p = get(root, *k);
    assert(*reinterpret_cast<int *>(p) == k->length);
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

void loop(){
  for(;;){
   w1();
  }
}


int main(){
  std::thread a(loop);

  a.join();

  // interiorがちゃんとカウントできてないぽい！keyの生成が悪かった？次回ちゃんとテスト
}

