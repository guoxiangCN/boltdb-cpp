cmake_minimum_required(VERSION 3.1.0)
project(boltdb)

add_subdirectory(deps/googletest)

include_directories(include)
include_directories(deps/googletest/googletest/include)

SET(SRCS page.cc 
         node.cc 
         meta.cc
         db.cc
         tx.cc
         cursor.cc
         freelist.cc
         fsync.cc 
         status.cc)

# static library
add_library(boltdb-static ${SRCS})

# testcases
add_executable(page_test tests/page_test.cc)
target_link_libraries(page_test boltdb-static gtest)

add_executable(tx_test tests/tx_test.cc)
target_link_libraries(tx_test boltdb-static gtest)