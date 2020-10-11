#ifndef MASSTREE_DEBUG_HELPER_H
#define MASSTREE_DEBUG_HELPER_H

#include <atomic>
#include <xmmintrin.h>

namespace masstree{

class ThreadHandler{
public:
  ThreadHandler() = default;

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

}

#endif //MASSTREE_DEBUG_HELPER_H
