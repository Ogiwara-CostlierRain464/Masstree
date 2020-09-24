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

TEST(LargeTest, layer0){
  Node *root = nullptr;
  for(size_t i = 0; i < 10000; ++i){
    Key k({},0);
    make_key(k);
    root = put_layer0(root, k, new int(9));
  }
  for(size_t i = 0; i < 10000; ++i){
    Key k({},0);
    make_key(k);
    root = remove_at_layer0(root, k);
  }
}

