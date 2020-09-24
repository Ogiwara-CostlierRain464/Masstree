#include <gtest/gtest.h>
#include <random>
#include "../include/operation.h"
#include "sample.h"

using namespace masstree;

class LargeTest: public ::testing::Test{};

KeySlice arr[] = {ONE, TWO, THREE, FOUR, FIVE};

void make_key(Key &k){
  size_t slices_len = (rand() % 50)+1;
  for(size_t i = 1; i <= slices_len; ++i){
    auto slice = arr[rand() % 5];
    k.slices.push_back(slice);
  }
  k.length = slices_len * 8;
}

static BorderNode *to_b(Node *n){
  return reinterpret_cast<BorderNode *>(n);
}

static InteriorNode *to_i(Node *n){
  return reinterpret_cast<InteriorNode *>(n);
}


TEST(LargeTest, layer0){
  Node *root = nullptr;
  std::array<Key, 10000> inserted_keys{};
  for(size_t i = 0; i < 10000; ++i){
    Key k({},0);
    make_key(k);
    root = put_layer0(root, k, new int(9));

    k.cursor = 0;
    auto p = get(root, k);
    ASSERT_TRUE(p != nullptr);
    EXPECT_EQ(*reinterpret_cast<int *>(p), 9);

    inserted_keys[i] = k;
  }
//  for(size_t i = 0; i < 10000; ++i){
//    Key k({},0);
//    make_key(k);
//    root = remove_at_layer0(root, k);
//  }

  // insert時のclearを忘れているのが原因
  // remove時に、ちゃんとsuffix消してるか？？？
  // 255のテスト足せ
}

