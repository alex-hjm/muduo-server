#include "CurrentThread.h"

#include <cxxabi.h>
#include <execinfo.h> // 声明了三个函数用于获取当前线程的函数调用堆栈
#include <stdlib.h>

namespace muduo {
namespace CurrentThread {
__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char *t_threadName = "unknown";

// 静态断言:编译期间的断言
static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

/**
 * @brief 返回栈的调用痕迹
 *
 * @param demangle 解构，还原函数
 * @return std::string
 */
std::string stackTrace(bool demangle) {
  std::string stack;
  const int max_frames = 200;
  void *frame[max_frames];

  /** int backtrace(void **buffer,int size)
   *  获取当前线程的调用堆栈,获取的信息将会被存放在指针列表中
   */

  // 栈回溯，保存各个栈帧的地址
  int nptrs = ::backtrace(frame, max_frames); // 返回实际获取的指针个数

  /** char ** backtrace_symbols (void *const *buffer, int size)
   *  将backtrace函数获取的信息转化为一个字符串数组
   */

  // 根据地址，转成相应的函数符号
  char **strings =
      ::backtrace_symbols(frame, nptrs); // 返回指向字符串数组的指针

  if (strings) {
    size_t len = 256;
    char *demangled = demangle ? static_cast<char *>(::malloc(len)) : nullptr;

    // 遍历堆栈中的函数
    for (int i = 1; i < nptrs; ++i) {
      if (demangle) {
        char *left_par = nullptr; // 左括号
        char *plus = nullptr;

        // 查看该函数名是否包含'(' 和 '+'
        for (char *p = strings[i]; *p; ++p) {
          if (*p == '(') {
            left_par = p;
          } else if (*p == '+') {
            plus = p;
          }
        }

        if (left_par && plus) {
          *plus = '\0';
          int status = 0;
          char *ret = abi::__cxa_demangle(left_par + 1, demangled, &len,
                                          &status); // 类名
          *plus = '+';

          // 0: The demangling operation succeeded.
          if (status == 0) {
            demangled = ret; // ret could be realloc()
            stack.append(strings[i], left_par + 1);
            stack.append(demangled);
            stack.append(plus);
            stack.push_back('\n');
            continue;
          }
        }
      }

      // Fallback to mangled names;
      stack.append(strings[i]);
      stack.push_back('\n');
    }
    free(demangled);
    free(strings);
  }
  return stack;
}
} // namespace CurrentThread
} // namespace muduo
