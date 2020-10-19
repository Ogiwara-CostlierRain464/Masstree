#include <gtest/gtest.h>
#include <random>
#include "sample.h"
#include "../src/put.h"
#include "../src/get.h"
#include "../src/remove.h"
#include "../src/alloc.h"
#include "helper.h"
#include "../src/masstree.h"
#include <xmmintrin.h>
#include <thread>

using namespace masstree;

class LargeTest: public ::testing::Test{};

Key *make_1layer_key(){
  KeySlice slice = rand() % 3;
  auto size = (rand() % 8) + 1;
  return new Key({slice}, size);
}

Key *make_1layer_variable_key(){
  KeySlice slice = rand() % 100 + 1;
  auto size = (rand() % 8) + 1;
  return new Key({slice}, size);
}

Key *make_key(){
  std::vector<KeySlice> vec{};
  size_t slices_len = (rand() % 2) + 1;
  for(size_t i = 1; i <= slices_len; ++i){
    auto slice = rand() % 3;
    vec.push_back(slice);
  }
  auto lastSize = (rand() % 8) + 1;
  return new Key(vec, lastSize);
}

static BorderNode *to_b(Node *n){
  return reinterpret_cast<BorderNode *>(n);
}

static InteriorNode *to_i(Node *n){
  return reinterpret_cast<InteriorNode *>(n);
}

static BorderNodeSnap snap(BorderNode *borderNode){
  return BorderNodeSnap::from(borderNode);
}

TEST(LargeTest, DISABLED_put_get){
  auto seed = time(0);
  srand(seed);

  constexpr size_t COUNT = 100000;

  Node *root = nullptr;
  GC gc{};
  std::array<Key *, COUNT> inserted_keys{};

  for(size_t i = 0; i < COUNT; ++i){

    auto k = make_key();
    root = put_at_layer0(root, *k, new Value(k->lastSliceSize), gc).second;

    k->reset();
    inserted_keys[i] = k;
  }

  for(int i = COUNT - 1; i >= 0; --i){
    auto k = inserted_keys[i];
    auto p = get(root, *k);
    EXPECT_EQ(*p, k->lastSliceSize);
    k->reset();
  }
}

TEST(LargeTest, DISABLED_put_get_remove){
  auto seed = time(nullptr);
  srand(seed);

  constexpr size_t COUNT = 50000;
  GC gc{};


  Node *root = nullptr;
  std::array<Key*, COUNT> inserted_keys{};
  for(size_t i = 0; i < COUNT; ++i){
    auto k = make_key();
    root = put_at_layer0(root, *k, new Value(k->lastSliceSize), gc).second;

    k->reset();
    inserted_keys[i] = k;
  }

  for(int i = COUNT - 1; i >= 0; --i){
    auto k = inserted_keys[i];
    auto p = get(root, *k);
    EXPECT_EQ(*p, k->lastSliceSize);
    k->reset();
  }

  for(size_t i = 0; i < COUNT; ++i){
    auto k = inserted_keys[i];
    root = remove_at_layer0(root, *k, gc);
    delete k;
  }

  gc.run();

  Alloc::print();
  Alloc::reset();
  printf("Seed was %ld\n", seed);
}

TEST(LargeTest, DISABLED_random_op){
  auto seed = time(0);
  srand(seed);

  constexpr size_t COUNT = 10000;
  GC gc{};

  Node *root = nullptr;
  for(size_t i = 0; i < COUNT; ++i){
    auto k = make_key();
    auto op = rand() % 2;
    switch (op) {
      case 0: {
        get(root, *k);
      }
      case 1: {
        root = put_at_layer0(root, *k, new Value(k->lastSliceSize), gc).second;
      }
      default: {
        root = remove_at_layer0(root, *k, gc);
      }
    }
    delete k;
  }

  gc.run();

  Alloc::print();
  Alloc::reset();
}


TEST(LargeTest, DISABLED_multi_put_remove_layer0_test){
  auto seed = time(nullptr);
  srand(seed);

  for(size_t i = 0; i < 10000; ++i){

    Masstree tree{};
    Key k0({0}, 1);
    GC _{};
    tree.put(k0, new Value(0), _);
    std::atomic_bool ready{false};

    auto w1 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 14; ++i){
        auto k = make_1layer_key();
        tree.put(*k, new Value(i), gc);
      }
    };

    auto w2 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 14; ++i){
        auto k = make_1layer_key();
        tree.remove(*k, gc);
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    ready = true;
    a.join();
    b.join();
  }
}

