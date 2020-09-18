#include <gtest/gtest.h>
#include "../include/key.h"
#include "../include/tree.h"
#include "sample.h"
#include "../include/bptree.h"

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

TEST(MasstreeTest, sample3){
  auto root = sample3();
  Key key({0x0001020304050607, 0x0A0B'0000'0000'0000}, 10);

  auto b = findBorder(root, key);
  EXPECT_EQ(b.first->key_len[0], BorderNode::key_len_layer);
}

TEST(MasstreeTest, get1){
  auto root = sample2();
  Key key({0x0001020304050607, 0x0A0B'0000'0000'0000}, 10);

  auto p = get(root, key);

  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p), 1);
}

TEST(MasstreeTest, get2){
  auto root = sample3();
  Key key({0x0001020304050607, 0x0C0D'0000'0000'0000}, 10);

  auto p = get(root, key);

  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p), 2);
}

TEST(MasstreeTest, get3){
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

TEST(MasstreeTest, get4){
  auto root = sample4();
  Key key({ 0x0101 }, 2);

  write(root, key, new int(23));
  auto p = get(root, key);

  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p), 23);
}

TEST(MasstreeTest, start_new_tree){
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

TEST(MasstreeTest, insert){
  Key key1({
    0x0102030405060708,
    0x0A0B000000000000
  }, 10);
  Key key2({
    0x1112131415161718
  }, 8);
  auto root = insert(nullptr, key1, new int(1));
  root = insert(root, key2, new int(2));
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

TEST(MasstreeTest, split){
  Node *root = nullptr;
  for(uint64_t i = 1; i <= 24; ++i){
    Key k({i}, 1);
    root = insert(root, k, new int(i));
  }

  print_sub_tree(root);

  Key k17({17},1);
  auto p = get(root, k17);
  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p),17);
}