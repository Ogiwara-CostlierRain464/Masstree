#ifndef MASSTREE_DEBUG_HELPER_H
#define MASSTREE_DEBUG_HELPER_H

#include <atomic>
#include <xmmintrin.h>
#include <functional>
#include <chrono>
#include <thread>

namespace masstree{

/**
 * Multi threadにおいて、逐次実行を実現させるための機構
 * handlerをgiveすることにより別のスレッドが処理を開始でき、
 * backされると自分のthreadが処理を再開する。
 * 使う場合には、各テストのはじめに.use()を呼び出して初期化する必要がある。
 */
class SequentialHandler{
public:
  SequentialHandler() = default;

  inline void giveAndWaitBackIfUsed() noexcept{
    if(isUsed()){
      give_flag = true;
      while (!back_flag){
        _mm_pause();
      }
    }
  }

  inline void waitGive() noexcept{
    assert(isUsed());
    while (!give_flag){
      _mm_pause();
    }
  }

  inline void back(){
    assert(isUsed());
    back_flag = true;
  }

  /**
   * このHandlerをテストで使う時には、テストの処理を引数として渡す。
   * 各フラグの初期化を自動で行えるように、このような形でHandlerを使用する。
   * @param f
   */
  void use(const std::function<void(void)>& f){
    used = true;
    give_flag = false;
    back_flag = false;
    f();
    used = false;
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
 *
 * 使う場合には、各テストのはじめに.use()を呼び出して初期化する必要がある。
 */
class Marker{
public:
  inline void markIfUsed(){
    if(isUsed()){
      marked = true;
    }
  }

  inline void use(const std::function<void(void)> &f){
    used = true;
    marked = false;
    f();
    used = false;
  }

  [[nodiscard]]
  inline bool isMarked() const{
    return marked;
  }

  [[nodiscard]]
  inline bool isNotMarked() const{
    return !isMarked();
  }

  [[nodiscard]]
  inline bool isUsed() const{
    return used;
  }

private:
  bool used{false};
  bool marked{false};
};

/**
 * 待機しているスレッドに通知し、一定時間処理を止めさせる。
 */
class Sleeper{
public:

  // sleepしている間に、他のthreadに処理を終えてもらう。
  // そしてその処理が終わるかどうかに寄らずに処理を再開する
  void sleepIfUsed(){
    if(used){
      give_flag = true;
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(0.1s);
    }
  }

  void waitGive()const{
    assert(used);
    while (!give_flag){
      _mm_pause();
    }
  }

  void use(const std::function<void(void)>& f){
    used = true;
    give_flag = false;
    f();
    used = false;
  }

private:
  bool used{false};
  std::atomic_bool give_flag{false};
};

}


#endif //MASSTREE_DEBUG_HELPER_H
