#include <gtest/gtest.h>
#include "../include/put.h"
#include "../include/get.h"
#include "../include/remove.h"
#include "sample.h"

using namespace masstree;

class RemoveTest: public ::testing::Test{};

//region 削除後のBorderNodeに要素が残るケース

TEST(RemoveTest, not_empty_border){
  auto b = new BorderNode;
  GC gc{};
  b->setIsRoot(true);

  b->setKeyLen(0, 1);
  b->setKeySlice(0, ONE);
  b->setKeyLen(1, 1);
  b->setKeySlice(1, TWO);
  b->setKeyLen(2, 1);
  b->setKeySlice(2, THREE);
  b->setPermutation(Permutation::fromSorted(3));

  Key k({TWO}, 1);
  auto pair = remove(b, k, nullptr, 0, gc);
  auto p = b->getPermutation();
  EXPECT_TRUE(pair.second == b);
  EXPECT_EQ(b->getKeySlice(p(0)), ONE);
  EXPECT_EQ(b->getKeySlice(p(1)), THREE);
  EXPECT_EQ(b->getKeySlice(1), TWO);
  EXPECT_EQ(b->getKeyLen(1), 10);
}

//endregion

//region Treeの中で、一番端のBorder Nodeが削除される場合

TEST(RemoveTest, left_most_1){
  auto a = new InteriorNode;
  auto b = new InteriorNode;
  auto c = new BorderNode;
  auto d = new BorderNode;
  GC gc{};

  a->setKeySlice(0, 25);
  a->setIsRoot(true);
  a->setChild(0, b);
  a->setNumKeys(1);
  b->setKeySlice(0, 15);
  b->setChild(0, c);
  b->setChild(1, d);
  b->setNumKeys(1);
  c->setKeyLen(0, 1);
  c->setKeySlice(0, 14);
  d->setKeyLen(0, 1);
  d->setKeySlice(0, 15);
  d->setKeyLen(1, 1);
  d->setKeySlice(1, 20);

  d->setParent(b);
  d->setPrev(c);
  c->setParent(b);
  c->setNext(d);
  b->setParent(a);

  Key k({14}, 1);
  auto pair = remove(a, k, nullptr, 0, gc);
  EXPECT_TRUE(pair.second == a);
  EXPECT_TRUE(a->getChild(0) == d);
}

TEST(RemoveTest, left_most_2){
  auto a = new InteriorNode;
  auto b = new InteriorNode;
  auto c = new BorderNode;
  auto d = new BorderNode;
  auto e = new BorderNode;
  GC gc{};

  a->setKeySlice(0, 15);
  a->setIsRoot(true);
  a->setChild(0, b);
  a->setNumKeys(1);
  b->setKeySlice(0, 13);
  b->setKeySlice(1, 14);
  b->setChild(0, c);
  b->setChild(1, d);
  b->setChild(2, e);
  b->setNumKeys(2);
  c->setKeyLen(0, 1);
  c->setKeySlice(0, 12);
  d->setKeyLen(0, 1);
  d->setKeySlice(0, 13);
  e->setKeyLen(0, 1);
  e->setKeySlice(0, 14);

  e->setParent(b);
  e->setPrev(d);
  d->setParent(b);
  d->setNext(e);
  d->setPrev(c);
  c->setParent(b);
  c->setNext(d);
  b->setParent(a);

  Key k({12}, 1);
  auto pair = remove(a, k, nullptr, 0, gc);
  EXPECT_TRUE(pair.second == a);
  EXPECT_EQ(b->getNumKeys(), 1);
  EXPECT_EQ(b->getKeySlice(0), 14);
}

TEST(RemoveTest, right_most){
  auto a = new InteriorNode;
  auto b = new InteriorNode;
  auto c = new BorderNode;
  auto d = new BorderNode;
  GC gc{};

  a->setKeySlice(0, 20);
  a->setIsRoot(true);
  a->setChild(1, b);
  a->setNumKeys(1);
  b->setKeySlice(0, 22);
  b->setChild(0, c);
  b->setChild(1, d);
  b->setNumKeys(1);
  c->setKeyLen(0, 1);
  c->setKeySlice(0, 21);
  d->setKeyLen(0, 1);
  d->setKeySlice(0, 22);

  d->setParent(b);
  d->setPrev(c);
  c->setParent(b);
  c->setNext(d);
  b->setParent(a);

  Key k({22}, 1);
  auto pair = remove(a, k, nullptr, 0, gc);
  EXPECT_TRUE(pair.second == a);
  EXPECT_TRUE(a->getChild(1) == c);
}

