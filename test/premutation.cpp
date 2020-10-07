#include <gtest/gtest.h>
#include "../include/tree.h"
#include "sample.h"

using namespace masstree;

class PermutationTest: public ::testing::Test{};

TEST(PermutationTest, getNumKeys){
  Permutation p{};
  p.body = 0b0000'0001'0010'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0011;
  auto num = p.getNumKeys();
  EXPECT_EQ(num, 3);
}

TEST(PermutationTest, setNumKeys){
  Permutation p{};
  p.body = 0b1011'0001'0010'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0011;
  p.setNumKeys(4);
  EXPECT_EQ(p.getNumKeys(), 4);
  EXPECT_EQ(p.getKeyIndex(1), 1);
}

TEST(PermutationTest, getKeyIndex){
  Permutation p{};
  // [3|1|2| ... |3]
  p.body = 0b0011'0001'0010'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0011;
  EXPECT_EQ(p.getNumKeys(), 3);
  EXPECT_EQ(p.getKeyIndex(0), 3);
  EXPECT_EQ(p.getKeyIndex(1), 1);
  EXPECT_EQ(p.getKeyIndex(2), 2);

  p.setKeyIndex(1,4);
  p.setKeyIndex(2,1);
  EXPECT_EQ(p.getNumKeys(), 3);
  EXPECT_EQ(p.getKeyIndex(0), 3);
  EXPECT_EQ(p.getKeyIndex(1), 4);
  EXPECT_EQ(p.getKeyIndex(2), 1);
}

TEST(PermutationTest, util){
  Permutation p{};
  p.setNumKeys(1);
  p.setKeyIndex(0, 3);
  EXPECT_EQ(p(0), 3);
}

TEST(PermutationTest, extractLinkOrValueWithIndexFor){

  BorderNode n;
  skipped_border(n);

  Key k({1}, 2);
  auto tuple = n.extractLinkOrValueWithIndexFor(k);
  EXPECT_EQ(std::get<2>(tuple), 4);
}

TEST(PermutationTest, firstUnusedSlotIndex){

  BorderNode n;
  skipped_border(n);
  EXPECT_EQ(n.firstUnusedSlotIndex(), 2);
}

TEST(PermutationTest, insert){
  Permutation p{};
  p.setKeyIndex(0, 3);
  p.setKeyIndex(1, 4);
  p.setKeyIndex(2, 5);
  p.setKeyIndex(3, 0);
  p.setKeyIndex(4, 1);
  p.setNumKeys(5);

  p.insert(3, 2);
  EXPECT_EQ(p.getKeyIndex(3), 2);
  EXPECT_EQ(p.getKeyIndex(4), 0);
  EXPECT_EQ(p.getKeyIndex(5), 1);
}