TEST(LargeTest, DISABLED_multi_remove_remove_layer0_test){
  auto seed = time(nullptr);
  srand(seed);

  for(size_t i = 0; i < 10000; ++i){

    Masstree tree{};
    Key k0({0}, 1);
    GC _{};
    tree.put(k0, new Value(0), _);
    std::atomic_bool ready{false};

    auto w1 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 14; ++i){
        auto k = make_1layer_key();
        tree.remove(*k, gc);
      }
    };

    auto w2 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 14; ++i){
        auto k = make_1layer_key();
        tree.remove(*k, gc);
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    ready = true;
    a.join();
    b.join();
  }
}

TEST(LargeTest, DISABLED_multi_put_put_layer0_test){
  auto seed = time(nullptr);
  srand(seed);

  for(size_t i = 0; i < 1000; ++i){

    Masstree tree{};
    Key k0({0}, 1);
    GC _{};
    tree.put(k0, new Value(0), _);
    std::atomic_bool ready{false};

    auto w1 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 8; ++i){
        auto k = make_1layer_key();
        tree.put(*k, new Value(i), gc);
      }
    };

    auto w2 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 7; ++i){
        auto k = make_1layer_key();
        tree.put(*k, new Value(i), gc);
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    ready = true;
    a.join();
    b.join();

    // check no doubling
    EXPECT_TRUE(true);
  }
}

TEST(LargeTest, DISABLED_multi_put_remove_remove_layer0_test){
  auto seed = time(nullptr);
  srand(seed);

  for(size_t i = 0; i < 10000; ++i){

    Masstree tree{};
    Key k0({0}, 1);
    GC _{};
    tree.put(k0, new Value(0), _);
    std::atomic_bool ready{false};

    auto w1 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 14; ++i){
        auto k = make_1layer_key();
        tree.put(*k, new Value(i), gc);
      }
    };

    auto w2 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 7; ++i){
        auto k = make_1layer_key();
        tree.remove(*k, gc);
      }
    };

    auto w3 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 7; ++i){
        auto k = make_1layer_key();
        tree.remove(*k, gc);
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    std::thread c(w3);
    ready = true;
    a.join();
    b.join();
    c.join();
  }
}



TEST(LargeTest, DISABLED_multi_new_layer_put_get){
  auto seed = time(nullptr); // UNSTABLE errorが出るseed
  srand(seed);

  for(size_t i = 0; i < 10000; ++i){

    std::atomic<Node*> root = nullptr;
    std::atomic_bool ready{false};

    auto w1 = [&root, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 15; ++i){
        auto k = make_key();
        root = put_at_layer0(root, *k, new Value(k->remainLength(0)), gc).second;
      }
    };

    auto w2 = [&root, &ready](){
      while (!ready){ _mm_pause(); }

      for(size_t i = 0; i < 15; ++i){
        auto k = make_key();
        get(root, *k);
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    ready = true;
    a.join();
    b.join();
  }
}

TEST(LargeTest, DISABLED_multi_new_layer_put_remove){
  auto seed = time(nullptr);
  srand(seed);

  for(size_t i = 0; i < 100000; ++i){

    Masstree tree{};
    Key k0({0}, 1);
    GC _{};
    tree.put(k0, new Value(0), _);
    std::atomic_bool ready{false};

    auto w1 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 14; ++i){
        auto k = make_key();
        tree.put(*k, new Value(i) ,gc);
      }
    };

    auto w2 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 14; ++i){
        auto k = make_key();
        tree.remove(*k, gc);
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    ready = true;
    a.join();
    b.join();
  }
}

TEST(LargeTest, DISABLED_multi_new_layer_put_remove_get){
  auto seed = time(nullptr);
  srand(seed);

  for(size_t i = 0; i < 50000; ++i){

    Masstree tree{};
    Key k0({0}, 1);
    GC _{};
    tree.put(k0, new Value(0), _);
    std::atomic_bool ready{false};

    auto w1 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 14; ++i){
        auto k = make_key();
        tree.put(*k, new Value(i) ,gc);
      }
    };

    auto w2 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 14; ++i){
        auto k = make_key();
        tree.remove(*k, gc);
      }
    };

    auto w3 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 14; ++i){
        auto k = make_key();
        tree.get(*k);
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    std::thread c(w3);
    ready = true;
    a.join();
    b.join();
    c.join();
  }
}

TEST(LargeTest, multi_layer0_put_get){
  for(size_t i = 0; i < 10000; ++i){

    Masstree tree{};
    Key k0({0}, 1);
    GC _{};
    tree.put(k0, new Value(0), _);
    std::atomic_bool ready{false};

    auto w1 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 30; ++i){
        auto k = make_1layer_variable_key();
        tree.put(*k, new Value(i) ,gc);
      }
    };

    auto w2 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 30; ++i){
        auto k = make_1layer_variable_key();
        tree.get(*k);
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    ready = true;
    a.join();
    b.join();
  }
}

