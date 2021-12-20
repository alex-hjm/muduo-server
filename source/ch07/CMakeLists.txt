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
add_executable(chat_server chat/server.cpp)
add_executable(chat_client chat/client.cpp)
add_executable(chat_server_threaded chat/server_threaded.cpp)
add_executable(chat_server_threaded_efficient chat/server_threaded_efficient.cpp)
add_executable(chat_server_threaded_highperformance chat/server_threaded_highperformance.cpp)

#lib
target_link_libraries(discard muduo_net muduo_base)
target_link_libraries(daytime muduo_net muduo_base)
target_link_libraries(timeclient muduo_net muduo_base)
target_link_libraries(chargen muduo_net muduo_base)
target_link_libraries(allinone muduo_net muduo_base)
target_link_libraries(filetransfer muduo_net muduo_base)
target_link_libraries(chat_server muduo_net muduo_base)
target_link_libraries(chat_client muduo_net muduo_base)
target_link_libraries(chat_server_threaded muduo_net muduo_base)
target_link_libraries(chat_server_threaded_efficient muduo_net muduo_base)
target_link_libraries(chat_server_threaded_highperformance muduo_net muduo_base)