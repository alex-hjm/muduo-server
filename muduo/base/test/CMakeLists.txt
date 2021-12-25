add_subdirectory(poll)
add_subdirectory(epoll)
add_subdirectory(thread_type1)
add_subdirectory(thread_type2)

add_executable(bind_func bind_func.cpp)
add_executable(Timestamp_unittest Timestamp_unittest.cpp)
add_executable(Atomic_unittest Atomic_unittest.cpp)
add_executable(Exception_test Exception_test.cpp)

target_link_libraries(Timestamp_unittest base)
target_link_libraries(Atomic_unittest base)
target_link_libraries(Exception_test base)


