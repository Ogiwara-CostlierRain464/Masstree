#ifndef MASSTREE_DEBUG_HELPER_H
#define MASSTREE_DEBUG_HELPER_H

#include <atomic>
#include <xmmintrin.h>

namespace masstree{

/**
 * Multi threadにおいて、逐次実行を実現させるための機構
 * handlerをgiveすることにより別のスレッドが処理を開始でき、
 * backされると自分のthreadが処理を再開する。
 */
class SequentialHandler{
public:
  SequentialHandler() = default;

  inline void giveAndWaitBack() noexcept{
    give_flag = true;
    while (!back_flag){
      _mm_pause();
    }
  }

  inline void waitGive() noexcept{
    while (!give_flag){
      _mm_pause();
    }
  }

  inline void back(){
    back_flag = true;
  }

  inline void use(){
    used = true;
  }

  [[nodiscard]]
  inline bool isUsed() const{
    return used;
  }

private:
  bool used = false;
  std::atomic_bool give_flag{false};
  std::atomic_bool back_flag{false};
};

/**
 * Code中の一点を通過したことをマークするために使用する。
 * Thread safeではないため、readはTest時のみ行う
 */
class Marker{
public:
  inline void mark(){
    marked = true;
  }

  [[nodiscard]]
  inline bool isMarked() const{
    return marked;
  }
private:
  bool marked{false};
};

}

#endif //MASSTREE_DEBUG_HELPER_H
