#ifndef BASE_CURRENTTHREAD_H
#define BASE_CURRENTTHREAD_H

#include <string>

namespace muduo {

namespace CurrentThread {
// internal
extern __thread int t_cachedTid;      // 真实线程Tid的缓存
extern __thread char t_tidString[32]; // 真实线程Tid的字符串表示形式
extern __thread int t_tidStringLength;
extern __thread const char *t_threadName;
void cacheTid();

inline int tid() {
  if (__builtin_expect(t_cachedTid == 0, 0)) {
    cacheTid();
  }
  return t_cachedTid;
}

inline const char *tidString() // for logging
{
  return t_tidString;
}

inline int tidStringLength() // for logging
{
  return t_tidStringLength;
}

inline const char *name() { return t_threadName; }

bool isMainThread();

void sleepUsec(int64_t usec); // for testing

std::string stackTrace(bool demangle);

} // namespace CurrentThread

} // namespace muduo

#endif // BASE_CURRENTTHREAD_H
