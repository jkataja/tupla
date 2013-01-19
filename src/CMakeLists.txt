cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(sup_source_files
	suffixsort.cpp
	sortseq.cpp
	sortpar.cpp
	sup.cpp
	main.cpp
)
add_executable(sup ${sup_source_files})
target_link_libraries(sup ${Boost_LIBRARIES})

set(supgrep_source_files
	supgrep.cpp
	sup.cpp
)
add_executable(supgrep ${supgrep_source_files})
target_link_libraries(supgrep ${Boost_LIBRARIES})

set(suptest_source_files
	suffixsort.cpp
	sortseq.cpp
	sortpar.cpp
	sup.cpp
	suptest.cpp
)
add_executable(suptest ${suptest_source_files})
target_link_libraries(suptest ${Boost_LIBRARIES})

