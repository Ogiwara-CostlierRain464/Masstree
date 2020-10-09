#include <gtest/gtest.h>
#include <random>
#include "sample.h"
#include "../include/put.h"
#include "../include/get.h"
#include "../include/remove.h"
#include "../include/alloc.h"

using namespace masstree;

class LargeTest: public ::testing::Test{};

Key *make_key(){
  std::vector<KeySlice> vec{};
  size_t slices_len = (rand() % 2) + 1;
  for(size_t i = 1; i <= slices_len; ++i){
    auto slice = rand() % 10;
    vec.push_back(slice);
  }
  auto lastSize = (rand() % 8) + 1;
  return new Key(vec, lastSize);
}

static BorderNode *to_b(Node *n){
  return reinterpret_cast<BorderNode *>(n);
}

static InteriorNode *to_i(Node *n){
  return reinterpret_cast<InteriorNode *>(n);
}

TEST(LargeTest, DISABLED_put_get){
  auto seed = time(0);
  srand(seed);

  constexpr size_t COUNT = 100000;

  Node *root = nullptr;
  std::array<Key *, COUNT> inserted_keys{};

  for(size_t i = 0; i < COUNT; ++i){

    auto k = make_key();
    root = put_at_layer0(root, *k, new Value(k->lastSliceSize));

    k->reset();
    inserted_keys[i] = k;
  }

  for(int i = COUNT - 1; i >= 0; --i){
    auto k = inserted_keys[i];
    auto p = get(root, *k);
    EXPECT_EQ(*p, k->lastSliceSize);
    k->reset();
  }
}

TEST(LargeTest, DISABLED_put_get_remove){
  auto seed = time(0);
  srand(seed);

  constexpr size_t COUNT = 10000;
  GC gc{};

  for(size_t mm = 0; mm < COUNT; ++mm){
  Node *root = nullptr;
  std::array<Key*, COUNT> inserted_keys{};
  for(size_t i = 0; i < COUNT; ++i){
    auto k = make_key();
    root = put_at_layer0(root, *k, new Value(9));

    k->reset();
    inserted_keys[i] = k;
  }

  for(int i = COUNT - 1; i >= 0; --i){
    auto k = inserted_keys[i];
    auto p = get(root, *k);
    EXPECT_EQ(*p, 9);
    k->reset();
  }

  for(size_t i = 0; i < COUNT; ++i){
    auto k = inserted_keys[i];
    root = remove_at_layer0(root, *k, gc);
    delete k;
  }

  Alloc::print();
  Alloc::reset();
}



  // ポインタが4444とかになってる、つまり、何かが原因でkey_sliceの中のデータが流れてきてる！
  // 他のケースでは、途中のkey_lenが0になってる！　これはremoveかinsertの時に間をつめるのを忘れている！
  // いいえ、pointer being freed was not allocatedなので
  // 初期化の失敗？
  // n_keysのassertで防げそう
  // TODO: suffixのリセット失敗の原因

  // insert時のclearを忘れているのが原因?
  // remove時に、ちゃんとsuffix消してるか？？？
}

