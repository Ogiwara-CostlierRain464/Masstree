#include <gtest/gtest.h>
#include <atomic>
#include <bitset>

using namespace std;

class Unit: public ::testing::Test{};

TEST_F(Unit, init){
  std::bitset<64> a = 1;
  a = a << 56;
  EXPECT_EQ(a.to_ullong(), 72057594037927936);
}

int main(int argc, char **argv){

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}