TEST(RemoveTest, right_most_2){
  auto a = new InteriorNode;
  auto b = new InteriorNode;
  auto c = new BorderNode;
  auto d = new BorderNode;
  auto e = new BorderNode;
  GC gc{};

  a->setKeySlice(0, 20);
  a->setIsRoot(true);
  a->setChild(1, b);
  a->setNumKeys(1);
  b->setKeySlice(0, 22);
  b->setKeySlice(1, 24);
  b->setChild(0, c);
  b->setChild(1, d);
  b->setChild(2, e);
  b->setNumKeys(2);
  c->setKeyLen(0, 1);
  c->setKeySlice(0, 20);
  d->setKeyLen(0, 1);
  d->setKeySlice(0, 22);
  d->setKeyLen(1 ,1);
  d->setKeySlice(1, 23);
  e->setKeyLen(0, 1);
  e->setKeySlice(0, 24);

  e->setParent(b);
  e->setPrev(d);
  d->setParent(b);
  d->setNext(e);
  d->setPrev(c);
  c->setParent(b);
  c->setNext(d);
  b->setParent(a);

  Key k({24}, 1);
  auto pair = remove(a, k, nullptr, 0, gc);
  EXPECT_EQ(b->getNumKeys(), 1);
  EXPECT_EQ(b->getKeySlice(0), 22);
}

//endregion

//region treeの中で、端以外に位置するBorderNodeをdelete
TEST(RemoveTest, middle1){
  auto a = new InteriorNode;
  auto b = new InteriorNode;
  auto c = new BorderNode;
  auto d = new BorderNode;
  GC gc{};

  a->setKeySlice(0, 25);
  a->setIsRoot(true);
  a->setChild(0, b);
  a->setNumKeys(1);
  b->setKeySlice(0, 15);
  b->setChild(0, c);
  b->setChild(1, d);
  b->setNumKeys(1);
  c->setKeyLen(0, 1);
  c->setKeySlice(0, 5);
  d->setKeyLen(0, 1);
  d->setKeySlice(0, 15);

  d->setParent(b);
  d->setPrev(c);
  c->setParent(b) ;
  c->setNext(d);
  b->setParent(a);

  Key k({15}, 1);
  auto pair = remove(a, k, nullptr, 0, gc);
  EXPECT_TRUE(pair.second == a);
  EXPECT_TRUE(a->getChild(0) == c);
  EXPECT_TRUE(c->getParent() == a);
}

TEST(RemoveTest, middle2){
  auto a = new InteriorNode;
  auto b = new InteriorNode;
  auto c = new BorderNode;
  auto d = new BorderNode;
  auto e = new BorderNode;
  auto f = new BorderNode;
  auto g = new BorderNode;
  GC gc{};

  a->setKeySlice(0, 20);
  a->setIsRoot(true);
  a->setChild(1, b);
  a->setNumKeys(1) ;
  b->setKeySlice(0, 21);
  b->setKeySlice(1, 22);
  b->setKeySlice(2, 23);
  b->setKeySlice(3, 24);
  b->setChild(0, c);
  b->setChild(1, d);
  b->setChild(2, e);
  b->setChild(3, f);
  b->setChild(4, g);
  b->setNumKeys(4);
  c->setKeyLen(0, 1);
  c->setKeySlice(0, 20);
  d->setKeyLen(0, 1);
  d->setKeySlice(0, 21);
  e->setKeyLen(0, 1);
  e->setKeySlice(0, 22);
  f->setKeyLen(0, 1);
  f->setKeySlice(0, 23);
  g->setKeyLen(0, 1);
  g->setKeySlice(0, 24);

  g->setParent(b);
  g->setPrev(f);
  f->setParent(b);
  f->setNext(g);
  f->setPrev(e);
  e->setParent(b);
  e->setNext(f);
  e->setPrev(d);
  d->setParent(b);
  d->setNext(e);
  d->setPrev(c);
  c->setParent(b);
  c->setNext(d);
  b->setParent(a);

  Key k({22}, 1);
  auto pair = remove(a, k, nullptr, 0, gc);
  EXPECT_EQ(b->getNumKeys(), 3);
  EXPECT_EQ(b->getKeySlice(0), 21);
  EXPECT_EQ(b->getKeySlice(1), 23);
  EXPECT_EQ(b->getKeySlice(2), 24);
  EXPECT_TRUE(b->getChild(2) == f);
}
//endregion

