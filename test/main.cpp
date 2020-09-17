#include <gtest/gtest.h>

using namespace std;

class Unit: public ::testing::Test{};

template<typename T>
void pop_front(std::vector<T> &v)
{
  if (!v.empty()) {
    v.erase(v.begin());
  }
}

TEST_F(Unit, init){
  vector<int> a = {1,2,3,4};
  EXPECT_EQ(a.front(), 1);
  pop_front(a);
  EXPECT_EQ(a.front(), 2);
  pop_front(a);
  EXPECT_EQ(a.front(), 3);
}

int main(int argc, char **argv){

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}