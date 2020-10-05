#include <gtest/gtest.h>
#include "../include/permutation.h"

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
  p.body = 0b0000'0001'0010'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0011;
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