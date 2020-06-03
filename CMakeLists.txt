project(pcse CXX)
cmake_minimum_required(VERSION 3.10.3)
set(CMAKE_CXX_STANDARD 17)

# tests
add_executable(tests EXCLUDE_FROM_ALL test.cpp)
find_package(Catch2 REQUIRED)
target_link_libraries(tests Catch2::Catch2)

# main
add_executable(pcse main.cpp)
