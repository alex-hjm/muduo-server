#ifndef BASE_NONCOPYABLE_H
#define BASE_NONCOPYABLE_H

namespace muduo {
class noncopyable {
public:
  noncopyable(const noncopyable &) = delete;    
  void operator=(const noncopyable &) = delete; 

protected:
  noncopyable() = default;
  ~noncopyable() = default;
};
} // namespace muduo

#endif // BASE_NOCOPYABLE_H
