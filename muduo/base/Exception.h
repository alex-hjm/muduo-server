#ifndef BASE_EXCEPTION_H
#define BASE_EXCEPTION_H

#include "Types.h"

#include <exception>


namespace muduo {
/**
 * @brief 提供了打印栈痕迹的功能
 *
 */
class Exception : public std::exception {
public:
  Exception(string what);
  ~Exception() noexcept override = default; 
  
  const char *what() const noexcept override
  {
    return message_.c_str();
  }
  const char *stackTrace() const throw()
  {
    return stack_.c_str();
  }

  string message_;
  string stack_;
};

} // namespace muduo

#endif // BASE_EXCEPTION_H
