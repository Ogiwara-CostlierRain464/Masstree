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
  EXPECT_EQ(root->getKeyLen(0), 8);
  EXPECT_EQ(root->getKeySlice(0), ONE);

  Key k2({ONE, AB}, 11);
  auto root2 = start_new_tree(k2, &i);
  EXPECT_EQ(root2->getKeyLen(0), BorderNode::key_len_has_suffix);
  EXPECT_EQ(root->getKeySlice(0), ONE);
//  EXPECT_EQ(root2->getKeySuffixes().get(0)->lastSliceSize, 3);
}

TEST(PutTest, check_break_invariant){
  BorderNode border{};
  Key k({ONE, AB}, 10);
  EXPECT_EQ(check_break_invariant(&border, k), std::nullopt);
  border.setKeyLen(1, BorderNode::key_len_has_suffix);
  border.setKeySlice(1, TWO);
  EXPECT_EQ(check_break_invariant(&border, k), std::nullopt);
  border.setKeyLen(0, BorderNode::key_len_has_suffix);
  border.setKeySlice(0, ONE);
  BigSuffix suffix({CD}, 3);
  border.getKeySuffixes().set(0, &suffix);
  EXPECT_EQ(check_break_invariant(&border, k), std::make_optional(0));
}


TEST(PutTest, handle_break_invariant){
  BorderNode borderNode{};
  int i = 4;
  borderNode.setKeyLen(0, 8);
  borderNode.setKeySlice(0, EIGHT);
  borderNode.setLV(0, LinkOrValue(&i));
  borderNode.setKeyLen(1, BorderNode::key_len_has_suffix);
  borderNode.setKeySlice(1, EIGHT);
  auto suffix = new BigSuffix({ONE, TWO, THREE, AB}, 2);
  borderNode.getKeySuffixes().set(1, suffix);
  borderNode.setLV(1, LinkOrValue(&i));
  int j = 5;
  Key k({EIGHT, ONE, TWO, CD}, 26);
  handle_break_invariant(&borderNode, k, &j, 1);

  ASSERT_EQ(borderNode.getKeyLen(1), BorderNode::key_len_layer);
  ASSERT_EQ(borderNode.getKeySuffixes().get(1), nullptr);
  auto next = reinterpret_cast<BorderNode *>(borderNode.getLV(1).next_layer);
  ASSERT_EQ(next->getKeyLen(0), BorderNode::key_len_layer);
  ASSERT_EQ(next->getKeySlice(0), ONE);
  next = reinterpret_cast<BorderNode *>(next->getLV(0).next_layer);
  ASSERT_EQ(next->getKeyLen(0), BorderNode::key_len_layer);
  ASSERT_EQ(next->getKeySlice(0), TWO);
  next = reinterpret_cast<BorderNode *>(next->getLV(0).next_layer);
  ASSERT_EQ(next->getKeyLen(0), 2);
  ASSERT_EQ(next->getKeySlice(0), CD);
  ASSERT_EQ(next->getKeyLen(1), BorderNode::key_len_has_suffix);
  ASSERT_EQ(next->getKeySlice(1), THREE);
  ASSERT_EQ(next->getKeySuffixes().get(1)->getCurrentSlice().slice, AB);
}

TEST(PutTest, insert_into_border){
  BorderNode border{};
  BorderNode next{};
  int i = 9;
  border.setKeyLen(0, 2);
  border.setKeySlice(0, ONE);
  border.setLV(0, LinkOrValue(&i));
  border.setKeyLen(1, BorderNode::key_len_has_suffix);
  border.setKeySlice(1, THREE);
  border.getKeySuffixes().set(1, new BigSuffix({FOUR}, 8));
  border.setLV(1, LinkOrValue(&i));
  border.setKeyLen(2, BorderNode::key_len_layer);
  border.setKeySlice(2, FOUR);
  border.setLV(2, LinkOrValue(&next));
  border.setKeyLen(3, 3);
  border.setKeySlice(3, FIVE);
  border.setLV(3, LinkOrValue(&i));

  Key k({TWO, FIVE}, 16);
  insert_into_border(&border, k, &i);

  EXPECT_EQ(border.getKeyLen(1), BorderNode::key_len_has_suffix);
  EXPECT_EQ(border.getKeySlice(1), TWO);
  EXPECT_TRUE(border.getKeySuffixes().get(1)->getCurrentSlice().slice == FIVE);
  EXPECT_EQ(border.getKeyLen(2), BorderNode::key_len_has_suffix);
  EXPECT_EQ(border.getKeySlice(2), THREE);
  EXPECT_TRUE(border.getKeySuffixes().get(2)->getCurrentSlice().slice == FOUR);
  EXPECT_EQ(border.getKeyLen(3), BorderNode::key_len_layer);
}




