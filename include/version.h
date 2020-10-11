#ifndef MASSTREE_VERSION_H
#define MASSTREE_VERSION_H

#include <cstdint>

namespace masstree{

/**
 * Version構造体を表す。
 * 4byteに収まるように、論文中のメモのように
 * オリジナルのものから変更してある。
 */
struct Version{

  /**
   * 論文中の"locked"に対応する。
   * versionの前後のスナップショットに対して排他論理和を取り、
   * bit列に変化があれば「他のthreadがlockを取った、もしくはlockを取った後に外した」
   * と判断する。
   *
   * NOTE: has lockedな状態が単にlocked, inserting, splitting, vinsert, vsplitの変化だけなのか、
   * それとも他の全ても含めるのか、で選択の余地がある
   * おそらく、特定のfieldの変化のみを追うことによって並行性性能をあげられそうだ
   */
  static constexpr uint32_t has_locked = 0;

  union {
    uint32_t body;
    struct {
      uint16_t v_split: 16;
      uint8_t v_insert: 8;
      bool : 2;
      bool is_border: 1;
      bool is_root: 1;
      bool deleted: 1;
      bool splitting: 1;
      bool inserting: 1;
      bool locked: 1;
    };
  };

  Version() noexcept
    : locked{false}
    , inserting{false}
    , splitting{false}
    , deleted{false}
    , is_root{false}
    , is_border{false}
    , v_insert{0}
    , v_split{0}
  {}

  uint32_t operator ^(const Version &rhs) const{
    return (body ^ rhs.body);
  }
};


}

#endif //MASSTREE_VERSION_H
