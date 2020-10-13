#include <gtest/gtest.h>
#include <random>
#include "sample.h"
#include "../include/put.h"
#include "../include/get.h"
#include "../include/remove.h"
#include "../include/alloc.h"
#include "helper.h"
#include <xmmintrin.h>
#include <thread>

using namespace masstree;

class LargeTest: public ::testing::Test{};

Key *make_1layer_key(){
  KeySlice slice = rand() % 3;
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
    root = put_at_layer0(root, *k, new Value(k->lastSliceSize), gc);

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
  auto seed = time(0);
  srand(seed);

  constexpr size_t COUNT = 100000;
  GC gc{};


  Node *root = nullptr;
  std::array<Key*, COUNT> inserted_keys{};
  for(size_t i = 0; i < COUNT; ++i){
    auto k = make_key();
    root = put_at_layer0(root, *k, new Value(k->lastSliceSize), gc);

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
        root = put_at_layer0(root, *k, new Value(k->lastSliceSize), gc);
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

TEST(LargeTest, multi_insert_border_layer0_test){
  auto seed = time(0);
  srand(seed);

  for(size_t i = 0; i < 14; ++i){

    Key k0({0}, 1);
    GC _{};
    std::atomic<Node*> root = put_at_layer0(nullptr, k0, new Value(0), _);
    std::atomic_bool ready{false};

    auto w1 = [&root, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 14; ++i){
        auto k = make_1layer_key();
        root = put_at_layer0(root, *k, new Value(k->remainLength(0)), gc);
      }
    };

    auto w2 = [&root, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 14; ++i){
        auto k = make_1layer_key();
        root = remove_at_layer0(root, *k, gc);
      }
    };

    std::thread a(w1);
    std::thread b(w2);
    ready = true;
    a.join();
    b.join();
  }
}

TEST(LargeTest, DISABLED_multi_insert_border_test){
  auto seed = 1602407569; // UNSTABLE errorが出るseed
  srand(seed);

  for(size_t i = 0; i < 1000000; ++i){

    std::atomic<Node*> root = nullptr;
    std::atomic_bool ready{false};

    auto w1 = [&root, &ready](){
      while (!ready){ _mm_pause(); }

      GC gc{};
      for(size_t i = 0; i < 15; ++i){
        auto k = make_key();
        root = put_at_layer0(root, *k, new Value(k->remainLength(0)), gc);
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
