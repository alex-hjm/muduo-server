#output
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/ch02)

#header
include_directories(${PROJECT_SOURCE_DIR}/source/recipes)

#lib
link_directories(${PROJECT_SOURCE_DIR}/lib)

#executable
add_executable(NonRecursiveMutex_test NonRecursiveMutex_test.cpp)
add_executable(SelfDeadLock SelfDeadLock.cpp)
add_executable(MutualDeadLock MutualDeadLock.cpp)
add_executable(CopyOnWrite_test CopyOnWrite_test.cpp)
add_executable(RequestInventory_test RequestInventory_test.cpp)
add_executable(RequestInventory_test2 RequestInventory_test2.cpp)

#lib
target_link_libraries(NonRecursiveMutex_test thread pthread)
target_link_libraries(SelfDeadLock thread pthread)
target_link_libraries(MutualDeadLock thread pthread)
target_link_libraries(CopyOnWrite_test thread pthread)
target_link_libraries(RequestInventory_test thread pthread)
target_link_libraries(RequestInventory_test2 thread pthread)