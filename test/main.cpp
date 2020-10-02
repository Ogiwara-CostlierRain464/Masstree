#include <gtest/gtest.h>

using namespace std;

class Unit: public ::testing::Test{};

// atomicにおいては、これは有効ではない、しっかりセッターゲッターを分ける必要性が出てくる。
class MyArray{
private:
  int *ptr;
  int size;

public:
  explicit MyArray(const int* p = nullptr, int s = 0){
    size = s;
    ptr = NULL;
    if (s != 0) {
      ptr = new int[s];
      for (int i = 0; i < s; i++)
        ptr[i] = p[i];
    }
  }

  int& operator[](std::size_t index){
    return ptr[index];
  }
};


TEST_F(Unit, init){

}

int main(int argc, char **argv){

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}