#include "Thread.h"
#include <iostream>

Thread::Thread() :  autoDelete_(false) {
  std::cout<<"Thread ..."<<std::endl;
}

Thread::~Thread() {
  std::cout<<"~Thread ..."<<std::endl;
}

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

