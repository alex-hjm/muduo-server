#output
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/ch06)

#header
include_directories(${PROJECT_SOURCE_DIR}/muduo/net)
include_directories(${PROJECT_SOURCE_DIR}/muduo/base)

#lib
link_directories(${PROJECT_SOURCE_DIR}/lib)

#executable
add_executable(echo echo/echo.cpp)
add_executable(finger finger/finger07.cpp)

#lib
target_link_libraries(echo muduo_net muduo_base)
target_link_libraries(finger muduo_net muduo_base)
