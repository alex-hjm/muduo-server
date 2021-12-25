#include "Thread.h"
#include <iostream>

Thread::Thread(const ThreadFunc &func) : func_(func), autoDelete_(false) {}

void Thread::start() { pthread_create(&threadId_, NULL, ThreadRoutine, this); }

void Thread::join() { pthread_join(threadId_, NULL); }

void *Thread::ThreadRoutine(void *arg) {
  Thread *thread = static_cast<Thread *>(arg);
  thread->run();
  if (thread->autoDelete_)
    delete thread;
  return NULL;
}

void Thread::setAutoDelete(bool autoDelete) { autoDelete_ = autoDelete; }

void Thread::run() { func_(); }