set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/ch01)

include_directories(${PROJECT_SOURCE_DIR}/include/thread)

link_directories(${PROJECT_SOURCE_DIR}/lib)

add_executable(Counter Counter.cpp)

target_link_libraries(Counter thread pthread)
