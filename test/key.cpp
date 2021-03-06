#include <gtest/gtest.h>
#include "sample.h"

using namespace masstree;

class KeyTest: public ::testing::Test{};

TEST(KeyTest, remainLength){
Key k({
  ONE,
  TWO,
  FIVE,
  THREE,
  FOUR,
  AB
  }, 2);

EXPECT_EQ(k.remainLength(3), 18);
}

TEST(KeyTest, equal){
  Key a({
  TWO,
  ONE
  },7);
  Key b({
    TWO,
    ONE
  },8);

  EXPECT_FALSE(a == b);
}

TEST(KeyTest, lastSliceSize){
  Key a({
    TWO,
    ONE
    },7);


  EXPECT_EQ(a.lastSliceSize, 7);
}

TEST(KeyTest, getCurrentSlice){
  Key a({
    TWO,
    EIGHT,
    ONE
    },4);

  a.next();
  EXPECT_EQ(a.getCurrentSlice().slice, EIGHT);
  a.next();
  EXPECT_EQ(a.getCurrentSlice().size, 4);
}
