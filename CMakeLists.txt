cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(sup)

option(USE_SSE4_2 "USE_SSE4_2" ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x -Wall -m64")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=corei7")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -funroll-loops -fprefetch-loop-arrays")

# mac ports
include_directories(/opt/local/include ${CMAKE_SOURCE_DIR}/dependencies/threadpool/include)
link_directories(/opt/local/lib)

find_package(Boost COMPONENTS system program_options iostreams thread REQUIRED)

add_subdirectory(src)
