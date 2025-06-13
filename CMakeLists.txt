cmake_minimum_required(VERSION 3.31)
project(EdenVirt)

set(CMAKE_CXX_STANDARD 23)
add_subdirectory(virtlib)
add_subdirectory(test)
add_executable(EdenVirt main.cpp)
