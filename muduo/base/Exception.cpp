#include "CurrentThread.h"
#include "Exception.h"

namespace muduo {
  Exception::Exception(std::string msg)
      : message_(std::move(msg)),
        stack_(CurrentThread::stackTrace(/*demangle=*/false)) {}
}
