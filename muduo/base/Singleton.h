#ifndef BASE_SINGLETON_H
#define BASE_SINGLETON_H

#include "noncopyable.h"

#include <assert.h>
#include <iostream>
#include <pthread.h>
#include <stdlib.h>

namespace muduo {
namespace detail {
/**
 * @brief 作用是当我们在进行模板特化的时候，会去选择那个正确的模板，避免失败
 *
 * @tparam T
 */
template <typename T> struct has_no_destroy {

  template <typename C> static char test(decltype(&C::no_destroy));
  template <typename C> static int32_t test(...);

  /** 如果是class类型，那么模板推断决议，将test<T>(0)推断至第一个版本
   *  sizeof(test<T>(0))为1 value为true
   *  如果是int、double等内置类型，推断至第二个版本，value为False
   */
  const static bool value = sizeof(test<T>(0)) == 1;
};
} // namespace detail

template <typename T> class Singleton : noncopyable {
public:
  Singleton() = delete; // 禁止使用该函数
  ~Singleton() = delete;

  /**
   * @brief 得到对象
   *
   * @return T&
   */
  static T &instance() {
    // 第一次调用会在init函数内部创建，pthread_once保证该函数只被调用一次！！！！
    // 并且pthread_once()能保证线程安全，效率高于mutex
    pthread_once(&ponce_, &Singleton::init);
    assert(value_ != NULL);
    return *value_; // 利用pthread_once只构造一次对象
  }

private:
  /**
   * @brief 客户端初始化该类
   *
   */
  static void init() {
    // 直接调用构造函数
    value_ = new T();
    // 当参数是类且没有"no_destroy"方法才会注册atexit的destroy
    if (!detail::has_no_destroy<T>::value) {
      ::atexit(destroy); // 登记atexit时调用的销毁函数，防止内存泄漏
    }
  }

  /**
   * @brief 程序结束后自动调用该函数销毁
   *
   */
  static void destroy() {
    // std::cout << "destroy()" << std::endl;
    // 用typedef定义了一个数组类型，数组的大小不能为-1，利用这个方法，如果是不完全类型，编译阶段就会发现错误
    typedef char T_must_be_complete_type
        [sizeof(T) == 0 ? -1 : 1]; // 要销毁这个类型，这个类型必须是完全类型
    T_must_be_complete_type dummy;
    (void)dummy;

    delete value_; // 销毁
    value_ = NULL; // 赋空
  }

private:
  static pthread_once_t ponce_;
  static T *value_;
};

template <typename T>
pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT; // 初始化pthread_once

template <typename T>
T *Singleton<T>::value_ = NULL; // 静态成员外部会初始化为空

} // namespace muduo

#endif // MYMUDUO_SINGLETON_H
