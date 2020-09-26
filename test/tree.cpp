#include <gtest/gtest.h>
#include <random>
#include "../include/key.h"
#include "../include/tree.h"
#include "sample.h"
#include "../include/operation.h"

using namespace masstree;

class TreeTest: public ::testing::Test{
};


struct DummyNode: public Node{
  int value;

  explicit DummyNode(int v)
  : value(v)
  {}
};

TEST(TreeTest, findChild1){
  InteriorNode n;
  DummyNode dn{0};

  n.child[0] = &dn;
  n.n_keys = 1;
  n.key_slice[0] = 0x01;

  auto n1 = reinterpret_cast<DummyNode *>(n.findChild(0x00));
  EXPECT_EQ(n1->value, 0);
}

TEST(TreeTest, findChild2){
  /**
   * || 1 ||
   *
   * (0)  (2)
   */

  InteriorNode n;
  DummyNode dn0{0};
  DummyNode dn2{2};

  n.n_keys = 1;
  n.child[0] = &dn0;
  n.key_slice[0] = 0x01;
  n.child[1] = &dn2;

  auto n1 = reinterpret_cast<DummyNode *>(n.findChild(0x01));
  EXPECT_EQ(n1->value, 2);
}

TEST(TreeTest, sample2){
  auto root = sample2();
  Key key({0x0001020304050607, 0x0A0B'0000'0000'0000}, 10);

  auto b = findBorder(root, key);
  EXPECT_EQ(*reinterpret_cast<int *>(b.first->lv[0].value), 1);
}

TEST(TreeTest, sample3){
  auto root = sample3();
  Key key({0x0001020304050607, 0x0A0B'0000'0000'0000}, 10);

  auto b = findBorder(root, key);
  EXPECT_EQ(b.first->key_len[0], BorderNode::key_len_layer);
}

TEST(TreeTest, get1){
  auto root = sample2();
  Key key({0x0001020304050607, 0x0A0B'0000'0000'0000}, 10);

  auto p = get(root, key);

  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p), 1);
}

TEST(TreeTest, get2){
  auto root = sample3();
  Key key({0x0001020304050607, 0x0C0D'0000'0000'0000}, 10);

  auto p = get(root, key);

  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p), 2);
}

TEST(TreeTest, get3){
  auto root = sample4();
  Key key({0x0101}, 2);

  auto p = get(root, key);

  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p), 22);

  Key key2({0x010600}, 3);

  p = get(root, key2);
  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p), 320);
}

TEST(TreeTest, get4){
  auto root = sample4();
  Key key({ 0x0101 }, 2);

  root = put(root, key, new int(23), nullptr, 0);
  auto p = get(root, key);

  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p), 23);
}

TEST(TreeTest, start_new_tree){
  Key key({
    0x0102030405060708,
    0x0102030405060708,
    0x0102030405060708,
    0x1718190000000000
  }, 27);
  auto root = start_new_tree(key, new int(100));
  auto p = get(root, key);
  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p), 100);
}

TEST(TreeTest, insert){
  Key key1({
    0x0102030405060708,
    0x0A0B000000000000
  }, 10);
  Key key2({
    0x1112131415161718
  }, 8);
  auto root = put(nullptr, key1, new int(1), nullptr, 0);
  root = put(root, key2, new int(2), nullptr, 0);
  auto p = get(root, key1);
  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p), 1);
}

BorderNode *to_b(Node *n){
  return reinterpret_cast<BorderNode *>(n);
}

InteriorNode *to_i(Node *n){
  return reinterpret_cast<InteriorNode *>(n);
}

TEST(TreeTest, split){
  // work at B+ tree test.

  Node *root = nullptr;
  for(uint64_t i = 1; i <= 10000; ++i){
    Key k({i}, 1);
    root = put(root, k, new int(i), nullptr, 0);
  }

  print_sub_tree(root);

  Key k2341({2341},1);
  auto l = findBorder(root, k2341);
  auto p = get(root, k2341);
  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p),2341);
}

