#include <gtest/gtest.h>
#include "../include/operation.h"
#include "sample.h"

using namespace masstree;

class RemoveTest: public ::testing::Test{};

//region 削除後のBorderNodeに要素が残るケース

TEST(RemoveTest, not_empty_border){
  auto b = new BorderNode;
  b->version.is_root = true;

  b->key_len[0] = 1;
  b->key_slice[0] = ONE;
  b->key_len[1] = 1;
  b->key_slice[1] = TWO;
  b->key_len[2] = 1;
  b->key_slice[2] = THREE;

  Key k({TWO}, 1);
  auto pair = remove(b, k, nullptr, 0);
  EXPECT_TRUE(pair.second == b);
  EXPECT_EQ(b->key_slice[0], ONE);
  EXPECT_EQ(b->key_slice[1], THREE);
}

//endregion

//region Treeの中で、一番端のBorder Nodeが削除される場合

TEST(RemoveTest, left_most_1){
  auto a = new InteriorNode;
  auto b = new InteriorNode;
  auto c = new BorderNode;
  auto d = new BorderNode;

  a->key_slice[0] = 25;
  a->version.is_root = true;
  a->child[0] = b;
  a->n_keys = 1;
  b->key_slice[0] = 15;
  b->child[0] = c;
  b->child[1] = d;
  b->n_keys = 1;
  c->key_len[0] = 1;
  c->key_slice[0] = 14;
  d->key_len[0] = 1;
  d->key_slice[0] = 15;
  d->key_len[1] = 1;
  d->key_slice[1] = 20;

  d->parent = b;
  d->prev = c;
  c->parent = b;
  c->next = d;
  b->parent = a;

  Key k({14}, 1);
  auto pair = remove(a, k, nullptr, 0);
  EXPECT_TRUE(pair.second == a);
  EXPECT_TRUE(a->child[0] == d);
}

TEST(RemoveTest, left_most_2){
  auto a = new InteriorNode;
  auto b = new InteriorNode;
  auto c = new BorderNode;
  auto d = new BorderNode;
  auto e = new BorderNode;

  a->key_slice[0] = 15;
  a->version.is_root = true;
  a->child[0] = b;
  a->n_keys = 1;
  b->key_slice[0] = 13;
  b->key_slice[1] = 14;
  b->child[0] = c;
  b->child[1] = d;
  b->child[2] = e;
  b->n_keys = 2;
  c->key_len[0] = 1;
  c->key_slice[0] = 12;
  d->key_len[0] = 1;
  d->key_slice[0] = 13;
  e->key_len[0] = 1;
  e->key_slice[0] = 14;

  e->parent = b;
  e->prev = d;
  d->parent = b;
  d->next = e;
  d->prev = c;
  c->parent = b;
  c->next = d;
  b->parent = a;

  Key k({12}, 1);
  auto pair = remove(a, k, nullptr, 0);
  EXPECT_TRUE(pair.second == a);
  EXPECT_EQ(b->n_keys, 1);
  EXPECT_EQ(b->key_slice[0], 14);
}

TEST(RemoveTest, right_most){
  auto a = new InteriorNode;
  auto b = new InteriorNode;
  auto c = new BorderNode;
  auto d = new BorderNode;

  a->key_slice[0] = 20;
  a->version.is_root = true;
  a->child[1] = b;
  a->n_keys = 1;
  b->key_slice[0] = 22;
  b->child[0] = c;
  b->child[1] = d;
  b->n_keys = 1;
  c->key_len[0] = 1;
  c->key_slice[0] = 21;
  d->key_len[0] = 1;
  d->key_slice[0] = 22;

  d->parent = b;
  d->prev = c;
  c->parent = b;
  c->next = d;
  b->parent = a;

  Key k({22}, 1);
  auto pair = remove(a, k, nullptr, 0);
  EXPECT_TRUE(pair.second == a);
  EXPECT_TRUE(a->child[1] == c);
}

TEST(RemoveTest, right_most_2){
  auto a = new InteriorNode;
  auto b = new InteriorNode;
  auto c = new BorderNode;
  auto d = new BorderNode;
  auto e = new BorderNode;

  a->key_slice[0] = 20;
  a->version.is_root = true;
  a->child[1] = b;
  a->n_keys = 1;
  b->key_slice[0] = 22;
  b->key_slice[1] = 24;
  b->child[0] = c;
  b->child[1] = d;
  b->child[2] = e;
  b->n_keys = 2;
  c->key_len[0] = 1;
  c->key_slice[0] = 20;
  d->key_len[0] = 1;
  d->key_slice[0] = 22;
  d->key_len[1] = 1;
  d->key_slice[1] = 23;
  e->key_len[0] = 1;
  e->key_slice[0] = 24;

  e->parent = b;
  e->prev = d;
  d->parent = b;
  d->next = e;
  d->prev = c;
  c->parent = b;
  c->next = d;
  b->parent = a;

  Key k({24}, 1);
  auto pair = remove(a, k, nullptr, 0);
  EXPECT_EQ(b->n_keys, 1);
  EXPECT_EQ(b->key_slice[0], 22);
}

