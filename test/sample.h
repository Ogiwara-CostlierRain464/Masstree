#ifndef MASSTREE_SAMPLE_H
#define MASSTREE_SAMPLE_H

#include "../include/tree.h"

using namespace masstree;

static Node *sample1(){
  auto root = new BorderNode;

  root->key_slice[0] = 0x01020304;
  root->key_len[0] = 4;
  root->lv[0].value = new int(1);
  root->key_slice[1] = 0x0102030405060708;
  root->key_len[1] = 8;
  root->lv[1].value = new int(2);
  root->version.is_root = true;

  return root;
}

static Node *sample2(){
  auto root = new BorderNode;

  root->key_len[0] = BorderNode::key_len_has_suffix;
  root->key_slice[0] = 0x0001020304050607;
  Key key({0x0001'0203'0405'0607, 0x0A0B'0000'0000'0000}, 10);
  root->key_suffixes.set(0,key,1);
  root->lv[0].value = new int(1);
  root->version.is_root = true;

  return root;
}

static Node *sample3(){
  auto l1_root = new BorderNode;
  auto l2_root = new BorderNode;

  l1_root->key_slice[0] = 0x0001020304050607;
  l1_root->key_len[0] = BorderNode::key_len_layer;
  l1_root->lv[0].next_layer = l2_root;
  l1_root->version.is_root = true;

  l2_root->key_slice[0] = 0x0A0B'0000'0000'0000;
  l2_root->key_len[0] = 2;
  l2_root->lv[0].value = new int(1);
  l2_root->key_slice[1] = 0x0C0D'0000'0000'0000;
  l2_root->key_len[1] = 2;
  l2_root->lv[1].value = new int(2);
  l2_root->version.is_root = true;

  return l1_root;
}

static Node *sample4(){
  auto root = new InteriorNode;
  auto _9 = new BorderNode;
  auto _11 = new BorderNode;
  auto _160 = new BorderNode;

  root->version.is_root = true;
  root->n_keys = 15;
  root->key_slice[0] = 0x0100;
  root->key_slice[1] = 0x0200;
  root->key_slice[2] = 0x0300;
  root->key_slice[3] = 0x0400;
  root->key_slice[4] = 0x0500;
  root->key_slice[5] = 0x0600;
  root->key_slice[6] = 0x0700;
  root->key_slice[7] = 0x0800;
  root->key_slice[8] = 0x0900;
  root->key_slice[9] = 0x010000;
  root->key_slice[10] = 0x010100;
  root->key_slice[11] = 0x010200;
  root->key_slice[12] = 0x010300;
  root->key_slice[13] = 0x010400;
  root->key_slice[14] = 0x010500;
  root->child[0] = _9;
  root->child[1] = _11;
  root->child[15] = _160;

  _9->key_len[0] = 1;
  _9->key_slice[0] = 0x09;
  _9->lv[0].value = new int(18);
  _9->parent = root;

  _11->key_len[0] = 2;
  _11->key_slice[0] = 0x0101;
  _11->lv[0].value = new int(22);
  _11->parent = root;

  _160->key_len[0] = 3;
  _160->key_slice[0] = 0x010600;
  _160->lv[0].value = new int(320);
  _160->parent = root;

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
  }, 8 * 2);

  Key k9({
    ONE,
    TWO,
    AB
  }, 16 + 2);

  return std::pair(k8, k9);
}

static std::pair<Key, Key> conflicting(){
  Key k1({
    ONE,
    TWO,
    AB
  }, 16 + 2);
  Key k2({
    ONE,
    TWO,
    CD
  }, 16 + 2);
  return std::pair(k1, k2);
}


#endif //MASSTREE_SAMPLE_H
