// excerpts from http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (giantchen at gmail dot com)

#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include "Mutex.h"
#include "Condition.h"

#include <boost/noncopyable.hpp>

namespace muduo
{

class CountDownLatch : boost::noncopyable
{
 public:

  explicit CountDownLatch(int count)//倒数几次
    : mutex_(),
      condition_(mutex_), //初始化顺序要与成员声明保持一致
      count_(count)
  {
  }

  void wait()//等待计数值变为0
  {
    MutexLockGuard lock(mutex_);
    while (count_ > 0)
    {
      condition_.wait();
    }
  }

  void countDown()//计数减一
  {
    MutexLockGuard lock(mutex_);
    --count_;
    if (count_ == 0)
      condition_.notifyAll();
  }

  int getCount() const
  {
    MutexLockGuard lock(mutex_);
    return count_;
  }

 private:
  mutable MutexLock mutex_; //顺序很重要，先mutex后condition
  Condition condition_;
  int count_;
};

}
#endif  // MUDUO_BASE_COUNTDOWNLATCH_H
