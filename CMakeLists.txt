cmake_minimum_required (VERSION 2.8)

project (muduo-server)

set(SRC ./source)

#create lib
add_subdirectory(include)

#src
add_subdirectory(${SRC}/ch01)
add_subdirectory(${SRC}/ch02)
