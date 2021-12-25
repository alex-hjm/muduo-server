#ifndef _THREAD_H_
#define _THREAD_H_

#include <functional>
#include <pthread.h>

class Thread {
public:
  typedef std::function<void()> ThreadFunc;
  explicit Thread(const ThreadFunc &func);
  void start();
  void join();
  void setAutoDelete(bool autoDelete);

private:
  static void *ThreadRoutine(void *arg);
  void run();
  ThreadFunc func_;
  pthread_t threadId_;
  bool autoDelete_;
};

#endif //_THREAD_H_