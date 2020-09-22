#include <gtest/gtest.h>
#include <random>
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

TEST(TreeTest, split){
  // work at B+ tree test.

  Node *root = nullptr;
  for(uint64_t i = 1; i <= 10000; ++i){
    Key k({i}, 1);
    root = insert(root, k, new int(i));
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
  insert(root, k, new int(3));


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
  root = insert(root, k, new int(1));
  Key k1({
    0x8888'8888'8888'8888,
    0x1111'1111'1111'1111,
    0x2222'2222'2222'2222,
    0x0C0D'0000'0000'0000
  },26);
  root = insert(root, k1, new int(2));
  // Key is mutable, but you can reset cursor.
  k1.cursor = 0;
  auto p = get(root, k1);
  assert(p != nullptr);
  EXPECT_EQ(*reinterpret_cast<int *>(p),2);

}

TEST(TreeTest, break_invariant3){
  auto pair = not_conflict_89();
  auto root = insert(nullptr, pair.first, new int(1));
  root = insert(root, pair.second, new int(7));
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

  auto root = insert(nullptr, k1, new int(1));
  root = insert(root, k2, new int(2));
  root = insert(root, k3, new int(3));
  root = insert(root, k4, new int(4));
  root = insert(root, k5, new int(5));
  root = insert(root, k6, new int(6));
  root = insert(root, k7, new int(7));
  root = insert(root, k9, new int(9));
  root = insert(root, k8, new int(8));

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
  root = insert(root, k, new int(4));
  root = insert(root, k1, new int(8));
  root = insert(root, k2, new int(6));
  k.cursor = 0;
  auto p = get(root, k);
  EXPECT_EQ(*reinterpret_cast<int *>(p), 6);
}

TEST(TreeTest, hard){
  Node *root = nullptr;
  for(size_t i = 0; i < 100; ++i){
    std::vector<KeySlice> slices{};
    size_t j;
    for(j = 1; j <= i; ++j){
      slices.push_back(TWO);
    }
    slices.push_back(AB);

    Key k(slices, (slices.size() -1) * 8 + 2);
    root = insert(root, k, new int(i));
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

}






