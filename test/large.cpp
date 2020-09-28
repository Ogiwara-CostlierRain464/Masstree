#include <gtest/gtest.h>
#include <random>
#include "sample.h"
#include "../include/put.h"
#include "../include/get.h"
#include "../include/remove.h"

using namespace masstree;

class LargeTest: public ::testing::Test{};

KeySlice arr[] = {ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, AB, CD};

void make_key(Key *k){
  size_t slices_len = (rand() % 100)+1;
  for(size_t i = 1; i <= slices_len; ++i){
    auto slice = arr[rand() % 11];
    k->slices.push_back(slice);
  }
  k->length = slices_len * 8;
}

static BorderNode *to_b(Node *n){
  return reinterpret_cast<BorderNode *>(n);
}

static InteriorNode *to_i(Node *n){
  return reinterpret_cast<InteriorNode *>(n);
}


TEST(LargeTest, DISABLED_layer0){
  srand(time(0));

  constexpr size_t COUNT = 1000000;

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
    EXPECT_EQ(*reinterpret_cast<int *>(p), 9);
    k->reset();
  }

  for(size_t i = 0; i < COUNT - 1; ++i){
    auto k = inserted_keys[i];
    root = remove_at_layer0(root, *k);
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
