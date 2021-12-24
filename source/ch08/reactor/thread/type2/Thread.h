#ifndef _THREAD_H_
#define _THREAD_H_

#include <functional>
#include <pthread.h>

class Thread {
public:
  Thread();
  virtual ~Thread();
  void start();
  void join();
  void setAutoDelete(bool autoDelete);

private:
  static void *ThreadRoutine(void *arg);
  virtual void run() = 0;
  pthread_t threadId_;
  bool autoDelete_;
};

#endif //_THREAD_H_