#ifndef MASSTREE_SAMPLE_H
#define MASSTREE_SAMPLE_H

#include "../include/tree.h"

using namespace masstree;

static Node *sample1(){
  auto root = new BorderNode;

  root->setKeySlice(0, 0x01020304);
  root->setKeyLen(0, 4);
  root->setLV(0, LinkOrValue(new Value(1)));
  root->setKeySlice(1, 0x0102030405060708);
  root->setKeyLen(1, 8);
  root->setLV(1, LinkOrValue(new Value(2)));
  root->setIsRoot(true);
  root->setPermutation(Permutation::fromSorted(2));

  return root;
}

static Node *sample2(){
  auto root = new BorderNode;

  root->setKeyLen(0, BorderNode::key_len_has_suffix);
  root->setKeySlice(0, 0x0001020304050607);
  Key key({0x0001'0203'0405'0607, 0x0A0B'0000'0000'0000}, 2);
  root->getKeySuffixes().set(0,key,1);
  root->setLV(0, LinkOrValue(new Value(1)));
  root->setIsRoot(true);
  root->setPermutation(Permutation::fromSorted(1));

  return root;
}

static Node *sample3(){
  auto l1_root = new BorderNode;
  auto l2_root = new BorderNode;

  l1_root->setKeySlice(0, 0x0001020304050607);
  l1_root->setKeyLen(0, BorderNode::key_len_layer);
  l1_root->setLV(0, LinkOrValue(l2_root));
  l1_root->setIsRoot(true);
  l1_root->setPermutation(Permutation::fromSorted(1));

  l2_root->setKeySlice(0, 0x0A0B'0000'0000'0000);
  l2_root->setKeyLen(0 ,2);
  l2_root->setLV(0, LinkOrValue(new Value(1)));
  l2_root->setKeySlice(1, 0x0C0D'0000'0000'0000);
  l2_root->setKeyLen(1, 2);
  l2_root->setLV(1, LinkOrValue(new Value(2)));
  l2_root->setIsRoot(true);
  l2_root->setPermutation(Permutation::fromSorted(2));

  return l1_root;
}

static Node *sample4(){
  auto root = new InteriorNode;
  auto _9 = new BorderNode;
  auto _11 = new BorderNode;
  auto _160 = new BorderNode;

  root->setIsRoot(true);
  root->setNumKeys(15);
  root->setKeySlice(0, 0x0100);
  root->setKeySlice(1, 0x0200);
  root->setKeySlice(2, 0x0300);
  root->setKeySlice(3, 0x0400);
  root->setKeySlice(4, 0x0500);
  root->setKeySlice(5, 0x0600);
  root->setKeySlice(6, 0x0700);
  root->setKeySlice(7, 0x0800);
  root->setKeySlice(8, 0x0900);
  root->setKeySlice(9, 0x010000);
  root->setKeySlice(10, 0x010100);
  root->setKeySlice(11, 0x010200);
  root->setKeySlice(12, 0x010300);
  root->setKeySlice(13, 0x010400);
  root->setKeySlice(14, 0x010500);
  root->setChild(0, _9);
  root->setChild(1, _11);
  root->setChild(15, _160);

  _9->setKeyLen(0, 1);
  _9->setKeySlice(0, 0x09);
  _9->setLV(0, LinkOrValue(new Value(18)));
  _9->setParent(root);

  _11->setKeyLen(0, 2);
  _11->setKeySlice(0, 0x0101);
  _11->setLV(0, LinkOrValue(new Value(22)));
  _11->setParent(root);

  _160->setKeyLen(0, 3);
  _160->setKeySlice(0, 0x010600);
  _160->setLV(0, LinkOrValue(new Value(320)));
  _160->setParent(root);

  return root;
}

/**
 * 0x1111111111111111 => 1229782938247303441
 * 0x2222222222222222 => 2459565876494606882
 * 0x3333333333333333 => 3689348814741910323
 * 0x4444444444444444 => 4919131752989213764
 * 0x5555555555555555 => 6148914691236517205
 * 0x6666666666666666 => 7378697629483820646
 * 0x7777777777777777 => 8608480567731124087
 * 0x8888888888888888 => 9838263505978427528
 * 0x9999999999999999 => 11068046444225730969
 *
 * 0x0102030000000000 => 72623842526232576
 * 0x0204060800000000 => 145247719412203520
 * 0x0A0B000000000000 => 723672165123096576
 * 0x0C0D000000000000 => 868350303152373760
 */

static constexpr uint64_t ONE = 0x1111111111111111;
static constexpr uint64_t TWO = 0x2222222222222222;
static constexpr uint64_t THREE = 0x3333333333333333;
static constexpr uint64_t FOUR = 0x4444444444444444;
static constexpr uint64_t FIVE = 0x5555555555555555;
static constexpr uint64_t SIX = 0x6666666666666666;
static constexpr uint64_t SEVEN = 0x7777777777777777;
static constexpr uint64_t EIGHT = 0x8888888888888888;
static constexpr uint64_t NINE = 0x9999999999999999;

static constexpr uint64_t ONE_2_3 = 0x0102030000000000;
static constexpr uint64_t TWO_4_6_8 = 0x0204060800000000;
static constexpr uint64_t AB = 0x0A0B000000000000;
static constexpr uint64_t CD = 0x0C0D000000000000;

static std::pair<Key, Key> not_conflict_89(){
  Key k8({
    ONE,
    TWO
  }, 8);

  Key k9({
    ONE,
    TWO,
    AB
  }, 2);

  return std::pair(k8, k9);
}

static void skipped_border(BorderNode &n){
  n.setKeyLen(0, 5);
  n.setKeySlice(0, 2);
  n.setKeyLen(1, 7);
  n.setKeySlice(1, 2);
  n.setKeyLen(2, 0);
  n.setKeySlice(2, 0);

  n.setKeyLen(3, 1);
  n.setKeySlice(3, 1);
  n.setKeyLen(4, 2);
  n.setKeySlice(4, 1);
  n.setKeyLen(5, 8);
  n.setKeySlice(5, 1);

  Permutation p;
  n.setPermutation(Permutation::from({
    3,4,5,0,1
  }));
}

static void full_unsorted_border(BorderNode &n){
  for(size_t i = 0; i <= 7; ++i){
    n.setKeyLen(i, i+1);
    n.setKeySlice(i, TWO);
  }
  for(size_t i = 8; i <= 14; ++i){
    n.setKeyLen(i, i-7);
    n.setKeySlice(i, ONE);
  }
  n.setPermutation(Permutation::from({
    8,9,10,11,12,13,14,0,1,2,3,4,5,6,7
  }));
}

#endif //MASSTREE_SAMPLE_H
