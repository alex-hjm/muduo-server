set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/ch01)

include_directories(${PROJECT_SOURCE_DIR}/include/thread)

link_directories(${PROJECT_SOURCE_DIR}/lib)

set(LINK_LIB thread pthread)

#executable
add_executable(Counter Counter.cpp)
add_executable(Observer Observer.cpp)
add_executable(Observer_safe Observer_safe.cpp)
add_executable(SharedPtr SharedPtr.cpp)
add_executable(StockFactory StockFactory.cpp)

#lib
target_link_libraries(Counter ${LINK_LIB})
target_link_libraries(Observer_safe ${LINK_LIB})
target_link_libraries(SharedPtr ${LINK_LIB})
target_link_libraries(StockFactory ${LINK_LIB})