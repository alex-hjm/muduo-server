add_subdirectory(poll)
add_subdirectory(epoll)
add_subdirectory(thread_type1)
add_subdirectory(thread_type2)

add_executable(bind_func bind_func.cpp)
add_executable(TimeZone_unittest TimeZone_unittest.cpp)
add_executable(Timestamp_unittest Timestamp_unittest.cpp)
add_executable(Atomic_unittest Atomic_unittest.cpp)
add_executable(Exception_test Exception_test.cpp)
add_executable(Mutex_test Mutex_test.cpp)
add_executable(Thread_test Thread_test.cpp)
add_executable(Thread_bench Thread_bench.cpp)
add_executable(BlockingQueue_test BlockingQueue_test.cpp)
add_executable(BlockingQueue_bench BlockingQueue_bench.cpp)
add_executable(ThreadPool_test ThreadPool_test.cpp)
add_executable(Singleton_test Singleton_test.cpp)
add_executable(ThreadLocal_test ThreadLocal_test.cpp)
add_executable(ThreadLocalSingleton_test ThreadLocalSingleton_test.cpp)
add_executable(LogStream_bench LogStream_bench.cpp)
add_executable(Logging_test Logging_test.cpp)
add_executable(Tprint Tprint.cpp)
add_executable(Fork_test Fork_test.cpp)
add_executable(ProcessInfo_test ProcessInfo_test.cpp)
add_executable(FileUtil_test FileUtil_test.cpp)

target_link_libraries(TimeZone_unittest base)
target_link_libraries(Timestamp_unittest base)
target_link_libraries(Atomic_unittest base)
target_link_libraries(Exception_test base)
target_link_libraries(Mutex_test base)
target_link_libraries(Thread_test base)
target_link_libraries(Thread_bench base)
target_link_libraries(BlockingQueue_test base)
target_link_libraries(BlockingQueue_bench base)
target_link_libraries(ThreadPool_test base)
target_link_libraries(Singleton_test base)
target_link_libraries(ThreadLocal_test base)
target_link_libraries(ThreadLocalSingleton_test base)
target_link_libraries(LogStream_bench base)
target_link_libraries(Logging_test base)
target_link_libraries(ProcessInfo_test base)
target_link_libraries(FileUtil_test base)