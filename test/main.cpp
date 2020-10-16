#include <gtest/gtest.h>
#include <atomic>
#include <bitset>

using namespace std;

class Unit: public ::testing::Test{};



TEST_F(Unit, binary){

}

TEST_F(Unit, init){
}




int main(int argc, char **argv){
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}