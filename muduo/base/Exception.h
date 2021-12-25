#ifndef BASE_EXCEPTION_H
#define BASE_EXCEPTION_H

#include <exception>
#include <string>

namespace muduo {
/**
 * @brief 提供了打印栈痕迹的功能
 *
 */
class Exception : public std::exception {
public:
  Exception(std::string what);
  ~Exception() noexcept override = default; 
  
  const char *what() const noexcept override
  {
    return message_.c_str();
  }
  const char *stackTrace() const throw()
  {
    return stack_.c_str();
  }

  std::string message_;
  std::string stack_;
};

} // namespace muduo

#endif // BASE_EXCEPTION_H
