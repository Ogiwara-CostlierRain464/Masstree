#include <gtest/gtest.h>
#include "../include/key.h"

using namespace masstree;

class KeyTest: public ::testing::Test{
};

TEST(KeyTest, equal){
  Key a({0x01},1);
  Key b({0x01},1);

  EXPECT_TRUE(a == b);
}
