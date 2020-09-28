#include <gtest/gtest.h>
#include "../include/put.h"
#include "../include/get.h"
#include "../include/remove.h"
#include "sample.h"

using namespace masstree;

class PutTest: public ::testing::Test{};


TEST(PutTest, start_new_tree){
  Key k({ONE}, 8);
  int i = 0;
  auto root = start_new_tree(k, &i);
  EXPECT_EQ(root->key_len[0], 8);
  EXPECT_EQ(root->key_slice[0], ONE);

  Key k2({ONE, AB}, 11);
  auto root2 = start_new_tree(k2, &i);
  EXPECT_EQ(root2->key_len[0], BorderNode::key_len_has_suffix);
  EXPECT_EQ(root->key_slice[0], ONE);
  EXPECT_EQ(root2->key_suffixes.get(0)->lastSliceSize, 3);
}

TEST(PutTest, check_break_invariant){
  BorderNode border{};
  Key k({ONE, AB}, 10);
  EXPECT_EQ(check_break_invariant(&border, k), std::nullopt);
  border.key_len[1] = BorderNode::key_len_has_suffix;
  border.key_slice[1] = TWO;
  EXPECT_EQ(check_break_invariant(&border, k), std::nullopt);
  border.key_len[0] = BorderNode::key_len_has_suffix;
  border.key_slice[0] = ONE;
  BigSuffix suffix({CD}, 3);
  border.key_suffixes.set(0, &suffix);
  EXPECT_EQ(check_break_invariant(&border, k), std::make_optional(0));
}


TEST(PutTest, handle_break_invariant){
  BorderNode borderNode{};
  int i = 4;
  borderNode.key_len[0] = 8;
  borderNode.key_slice[0] = EIGHT;
  borderNode.lv[0].value =  &i;
  borderNode.key_len[1] = BorderNode::key_len_has_suffix;
  borderNode.key_slice[1] = EIGHT;
  auto suffix = new BigSuffix({ONE, TWO, THREE, AB}, 2);
  borderNode.key_suffixes.set(1, suffix);
  borderNode.lv[1].value = &i;
  int j = 5;
  Key k({EIGHT, ONE, TWO, CD}, 26);
  handle_break_invariant(&borderNode, k, &j, 1);

  ASSERT_EQ(borderNode.key_len[1], BorderNode::key_len_layer);
  ASSERT_EQ(borderNode.key_suffixes.get(1), nullptr);
  auto next = reinterpret_cast<BorderNode *>(borderNode.lv[1].next_layer);
  ASSERT_EQ(next->key_len[0], BorderNode::key_len_layer);
  ASSERT_EQ(next->key_slice[0], ONE);
  next = reinterpret_cast<BorderNode *>(next->lv[0].next_layer);
  ASSERT_EQ(next->key_len[0], BorderNode::key_len_layer);
  ASSERT_EQ(next->key_slice[0], TWO);
  next = reinterpret_cast<BorderNode *>(next->lv[0].next_layer);
  ASSERT_EQ(next->key_len[0], 2);
  ASSERT_EQ(next->key_slice[0], CD);
  ASSERT_EQ(next->key_len[1], BorderNode::key_len_has_suffix);
  ASSERT_EQ(next->key_slice[1], THREE);
  ASSERT_EQ(next->key_suffixes.get(1)->getCurrentSlice().slice, AB);
}

TEST(PutTest, insert_into_border){
  BorderNode border{};
  BorderNode next{};
  int i = 9;
  border.key_len[0] = 2;
  border.key_slice[0] = ONE;
  border.lv[0].value = &i;
  border.key_len[1] = BorderNode::key_len_has_suffix;
  border.key_slice[1] = THREE;
  border.key_suffixes.set(1, new BigSuffix({FOUR}, 8));
  border.lv[1].value = &i;
  border.key_len[2] = BorderNode::key_len_layer;
  border.key_slice[2] = FOUR;
  border.lv[2].next_layer = &next;
  border.key_len[3] = 3;
  border.key_slice[3] = FIVE;
  border.lv[3].value = &i;

  Key k({TWO, FIVE}, 16);
  insert_into_border(&border, k, &i);

  EXPECT_EQ(border.key_len[1], BorderNode::key_len_has_suffix);
  EXPECT_EQ(border.key_slice[1], TWO);
  EXPECT_TRUE(border.key_suffixes.get(1)->getCurrentSlice().slice == FIVE);
  EXPECT_EQ(border.key_len[2], BorderNode::key_len_has_suffix);
  EXPECT_EQ(border.key_slice[2], THREE);
  EXPECT_TRUE(border.key_suffixes.get(2)->getCurrentSlice().slice == FOUR);
  EXPECT_EQ(border.key_len[3], BorderNode::key_len_layer);
}




