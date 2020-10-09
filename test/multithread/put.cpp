#include "../../include/put.h"
#include "../../include/get.h"
#include "../../include/remove.h"
#include "../sample.h"
#include <gtest/gtest.h>
#include <thread>

using namespace masstree;

class MultiPutTest: public ::testing::Test{};

/**
 * reader thread would observe whether old value
 * or new value, not holy mixture value.
 *
 * @see §4.6.1
 */
TEST(MultiPutTest, updates){
  Node* root = nullptr;
  Key k({1}, 1);
  root = put_at_layer0(root, k, new Value(1));

  constexpr size_t COUNT = 10000;
  auto w1 = [&root, &k](){
    for(size_t i = 0; i < COUNT; ++i){
      put_at_layer0(root, k, new Value(i));
    }
  };

  auto w2 = [&root, &k](){
    for(size_t i = 0; i < COUNT; ++i){
      auto val = get(root, k)->getBody();
      EXPECT_TRUE(0 <= val and val < COUNT);
    }
  };

  std::thread a(w1);
  std::thread b(w2);
  a.join();
  b.join();
}

TEST(MultiPutTest, border_inserts){
  Key k({1}, 1);
  auto root = put_at_layer0(nullptr, k, new Value(1));
}
