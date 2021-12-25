#ifndef BASE_THREAD_H
#define BASE_THREAD_H

#include "Atomic.h"
#include "CountDownLatch.h"

#include <functional>
#include <memory>
#include <pthread.h>

using namespace std;

namespace muduo {
class Thread : noncopyable {
public:
  typedef function<void()> ThreadFunc; // 线程执行的函数对象

  explicit Thread(ThreadFunc, const string &name = string());
  ~Thread();

  void start();
  int join(); // return pthread_join();

  bool started() const { return started_; }

  pid_t tid() { return tid_; }

  const string &name() const { return name_; }

  static int numCreated() { return numCreated_.get(); }

private:
  void setDefaultName();

  bool started_;
  bool joined_;
  pthread_t pthreadId_;
  pid_t tid_;
  ThreadFunc func_;
  string name_;
  CountDownLatch latch_;

  static AtomicInt32 numCreated_; 
};
} // namespace muduo

#endif // MYMUDUO_THREAD_H
