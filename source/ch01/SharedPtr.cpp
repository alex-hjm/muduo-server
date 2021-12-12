#include <boost/shared_ptr.hpp>
#include "Mutex.h"

class Foo
{

};


class SharedPtr
{
   muduo::MutexLock mutex;
   boost::shared_ptr<Foo> globalPtr;

   void doit(const boost::shared_ptr<Foo> & pFoo);

   void read()
   {
      boost::shared_ptr<Foo> localPtr;
      {
         muduo::MutexLockGuard lock(mutex);
         localPtr=globalPtr;
      }
      //读写localPtr也无需加锁
      doit(localPtr);
   }

   void write()
   {
      boost::shared_ptr<Foo> newPtr(new Foo);//对象创建在临界区之外,缩短临界区长度
      globalPtr.reset();//临界区外销毁对象
      {
         muduo::MutexLockGuard lock(mutex);
         globalPtr=newPtr;
      }
      //读写localPtr也无需加锁
      doit(newPtr);  
   }

   void destory()
   {
        boost::shared_ptr<Foo> localPtr;//
      {
         muduo::MutexLockGuard lock(mutex);
         globalPtr.swap(localPtr);
      }
   }
};