#include "../CurrentThread.h"
#include "../Thread.h"

#include <iostream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <sys/syscall.h>

using namespace std;

void mysleep(int seconds) {
  timespec t = {seconds, 0};
  nanosleep(&t, NULL);
};

void threadFunc() { printf("tid = %d\n", muduo::CurrentThread::tid()); }

void threadFunc2(int x) {
  printf("tid = %d, x = %d\n", muduo::CurrentThread::tid(), x);
}

void threadFunc3() {
  printf("tid = %d\n", muduo::CurrentThread::tid());
  mysleep(1); // 睡1秒
}

class Foo {
public:
  explicit Foo(double x) : x_(x) {}

  void memberFunc() {
    printf("tid = %d, Foo::x_ = %f\n", muduo::CurrentThread::tid(), x_);
  }

  void memberFunc2(const std::string &text) {
    printf("tid = %d, Foo::x_ = %f, text = %s\n", muduo::CurrentThread::tid(),
           x_, text.c_str());
  }

private:
  double x_;
};

int main(int argc, char **argv) {
  //                                       ::syscall(SYS_gettid)
  printf("pid = %d, tid = %d\n", ::getpid(), muduo::CurrentThread::tid());

  muduo::Thread t1(threadFunc);
  t1.start();
  printf("t1.tid = %d\n", t1.tid());
  t1.join();

  muduo::Thread t2(std::bind(threadFunc2, 42),
                   "thread for free function with argument");
  t2.start();
  printf("t2.tid = %d\n", t2.tid());
  t2.join();

  Foo foo(87.53);
  muduo::Thread t3(std::bind(&Foo::memberFunc, &foo),
                   "thread for member function without argument");
  t3.start();
  printf("t3.tid = %d\n", t3.tid());
  t3.join();

  muduo::Thread t4(
      std::bind(&Foo::memberFunc2, std::ref(foo), std::string("mu duo")));
  t4.start();
  printf("t4.tid = %d\n", t4.tid());
  t4.join();

  {
    muduo::Thread t5(threadFunc3);
    t5.start();
    printf("t5.tid = %d\n", t5.tid());
    // t5 may destruct eariler than thread creation.
  }

  mysleep(2);

  {
    muduo::Thread t6(threadFunc3);
    t6.start();
    printf("t6.tid = %d\n", t6.tid());
    mysleep(2);
    // t6 destruct later than thread creation.
  }

  sleep(2);
  printf("number of created threads %d\n", muduo::Thread::numCreated());
  return 0;
}
