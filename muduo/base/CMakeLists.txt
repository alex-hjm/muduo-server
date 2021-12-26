file(GLOB SRC_LIST "*.cpp" )
file(GLOB INC_LIST "*.h" )

add_library(base ${INC_LIST} ${SRC_LIST})

message(${SRC_LIST} ${INC_LIST})

target_link_libraries(base pthread)