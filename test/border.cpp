#include <gtest/gtest.h>
#include "sample.h"

using namespace masstree;


class BorderNodeTest: public ::testing::Test{};

TEST(BorderNodeTest, extractLinkOrValueWithIndexFor){
  BorderNode borderNode{};

  borderNode.setKeyLen(0, 2);
  borderNode.setKeySlice(0, ONE);
  Value val(1);
  borderNode.setLV(0, LinkOrValue(&val));
  borderNode.setKeyLen(1, BorderNode::key_len_has_suffix);
  borderNode.setKeySlice(1, ONE);
  BigSuffix suffix({TWO}, 2);
  borderNode.getKeySuffixes().set(1, &suffix);
  borderNode.setLV(1, LinkOrValue(&val));
  BorderNode next_layer{};
  borderNode.setKeyLen(2, BorderNode::key_len_layer);
  borderNode.setKeySlice(2, TWO);
  borderNode.setLV(2, LinkOrValue(&next_layer));

  auto p = Permutation::fromSorted(3);
  borderNode.setPermutation(p);

  Key k({ONE}, 2);
  auto tuple = borderNode.extractLinkOrValueWithIndexFor(k);
  EXPECT_EQ(std::get<0>(tuple), ExtractResult::VALUE);
  EXPECT_EQ(std::get<1>(tuple).value, &val);
  EXPECT_EQ(std::get<2>(tuple), 0);
  Key k2({ONE, TWO}, 2);
  auto tuple2 = borderNode.extractLinkOrValueWithIndexFor(k2);
  EXPECT_EQ(std::get<0>(tuple2), ExtractResult::VALUE);
  EXPECT_EQ(std::get<1>(tuple2).value, &val);
  EXPECT_EQ(std::get<2>(tuple2), 1);
  Key k3({TWO, THREE}, 8);
  auto tuple3 = borderNode.extractLinkOrValueWithIndexFor(k3);
  EXPECT_EQ(std::get<0>(tuple3), ExtractResult::LAYER);
  EXPECT_EQ(std::get<1>(tuple3).next_layer, &next_layer);
  EXPECT_EQ(std::get<2>(tuple3), 2);
  Key k4({ONE}, 8);
  auto tuple4 = borderNode.extractLinkOrValueWithIndexFor(k4);
  EXPECT_EQ(std::get<0>(tuple4), ExtractResult::NOTFOUND);
}

TEST(BorderNodeTest, numberOfKeys){
  auto node = reinterpret_cast<BorderNode *>(sample1());
  EXPECT_EQ(node->getPermutation().getNumKeys(), 2);
}

TEST(BorderNodeTest, sort){
  BorderNode n;
  full_unsorted_border(n);
  n.lock();
  n.setSplitting(true);
  n.sort();
  n.unlock();
  EXPECT_EQ(n.getKeyLen(0),1);
  EXPECT_EQ(n.getKeySlice(0),ONE);
  EXPECT_EQ(n.getKeyLen(7),1);
  EXPECT_EQ(n.getKeySlice(7),TWO);
}