#ifndef BASE_COUNTDOWNLATCH_H
#define BASE_COUNTDOWNLATCH_H

#include "Condition.h"
#include "Mutex.h"

namespace muduo {
/**
 * @brief  倒计时计数器
 *
 */
class CountDownLatch : noncopyable {
public:
  explicit CountDownLatch(int count);

  void wait();

  void countDown();

  int getCount() const;

private:
  /**
   * @brief 被mutable修饰的变量，将永远处于可变的状态，即使在一个const函数中
   *
   */
  mutable MutexLock mutex_;

  /**
   * @brief GUARDED_BY声明了数据成员被给定的监护权保护。
   *        对于数据的读操作需要共享的访问权限，而写操作需要独占的访问权限。
   *
   * @return Condition
   */
  Condition condition_ GUARDED_BY(mutex_);
  int count_ GUARDED_BY(mutex_);
};
} // namespace muduo

#endif // BASE_COUNTDOWNLATCH_H
