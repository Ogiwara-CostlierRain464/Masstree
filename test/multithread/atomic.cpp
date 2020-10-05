#include <gtest/gtest.h>
#include <thread>
#include <atomic>


class AtomicTest: public ::testing::Test{};

std::atomic<bool> x = false;
bool y = false;
int z = 0;

void w1(){
  x.store(true, std::memory_order_relaxed);
  __atomic_store_n(&y, true, __ATOMIC_RELEASE);
}

void w2(){
  while(!__atomic_load_n(&y, __ATOMIC_ACQUIRE));
  if(x.load(std::memory_order_relaxed))
    ++z;
}

TEST(AtomicTest, first){
  std::thread a(w1);
  std::thread b(w2);
  a.join();
  b.join();
  EXPECT_TRUE(z != 0);



  /**
   * その上で、fieldを作ってあげる
   * ポインタの露出は問題ない
   * 問題なのは、各nodeのfieldに複数スレッドが同時にアクセス
   *
   * fieldをatomicで保護し、ゲッターセッターを増設する？
   *
   * あと、アライメント、処理の共通化、とかも一応
   *
   * ローカル変数と共有領域をちゃんと識別
   * 共有領域は、全てatomicで明示的に保護する
   * 必ず型か何かでラップする
   *
   * テスト方法としては、とりあえずroot　borderに
   * 対しての出し入れのテストを行う
   *
   * そのあとで、やっとpermutationをかく。
   */
}

