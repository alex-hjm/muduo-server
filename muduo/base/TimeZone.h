#ifndef BASE_TIMEZONE_H
#define BASE_TIMEZONE_H

#include "copyable.h"
#include <memory>
#include <time.h>

namespace muduo {
// TimeZone for 1970~2030
class TimeZone : public muduo::copyable {

public:
  explicit TimeZone(const char *zonefile);
  /**
   * @brief Construct a new Time Zone object
   * 
   * @param eastOfUtc  UTC时间 中国内地的时间与UTC的时差为+8，也就是UTC+8
   * @param tzname 
   */
  TimeZone(int eastOfUtc, const char *tzname);
  TimeZone() = default;

  bool valid() const { return static_cast<bool>(data_); }

  struct tm toLocalTime(time_t secondsSinceEpoch) const;
  time_t fromLocalTime(const struct tm &) const;
  
  // gmtime(3)
  static struct tm toUtcTime(time_t secondsSinceEpoch, bool yday = false);
  // timegm(3)
  static time_t fromUtcTime(const struct tm &);
  // year in [1900..2500], month in [1..12], day in [1..31]
  static time_t fromUtcTime(int year, int month, int day, int hour, int minute,
                            int seconds);

  struct Data;

private:
  std::shared_ptr<Data> data_;
};

} // namespace muduo

#endif // MYMUDUO_TIMEZONE_H