TEST(PutTest, split_keys_among1){
  InteriorNode p{};
  InteriorNode p1{};
  InteriorNode n1{};
  InteriorNode d1{};
  // p should full
  p.n_keys = Node::ORDER - 1;
  for(size_t i = 0; i < 16; ++i)
    p.child[i] = &d1;

  p.key_slice[0] = 0;
  p.key_slice[1] = 1;
  p.key_slice[2] = 2;
  p.key_slice[3] = 3;
  p.key_slice[4] = 4;
  p.key_slice[5] = 5;
  p.key_slice[6] = 6;
  p.key_slice[7] = 7;
  p.key_slice[8] = 9;
  p.key_slice[9] = 10;
  p.key_slice[10] = 11;
  p.key_slice[11] = 12;
  p.key_slice[12] = 13;
  p.key_slice[13] = 14;
  p.key_slice[14] = 15;
  split_keys_among(&p, &p1, 8, &n1, 7);
  EXPECT_EQ(p.n_keys, 7);
  EXPECT_EQ(p.key_slice[7], 0);
  EXPECT_EQ(p.child[8], nullptr);
  EXPECT_EQ(p1.n_keys, 8);
  EXPECT_EQ(p.key_slice[7], 0);
  EXPECT_EQ(p.child[8], nullptr);
}


TEST(PutTest, slice_table){
  // NOTE: より効率的なアルゴリズムを考えるべき

  BorderNode n{};
  n.key_len[0] = 1;
  n.key_slice[0] = ONE;
  n.key_len[1] = 2;
  n.key_slice[1] = ONE;
  n.key_len[2] = 3;
  n.key_slice[2] = ONE;
  n.key_len[3] = 4;
  n.key_slice[3] = ONE;
  n.key_len[4] = BorderNode::key_len_layer;
  n.key_slice[4] = ONE;

  n.key_len[5] = 1;
  n.key_slice[5] = TWO;
  n.key_len[6] = 2;
  n.key_slice[6] = TWO;
  n.key_len[7] = 3;
  n.key_slice[7] = TWO;
  n.key_len[8] = 4;
  n.key_slice[8] = TWO;
  n.key_len[9] = BorderNode::key_len_layer;
  n.key_slice[9] = TWO;

  n.key_len[10] = 1;
  n.key_slice[10] = FOUR;
  n.key_len[11] = 2;
  n.key_slice[11] = FOUR;
  n.key_len[12] = 3;
  n.key_slice[12] = FOUR;
  n.key_len[13] = 4;
  n.key_slice[13] = FOUR;
  n.key_len[14] = BorderNode::key_len_layer;
  n.key_slice[14] = FOUR;

  std::vector<std::pair<KeySlice, size_t>> table;
  std::vector<KeySlice> found;

  create_slice_table(&n, table, found);
  EXPECT_EQ(table[0], std::make_pair(ONE, (size_t)0));
  EXPECT_EQ(found[0], ONE);
  EXPECT_EQ(table[1], std::make_pair(TWO, (size_t)5));
  EXPECT_EQ(found[1], TWO);
  EXPECT_EQ(table[2], std::make_pair(FOUR, (size_t)10));
  EXPECT_EQ(found[2], FOUR);

  EXPECT_EQ(split_point(AB, table, found), 1);
  EXPECT_EQ(split_point(ONE, table, found), 5);
  EXPECT_EQ(split_point(TWO, table, found), 5);
  EXPECT_EQ(split_point(THREE, table, found), 10);
  EXPECT_EQ(split_point(FOUR, table, found), 10);
  EXPECT_EQ(split_point(FIVE, table, found), 15);
}

TEST(PutTest, split_keys_among2){
  BorderNode n{};
  BorderNode n1{};

  int i = 9;
  n.key_len[0] = 1;
  n.key_slice[0] = 110;
  n.lv[0].value = &i;

  n.key_len[1] = 2;
  n.key_slice[1] = 110;
  n.lv[1].value = &i;

  n.key_len[2] = 3;
  n.key_slice[2] = 110;
  n.lv[2].value = &i;

  n.key_len[3] = BorderNode::key_len_has_suffix;
  n.key_slice[3] = 110;
  n.key_suffixes.set(3, new BigSuffix({AB}, 2));
  n.lv[3].value = &i;


  n.key_len[4] = 1;
  n.key_slice[4] = 111;
  n.lv[4].value = &i;

  n.key_len[5] = 2;
  n.key_slice[5] = 111;
  n.lv[5].value = &i;

  n.key_len[6] = 3;
  n.key_slice[6] = 111;
  n.lv[6].value = &i;

  n.key_len[7] = BorderNode::key_len_has_suffix;
  n.key_slice[7] = 111;
  n.key_suffixes.set(7, new BigSuffix({CD}, 2));
  n.lv[7].value = &i;


  n.key_len[8] = 1;
  n.key_slice[8] = 113;
  n.lv[8].value = &i;

  n.key_len[9] = 2;
  n.key_slice[9] = 113;
  n.lv[9].value = &i;

  n.key_len[10] = 3;
  n.key_slice[10] = 113;
  n.lv[10].value = &i;

  n.key_len[11] = BorderNode::key_len_layer;
  n.key_slice[11] = 113;
  BorderNode next{};
  n.lv[11].next_layer = &next;


  n.key_len[12] = 1;
  n.key_slice[12] = 114;
  n.lv[12].value = &i;

  n.key_len[13] = 2;
  n.key_slice[13] = 114;
  n.lv[13].value = &i;

  n.key_len[14] = 3;
  n.key_slice[14] = 114;
  n.lv[14].value = &i;

  Key k({112, AB}, 10);
  split_keys_among(&n, &n1, k, &i);
  EXPECT_EQ(n.key_len[8], 0);
  EXPECT_EQ(n1.key_suffixes.get(1), nullptr);
}