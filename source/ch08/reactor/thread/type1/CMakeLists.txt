set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/ch08)

add_executable(Thread_test Thread_test.cpp Thread.cpp)

target_link_libraries(Thread_test pthread)