//endregion

//region treeの中で、端以外に位置するBorderNodeをdelete
TEST(RemoveTest, middle1){
  auto a = new InteriorNode;
  auto b = new InteriorNode;
  auto c = new BorderNode;
  auto d = new BorderNode;

  a->key_slice[0] = 25;
  a->version.is_root = true;
  a->child[0] = b;
  a->n_keys = 1;
  b->key_slice[0] = 15;
  b->child[0] = c;
  b->child[1] = d;
  b->n_keys = 1;
  c->key_len[0] = 1;
  c->key_slice[0] = 5;
  d->key_len[0] = 1;
  d->key_slice[0] = 15;

  d->parent = b;
  d->prev = c;
  c->parent = b;
  c->next = d;
  b->parent = a;

  Key k({15}, 1);
  auto pair = remove(a, k, nullptr, 0);
  EXPECT_TRUE(pair.second == a);
  EXPECT_TRUE(a->child[0] == c);
  EXPECT_TRUE(c->parent == a);
}

TEST(RemoveTest, middle2){
  auto a = new InteriorNode;
  auto b = new InteriorNode;
  auto c = new BorderNode;
  auto d = new BorderNode;
  auto e = new BorderNode;
  auto f = new BorderNode;
  auto g = new BorderNode;

  a->key_slice[0] = 20;
  a->version.is_root = true;
  a->child[1] = b;
  a->n_keys = 1;
  b->key_slice[0] = 21;
  b->key_slice[1] = 22;
  b->key_slice[2] = 23;
  b->key_slice[3] = 24;
  b->child[0] = c;
  b->child[1] = d;
  b->child[2] = e;
  b->child[3] = f;
  b->child[4] = g;
  b->n_keys = 4;
  c->key_len[0] = 1;
  c->key_slice[0] = 20;
  d->key_len[0] = 1;
  d->key_slice[0] = 21;
  e->key_len[0] = 1;
  e->key_slice[0] = 22;
  f->key_len[0] = 1;
  f->key_slice[0] = 23;
  g->key_len[0] = 1;
  g->key_slice[0] = 24;

  g->parent = b;
  g->prev = f;
  f->parent = b;
  f->next = g;
  f->prev = e;
  e->parent = b;
  e->next = f;
  e->prev = d;
  d->parent = b;
  d->next = e;
  d->prev = c;
  c->parent = b;
  c->next = d;
  b->parent = a;

  Key k({22}, 1);
  auto pair = remove(a, k, nullptr, 0);
  EXPECT_EQ(b->n_keys, 3);
  EXPECT_EQ(b->key_slice[0], 21);
  EXPECT_EQ(b->key_slice[1], 23);
  EXPECT_EQ(b->key_slice[2], 24);
  EXPECT_TRUE(b->child[2] == f);
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

  upper_node->lv[upper_index].next_layer = a;
  a->key_slice[0] = 25;
  a->child[0] = b;
  a->child[1] = c;
  a->n_keys = 1;
  a->version.is_root = true;
  b->key_len[0] = 1;
  b->key_slice[0] = 9;
  c->key_slice[0] = 45;
  c->key_slice[1] = 60;
  c->child[0] = d;
  c->child[1] = e;
  c->child[2] = f;
  c->n_keys = 2;
  d->key_len[0] = 1;
  d->key_slice[0] = 35;
  e->key_len[0] = 1;
  e->key_slice[0] = 55;
  f->key_len[0] = 1;
  f->key_slice[0] = 78;

  f->parent = c;
  f->prev = e;
  e->parent = c;
  e->next = f;
  e->prev = d;
  d->parent = c;
  d->next = e;
  d->prev = b;
  b->parent = a;
  b->next = d;
  c->parent = a;
  b->parent = a;

  Key k({9}, 1);
  auto pair = remove(a, k, upper_node, upper_index);
  ASSERT_TRUE(upper_node->lv[upper_index].next_layer == c);
  EXPECT_TRUE(c->parent == nullptr);
  EXPECT_TRUE(c->version.is_root);
  EXPECT_EQ(d->prev, nullptr);
  // bがいつ解放されるだろうか
}

