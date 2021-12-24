#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdint.h>

namespace muduo {
namespace detail {
template <typename T> class AtomicIntegerT {
public:
  AtomicIntegerT() : value_(0) {}

  /**
   * @brief 返回操作之前的值
   *
   * @return T
   */
  T get() { return __sync_val_compare_and_swap(&value_, 0, 0); }

  /**
   * @brief T加上x并返回操作之前的值
   *
   * @param x
   * @return T
   */
  T getAndAdd(T x) { return __sync_fetch_and_add(&value_, x); }

  /**
   * @brief 将T设为newValue并返回操作之前的值
   *
   * @param newValue
   * @return T
   */
  T getAndSet(T newValue) {
    return __sync_lock_test_and_set(&value_, newValue);
  }

  /**
   * @brief T 加上x，然后返回操作之后的值
   *
   * @param x
   * @return T
   */
  T addAndGet(T x) { return getAndAdd(x) + x; }

  /**
   * @brief 前置递增
   *
   * @return T
   */
  T incrementAndGet() { return addAndGet(1); }

  /**
   * @brief 前置递减
   *
   * @return T
   */
  T decrementAndGet() { return addAndGet(-1); }

  void add(T x) { getAndAdd(x); }

  void increment() { incrementAndGet(); }

  void decrement() { decrementAndGet(); }

private:
  volatile T value_;
};
} // namespace detail

typedef detail::AtomicIntegerT<int32_t> AtomicInt32;
typedef detail::AtomicIntegerT<int64_t> AtomicInt64;

} // namespace muduo

#endif // ATOMIC_H