//region root付近
TEST(RemoveTest, new_root){
  auto upper_node = new BorderNode;
  size_t upper_index = 0;
  auto a = new InteriorNode;
  auto b = new BorderNode;
  auto c = new InteriorNode;
  auto d = new BorderNode;
  auto e = new BorderNode;
  auto f = new BorderNode;
  GC gc{};

  upper_node->setLV(upper_index, LinkOrValue(a));
  a->setKeySlice(0, 25);
  a->setChild(0, b);
  a->setChild(1, c);
  a->setNumKeys(1);
  a->setIsRoot(true);
  b->setKeyLen(0, 1);
  b->setKeySlice(0, 9);
  c->setKeySlice(0, 45);
  c->setKeySlice(1, 60);
  c->setChild(0, d);
  c->setChild(1, e);
  c->setChild(2, f);
  c->setNumKeys(2);
  d->setKeyLen(0, 1);
  d->setKeySlice(0, 35);
  e->setKeyLen(0, 1);
  e->setKeySlice(0, 55);
  f->setKeyLen(0, 1);
  f->setKeySlice(0, 78);

  f->setParent(c);
  f->setPrev(e);
  e->setParent(c);
  e->setNext(f);
  e->setPrev(d);
  d->setParent(c);
  d->setNext(e);
  d->setPrev(b);
  b->setParent(a);
  b->setNext(d);
  c->setParent(a);
  b->setParent(a);

  Key k({9}, 1);
  auto pair = remove(a, k, upper_node, upper_index, gc);
  ASSERT_TRUE(upper_node->getLV(upper_index).next_layer == c);
  EXPECT_TRUE(c->getParent() == nullptr);
  EXPECT_TRUE(c->getIsRoot());
  EXPECT_EQ(d->getPrev(), nullptr);
  // bがいつ解放されるだろうか
}

TEST(RemoveTest, remove_layer_1){
  auto upper_b = new BorderNode;

  // pull upしてきたBorderNodeのサイズが1の時
  auto a = new InteriorNode;
  auto b = new BorderNode;
  auto c = new BorderNode;
  GC gc{};
  a->setKeySlice(0, 10);
  a->setChild(0, b);
  a->setChild(1, c);
  a->setNumKeys(1);
  a->setIsRoot(true);
  b->setKeyLen(0, 1);
  b->setKeySlice(0, 9);
  c->setKeyLen(0, 1);
  c->setKeySlice(0, 10);

  c->setParent(a);
  c->setPrev(b) ;
  b->setParent(a);
  b->setNext(c);

  Key k({9}, 1);
  auto pair = remove(a, k, upper_b, 0, gc);
  EXPECT_EQ(pair.first, NewRoot);
  EXPECT_TRUE(pair.second == c);
}

TEST(RemoveTest, remove_layer_2){
  // rootのBorderNodeがサイズ1になった時
  auto upper_b = new BorderNode;
  auto b = new BorderNode;
  GC gc{};
  b->setKeyLen(0, 1);
  b->setKeySlice(0, 9);
  b->setKeyLen(1, 1);
  b->setKeySlice(1, 10);
  b->setIsRoot(true);

  Key k({9}, 1);
  auto pair = remove(b, k, upper_b, 0, gc);
  EXPECT_EQ(pair.first, NotChange);
}

TEST(RemoveTest, remove_all_layer){
  auto a = new BorderNode;
  auto b = new BorderNode;
  auto c = new BorderNode;
  auto d = new BorderNode;
  GC gc{};

  a->setKeyLen(0, BorderNode::key_len_layer);
  a->setKeySlice(0, ONE);
  a->setIsRoot(true);
  a->setLV(0, LinkOrValue(b));

  b->setKeyLen(0, BorderNode::key_len_layer);
  b->setKeySlice(0, TWO);
  b->setIsRoot(true);
  b->setLV(0, LinkOrValue(c));

  c->setKeyLen(0, BorderNode::key_len_layer);
  c->setKeySlice(0, THREE);
  c->setIsRoot(true);
  c->setLV(0, LinkOrValue(d));

  d->setKeyLen(0, 8);
  d->setKeySlice(0, FOUR);
  d->setIsRoot(true);
  d->setLV(0, LinkOrValue(new Value(9)));

  Key k({
    ONE,TWO,THREE,FOUR
  }, 8);

  auto pair = remove(a, k, nullptr, 0, gc);
  EXPECT_EQ(pair.first, LayerDeleted);
}


