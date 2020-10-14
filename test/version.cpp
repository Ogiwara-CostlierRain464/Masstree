#include <gtest/gtest.h>
#include "../src/version.h"

using namespace masstree;

class VersionTest: public ::testing::Test{};

TEST(VersionTest, bit){
  auto num = 0b1000'0000'0000'0000'0000'0000'0000'0000;
  Version v;
  v.body = num;
  EXPECT_TRUE(v.locked);
}

TEST(VersionTest, has_locked){
  Version v{};
  auto before = v;
  v.locked = true;
  v.inserting = true;
  EXPECT_TRUE((before ^ v) > 0);
  v.locked = false;
  v.inserting = false;
  ++v.v_insert;
  EXPECT_TRUE((before ^ v) > 0);
}