TEST(LargeTest, multi_layer0_put_remove){
  auto seed = time(nullptr);
  srand(seed);
  for(size_t i = 0; i < 5000; ++i){

    Masstree tree{};
    Key k0({0}, 1);
    GC _{};
    tree.put(k0, new Value(0), _);
    std::atomic_bool ready{false};

    constexpr size_t COUNT = 100;

    std::array<Key*, COUNT> inserts{};

    auto w1 = [&tree, &ready, &inserts](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t j = 0; j < COUNT; ++j){
        auto k = make_1layer_variable_key();
        tree.put(*k, new Value(k->lastSliceSize) ,gc);
        inserts[j] = k;
      }
    };

    auto w2 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t j = 0; j < COUNT; ++j){
        auto k = make_1layer_variable_key();
        tree.remove(*k, gc);
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    ready = true;
    a.join();
    b.join();

    for(size_t j = 0; j < COUNT; ++j){
      auto k = inserts[j];
      auto p = tree.get(*k);
      if(p != nullptr){
        ASSERT_EQ(p->getBody(), k->lastSliceSize);
      }
    }

  }
}

TEST(LargeTest, multi_layer0_put_put){
  auto seed = time(nullptr);
  srand(seed);
  for(size_t i = 0; i < 5000; ++i){

    Masstree tree{};
    Key k0({0}, 1);
    GC _{};
    tree.put(k0, new Value(0), _);
    std::atomic_bool ready{false};

    constexpr size_t COUNT = 100;

    std::array<Key*, COUNT> inserts{};

    auto w1 = [&tree, &ready, &inserts](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t j = 0; j < COUNT; ++j){
        auto k = make_1layer_variable_key();
        tree.put(*k, new Value(k->lastSliceSize) ,gc);
        inserts[j] = k;
      }
    };

    auto w2 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t j = 0; j < COUNT; ++j){
        auto k = make_1layer_variable_key();
        tree.put(*k, new Value(k->lastSliceSize) ,gc);
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    ready = true;
    a.join();
    b.join();

    for(size_t j = 0; j < COUNT; ++j){
      auto k = inserts[j];
      auto p = tree.get(*k);
      if(p != nullptr){
        ASSERT_EQ(p->getBody(), k->lastSliceSize);
      }
    }

  }
}

TEST(LargeTest, DISABLED_multi_layer0_remove_remove){
  auto seed = time(nullptr);
  srand(seed);
  for(size_t i = 0; i < 5000; ++i){

    Masstree tree{};
    Key k0({0}, 1);
    GC _{};
    tree.put(k0, new Value(0), _);
    std::atomic_bool ready{false};

    constexpr size_t COUNT = 100;

    std::array<Key*, COUNT> inserts{};

    auto w1 = [&tree, &ready, &inserts](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t j = 0; j < COUNT; ++j){
        auto k = make_1layer_variable_key();
        tree.put(*k, new Value(k->lastSliceSize) ,gc);
        inserts[j] = k;
      }
    };

    auto w2 = [&tree, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t j = 0; j < COUNT; ++j){
        auto k = make_1layer_variable_key();
        tree.put(*k, new Value(k->lastSliceSize) ,gc);
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    ready = true;
    a.join();
    b.join();

    for(size_t j = 0; j < COUNT; ++j){
      auto k = inserts[j];
      auto p = tree.get(*k);
      if(p != nullptr){
        ASSERT_EQ(p->getBody(), k->lastSliceSize);
      }
    }

  }
}

TEST(LargeTest, DISABLED_layer0_put){
  auto seed = time(nullptr);
  srand(seed);
  for(size_t i = 0; i < 5000; ++i){

    Masstree tree{};
    Key k0({0}, 1);
    GC gc{};
    tree.put(k0, new Value(0), gc);
    std::atomic_bool ready{false};

    constexpr size_t COUNT = 200;

    std::array<Key*, COUNT> inserts{};

    for(size_t j = 0; j < COUNT; ++j){
      auto k = make_1layer_variable_key();
      tree.put(*k, new Value(k->lastSliceSize), gc);
      inserts[j] = k;
    }

    for(size_t j = 0; j < COUNT; ++j){
      auto k = inserts[j];
      auto p = tree.get(*k);
      if(p != nullptr){
        ASSERT_EQ(p->getBody(), k->lastSliceSize);
      }
    }

  }
}