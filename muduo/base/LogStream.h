#ifndef BASE_LOGSTREAM_H
#define BASE_LOGSTREAM_H

#include "StringPiece.h"
#include "noncopyable.h"
#include "Types.h"
#include <assert.h>
#include <string.h>

namespace muduo {
namespace detail {
// 小的缓冲区大小
const int kSmallBuffer = 4000;
// 大的缓冲区大小
const int kLargeBuffer = 4000 * 1000;

/**
 * @brief 固定大小的buffer
 *
 * @tparam SIZE
 */
template <int SIZE> class FixedBuffer : noncopyable {
public:
  FixedBuffer() : cur_(data_) { setCookie(cookieStart); }

  ~FixedBuffer() { setCookie(cookieEnd); }

  /**
   * @brief 添加数据
   *
   */
  void append(const char * /*restrict*/ buf, size_t len) {
    // 如果可用数据足够，就拷贝过去，同时移动当前指针。
    if (implicit_cast<size_t>(avail()) > len) {
      memcpy(cur_, buf, len);
      cur_ += len;
    }
  }

  /**
   * @brief 返回首地址
   *
   * @return const char*
   */
  const char *data() const { return data_; }

  /**
   * @brief 返回缓冲区已有数据长度
   *
   * @return int
   */
  int length() const { return static_cast<int>(cur_ - data_); }

  /**
   * @brief 返回当前数据末端地址
   *
   * @return char*
   */
  char *current() { return cur_; }

  /**
   * @brief 返回剩余可用地址
   *
   * @return int
   */
  int avail() const { return static_cast<int>(end() - cur_); }

  /**
   * @brief cur_前移
   *
   * @param len
   */
  void add(size_t len) { cur_ += len; }

  /**
   * @brief 重置，不清数据，只需让cur指回首地址
   *
   */
  void reset() { cur_ = data_; }

  /**
   * @brief 清零
   *
   */
  void bzero() { memset(data_, 0, sizeof(data_)); }

  /**
   * @brief 调试字符串
   *
   * @return const char*
   */
  const char *debugString();

  /**
   * @brief 设置回调函数
   *
   * @param cookie
   */
  void setCookie(void (*cookie)()) { cookie_ = cookie; }

  /**
   * @brief 把数据转换为字符串
   *
   * @return string
   */
  string toString() const { return string(data_, length()); }

  /**
   * @brief 返回string类型
   *
   * @return StringPiece
   */
  StringPiece toStringPiece() const { return StringPiece(data_, length()); }

private:
  /**
   * @brief 返回尾指针
   *
   * @return const char*
   */
  const char *end() const { return data_ + sizeof data_; }

  // Must be outline function for cookies.
  static void cookieStart();
  static void cookieEnd();

  void (*cookie_)(); // 回调函数
  char data_[SIZE];  // 缓冲区
  char *cur_;        // 指针，指向缓冲区读写的位置
};
} // namespace detail

// 日志流
class LogStream : noncopyable {
  typedef LogStream self;

public:
  // 缓冲区，使用smallbuffer
  typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;

  // 针对不同类型重载operator <<
  // 别的类如果调用LogStream的<<实际上是把内容追加到LogStream的缓冲区
  self &operator<<(bool v) {
    buffer_.append(v ? "1" : "0", 1);
    return *this;
  }

  self &operator<<(short);
  self &operator<<(unsigned short);
  self &operator<<(int);
  self &operator<<(unsigned int);
  self &operator<<(long);
  self &operator<<(unsigned long);
  self &operator<<(long long);
  self &operator<<(unsigned long long);
  self &operator<<(const void *);

  self &operator<<(float v) {
    *this << static_cast<double>(v);
    return *this;
  }
  self &operator<<(double);
  self &operator<<(long double);

  self &operator<<(char v) {
    buffer_.append(&v, 1);
    return *this;
  }

  self &operator<<(const char *str) {
    if (str) {
      buffer_.append(str, strlen(str));
    } else {
      buffer_.append("(null)", 6);
    }
    return *this;
  }

  self &operator<<(const unsigned char *str) {
    // C++中的reinterpret_cast主要是将数据从一种类型的转换为另一种类型。
    // 所谓“通常为操作数的位模式提供较低层的重新解释”也就是说将数据以二进制存在形式的重新解释。
    return operator<<(reinterpret_cast<const char *>(str));
  }

  self &operator<<(const string &v) {
    buffer_.append(v.c_str(), v.size());
    return *this;
  }

  self &operator<<(const StringPiece &v) {
    buffer_.append(v.data(), v.size());
    return *this;
  }

  self &operator<<(const Buffer &v) {
    *this << v.toStringPiece();
    return *this;
  }

  void append(const char *data, int len) { buffer_.append(data, len); }
  const Buffer &buffer() const { return buffer_; }
  void resetBuffer() {
    buffer_.reset(); // 只指回首地址，数据不清空
  }

private:
  // 静态检查
  void staticCheck();

  // 格式化整数
  template <typename T> void formatInteger(T);

  Buffer buffer_;
  // 用来保证缓冲区不溢出
  // 这个表示最大的数值类型占用的空间的大小
  static const int kMaxNumericSize = 32;
};

// 格式化类
class Fmt {
private:
  char buf_[32];
  int length_;

public:
  template <typename T> Fmt(const char *fmt, T val);

  const char *data() const { return buf_; }
  int length() const { return length_; }
};

inline LogStream &operator<<(LogStream &s, const Fmt &fmt) {
  s.append(fmt.data(), fmt.length());
  return s;
}

// Format quantity n in SI units (k, M, G, T, P, E).
// The returned string is atmost 5 characters long.
// Requires n >= 0
string formatSI(int64_t n);

// Format quantity n in IEC (binary) units (Ki, Mi, Gi, Ti, Pi, Ei).
// The returned string is atmost 6 characters long.
// Requires n >= 0
string formatIEC(int64_t n);
} // namespace muduo

#endif // MYMUDUO_LOGSTREAM_H
