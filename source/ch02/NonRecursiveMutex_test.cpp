#include "Mutex.h"
#include "Thread.h"
#include <vector>
#include <stdio.h>

using namespace muduo;

class Foo
{
 public:
  void doit() const;
};

MutexLock mutex;
std::vector<Foo> foos;

void post(const Foo& f)
{
  MutexLockGuard lock(mutex);
  foos.push_back(f);
}

void traverse()
{
  MutexLockGuard lock(mutex);
  for (std::vector<Foo>::const_iterator it = foos.begin();
      it != foos.end(); ++it)
  {
    it->doit();//间接调用post(),mutex非递归->死锁 /mutex递归->vector迭代器可能失效
  }
}

void Foo::doit() const
{
  Foo f;
  post(f);
}

int main()
{
  Foo f;
  post(f);
  traverse();
}

