cmake_minimum_required (VERSION 2.8)

project (CQLite)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

include(CTest)

add_library(unity STATIC unity/src/unity.c)
target_include_directories(unity PUBLIC unity/src)

add_subdirectory(src)
add_subdirectory(test)