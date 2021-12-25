#include "Condition.h"
#include <error.h>

/**
 * @brief 超时处理 returns true if time true, false otherwise;
 *
 * @param seconds
 * @return true
 * @return false
 */
bool muduo::Condition::waitForSeconds(double seconds) {
  struct timespec abstime;
  // CLOCK_REALTIME : Identifier for system-wide realtime clock.
  clock_gettime(CLOCK_REALTIME, &abstime); // 系统时间

  const int64_t kNanoSecondsPerSecond = 1000000000;
  int64_t nanoseconds =
      static_cast<int64_t>(seconds * kNanoSecondsPerSecond); // 纳秒

  // 系统时间 + 等待时间
  abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) /
                                        kNanoSecondsPerSecond); // 秒
  abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) %
                                      kNanoSecondsPerSecond); // 纳秒

  MutexLock::UnassignGuard ug(mutex_);

  // 条件变量    互斥锁   等待时间（系统时间 + 等待时间）
  return ETIMEDOUT ==
         pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime);
}