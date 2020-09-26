#include <gtest/gtest.h>
#include "../include/operation.h"
#include "sample.h"

using namespace masstree;

class OperationTest: public ::testing::Test{};

TEST(OperationTest, start_new_tree){
  Key k({ONE}, 8);
  int i = 0;
  auto root = start_new_tree(k, &i);
  EXPECT_EQ(root->key_len[0], 8);
  Key k2({ONE, AB}, 11);
  auto root2 = start_new_tree(k2, &i);
  EXPECT_EQ(root2->key_len[0], BorderNode::key_len_has_suffix);
  EXPECT_EQ(root2->key_suffixes.get(0)->lastSliceSize, 3);
}

TEST(OperationTest, check_break_invariant){
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

TEST(OperationTest, handle_break_invariant){
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

TEST(OperationTest, split_keys_among1){
  InteriorNode p{};
  InteriorNode p1{};
  InteriorNode n1{};
  InteriorNode d1{};
  // p should full
  p.n_keys = Node::ORDER - 1;
  for(size_t i = 0; i < 16; ++i)
    p.child[i] = &d1;

  p.key_slice[0] = 0x0100'0000'0000'0000;
  p.key_slice[1] = 0x0200'0000'0000'0000;
  p.key_slice[2] = 0x0300'0000'0000'0000;
  p.key_slice[3] = 0x0400'0000'0000'0000;
  p.key_slice[4] = 0x0500'0000'0000'0000;
  p.key_slice[5] = 0x0600'0000'0000'0000;
  p.key_slice[6] = 0x0700'0000'0000'0000;
  p.key_slice[7] = 0x0800'0000'0000'0000;
  p.key_slice[8] = 0x0900'0000'0000'0000;
  p.key_slice[9] = 0x0A00'0000'0000'0000;
  p.key_slice[10] = 0x0B00'0000'0000'0000;
  p.key_slice[11] = 0x0C00'0000'0000'0000;
  p.key_slice[12] = 0x0D00'0000'0000'0000;
  p.key_slice[13] = 0x0E00'0000'0000'0000;
  p.key_slice[14] = 0x0F00'0000'0000'0000;
  split_keys_among(&p, &p1, 0x0500'0000'0000'0001, &n1, 5);
}

TEST(OperationTest, slice_table){
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

