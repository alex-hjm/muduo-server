#output
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/ch07)

#header
include_directories(${PROJECT_SOURCE_DIR}/muduo/net)
include_directories(${PROJECT_SOURCE_DIR}/muduo/base)
include_directories(${PROJECT_SOURCE_DIR}/source/ch06)
include_directories(${PROJECT_SOURCE_DIR}/source/ch07)

#lib
link_directories(${PROJECT_SOURCE_DIR}/lib)

#executable
add_executable(discard discard/discard.cpp)
add_executable(daytime daytime/daytime.cpp)
add_executable(time time/time.cpp)
add_executable(timeclient timeclient/timeclient.cpp)
add_executable(chargen chargen/chargen.cpp)
add_executable(allinone allinone/allinone.cpp)
add_executable(filetransfer filetransfer/download3.cpp)

#lib
target_link_libraries(discard muduo_net muduo_base)
target_link_libraries(daytime muduo_net muduo_base)
target_link_libraries(timeclient muduo_net muduo_base)
target_link_libraries(chargen muduo_net muduo_base)
target_link_libraries(allinone muduo_net muduo_base)
target_link_libraries(filetransfer muduo_net muduo_base)