set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# thread lib
add_library (thread SHARED ./thread/Thread.cpp)