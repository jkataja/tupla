sup
===

Parallel suffix sorting in shared memory

build
=====

OS X

	$ cmake -D CMAKE_C_COMPILER=gcc-mp-4.6 -D CMAKE_CXX_COMPILER=g++-mp-4.6 -D CMAKE_BUILD_TYPE=Release .
	$ make

usage
=====

	Usage: sup [option]... input-file
	Parallel suffix sorting in shared memory.

	Options:
	  -j [ --jobs ] arg (=4) concurrency level [1,64]
	  -h [ --help ]          show this help
	  -l [ --lcp ]           compute LCP array as well
	  -f [ --force ]         force overwrite of existing output
	  -v [ --validate ]      validate generated suffix array (slow)
	  -o [ --output ]        print generated suffix array to stderr
	  -b [ --benchmark ]     do not output file(s)

