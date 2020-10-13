#include "../../include/put.h"
#include "../../include/get.h"
#include "../../include/remove.h"
#include "../sample.h"
#include <gtest/gtest.h>
#include <thread>

using namespace masstree;

class MultiRemoveTest: public ::testing::Test{};

/**
 * getとremoveが同時に走る場合
 * Masstreeのポリシーは高々no lost keyなので、並行で削除されたKeyのvalueを返しても良い
 */
TEST(MutiRemoveTest, get_and_remove_at_root_border_layer0){
  get_handler1.use([](){
    Key k0({0}, 1);
    Key k1({1}, 1);
    GC un_use{};
    auto root = put_at_layer0(nullptr, k0, new Value(0), un_use);
    root = put_at_layer0(root, k1, new Value(1), un_use);
    auto w1 = [root, k1]()mutable{
      auto p = get(root, k1);
      EXPECT_EQ(p->getBody(), 1);
    };
    auto w2 = [root, k1]()mutable{
      get_handler1.waitGive();
      GC gc{};
      root = remove_at_layer0(root, k1, gc);
      get_handler1.back();
    };

    std::thread a(w1);
    std::thread b(w2);
    a.join();
    b.join();
  });
}

/**
 * putとremoveが同時に走る場合
 * write-write conflictなので、lockで対処する。
 */
TEST(MutiRemoveTest, put_and_remove_at_root_border_layer0){
  Key k0({0} , 1);
  Key k1({1} , 1);
  GC _{};
  std::atomic<Node*> root = put_at_layer0(nullptr, k0, new Value(0), _);
  auto w1 = [&root, k1]()mutable{
    GC gc{};
    root = put_at_layer0(root, k1, new Value(1), gc);
  };
  auto w2 = [&root, k1]()mutable{
    GC gc{};
    root = remove_at_layer0(root, k1, gc);
  };

  std::thread a(w1);
  std::thread b(w2);
  a.join();
  b.join();
  auto p = get(root, k1);
  if(p != nullptr){
    EXPECT_EQ(p->getBody(), 1);
  }
}