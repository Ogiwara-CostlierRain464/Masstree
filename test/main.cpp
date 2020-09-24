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
}

int main(int argc, char **argv){

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}