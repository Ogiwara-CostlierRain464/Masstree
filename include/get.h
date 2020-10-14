#ifndef MASSTREE_GET_H
#define MASSTREE_GET_H

#include "tree.h"
#include "debug_helper.h"
#include <algorithm>
#include <xmmintrin.h>

namespace masstree{

#ifndef NDEBUG
// lvを取得する直後のポイント
static SequentialHandler get_handler0{};
// lvを取得した直後のポイント　
static SequentialHandler get_handler1{};
// has_lockedである事が確認された直後のポイント
static Marker has_locked_marker{};
// UNSTABLEである事が確認された直後のポイント
static Marker was_unstable_marker{};
#endif

[[maybe_unused]]
static Value *get(Node *root, Key &k){
  if(root == nullptr){
    // Layer0が空の時にのみ、ここにくる
    assert(k.cursor == 0);
    return nullptr;
  }
retry:
  auto n_v = findBorder(root, k); auto n = n_v.first; auto v = n_v.second;
forward:
  if(v.deleted){
    if(v.is_root){
      // 探していたKeyが上のLayerに行ってしまった時、あるいはLayer0が消えた時
      // 他のremoveによってここに到達するが、それは今まさに探そうとしているKeyに対してremoveされたからなので、
      // ここではnullptrを返す。
      return nullptr;
    }else{
      goto retry;
    }
  }
#ifndef NDEBUG
  get_handler0.giveAndWaitBackIfUsed();
#endif
  auto t_lv = n->extractLinkOrValueFor(k); auto t = t_lv.first; auto lv = t_lv.second;
#ifndef NDEBUG
  get_handler1.giveAndWaitBackIfUsed();
#endif
  if((n->getVersion() ^ v) > Version::has_locked){
#ifndef NDEBUG
    has_locked_marker.markIfUsed();
#endif
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
  }else{
    assert(t == UNSTABLE);
#ifndef NDEBUG
    was_unstable_marker.markIfUsed();
#endif
    goto forward;
  }
}

}

#endif //MASSTREE_GET_H
