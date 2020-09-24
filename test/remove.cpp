#include <gtest/gtest.h>
#include "../include/operation.h"

using namespace masstree;

class RemoveTest: public ::testing::Test{};

TEST(RemoveTest, left_or_right_most){
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
}