TEST(RemoveTest, remove_all_layer2){
  auto a = new BorderNode;
  auto b = new BorderNode;
  auto c = new BorderNode;
  auto d = new BorderNode;
  GC gc{};

  a->setKeyLen(0, BorderNode::key_len_layer);
  a->setKeySlice(0, ONE);
  a->setIsRoot(true);
  a->setLV(0, LinkOrValue(b));

  b->setKeyLen(0, BorderNode::key_len_layer);
  b->setKeySlice(0, TWO);
  b->setIsRoot(true);
  b->setLV(0, LinkOrValue(c));

  c->setIsRoot(true);
  c->setKeyLen(0, BorderNode::key_len_layer);
  c->setKeySlice(0, THREE);
  c->setLV(0, LinkOrValue(d));
  c->setKeyLen(1, 8);
  c->setKeySlice(1, FIVE);
  c->setLV(1, LinkOrValue(new Value(8)));

  d->setKeyLen(0, 8);
  d->setKeySlice(0, FOUR);
  d->setIsRoot(true);
  d->setLV(0, LinkOrValue(new Value(9)));

  Key k({
    ONE,TWO,THREE,FOUR
  }, 8);

  auto pair = remove(a, k, nullptr, 0, gc);
  EXPECT_EQ(pair.first, NotChange);
  EXPECT_EQ(c->getKeySlice(0), FIVE);
}

TEST(RemoveTest, at_layer0_1){
  // pull upしてきたBorderNodeのサイズが1の時
  auto a = new InteriorNode;
  auto b = new BorderNode;
  auto c = new BorderNode;
  GC gc{};
  a->setKeySlice(0, 10);
  a->setChild(0, b);
  a->setChild(1, c);
  a->setNumKeys(1);
  a->setIsRoot(true);
  b->setKeyLen(0, 1);
  b->setKeySlice(0, 9);
  c->setKeyLen(0, 1);
  c->setKeySlice(0, 10);

  c->setParent(a);
  c->setPrev(b);
  b->setParent(a);
  b->setNext(c);

  Key k({9}, 1);
  auto pair = remove(a, k, nullptr, 0, gc);
  EXPECT_EQ(pair.first, NewRoot);
}

TEST(RemoveTest, at_layer0_2){
  // rootのBorderNodeがサイズ1になった時
  auto b = new BorderNode;
  GC gc{};
  b->setKeyLen(0, 1);
  b->setKeySlice(0, 9);
  b->setKeyLen(1, 1);
  b->setKeySlice(1, 10);
  b->setIsRoot(true);

  Key k({9}, 1);
  auto pair = remove(b, k, nullptr, 0, gc);
  EXPECT_EQ(pair.first, NotChange);
}
//endregion

TEST(RemoveTest, handle_delete_layer_in_remove){
  auto upper = new BorderNode{};
  auto n = new BorderNode{};
  GC gc{};
  auto upper_index = 4;

  upper->setKeyLen(upper_index, BorderNode::key_len_layer);
  upper->setKeySlice(upper_index, ONE);
  upper->setLV(upper_index, LinkOrValue(n));

  Value v(1);
  n->setKeyLen(1, BorderNode::key_len_has_suffix);
  n->setKeySlice(1, TWO);
  n->getKeySuffixes().set(1, new BigSuffix({THREE}, 8));
  n->setLV(1, LinkOrValue(&v));
  n->setPermutation(Permutation::from({1}));
  n->setIsRoot(true);

  handle_delete_layer_in_remove(n, upper, upper_index, gc);

  EXPECT_EQ(upper->getKeyLen(upper_index), BorderNode::key_len_has_suffix);
  EXPECT_EQ(upper->getLV(upper_index).value, &v);
}