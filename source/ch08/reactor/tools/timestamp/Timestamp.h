#ifndef TIMESTAMP_H
#define TIMESTAMP_H
#include <string>
#include <time.h>

namespace muduo {
class Timestamp {
public:
  Timestamp() : microSecondsSinceEpoch_(0) {}

  explicit Timestamp(int64_t microSecondsSinceEpoch)
      : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

  void swap(Timestamp &rhs) {
    std::swap(microSecondsSinceEpoch_, rhs.microSecondsSinceEpoch_);
  }

  /**
   * @brief such: 1422366082.375828
   *
   * @return std::string
   */
  std::string toString() const;

  /**
   * @brief such: 20150127 21:46:45:376742
   *
   * @return std::string
   */
  std::string toFormattedString() const;

  /**
   * @brief 该时间戳对象是否是有效的
   *
   * @return true
   * @return false
   */
  bool valid() const { return microSecondsSinceEpoch_ > 0; }

  /**
   * @brief 返回精确的微秒时间戳
   *
   * @return int64_t
   */
  int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

  /**
   * @brief 返回以s为单位的普通时间戳
   *
   * @return time_t
   */
  time_t secondsSinceEpoch() const {
    return static_cast<time_t>(microSecondsSinceEpoch_ /
                               kMicroSecondsPerSecond);
  }

  /**
   * @brief 获取当前时间
   *
   * @return Timestamp
   */
  static Timestamp now();

  /**
   * @brief 获取一个不可用的时间戳
   *
   * @return Timestamp
   */
  static Timestamp invalid();

  static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
  /**
   * @brief 以微秒为单位
   *
   */
  int64_t microSecondsSinceEpoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
  return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator>(Timestamp lhs, Timestamp rhs) {
  return lhs.microSecondsSinceEpoch() > rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
  return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

/**
 * @brief 返回两个时间戳的差
 *
 * @param high 较大的时间
 * @param low 较小的时间
 * @return double 整数部分以s为单位
 */
inline double timeDifference(Timestamp high, Timestamp low) {
  int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
  return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

/**
 * @brief 时间戳加上秒数生成新的时间戳
 *
 * @param timestamp
 * @param seconds
 * @return Timestamp
 */
inline Timestamp addTimer(Timestamp timestamp, double seconds) {
  int64_t delta =
      static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
  return Timestamp((timestamp.microSecondsSinceEpoch() + delta));
}

} // namespace muduo

#endif // TIMESTAMP_H