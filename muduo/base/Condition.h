#ifndef BASE_CONDITION_H
#define BASE_CONDITION_H

#include "Mutex.h"

namespace muduo {
class Condition : noncopyable {
public:
  explicit Condition(MutexLock &mutex) : mutex_(mutex) {
    MCHECK(pthread_cond_init(&pcond_, NULL));
  }

  ~Condition() { MCHECK(pthread_cond_destroy(&pcond_)); }

  void wait() {
    MutexLock::UnassignGuard ug(mutex_);
    /** pthread_cond_wait()
     *  阻塞当前线程别的线程使用pthread_cond_signal()或
     *  pthread_cond_broadcast来唤醒它
     */
    MCHECK(pthread_cond_wait(&pcond_, mutex_.getPthreadMutex()));
  }

  /**
   * @brief 等待数秒，如果超时退出返回True，否则返回False
   * 
   * @param seconds 
   * @return true 
   * @return false 
   */
  bool waitForSeconds(double seconds);

  /**
   * @brief 通知某个线程
   * 
   */
  void notify() { MCHECK(pthread_cond_signal(&pcond_)); }

  /**
   * @brief 通知所有等待的线程
   * 
   */
  void notifyAll() { MCHECK(pthread_cond_broadcast(&pcond_)); }

private:
  MutexLock &mutex_;
  pthread_cond_t pcond_; // 条件变量
};
} // namespace muduo

#endif // BASE_CONDITION_H
