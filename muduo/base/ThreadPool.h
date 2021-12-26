#ifndef BASE_THREADPOOL_H
#define BASE_THREADPOOL_H

#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"

#include <deque>
#include <vector>

using namespace std;

namespace muduo {
class ThreadPool : noncopyable {
public:
  typedef function<void()> Task;

  explicit ThreadPool(const string &nameArg = string("ThreadPool"));
  ~ThreadPool();

  // Must be called before start()
  /**
   * @brief 设置队列长度的最大值
   *
   * @param maxSize
   */
  void setMaxQueueSize(int maxSize) { this->maxQueueSize_ = maxSize; }
  /**
   * @brief 设置线程初始化回调函数
   *
   * @param cb
   */
  void setThreadInitCallback(const Task &cb) { threadInitCallback_ = cb; }

  /**
   * @brief 启动线程池
   *
   * @param numThreads
   */
  void start(int numThreads);
  /**
   * @brief 停止线程池
   * 
   */
  void stop();

  const string &name() const { return name_; }

  size_t queueSize() const;

  /**
   * @brief 将任务f加入线程池运行
   *
   * @param f
   */
  void run(Task f); //

private:
  bool isFull() const REQUIRES(mutex_);
  /**
   * @brief 运行run中添加的任务
   *
   */
  void runInThread();
  /**
   * @brief 从queue中获取任务
   *
   * @return Task
   */
  Task take();

  mutable MutexLock mutex_;
  // 用来告知线程池里面的线程当前是否有尚未执行的任务（即任务列表是否空）
  Condition notEmpty_ GUARDED_BY(mutex_);
  // 用来告知传递任务的线程当前线程池的任务列表是否已经塞满
  Condition notFull_ GUARDED_BY(mutex_);
  string name_;
  Task threadInitCallback_;
  vector<unique_ptr<muduo::Thread>> threads_;
  deque<Task> queue_ GUARDED_BY(mutex_);
  size_t maxQueueSize_;
  bool running_;
};
} // namespace muduo

#endif // BASE_THREADPOOL_H
