#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include "../../include/tree.h"

using namespace masstree;

class AtomicTest: public ::testing::Test{};


struct A{
  long long a;
  long long b;
  long long c;
  long long d;
  long long e;
  long long f;
  long long g;
  long long: 8;
  __int128 h;
  long long i;
  long long j;
  long long k;
  long long l;
  long long m;
  long long n;
  long long o;
};

long long a = 0;
alignas(64) long long b = 0;
alignas(16) __int128 c;

void verify(){
  static_assert(sizeof(__int128) == 16);
  static_assert(sizeof(a) == 8);
  static_assert(alignof(a) == 8);
  static_assert(sizeof(b) == 8);
  static_assert(alignof(b) == 64);
  static_assert(sizeof(A) == 144);
  printf("%lu\n", sizeof(A));
  static_assert(alignof(A) == 16);
}

void w1(){
  for(size_t i = 0; i < 100000000; ++i){
    c = i;
  }
}

void w2(){
  for(size_t i = 0; i < 100000000; ++i){
    auto cp = c;
    assert(0 <= cp and cp <= 100000000);
  }
}

TEST(AtomicTest, first){
  std::thread one(w1);
  std::thread two(w2);

  verify();

  one.join();
  two.join();


  std::array<std::atomic<Node*>, 16> g{};
  Node *temp_child[17] = {};

  for(size_t i =0 ;i < 9; ++i){
    temp_child[i] = g[i].load(std::memory_order_acquire);

  }

  g[9].store(temp_child[9], std::memory_order_release);

  /**
   * アライメントはとりあえず、「心配」はいらなさそう
   * 問題はatomicで適切なinterfaceの提供が怠そう
   * atomicにアクセスする必要があるものは、やはり型でラップしてあげたい。
   * 型のラップもダルいし…
   * かといって外に露出するのも…
   * 愚直に、全てのfieldをatomicにしてみると…？
   *
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

