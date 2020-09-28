#include <gtest/gtest.h>
#include "sample.h"

using namespace masstree;


class BorderNodeTest: public ::testing::Test{};

TEST(BorderNodeTest, extractLinkOrValueWithIndexFor){
  BorderNode borderNode{};

  borderNode.key_len[0] = 2;
  borderNode.key_slice[0] = ONE;
  int i = 0;
  borderNode.lv[0].value = &i;
  borderNode.key_len[1] = BorderNode::key_len_has_suffix;
  borderNode.key_slice[1] = ONE;
  BigSuffix suffix({TWO}, 2);
  borderNode.key_suffixes.set(1, &suffix);
  borderNode.lv[1].value = &i;
  BorderNode next_layer{};
  borderNode.key_len[2] = BorderNode::key_len_layer;
  borderNode.key_slice[2] = TWO;
  borderNode.lv[2].next_layer = &next_layer;

  Key k({ONE}, 2);
  auto tuple = borderNode.extractLinkOrValueWithIndexFor(k);
  EXPECT_EQ(std::get<0>(tuple), ExtractResult::VALUE);
  EXPECT_EQ(std::get<1>(tuple).value, &i);
  EXPECT_EQ(std::get<2>(tuple), 0);
  Key k2({ONE, TWO}, 10);
  auto tuple2 = borderNode.extractLinkOrValueWithIndexFor(k2);
  EXPECT_EQ(std::get<0>(tuple2), ExtractResult::VALUE);
  EXPECT_EQ(std::get<1>(tuple2).value, &i);
  EXPECT_EQ(std::get<2>(tuple2), 1);
  Key k3({TWO, THREE}, 16);
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
  EXPECT_EQ(node->numberOfKeys(), 2);
}

