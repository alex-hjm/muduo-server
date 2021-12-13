set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/ch02)

include_directories(${PROJECT_SOURCE_DIR}/include/thread)

link_directories(${PROJECT_SOURCE_DIR}/lib)

set(LINK_LIB thread pthread)

#executable
add_executable(NonRecursiveMutex_test NonRecursiveMutex_test.cpp)
add_executable(SelfDeadLock SelfDeadLock.cpp)
add_executable(MutualDeadLock MutualDeadLock.cpp)
add_executable(CopyOnWrite_test CopyOnWrite_test.cpp)
add_executable(RequestInventory_test RequestInventory_test.cpp)
add_executable(RequestInventory_test2 RequestInventory_test2.cpp)
add_executable(Customer Customer.cpp)
#lib
target_link_libraries(NonRecursiveMutex_test ${LINK_LIB})
target_link_libraries(SelfDeadLock ${LINK_LIB})
target_link_libraries(MutualDeadLock ${LINK_LIB})
target_link_libraries(CopyOnWrite_test ${LINK_LIB})
target_link_libraries(RequestInventory_test ${LINK_LIB})
target_link_libraries(RequestInventory_test2 ${LINK_LIB})
target_link_libraries(Customer ${LINK_LIB})