TEST(TreeTest, break_invariant){
  auto root = sample2();
  Key k({0x0001'0203'0405'0607, 0x0C0D'0000'0000'0000}, 10);
  // kがここで変わってしまう。
  put(root, k, new int(3), nullptr, 0);


  Key k2({0x0001'0203'0405'0607, 0x0C0D'0000'0000'0000}, 10);
  auto p = get(root, k2);
  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p),3);
}

TEST(TreeTest, break_invariant2){
  Node *root = nullptr;
  Key k({
    0x8888'8888'8888'8888,
    0x1111'1111'1111'1111,
    0x2222'2222'2222'2222,
    0x3333'3333'3333'3333,
    0x0A0B'0000'0000'0000
  },34);
  root = put(root, k, new int(1), nullptr, 0);
  Key k1({
    0x8888'8888'8888'8888,
    0x1111'1111'1111'1111,
    0x2222'2222'2222'2222,
    0x0C0D'0000'0000'0000
  },26);
  root = put(root, k1, new int(2), nullptr, 0);
  // Key is mutable, but you can reset cursor.
  k1.cursor = 0;
  auto p = get(root, k1);
  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p),2);

}

TEST(TreeTest, break_invariant3){
  auto pair = not_conflict_89();
  auto root = put(nullptr, pair.first, new int(1), nullptr, 0);
  root = put(root, pair.second, new int(7), nullptr, 0);
  pair.second.cursor = 0;
  auto p = get(root, pair.second);
  EXPECT_EQ(*reinterpret_cast<int *>(p), 7);
}

TEST(TreeTest, p){
  KeySlice slice = 0x0001020304050607;
  Key k1({slice}, 1);
  Key k2({slice}, 2);
  Key k3({slice}, 3);
  Key k4({slice}, 4);
  Key k5({slice}, 5);
  Key k6({slice}, 6);
  Key k7({slice}, 7);
  Key k8({slice}, 8);
  Key k9({slice, CD}, 10);

  auto root = put(nullptr, k1, new int(1), nullptr, 0);
  root = put(root, k2, new int(2), nullptr, 0);
  root = put(root, k3, new int(3), nullptr, 0);
  root = put(root, k4, new int(4), nullptr, 0);
  root = put(root, k5, new int(5), nullptr, 0);
  root = put(root, k6, new int(6), nullptr, 0);
  root = put(root, k7, new int(7), nullptr, 0);
  root = put(root, k9, new int(9), nullptr, 0);
  root = put(root, k8, new int(8), nullptr, 0);

  k8.cursor = 0;
  auto p = get(root, k8);
  EXPECT_EQ(*reinterpret_cast<int *>(p), 8);
}

TEST(TreeTest, duplex){
  Key k({
    EIGHT,
    EIGHT,
    CD
  },18);
  Key k1({
    EIGHT,
    AB
  },10);
  Key k2({
    EIGHT,
    EIGHT,
    CD
  },18);

  Node *root = nullptr;
  root = put(root, k, new int(4), nullptr, 0);
  root = put(root, k1, new int(8), nullptr, 0);
  root = put(root, k2, new int(6), nullptr, 0);
  k.cursor = 0;
  auto p = get(root, k);
  EXPECT_EQ(*reinterpret_cast<int *>(p), 6);
}

TEST(TreeTest, layer_change){
  Node *root = nullptr;
  Key k1({
    ONE, TWO, FIVE
  }, 24);
  root = put_at_layer0(root, k1, new int(5));
  Key k2({
    ONE, TWO, THREE, FOUR
  }, 32);
  root = put_at_layer0(root, k2, new int(4));

  k1.reset();
  auto p = get(root, k1);
  ASSERT_EQ(*reinterpret_cast<int *>(p), 5);

  k2.reset();
  root = remove_at_layer0(root, k2);

  k1.reset();
  auto p2 = get(root, k1);
  ASSERT_EQ(*reinterpret_cast<int *>(p2), 5);
}