TEST(PutTest, split_keys_among1){
  InteriorNode p{};
  InteriorNode p1{};
  InteriorNode n1{};
  InteriorNode d1{};
  // p should full
  p.setNumKeys(Node::ORDER - 1);
  for(size_t i = 0; i < 16; ++i)
    p.setChild(i, &d1);

  p.setKeySlice(0, 0);
  p.setKeySlice(1, 1);
  p.setKeySlice(2, 2);
  p.setKeySlice(3, 3);
  p.setKeySlice(4, 4);
  p.setKeySlice(5, 5);
  p.setKeySlice(6, 6);
  p.setKeySlice(7, 7);
  p.setKeySlice(8, 9);
  p.setKeySlice(9, 10);
  p.setKeySlice(10, 11);
  p.setKeySlice(11, 12);
  p.setKeySlice(12, 13);
  p.setKeySlice(13, 14);
  p.setKeySlice(14, 15);
  std::optional<KeySlice> k_prime{};
  split_keys_among(&p, &p1, 8, &n1, 7, k_prime);
  EXPECT_EQ(p.getNumKeys(), 7);
  EXPECT_EQ(p.getKeySlice(7), 0);
  EXPECT_EQ(p.getChild(8), nullptr);
  EXPECT_EQ(p1.getNumKeys(), 8);
  EXPECT_EQ(p.getKeySlice(7), 0);
  EXPECT_EQ(p.getChild(8), nullptr);
}


TEST(PutTest, slice_table){
  // NOTE: より効率的なアルゴリズムを考えるべき

  BorderNode n{};
  n.setKeyLen(0, 1);
  n.setKeySlice(0, ONE);
  n.setKeyLen(1, 2);
  n.setKeySlice(1, ONE);
  n.setKeyLen(2, 3);
  n.setKeySlice(2, ONE);
  n.setKeyLen(3, 4);
  n.setKeySlice(3, ONE);
  n.setKeyLen(4, BorderNode::key_len_layer);
  n.setKeySlice(4, ONE);

  n.setKeyLen(5, 1);
  n.setKeySlice(5, TWO);
  n.setKeyLen(6, 2);
  n.setKeySlice(6, TWO);
  n.setKeyLen(7, 3);
  n.setKeySlice(7, TWO);
  n.setKeyLen(8, 4);
  n.setKeySlice(8, TWO);
  n.setKeyLen(9, BorderNode::key_len_layer);
  n.setKeySlice(9, TWO);

  n.setKeyLen(10, 1);
  n.setKeySlice(10, FOUR);
  n.setKeyLen(11, 2);
  n.setKeySlice(11, FOUR);
  n.setKeyLen(12, 3);
  n.setKeySlice(12, FOUR);
  n.setKeyLen(13, 4);
  n.setKeySlice(13, FOUR);
  n.setKeyLen(14, BorderNode::key_len_layer);
  n.setKeySlice(14, FOUR);

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
  EXPECT_EQ(split_point(ONE, table, found), 5 + 1);
  EXPECT_EQ(split_point(TWO, table, found), 5);
  EXPECT_EQ(split_point(THREE, table, found), 10 + 1);
  EXPECT_EQ(split_point(FOUR, table, found), 10);
  EXPECT_EQ(split_point(FIVE, table, found), 15);
}

TEST(PutTest, split_keys_among2){
  BorderNode n{};
  BorderNode n1{};

  int i = 9;
  n.setKeyLen(0, 1);
  n.setKeySlice(0, 110);
  n.setLV(0, LinkOrValue(&i));

  n.setKeyLen(1, 2);
  n.setKeySlice(1, 110);
  n.setLV(1, LinkOrValue(&i));

  n.setKeyLen(2, 3);
  n.setKeySlice(2, 110);
  n.setLV(2, LinkOrValue(&i));

  n.setKeyLen(3, BorderNode::key_len_has_suffix);
  n.setKeySlice(3, 110);
  n.getKeySuffixes().set(3, new BigSuffix({AB}, 2));
  n.setLV(3, LinkOrValue(&i));


  n.setKeyLen(4, 1);
  n.setKeySlice(4, 111);
  n.setLV(4, LinkOrValue(&i));

  n.setKeyLen(5, 2);
  n.setKeySlice(5, 111);
  n.setLV(5, LinkOrValue(&i));

  n.setKeyLen(6, 3);
  n.setKeySlice(6, 111);
  n.setLV(6, LinkOrValue(&i));

  n.setKeyLen(7, BorderNode::key_len_has_suffix);
  n.setKeySlice(7, 111);
  n.getKeySuffixes().set(7, new BigSuffix({CD}, 2));
  n.setLV(7, LinkOrValue(&i));


  n.setKeyLen(8 ,1);
  n.setKeySlice(8, 113);
  n.setLV(8, LinkOrValue(&i));

  n.setKeyLen(9 ,2);
  n.setKeySlice(9, 113);
  n.setLV(9, LinkOrValue(&i));

  n.setKeyLen(10, 3);
  n.setKeySlice(10, 113);
  n.setLV(10, LinkOrValue(&i));

  n.setKeyLen(11, BorderNode::key_len_layer);
  n.setKeySlice(11, 113);
  BorderNode next{};
  n.setLV(11, LinkOrValue(&next));


  n.setKeyLen(12, 1);
  n.setKeySlice(12, 114);
  n.setLV(12, LinkOrValue(&i));

  n.setKeyLen(13, 2);
  n.setKeySlice(13 ,114);
  n.setLV(13, LinkOrValue(&i));

  n.setKeyLen(14, 3);
  n.setKeySlice(14, 114);
  n.setLV(14, LinkOrValue(&i));

  Key k({112, AB}, 10);
  split_keys_among(&n, &n1, k, &i);
  EXPECT_EQ(n.getKeyLen(8), 9);
  EXPECT_EQ(n1.getKeySuffixes().get(1), nullptr);
}