cmake_minimum_required(VERSION 3.16)
project(Mymalloc C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -pthread -Wall  -Werror -Wno-error=unused-variable  -Wno-error=unused-function")

if(COVERAGE)
set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

add_library(mymalloc mymalloc.c page_alloc.c dllist.c)

add_executable(test_mymalloc test_mymalloc.c)
target_link_libraries(test_mymalloc mymalloc m cunit)

add_executable(log_reader log_reader.c)

include(CTest)

add_test(NAME test_mymalloc  COMMAND test_mymalloc)
