#include <gtest/gtest.h>
#include <atomic>
#include <bitset>
#include <random>
#include "../src/tree.h"

using namespace std;
using namespace masstree;

class Unit: public ::testing::Test{};



TEST_F(Unit, binary){
 InteriorNode n{};
 n.setNumKeys(3);
 n.decNumKeys();
 EXPECT_TRUE(n.getNumKeys() == 2);
}

TEST_F(Unit, init){
}




int main(int argc, char **argv){
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}