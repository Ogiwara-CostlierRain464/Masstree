#include <gtest/gtest.h>
#include <random>
#include "../include/key.h"
#include "../include/tree.h"
#include "sample.h"
#include "../include/put.h"
#include "../include/get.h"
#include "../include/remove.h"

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

  n.setChild(0, &dn);
  n.setNumKeys(1);
  n.setKeySlice(0, 0x01);

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

  n.setNumKeys(1);
  n.setChild(0, &dn0);
  n.setKeySlice(0, 0x01);
  n.setChild(1, &dn2);

  auto n1 = reinterpret_cast<DummyNode *>(n.findChild(0x01));
  EXPECT_EQ(n1->value, 2);
}

TEST(TreeTest, sample2){
  auto root = sample2();
  Key key({0x0001020304050607, 0x0A0B'0000'0000'0000}, 2);

  auto b = findBorder(root, key);
  EXPECT_EQ(*b.first->getLV(0).value, 1);
}

TEST(TreeTest, sample3){
  auto root = sample3();
  Key key({0x0001020304050607, 0x0A0B'0000'0000'0000}, 2);

  auto b = findBorder(root, key);
  EXPECT_EQ(b.first->getKeyLen(0), BorderNode::key_len_layer);
}

TEST(TreeTest, get1){
  auto root = sample2();
  Key key({0x0001020304050607, 0x0A0B'0000'0000'0000}, 2);

  auto p = get(root, key);

  assert(p != nullptr);
  EXPECT_EQ(*p, 1);
}

TEST(TreeTest, get2){
  auto root = sample3();
  Key key({0x0001020304050607, 0x0C0D'0000'0000'0000}, 2);

  auto p = get(root, key);

  ASSERT_TRUE(p != nullptr);
  EXPECT_EQ(*p, 2);
}

TEST(TreeTest, get3){
  auto root = sample4();
  Key key({0x0101}, 2);

  auto p = get(root, key);

  ASSERT_TRUE(p != nullptr);
  EXPECT_EQ(*p, 22);

  Key key2({0x010600}, 3);

  p = get(root, key2);
  assert(p != nullptr);
  EXPECT_EQ(*p, 320);
}

TEST(TreeTest, get4){
  auto root = sample4();
  Key key({ 0x0101 }, 2);
  GC gc{};

  root = put(root, key, new Value(23), nullptr, 0, gc);
  auto p = get(root, key);

  assert(p != nullptr);
  EXPECT_EQ(*p, 23);
}

TEST(TreeTest, start_new_tree){
  Key key({
    0x0102030405060708,
    0x0102030405060708,
    0x0102030405060708,
    0x1718190000000000
  }, 3);
  auto root = start_new_tree(key, new Value(100));
  auto p = get(root, key);
  assert(p != nullptr);
  EXPECT_EQ(*p, 100);
}

TEST(TreeTest, insert){
  Key key1({
    0x0102030405060708,
    0x0A0B000000000000
  }, 2);
  Key key2({
    0x1112131415161718
  }, 8);
  GC gc{};
  auto root = put(nullptr, key1, new Value(1), nullptr, 0, gc);
  root = put(root, key2, new Value(2), nullptr, 0, gc);
  auto p = get(root, key1);
  assert(p != nullptr);
  EXPECT_EQ(*p, 1);
}

BorderNode *to_b(Node *n){
  return reinterpret_cast<BorderNode *>(n);
}

InteriorNode *to_i(Node *n){
  return reinterpret_cast<InteriorNode *>(n);
}

TEST(TreeTest, split){
  // work at B+ tree test.
  GC gc{};
  Node *root = nullptr;
  for(uint64_t i = 1; i <= 10000; ++i){
    Key k({i}, 1);
    root = put(root, k, new Value(i), nullptr, 0, gc);
  }

//  print_sub_tree(root);

  Key k2341({2341},1);
  auto l = findBorder(root, k2341);
  auto p = get(root, k2341);
  assert(p != nullptr);
  EXPECT_EQ(*p,2341);
}

TEST(TreeTest, break_invariant){
  auto root = sample2();
  GC gc{};
  Key k({0x0001'0203'0405'0607, 0x0C0D'0000'0000'0000}, 2);
  // kがここで変わってしまう。
  put(root, k, new Value(3), nullptr, 0, gc);


  Key k2({0x0001'0203'0405'0607, 0x0C0D'0000'0000'0000}, 2);
  auto p = get(root, k2);
  assert(p != nullptr);
  EXPECT_EQ(*p,3);
}

TEST(TreeTest, break_invariant2){
  Node *root = nullptr;
  GC gc{};
  Key k({
    0x8888'8888'8888'8888,
    0x1111'1111'1111'1111,
    0x2222'2222'2222'2222,
    0x3333'3333'3333'3333,
    0x0A0B'0000'0000'0000
  },2);
  root = put(root, k, new Value(1), nullptr, 0, gc);
  Key k1({
    0x8888'8888'8888'8888,
    0x1111'1111'1111'1111,
    0x2222'2222'2222'2222,
    0x0C0D'0000'0000'0000
  },2);
  root = put(root, k1, new Value(2), nullptr, 0, gc);
  // Key is mutable, but you can reset cursor.
  k1.cursor = 0;
  auto p = get(root, k1);
  assert(p != nullptr);
  EXPECT_EQ(*p,2);

}

TEST(TreeTest, break_invariant3){
  auto pair = not_conflict_89();
  GC gc{};
  auto root = put(nullptr, pair.first, new Value(1), nullptr, 0, gc);
  root = put(root, pair.second, new Value(7), nullptr, 0, gc);
  pair.second.cursor = 0;
  auto p = get(root, pair.second);
  EXPECT_EQ(*p, 7);
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
  Key k9({slice, CD}, 2);

  GC gc{};
  auto root = put(nullptr, k1, new Value(1), nullptr, 0, gc);
  root = put(root, k2, new Value(2), nullptr, 0, gc);
  root = put(root, k3, new Value(3), nullptr, 0, gc);
  root = put(root, k4, new Value(4), nullptr, 0, gc);
  root = put(root, k5, new Value(5), nullptr, 0, gc);
  root = put(root, k6, new Value(6), nullptr, 0, gc);
  root = put(root, k7, new Value(7), nullptr, 0, gc);
  root = put(root, k9, new Value(9), nullptr, 0, gc);
  root = put(root, k8, new Value(8), nullptr, 0, gc);

  k8.cursor = 0;
  auto p = get(root, k8);
  EXPECT_EQ(*p, 8);
}

TEST(TreeTest, duplex){
  Key k({
    EIGHT,
    EIGHT,
    CD
  },2);
  Key k1({
    EIGHT,
    AB
  },2);
  Key k2({
    EIGHT,
    EIGHT,
    CD
  },2);

  Node *root = nullptr;
  GC gc{};
  root = put(root, k, new Value(4), nullptr, 0, gc);
  root = put(root, k1, new Value(8), nullptr, 0, gc);
  root = put(root, k2, new Value(6), nullptr, 0, gc);
  k.cursor = 0;
  auto p = get(root, k);
  EXPECT_EQ(*p, 6);
}

TEST(TreeTest, layer_change){
  Node *root = nullptr;
  GC gc{};
  Key k1({
    ONE, TWO, FIVE
  }, 8);
  root = put_at_layer0(root, k1, new Value(5), gc);
  Key k2({
    ONE, TWO, THREE, FOUR
  }, 8);
  root = put_at_layer0(root, k2, new Value(4), gc);

  k1.reset();
  auto p = get(root, k1);
  ASSERT_EQ(*p, 5);

  k2.reset();
  root = remove_at_layer0(root, k2, gc);

  k1.reset();
  auto p2 = get(root, k1);
  ASSERT_EQ(*p2, 5);
}






