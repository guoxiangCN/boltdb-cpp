cmake_minimum_required(VERSION 3.1.0)
project(boltdb)

add_subdirectory(deps/googletest)

include_directories(include)
include_directories(deps/googletest/googletest/include)

add_library(boltdb-static page.cc node.cc)

add_executable(page_test tests/page_test.cc)
target_link_libraries(page_test boltdb-static gtest)