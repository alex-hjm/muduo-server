#ifndef BASE_DATA_H
#define BASE_DATA_H

#include "copyable.h"
#include "Types.h"

#include <string>

struct tm;

namespace muduo {
class Date : public muduo::copyable {
public:
  struct YearMonthDay {
    int year;  // [1900 .. 2500]
    int month; // [1 .. 12]
    int day;   // [1 .. 31]
  };

  static const int kDaysPerWeek = 7;
  static const int kJulianDayOf1970_01_01;

  // Constucts an invalid Date
  Date() : julianDayNumber_(0) {}

  Date(int year, int month, int day);

  // Constructs a yyyy-mm-dd Date
  explicit Date(int julianDayNum) : julianDayNumber_(julianDayNum) {}

  explicit Date(const struct tm &);

  void swap(Date &that) { std::swap(julianDayNumber_, that.julianDayNumber_); }

  bool valid() const { return julianDayNumber_ > 0; }

  // Converts to yyyy-dd-mm format
  /**
   * @brief 将Date格式化为2015-01-28
   *
   * @return string
   */
  string toIsoString() const;

  /**
   * @brief  将本日期转化为一个年月日对象
   *
   * @return struct YearMonthDay
   */
  struct YearMonthDay yearMonthDay() const;

  int year() const { return yearMonthDay().year; }

  int month() const { return yearMonthDay().month; }

  int day() const { return yearMonthDay().day; }

  // [0, 1, ..., 6] => [Sunday, Monday, ..., Saturday ]
  int weekDay() const { return (julianDayNumber_ + 1) % kDaysPerWeek; }

  int julianDayNumber() const { return julianDayNumber_; }

private:
  int julianDayNumber_;
};

inline bool operator<(Date x, Date y) {
  return x.julianDayNumber() < y.julianDayNumber();
}

inline bool operator==(Date x, Date y) {
  return x.julianDayNumber() == y.julianDayNumber();
}
} // namespace muduo

#endif // BASE_DATA_H