TEST(RemoveTest, remove_layer_1){
  auto upper_b = new BorderNode;

  // pull upしてきたBorderNodeのサイズが1の時
  auto a = new InteriorNode;
  auto b = new BorderNode;
  auto c = new BorderNode;
  a->key_slice[0] = 10;
  a->child[0] = b;
  a->child[1] = c;
  a->n_keys = 1;
  a->version.is_root = true;
  b->key_len[0] = 1;
  b->key_slice[0] = 9;
  c->key_len[0] = 1;
  c->key_slice[0] = 10;

  c->parent = a;
  c->prev = b;
  b->parent = a;
  b->next = c;

  Key k({9}, 1);
  auto pair = remove(a, k, upper_b, 0);
  EXPECT_EQ(pair.first, NewRoot);
  EXPECT_TRUE(pair.second == c);
}

TEST(RemoveTest, remove_layer_2){
  // rootのBorderNodeがサイズ1になった時
  auto upper_b = new BorderNode;
  auto b = new BorderNode;
  b->key_len[0] = 1;
  b->key_slice[0] = 9;
  b->key_len[1] = 1;
  b->key_slice[1] = 10;
  b->version.is_root = true;

  Key k({9}, 1);
  auto pair = remove(b, k, upper_b, 0);
  EXPECT_EQ(pair.first, NotChange);
}

TEST(RemoveTest, remove_all_layer){
  auto a = new BorderNode;
  auto b = new BorderNode;
  auto c = new BorderNode;
  auto d = new BorderNode;

  a->key_len[0] = BorderNode::key_len_layer;
  a->key_slice[0] = ONE;
  a->version.is_root = true;
  a->lv[0].next_layer = b;

  b->key_len[0] = BorderNode::key_len_layer;
  b->key_slice[0] = TWO;
  b->version.is_root = true;
  b->lv[0].next_layer = c;

  c->key_len[0] = BorderNode::key_len_layer;
  c->key_slice[0] = THREE;
  c->version.is_root = true;
  c->lv[0].next_layer = d;

  d->key_len[0] = 8;
  d->key_slice[0] = FOUR;
  d->version.is_root = true;
  d->lv[0].value = new int(9);

  Key k({
    ONE,TWO,THREE,FOUR
  }, 32);

  auto pair = remove(a, k, nullptr, 0);
  EXPECT_EQ(pair.first, LayerDeleted);
}


TEST(RemoveTest, remove_all_layer2){
  auto a = new BorderNode;
  auto b = new BorderNode;
  auto c = new BorderNode;
  auto d = new BorderNode;

  a->key_len[0] = BorderNode::key_len_layer;
  a->key_slice[0] = ONE;
  a->version.is_root = true;
  a->lv[0].next_layer = b;

  b->key_len[0] = BorderNode::key_len_layer;
  b->key_slice[0] = TWO;
  b->version.is_root = true;
  b->lv[0].next_layer = c;

  c->version.is_root = true;
  c->key_len[0] = BorderNode::key_len_layer;
  c->key_slice[0] = THREE;
  c->lv[0].next_layer = d;
  c->key_len[1] = 8;
  c->key_slice[1] = FIVE;
  c->lv[1].value = new int(8);

  d->key_len[0] = 8;
  d->key_slice[0] = FOUR;
  d->version.is_root = true;
  d->lv[0].value = new int(9);

  Key k({
    ONE,TWO,THREE,FOUR
  }, 32);

  auto pair = remove(a, k, nullptr, 0);
  EXPECT_EQ(pair.first, NotChange);
  EXPECT_EQ(c->key_slice[0], FIVE);
}

TEST(RemoveTest, at_layer0_1){
  // pull upしてきたBorderNodeのサイズが1の時
  auto a = new InteriorNode;
  auto b = new BorderNode;
  auto c = new BorderNode;
  a->key_slice[0] = 10;
  a->child[0] = b;
  a->child[1] = c;
  a->n_keys = 1;
  a->version.is_root = true;
  b->key_len[0] = 1;
  b->key_slice[0] = 9;
  c->key_len[0] = 1;
  c->key_slice[0] = 10;

  c->parent = a;
  c->prev = b;
  b->parent = a;
  b->next = c;

  Key k({9}, 1);
  auto pair = remove(a, k, nullptr, 0);
  EXPECT_EQ(pair.first, NewRoot);
}

TEST(RemoveTest, at_layer0_2){
  // rootのBorderNodeがサイズ1になった時
  auto b = new BorderNode;
  b->key_len[0] = 1;
  b->key_slice[0] = 9;
  b->key_len[1] = 1;
  b->key_slice[1] = 10;
  b->version.is_root = true;

  Key k({9}, 1);
  auto pair = remove(b, k, nullptr, 0);
  EXPECT_EQ(pair.first, NotChange);
}
//endregion