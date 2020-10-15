#ifndef MASSTREE_REMOVE_H
#define MASSTREE_REMOVE_H

#include "tree.h"
#include "alloc.h"
#include "gc.h"
#include <algorithm>


namespace masstree{


enum RootChange : uint8_t {
  NotChange,
  NewRoot,
  LayerDeleted
};

/**
 * removeの処理において、Layerのdeleteの処理を行う。
 * §4.6.3での記述と同じように、下のlayerを消してから、上のlayerの削除の処理に移動する。
 * @param n
 * @param upper_layer
 * @param upper_index
 */
static void handle_delete_layer_in_remove(BorderNode *n, BorderNode *upper_layer, size_t upper_index, GC &gc){
  auto p = n->getPermutation();
  assert(n->getParent() == nullptr);
  assert(p.getNumKeys() == 1);
  assert(n->getIsRoot());
  assert(upper_layer != nullptr);
  assert(n->getLocked());

  BigSuffix *upper_suffix;
  if(n->getKeyLen(p(0)) == BorderNode::key_len_has_suffix){
    auto old_suffix = n->getKeySuffixes().get(p(0));
    old_suffix->insertTop(n->getKeySlice(p(0)));
    // suffixをそのまま再利用
    upper_suffix = old_suffix;
  }else{
    // Key TerminalとなるBorderに対する処理なので、BorderNode::key_len_layerにはなりえない。
    assert(1 <= n->getKeyLen(p(0)) and n->getKeyLen(p(0)) <= 8);
    upper_suffix = new BigSuffix({n->getKeySlice(p(0))}, n->getKeyLen(p(0)));
#ifndef NDEBUG
    Alloc::incSuffix();
#endif
  }

  // n -> upper_layerの順で
  upper_layer->lock();

  /**
   * まず、UNSTABLEとマークする
   * 次に、KeySuffixへのリンクを貼る
   * そして、lvを書き換える
   * そして、HAS_SUFFIXに書き換える
   * 最後に、nの方のlvとKeySuffixをunrefしておく
   */
  upper_layer->setKeyLen(upper_index, BorderNode::key_len_unstable);
  assert(upper_layer->getKeySuffixes().get(upper_index) == nullptr);
  upper_layer->getKeySuffixes().set(upper_index, upper_suffix);
  upper_layer->setLV(upper_index, n->getLV(p(0)));
  upper_layer->setKeyLen(upper_index, BorderNode::key_len_has_suffix);

    // 元のValueをクリアにしておく。
  n->setLV(p(0), LinkOrValue{});
  n->getKeySuffixes().set(p(0), nullptr);

  n->setDeleted(true);
  gc.add(n);

  // n -> upper_layer の順番でunlockする
  n->unlock();
  upper_layer->unlock();
}



/**
 * removeの処理で、Borderがdeleteされ、parentの配置が変わる時の
 * 処理。
 * Borderがdeleteされるのは、その中の要素数が0の時のみ。
 *
 * @param n
 * @return new root
 */
static std::pair<RootChange, Node*> delete_border_node_in_remove(BorderNode *n, BorderNode *upper_layer, size_t upper_index, GC &gc){
  assert(n->getLocked());
  auto per = n->getPermutation();
  // すでに要素は削除済み
  assert(per.getNumKeys() == 0);

  if(n->getIsRoot()){
    // Layer 0以外ではここには到達しない
    assert(n->getParent() == nullptr);
    assert(upper_layer == nullptr);
    n->setDeleted(true);
    gc.add(n);
    return std::make_pair(LayerDeleted, nullptr);
  }

  auto p = n->getParent();
  // n -> pの順でlockをとる
  p->lock();
  auto n_index = p->findChildIndex(n);

  if(p->getNumKeys() >= 2){
    // 左にシフト
    if(n_index == 0){
      for(size_t i = 0; i <= 13; ++i){
        p->setKeySlice(i, p->getKeySlice(i+1));
      }
      for(size_t i = 0; i <= 14; ++i){
        p->setChild(i, p->getChild(i+1));
      }
    }else{
      for(size_t i = n_index-1; i <= 13; ++i){
        p->setKeySlice(i, p->getKeySlice(i+1));
      }
      for(size_t i = n_index; i <= 14; ++i){
        p->setChild(i, p->getChild(i+1));
      }
    }
    p->decNumKeys();

    n->connectPrevAndNext();
    n->setDeleted(true);
    gc.add(n);
    // rootの変更無し
  }else{
    assert(p->getNumKeys() == 1);
    auto pull_up_index = n_index == 1 ? 0 : 1;
    auto pull_up_node = p->getChild(pull_up_index);
    if(p->getIsRoot()){
      assert(p->getParent() == nullptr);
      // rootが変わり、upper_layerからの付け替えが必要になる
      if(upper_layer == nullptr){
        // Layer0の時
        // is_rootをtrueにしてからparentをnullptrにする
        pull_up_node->setIsRoot(true);
        pull_up_node->setParent(nullptr);
        n->connectPrevAndNext();
        p->setDeleted(true);
        n->setDeleted(true);
        gc.add(p);
        gc.add(n);
        return std::make_pair(NewRoot, pull_up_node);
      }else{
        // upper layerの更新
        // is_rootをtrueにしてからparentをnullptrにする
        pull_up_node->setIsRoot(true);
        pull_up_node->setParent(nullptr);
        upper_layer->setLV(upper_index, LinkOrValue(pull_up_node));

        n->connectPrevAndNext();
        p->setDeleted(true);
        n->setDeleted(true);
        gc.add(p);
        gc.add(n);
        return std::make_pair(NewRoot, pull_up_node);
      }
    }else{
      auto pp = p->getParent();
      auto p_index = pp->findChildIndex(p);
      pp->setChild(p_index, pull_up_node);
      pull_up_node->setParent(pp);

      n->connectPrevAndNext();
      p->setDeleted(true);
      n->setDeleted(true);
      gc.add(p);
      gc.add(n);
    }
  }

  // n -> p の順でunlock
  n->unlock();
  p->unlock();

  return std::make_pair(NotChange, nullptr);
}

/**
 * treeから該当するkey-valueを削除する。
 * @param root
 * @param k
 * @param upper_layer rootをnext_layerとして持つnode
 * @param upper_index rootをnext_layerとして持つnode中のindex
 * @return 新しいroot
 */
[[maybe_unused]]
static std::pair<RootChange, Node*> remove(Node *root, Key &k, BorderNode *upper_layer, size_t upper_index, GC &gc){
  if(root == nullptr){
    // Layer0以外では起きえない
    assert(upper_layer == nullptr);
    return std::make_pair(NotChange, nullptr);
  }

retry:
  auto n_v = findBorder(root, k); auto n = n_v.first; auto v = n_v.second;
  n->lock();
  /**
   * removeの場合はfindBorderでnをゲットしたら、すぐにlockをする
   * findBorderとlockの間にそのnodeがdeletedになるかもしれないし、
   * 値を削除すべきBorderNodeがsplitによって移動されるかもしれないし、
   * 他のremoveが該当するキーを消すかもしれない(あるいは、同じkeyのremoveが二度呼ばれるかもしれない)
   * ここでキーとなるのが、値の削除が発生してもkeyが左に動かない、という不変性である！
   */
forward:
  assert(n->getLocked());
  if(v.deleted){
    n->unlock();
    if(v.is_root){
      // 他のremoveが代わりに消したことになる
      // よって、ここで処理は終わる。
      return std::make_pair(NotChange, root);
    }else{
      goto retry;
    }
  }
  auto t_lv_i = n->extractLinkOrValueWithIndexFor(k);
  auto t = std::get<0>(t_lv_i);
  auto lv = std::get<1>(t_lv_i);
  auto index = std::get<2>(t_lv_i);
  if(Version::splitHappened(v, n->getVersion())){
    // findBorderとlockの間でsplit処理が起きたら
    v = n->getVersion();
    auto next = n->getNext();

    while (!v.deleted and next != nullptr){
      next->lock();
      n->unlock(); // Hand-over-Hand locking
      if(k.getCurrentSlice().slice >= next->lowestKey()){
        n = next; v = n->getVersion(); next = n->getNext();
      }else{
        break;
      }
    }
    goto forward;
  }else if(t == NOTFOUND){
    // 何もしない?
    // 何らかの形でユーザに通知を行うべきだろうか？
    n->unlock();
  }else if(t == VALUE){
    /**
     * 削除
     *
     * 要素が一個しかないBorderNodeがrootならそのLayerは消えて、
     * 上のLayerのsuffixにセットする。
     * そして、上のLayerからそのKeyのremoveをやり直す。(そのLayerのrootから)
     *
     * そうでないなら、
     * まずborder nodeから消す
     * border node内で左シフト
     * 次にborder node内の個数を調べる
     * border nodeにまだあるならそれで終わり

     * border nodeが空になるなら、parentとしてのinteriorの変更が発生
     *
     * parentが残り二つ以上の時
     * 左シフト
     *
     * parentが一つの時
     * - root 付近の時
     * upper_layerからの繋ぎ直し、is_rootの更新
     *
     * - そうでない時
     * grand parentからリンクを伸ばす
     *
     * NOTE: どのケースにおいても、先に親としてのinteriorに先に
     * ロックをかける必要がありそうだ
     */

    // lockを取ってからextractLinkを行ったので、これは実行されない。
//    if(n->isKeyRemoved(index)){
//      n->unlock();
//      return std::make_pair(NotChange, root);
//    }

    auto p = n->getPermutation();
    if(n->getIsRoot() and p.getNumKeys() == 1 and upper_layer != nullptr){
      // layer0の時以外で、残りの要素数が1のBorderNodeがRootの時
      // 要素数への変更は生じない
      handle_delete_layer_in_remove(n, upper_layer, upper_index, gc);
      return std::make_pair(LayerDeleted, nullptr);
    }

    n->markKeyRemoved(index);
    p.removeIndex(index);
    n->setPermutation(p);

    auto current_num_keys = p.getNumKeys();

    if(current_num_keys == 0){
      auto pair = delete_border_node_in_remove(n, upper_layer, upper_index, gc);
      if(pair.first != NotChange){
        return pair;
      }
    }else{
      n->unlock();
    }
  }else if(t == LAYER){
    n->unlock();
    k.next();
    auto pair = remove(lv.next_layer, k, n, index, gc);
    if(pair.first == LayerDeleted){
      k.back();
      goto retry;
    }
  }else{
    // t == UNSTABLE
    assert(false);
  }

  return std::make_pair(NotChange, root);
}

[[nodiscard]]
static Node *remove_at_layer0(Node *root, Key &k, GC &gc){
  return remove(root, k, nullptr, 0, gc).second;
}

}

#endif //MASSTREE_REMOVE_H
