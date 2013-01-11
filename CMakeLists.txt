cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(sup)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++ -Wall")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
#g++-mp-4.6
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fast")

# FIXME arch
set(CMAKE_CXX_FLAGS "-Wall")

# TODO optimizations
#-march=corei7

# ports gcc:
# cmake -D CMAKE_C_COMPILER=gcc-mp-4.6 -D CMAKE_CXX_COMPILER=g++-mp-4.6 . 

# mac ports:
include_directories(/opt/local/include)
link_directories(/opt/local/lib)

find_package(Boost COMPONENTS system program_options iostreams thread REQUIRED)

add_subdirectory(src)
