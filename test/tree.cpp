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

  write(root, key, new int(23));
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

TEST(Tree, duplex){
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

TEST(TreeTest, hard){
  Node *root = nullptr;
  for(size_t i = 0; i < 10000; ++i){
    std::vector<KeySlice> slices{};
    size_t j;
    for(j = 1; j <= i; ++j){
      slices.push_back(TWO);
    }
    slices.push_back(AB);

    Key k(slices, (slices.size() -1) * 8 + 2);
    root = put(root, k, new int(i), nullptr, 0);
  }

  Key k({TWO, TWO, TWO, AB}, 26);
  auto p = get(root, k);
  EXPECT_TRUE(p != nullptr);

  print_sub_tree(root);

  // 現在は、「keyはunique」という前提は置かれているが、「key sliceはunique」という前提が
  // 破綻している。interior nodeには同じkey sliceが現れないようにしなければならない。
  // 今度は、重複するkeyを先に見つけられていない。
}


// とりあえず、先にテストをベタがきする


class BigSuffixTest: public ::testing::Test{
};


TEST(BigSuffixTest, from){
  BigSuffix *suffix;
  {
    Key k({
            ONE,
            TWO,
            THREE,
            AB
          }, 26);

    suffix = BigSuffix::from(k, 1);
  }
  // suffixにコピー済み

  EXPECT_EQ(suffix->slices[0], TWO);
  EXPECT_EQ(suffix->slices[1], THREE);
  EXPECT_EQ(suffix->slices[2], AB);
  EXPECT_EQ(suffix->lastSliceSize, 2);

  suffix->next();
  suffix->next();
  EXPECT_EQ(suffix->getCurrentSlice().slice, AB);
}


TEST(BigSuffixTest, isSame){
  auto suffix = BigSuffix({
    TWO,
    ONE,
    AB
  }, 2);

  Key k({
    EIGHT,
    NINE,
    TWO,
    ONE,
    AB
  }, 34);

  EXPECT_TRUE(suffix.isSame(k, 2));

  auto suffix1 = BigSuffix({
    TWO,
    AB
  }, 2);

  Key k1({
    TWO,
    AB
  }, 12);

  EXPECT_FALSE(suffix1.isSame(k1, 0));
}



class KeyTest: public ::testing::Test{};

TEST(KeyTest, equal){
  Key a({
    TWO,
    ONE
  },15);
  Key b({
    TWO,
    ONE
  },16);

  EXPECT_FALSE(a == b);
}

TEST(KeyTest, lastSliceSize){
  Key a({
    TWO,
    ONE
  },15);


  EXPECT_EQ(a.lastSliceSize(), 7);
}

TEST(KeyTest, getCurrentSlice){
  Key a({
    TWO,
    EIGHT,
    ONE
  },20);

  a.next();
  EXPECT_EQ(a.getCurrentSlice().slice, EIGHT);
  a.next();
  EXPECT_EQ(a.getCurrentSlice().size, 4);
}


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






