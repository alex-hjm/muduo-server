file(GLOB SRC_LIST "*.cpp")

add_library(base ${SRC_LIST})

target_link_libraries(base pthread)