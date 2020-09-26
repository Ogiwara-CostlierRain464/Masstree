#include <gtest/gtest.h>
#include "../include/operation.h"
#include "sample.h"

using namespace masstree;



class BigSuffixTest: public ::testing::Test{};


TEST(BigSuffixTest, from){
  BigSuffix *suffix;
  {
    Key k({
      ONE,
      TWO,
      THREE,
      AB
    }, 26);

    suffix = BigSuffix::from(k, 1);
  }
  // suffixにコピー済み

  EXPECT_EQ(suffix->slices[0], TWO);
  EXPECT_EQ(suffix->slices[1], THREE);
  EXPECT_EQ(suffix->slices[2], AB);
  EXPECT_EQ(suffix->lastSliceSize, 2);

  suffix->next();
  suffix->next();
  EXPECT_EQ(suffix->getCurrentSlice().slice, AB);
}


TEST(BigSuffixTest, isSame){
  auto suffix = BigSuffix({
    TWO,
    ONE,
    AB
  }, 2);

  Key k({
    EIGHT,
    NINE,
    TWO,
    ONE,
    AB
  }, 34);

  EXPECT_TRUE(suffix.isSame(k, 2));

  auto suffix1 = BigSuffix({
    TWO,
    AB
  }, 2);

  Key k1({
    TWO,
    AB
  }, 12);

  EXPECT_FALSE(suffix1.isSame(k1, 0));
}
