#include <gtest/gtest.h>
#include <atomic>

using namespace std;

class Unit: public ::testing::Test{};

TEST_F(Unit, init){
  std::atomic<int *> a{nullptr};
  a.store(new int(8), std::memory_order_acquire);
  EXPECT_EQ(*a.load(std::memory_order_acquire), 8);
}

int main(int argc, char **argv){

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}