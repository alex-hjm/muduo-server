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
add_executable(server_basic sudoku/server_basic.cpp sudoku/sudoku.cpp)
add_executable(server_threadpool sudoku/server_threadpool.cpp sudoku/sudoku.cpp)
add_executable(server_multiloop sudoku/server_multiloop.cpp sudoku/sudoku.cpp)

#lib
target_link_libraries(echo muduo_net muduo_base)
target_link_libraries(finger muduo_net muduo_base)
target_link_libraries(server_basic muduo_net muduo_base)
target_link_libraries(server_threadpool muduo_net muduo_base)
target_link_libraries(server_multiloop muduo_net muduo_base)
