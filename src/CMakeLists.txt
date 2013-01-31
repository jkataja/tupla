cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(tupla_source_files
	suffixsort.cpp
	sortseq.cpp
	sortpar.cpp
	tupla.cpp
	main.cpp
)
add_executable(tupla ${tupla_source_files})
target_link_libraries(tupla ${Boost_LIBRARIES})

set(tuplatest_source_files
	suffixsort.cpp
	sortseq.cpp
	sortpar.cpp
	tupla.cpp
	tuplatest.cpp
)
add_executable(tuplatest ${tuplatest_source_files})
target_link_libraries(tuplatest ${Boost_LIBRARIES})

