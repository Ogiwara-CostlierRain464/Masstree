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
TEST(MultiPutTest, update1){
  GC gc{};
  Node* root = nullptr;
  Key k({1}, 1);
  root = put_at_layer0(root, k, new Value(1), gc).second;

  constexpr size_t COUNT = 100;
  auto w1 = [&root, &k](){
    GC gc{};
    for(size_t i = 0; i < COUNT; ++i){
      root = put_at_layer0(root, k, new Value(i), gc).second;
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

/**
 * No key rearrangement, and therefore no version increment, is required.
 * @see §4.6.2
 */
TEST(MultiPutTest, border_inserts1){
  for(size_t i = 0; i < 1000; ++i) {
    GC gc{};
    Key k({0}, 1);
    auto root = put_at_layer0(nullptr, k, new Value(0), gc).second;

    std::atomic_bool ready{false};
    auto w1 = [&ready, &root](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 15; ++i){
        Key k({i}, 1);
        root = put_at_layer0(root, k, new Value(i), gc).second;
      }
    };

    auto w2 = [&ready, &root](){
      while (!ready){ _mm_pause(); }

      for(size_t i = 0; i < 15; ++i){
        Key k({i}, 1);
        get(root, k);
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    ready.store(true);
    a.join();
    b.join();
  }
}

/**
 * when removed slots are reused, Masstree must update the v-insert field.
 *
 * @see §4.6.5
 */
TEST(MultiPutTest, border_inserts2){
  get_handler1.use([](){
    has_locked_marker.use([](){
      GC gc{};
      Key k0({0}, 1);
      Key k1({1}, 1);
      auto root = put_at_layer0(nullptr, k0, new Value(-1), gc).second;
      auto v1 = new Value(1);
      root = put_at_layer0(root, k1, v1, gc).second;

      auto w1 = [&root, &k1](){
        auto p = get(root, k1);
        EXPECT_TRUE(has_locked_marker.isMarked());
        EXPECT_TRUE(p == nullptr);
      };

      auto w2 = [&root, &k1, &v1](){
        get_handler1.waitGive();
        GC gc{};
        Key k2({2}, 2);
        root = remove_at_layer0(root, k1, gc);
        root = put_at_layer0(root, k2, new Value(2), gc).second;
        get_handler1.back();
        EXPECT_TRUE(gc.contain(v1));
      };

      std::thread a(w1);
      std::thread b(w2);
      a.join();
      b.join();
    });
  });
}

/**
 * getの実行中、別のスレッドがtree全体を削除するケース。
 */
TEST(MultiPutTest, border_inserts3){
  get_handler1.use([](){
    has_locked_marker.use([](){
      Key k1({1}, 1);
      GC gc{};
      auto root = put_at_layer0(nullptr, k1, new Value(1), gc).second;
      auto w1 = [&root, &k1](){
        auto p = get(root, k1);
        EXPECT_TRUE(has_locked_marker.isMarked());
        EXPECT_EQ(p, nullptr);
      };

      auto w2 = [&root, &k1](){
        get_handler1.waitGive();
        GC gc{};
        Key k2({2}, 2);
        root = remove_at_layer0(root, k1, gc);
        root = put_at_layer0(root, k2, new Value(2), gc).second;
        get_handler1.back();
      };

      std::thread a(w1);
      std::thread b(w2);
      a.join();
      b.join();
    });
  });
}

/**
 * putによってnew layerが作られるケース
 */
TEST(MultiPutTest, new_layers){

}