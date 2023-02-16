cmake_minimum_required(VERSION 3.24)
project(PPP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER mpic++)
set(CMAKE_CXX_FLAGS -fopenmp)

add_executable(PPP main.cpp sequential.cpp preform.cpp parallel.cpp)
