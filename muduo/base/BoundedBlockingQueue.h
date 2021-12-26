#ifndef BASE_BOUNDEDBLOCKINGQUEUE_H
#define BASE_BOUNDEDBLOCKINGQUEUE_H

#include "Condition.h"
#include "Mutex.h"

#include <assert.h>
#include <boost/circular_buffer.hpp>

namespace muduo {
/**
 * @brief 有上界的阻塞队列
 *
 * @tparam T
 */
template <typename T> class BoundedBlockingQueue : noncopyable {
public:
  explicit BoundedBlockingQueue(int maxsize)
      : mutex_(), notEmpty_(mutex_), notFull_(mutex_), queue_(maxsize) {}

  void put(const T &x) {
    MutexLockGuard lock(mutex_);
    while (queue_.full()) {
      notFull_.wait();
    }
    assert(!queue_.full());
    queue_.push_back(x);
    notEmpty_.notify();
  }

  void put(T &&X) {
    MutexLockGuard lock(mutex_);
    while (queue_.full()) {
      notFull_.wait();
    }
    assert(!queue_.full());
    queue_.push_back(std::move(X));
    notEmpty_.notify();
  }

  T take() {
    MutexLockGuard lock(mutex_);
    while (queue_.empty()) {
      notEmpty_.wait();
    }
    assert(!queue_.empty());
    T front(std::move(queue_.front()));
    queue_.pop_front();
    notFull_.notify();
    return front;
  }

  bool empty() const {
    MutexLockGuard lock(mutex_);
    return queue_.empty();
  }

  bool full() const {
    MutexLockGuard lock(mutex_);
    return queue_.full();
  }

  size_t size() const {
    MutexLockGuard lock(mutex_);
    return queue_.size();
  }

  size_t capacity() const {
    MutexLockGuard lock(mutex_);
    return queue_.capacity();
  }

private:
  mutable MutexLock mutex_; //保证互斥
  //通知消费者可以放入物品
  Condition notEmpty_ GUARDED_BY(mutex_);//唤醒等待的消费者
  //通知生产者可以放入物品
  Condition notFull_ GUARDED_BY(mutex_);//唤醒生产者
  boost::circular_buffer<T> queue_ GUARDED_BY(mutex_);
};
} // namespace muduo

#endif // BASE_BOUNDEDBLOCKINGQUEUE_H
