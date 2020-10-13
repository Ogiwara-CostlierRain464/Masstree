#ifndef MASSTREE_MASSTREE_H
#define MASSTREE_MASSTREE_H

#include "put.h"
#include "get.h"
#include "remove.h"

namespace masstree{
class Masstree{
public:
  Value *get(Key &key){
    auto root_ = root.load(std::memory_order_acquire);
    auto v = ::masstree::get(root_, key);
    key.reset();
    return v;
  }

  void put(Key &key, Value *value, GC &gc){
retry:
    auto old_root = root.load(std::memory_order_acquire);
    auto new_root = ::masstree::put_at_layer0(old_root, key, value, gc);
    key.reset();
    // treeが空だった場合
    if(old_root == nullptr){
      auto not_overlapped = root.compare_exchange_weak(old_root, new_root);
      if(not_overlapped){
        return;
      }else{
        // treeが他によって更新された場合は作った木を消して、もう一度処理をやり直す
        // old_rootがnullだった場合は、new_rootは必ずBorderNodeとなる
        assert(new_root->getIsBorder());
        gc.add(reinterpret_cast<BorderNode*>(new_root));
        goto retry;
      }
    }

    // treeの変更以外は、論文中のアルゴリズムでrootの変更を検知できるので、やり直しの必要はない
    if(old_root != new_root){
      root.store(new_root, std::memory_order_release);
    }
  }

  void remove(Key &key, GC &gc){
retry:
    auto old_root = root.load(std::memory_order_acquire);
    auto new_root = ::masstree::remove_at_layer0(old_root, key, gc);
    key.reset();
    // treeが空だった場合
    if(old_root == nullptr){
      auto not_overlapped = root.compare_exchange_weak(old_root, new_root);
      if(not_overlapped){
        return;
      }else{
        // treeが他によって更新された場合は作った木を消して、もう一度処理をやり直す
        // old_rootがnullだった場合は、new_rootは必ずBorderNodeとなる
        assert(new_root->getIsBorder());
        gc.add(reinterpret_cast<BorderNode*>(new_root));
        goto retry;
      }
    }

    // treeの変更以外は、論文中のアルゴリズムでrootの変更を検知できるので、やり直しの必要はない
    if(old_root != new_root){
      root.store(new_root, std::memory_order_release);
    }
  }



private:
  std::atomic<Node *> root{nullptr};
};
}

#endif //MASSTREE_MASSTREE_H
