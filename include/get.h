#ifndef MASSTREE_GET_H
#define MASSTREE_GET_H

#include "tree.h"
#include <algorithm>

namespace masstree{

static void *get(Node *root, Key &k){
  retry:
  auto n_v = findBorder(root, k); auto n = n_v.first; auto v = n_v.second;
  forward:
  if(v.deleted)
    goto retry;
  auto t_lv = n->extractLinkOrValueFor(k); auto t = t_lv.first; auto lv = t_lv.second;
  if((n->getVersion() ^ v) > Version::lock){
    v = n->stableVersion(); auto next = n->getNext();
    while(!v.deleted and next != nullptr and k.getCurrentSlice().slice >= next->lowestKey()){
      n = next; v = n->stableVersion(); next = n->getNext();
    }
    goto forward;
  }else if(t == NOTFOUND){
    return nullptr;
  }else if(t == VALUE){
    return lv.value;
  }else if(t == LAYER){
    root = lv.next_layer;
    // advance k to next slice
    k.next();
    goto retry;
  }else{ // t == UNSTABLE
    goto forward;
  }
}

}

#endif //MASSTREE_GET_H
