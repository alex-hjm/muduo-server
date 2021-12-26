#include "CountDownLatch.h"

using namespace muduo;

CountDownLatch::CountDownLatch(int count)
    : mutex_(), condition_(mutex_), // 初始化顺序要与成员声明保持一致
      count_(count) {}

void CountDownLatch::wait() {
  MutexLockGuard lock(mutex_); // 防止忘记给mutex解锁
  while (count_ > 0) {
    condition_.wait();
  }
}

void CountDownLatch::countDown() {
  MutexLockGuard lock(mutex_);
  --count_;
  if (count_ == 0) {
    condition_.notifyAll();
  }
}

int CountDownLatch::getCount() const {
  MutexLockGuard lock(mutex_);
  return count_;
}