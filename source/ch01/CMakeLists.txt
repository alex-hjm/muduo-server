#output
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/ch01)

#header
include_directories(${PROJECT_SOURCE_DIR}/source/recipes)

#lib
link_directories(${PROJECT_SOURCE_DIR}/lib)

#executable
add_executable(Counter Counter.cpp )
add_executable(Observer Observer.cpp)
add_executable(Observer_safe Observer_safe.cpp )
add_executable(SharedPtr SharedPtr.cpp )
add_executable(StockFactory StockFactory.cpp )

#lib
target_link_libraries(Counter thread pthread)
target_link_libraries(Observer thread pthread)
target_link_libraries(Observer_safe thread pthread)
target_link_libraries(SharedPtr thread pthread)
target_link_libraries(StockFactory thread pthread)