#include "CountDownLatch.h"

using namespace muduo;

CountDownLatch::CountDownLatch(int count)
    : mutex_(), condition_(mutex_), // 初始化顺序要与成员声明保持一致
      count_(count) {}

/**
 * @brief 等待计数值变为0
 *
 */
void CountDownLatch::wait() {
  MutexLockGuard lock(mutex_); // 防止忘记给mutex解锁
  while (count_ > 0) {
    condition_.wait();
  }
}

/**
 * @brief 计数减一
 *
 */
void CountDownLatch::countDown() {
  MutexLockGuard lock(mutex_);
  --count_;
  if (count_ == 0) {
    condition_.notifyAll();
  }
}

/**
 * @brief 获取计数值
 *
 * @return int
 */
int CountDownLatch::getCount() const {
  MutexLockGuard lock(mutex_);
  return count_;
}