#include <gtest/gtest.h>
#include "../src/tree.h"
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

  auto n = new BorderNode;
  skipped_border(n);

  Key k({1}, 2);
  auto tuple = n->extractLinkOrValueWithIndexFor(k);
  EXPECT_EQ(std::get<2>(tuple), 4);
}

TEST(PermutationTest, insert){
  auto p = Permutation::from({3,4,5,0,1});

  p.insert(3, 2);
  EXPECT_EQ(p(3), 2);
  EXPECT_EQ(p(4), 0);
  EXPECT_EQ(p(5), 1);
}

TEST(PermutationTest, removeIndex){
  auto p = Permutation::from({3,4,5,0,1});
  p.removeIndex(3);
  p.removeIndex(5);
  EXPECT_EQ(p(0), 4);
  EXPECT_EQ(p(1), 0);
  EXPECT_EQ(p.getNumKeys(), 3);
}


