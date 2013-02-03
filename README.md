tupla
=====

Parallel suffix sorting in shared memory. 

Build
=====

Requires gcc 4.6, boost and cmake to build.
The project includes boost::threadpool from http://threadpool.sourceforge.net/ 

The build targets Core i7 instruction set by default.
If the target CPU supports SSE4.2 extensions, the program uses
cmpistri instruction in string comparisons.
This instruction can match the common prefix of two strings at 16
characters at a time, improving the performance when computing 
Longest Common Prefix tables.
To target your local machine, replace the option '-march=corei7'
with '-march=native' in CMakeLists.txt in project root.

Linux

	$ cmake -D CMAKE_BUILD_TYPE=Release .
	$ make

OS X

Static boost libraries in Mac Ports may need to be recompiled to work correctly with gcc-mp-4.6 compiled binaries. See http://lists.macosforge.org/pipermail/macports-users/2011-November/026364.html


	$ cmake -D CMAKE_C_COMPILER=gcc-mp-4.6 -D CMAKE_CXX_COMPILER=g++-mp-4.6 -D CMAKE_BUILD_TYPE=Release .
	$ make

Usage
=====

The number of jobs (option --jobs) defaults to hardware threads
available on the system.

	Usage: tupla [option]... input-file
	Parallel suffix sorting in shared memory.

	Options:
	  -b [ --benchmark ]     Do not output file(s)
	  -f [ --force ]         Force overwrite of existing output
	  -h [ --help ]          Show this help and exit
	  -j [ --jobs ] arg (=4) Allow arg threads to run simultaneously [1,64]
	  -l [ --lcp ]           Compute LCP array as well
	  -n [ --count ] arg     Stop processing input after arg bytes
	  -o [ --output ]        Print generated suffix array to stderr
	  -v [ --validate ]      Validate generated suffix array (slow)

