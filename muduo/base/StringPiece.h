#ifndef BASE_STRINGPIECE_H
#define BASE_STRINGPIECE_H

#include "Types.h"

#include <iosfwd> // for ostream forward-declaration
#include <string.h>

// 这是Google的一段开源代码
// 可以接受C-style和string两种参数,同时避免了内存分配的开销
namespace muduo {
/**
 * @brief 使用它来传递C风格字符串
 *
 */
class StringArg // copyable
{
public:
  StringArg(const char *str) : str_(str) {}

  StringArg(const string &str) : str_(str.c_str()) {}

  const char *c_str() const { return str_; }

private:
  const char *str_;
};

class StringPiece {
private:
  const char *ptr_;
  int length_;

public:
  /**
   * @brief 这里构造函数都是非explicit，这样可以在传参时(const char* or
   * string)进行隐式转换
   */
  StringPiece() : ptr_(NULL), length_(0) {}
  StringPiece(const char *str)
      : ptr_(str), length_(static_cast<int>(strlen(ptr_))) {}
  StringPiece(const unsigned char *str) // 需要进行指针转换
      : ptr_(reinterpret_cast<const char *>(str)),
        length_(static_cast<int>(strlen(ptr_))) {}
  StringPiece(const string &str)
      : ptr_(str.data()), length_(static_cast<int>(str.size())) {}
  StringPiece(const char *offset, int len) : ptr_(offset), length_(len) {}

  /**
   * @brief data函数是不安全的，因为ptr指示的内容未必以'\0'结尾
   */
  const char *data() const { return ptr_; }
  int size() const { return length_; }
  bool empty() const { return length_ == 0; }
  const char *begin() const { return ptr_; }
  const char *end() const { return ptr_ + length_; }

  void clear() {
    ptr_ = NULL;
    length_ = 0;
  }
  void set(const char *buffer, int len) {
    ptr_ = buffer;
    length_ = len;
  }
  void set(const char *str) {
    ptr_ = str;
    length_ = static_cast<int>(strlen(str));
  }
  /**
   * @brief void*与unsigned char*一样，需要使用reinterpret_cast进行指针转换
   */
  void set(const void *buffer, int len) {
    ptr_ = reinterpret_cast<const char *>(buffer);
    length_ = len;
  }

  char operator[](int i) const { return ptr_[i]; }
  /**
   * @brief 删除长度为n的前缀
   *
   * @param n
   */
  void remove_prefix(int n) {
    ptr_ += n;
    length_ -= n;
  }
  /**
   * @brief 删除长度为n的后缀
   *
   * @param n
   */
  void remove_suffix(int n) { length_ -= n; }

  bool operator==(const StringPiece &x) const {
    return ((length_ == x.length_) && (memcmp(ptr_, x.ptr_, length_) == 0));
  }
  bool operator!=(const StringPiece &x) const { return !(*this == x); }

//这段宏定义了一个通用的比较操作符模板
#define STRINGPIECE_BINARY_PREDICATE(cmp, auxcmp)                              \
  bool operator cmp(const StringPiece &x) const {                              \
    int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);   \
    return ((r auxcmp 0) || ((r == 0) && (length_ cmp x.length_)));            \
  }
  // 下面四行代码分别定义了 < <= >= >四个操作符
  STRINGPIECE_BINARY_PREDICATE(<, <);
  STRINGPIECE_BINARY_PREDICATE(<=, <);
  STRINGPIECE_BINARY_PREDICATE(>=, >);
  STRINGPIECE_BINARY_PREDICATE(>, >);
#undef STRINGPIECE_BINARY_PREDICATE

  /**
   * @brief 比较两个字符串大小，类似strcmp的返回值
   *
   * @param x
   * @return int
   */
  int compare(const StringPiece &x) const {
    int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
    if (r == 0) {
      if (length_ < x.length_)
        r = -1;
      else if (length_ > x.length_)
        r = +1;
    }
    return r;
  }

  /**
   * @brief 使用ptr和length指示的字节，构造一个string对象
   *
   * @return string
   */
  string as_string() const { return string(data(), size()); }
  /**
   * @brief 把其中的值赋给一个已经存在的string对象
   *
   * @param target
   */
  void CopyToString(string *target) const { target->assign(ptr_, length_); }

  // Does "this" start with "x"
  /**
   * @brief  判断该字符串是否以x开头
   *
   * @param x
   * @return true
   * @return false
   */
  bool starts_with(const StringPiece &x) const {
    return ((length_ >= x.length_) && (memcmp(ptr_, x.ptr_, x.length_) == 0));
  }
};

} // namespace muduo

// ------------------------------------------------------------------
// Functions used to create STL containers that use StringPiece
//  Remember that a StringPiece's lifetime had better be less than
//  that of the underlying string or char*.  If it is not, then you
//  cannot safely store a StringPiece into an STL container
// ------------------------------------------------------------------

// 提供某些STL容易所需要的trait特性，这里涉及到STL Traits的概念和POD的概念
// 是否具有非平凡的构造函数
// 是否具有非平凡的复制构造函数
// 是否具有非平凡的赋值运算符
// 是否具有非平凡的析构函数
// 是否是POD类型
#ifdef HAVE_TYPE_TRAITS
// This makes vector<StringPiece> really fast for some STL implementations
template <> struct __type_traits<muduo::StringPiece> {
  typedef __true_type has_trivial_default_constructor;
  typedef __true_type has_trivial_copy_constructor;
  typedef __true_type has_trivial_assignment_operator;
  typedef __true_type has_trivial_destructor;
  typedef __true_type is_POD_type;
};
#endif

// allow StringPiece to be logged
std::ostream &operator<<(std::ostream &o, const muduo::StringPiece &piece);

#endif // MUDUO_BASE_STRINGPIECE_H
