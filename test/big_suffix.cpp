#include <gtest/gtest.h>
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
    }, 2);

    suffix = BigSuffix::from(k, 1);
  }
  // suffixにコピー済み

  EXPECT_EQ(suffix->getCurrentSlice().slice, TWO);
  suffix->next();
  EXPECT_EQ(suffix->getCurrentSlice().slice, THREE);
  suffix->next();
  EXPECT_EQ(suffix->getCurrentSlice().slice, AB);
  EXPECT_EQ(suffix->getCurrentSlice().size, 2);
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
  }, 2);

  EXPECT_TRUE(suffix.isSame(k, 2));

  auto suffix1 = BigSuffix({
    TWO,
    AB
  }, 2);

  Key k1({
    TWO,
    AB
  }, 4);

  EXPECT_FALSE(suffix1.isSame(k1, 0));
}

TEST(BigSuffixTest, insertTop){
  BigSuffix a({
    ONE,
    TWO
  }, 8);

  a.insertTop(THREE);
  EXPECT_EQ(a.getCurrentSlice().slice, THREE);
  a.next();
  EXPECT_EQ(a.getCurrentSlice().slice, ONE);
  a.next();
  EXPECT_EQ(a.getCurrentSlice().slice, TWO);

}
