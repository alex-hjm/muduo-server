#define __STDC_FORMAT_MACROS
#include <inttypes.h> //int64_t
#undef __STDC_FORMAT_MACROS

#include "Timestamp.h"
#include <sys/time.h>

using namespace muduo;

static_assert(sizeof(Timestamp) == sizeof(int64_t),
              "sizeof(Timestamp) != sizeof(int64_t)");

std::string Timestamp::toString() const {
  char buf[32] = {0};
  int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
  int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
  // int64_t在32bit中是lld，在64bit中是ld ,跨平台使用PRId64
  snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 "", seconds,
           microseconds);

  return buf;
}

std::string Timestamp::toFormattedString(bool showMicroseconds) const {
  char buf[32];
  time_t seconds =
      static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
  struct tm tm_time;
  gmtime_r(&seconds, &tm_time); // 将数据存储到用户提供的结构体中

  if (showMicroseconds) {
    int microseconds =
        static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d:%06d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);
  } else {
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
  }
  return buf;
}

Timestamp Timestamp::now() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t seconds = tv.tv_sec;// 秒
  return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);// 微秒
}
