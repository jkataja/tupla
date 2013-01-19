#include "suffixsort.hpp"
#include "sortpar.hpp"
#include "sup.hpp"

#include <stdexcept>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <boost/thread/thread.hpp>

using namespace sup;

sup::sortpar::sortpar(const char * text, const uint32 len, const uint32 jobs,
		std::ostream& err)
	: suffixsort(text, len, err), jobs(jobs)
{
}

sup::sortpar::~sortpar()
{
}

void sup::sortpar::build_lcp()
{
	if (!finished_sa) {
		throw std::runtime_error("suffix array not complete");
	}
	
	lcp = new uint32[len];
	memset(lcp, 0, (len * sizeof(uint32)) );

	// XXX spa lecture10 luentokalvoista
	/*
	uint32 l = 0;
	for (size_t i = 0 ; i < len - 1 ; ++i) {
		uint32 j = sa[k - 1]; // XXX
		while (*(text + i + l) == *(text + j + l)) // XXX
			++l;
		lcp[ isa[i] ] = l;
		if (l > 0) --l;
	}
	finished_lcp = true;
	*/

}

void sup::sortpar::tccount(uint32 ptext, uint32 n, uint32 * pcount)
{
	for (size_t i = ptext ; i < ptext+n ; ++i) 
		++pcount[  (uint8)*(text + i) ];
}

void sup::sortpar::tcsort(uint32 ptext, uint32 n, uint32 * pcount,
		uint32 * group)
{
	uint32 p = 0; // Starting index of sorted group
	uint32 sl = 0; // Length of sorted groups following p
	
	for (size_t i = ptext ; i < ptext+n ; ++i) {
		// Counting sort f..g
		sa[ pcount[  (uint8)*(text + i) ]++ ] = i;
		// Sorting key for range f..g is g
		isa[i] = group[ (uint8)*(text + i) ];
		// Increase sorted group length
		if (uint32 s = sorted[i]) {
			sl += s; 
			continue;
		} 
		// Combine sorted group before i
		if (sl > 0) {
			sorted[p] = sl; 
			sl = 0;
		}
		p = i + 1;
	}
}

void sup::sortpar::init()
{
	uint32 group[Alpha] = { Z256 };
	uint32 count[Alpha] = { Z256 };

	sa = new uint32[len];
	isa = new uint32[len];
	sorted = new uint32[len];

	uint32 * tcount = new uint32[Alpha * jobs];

	memset(sa, 0, (len * sizeof(uint32)) );
	memset(isa, 0, (len * sizeof(uint32)) );
	memset(sorted, 0, (len * sizeof(uint32)) );
	memset(tcount, 0, (Alpha * jobs * sizeof(uint32)) );

	size_t tstep = std::max(JobMinInput, (len/jobs) + 1); 

	// Count character occurences
	{
		boost::thread_group tccount_group;
		size_t ptext = 0;
		size_t tlen = len;

		for (size_t j = 0 ; j < jobs ; ++j) {
			uint32 n = std::min(tlen, tstep);
			uint32 * pcount = (tcount + (Alpha * j));

			tccount_group.create_thread(
					boost::bind(&sup::sortpar::tccount, this, 
							ptext, n, pcount) );

			if (tlen <= tstep) break;
			tlen -= tstep; ptext += tstep;
		}
		tccount_group.join_all();
	}
	// Merge
	for (size_t i = 0 ; i < (jobs * Alpha) ; ++i)
		count[i & 0xFF] += tcount[i];
	
	// Multiple nulls in input
	if (count[0] != 1) 
		throw std::runtime_error("input contains multiple nulls");

	// Assign initial group of each character and build counting sort tables
	uint32 f = 0; // First index in group
	for (size_t i = 0 ; i < Alpha ; ++i) {
		uint32 n = count[i];
		uint32 tn = tcount[i]; // Count in thread segment
		uint32 g = f + n - 1; // Last position in group f..g

		group[i] = g; // Assign group sorting key to last index in f..g
		sorted[f] = (n == 1); // Singleton group is sorted

		// Counting sort starts from f for first and f+tn for following threads
		tcount[i] = f; 
		for (size_t j = 1 ; j < jobs ; ++j) {
			uint32 tin = tcount[(j * Alpha) + i];
			tcount[(j * Alpha) + i] = f + tn;
			tn += tin;
		}

		f += n;
	}

	// Counting sort
	{
		boost::thread_group tcsort_group;
		size_t ptext = 0;
		size_t tlen = len;

		for (size_t j = 0 ; j < jobs ; ++j) {
			uint32 n = std::min(tlen, tstep);
			uint32 * pcount = (tcount + (Alpha * j));

			tcsort_group.create_thread(
					boost::bind(&sup::sortpar::tcsort, this, 
							ptext, n, pcount, group) );

			if (tlen <= tstep) break;
			tlen -= tstep; ptext += tstep;
		}
		tcsort_group.join_all();
	}

	// TODO Merge sorted[] at border
	/*
	size_t tlen = len;
	for (size_t j = 1 ; j < jobs ; ++j) {
		uint32 n = std::min(tlen, tstep);

		if (tlen <= tstep) break;
		tlen -= tstep; ptext += tstep;
	}
	*/

	delete [] tcount;
}

uint32 sup::sortpar::doubling()
{
	err << SELF << ": not implemented" << std::endl;
	/*
The next ((j + 1)-th) doubling step for prosessor i works as follows. Set 
Ai[k] = ((R[xik], R[xik + 2(j+1) - 1]), xik) 
and sort the array by the values (R[xik], R[xik + 2^(j+1) - 1]), 
and replace the values with the ranks of the pairs. Here omitting the
special cases of indexes exceeding |T|. 
Once the array is sorted, the head of the
array where ranks are below the pivot S^(i−1)
and tail of the array where ranks are
above S^i are cut oﬀ.
This process is repeated on all processors i at most O(log n) times
until all ranks are unique. With perfect choice of pivots, the
running time of distributed doubling algorithm is O(n + N log 2
n/p), where n ≤ N ≤ np is the size of the global rank table after
ﬁrst round (again T = c n is the worst case). This bound assumes
comparison sort is used for sorting the pairs; better bounds can
be achieved using other models of computation. When assuming T is
sampled from uniform distribution, we have N ≤ n⌈p/σ⌉ (for each
character there is the same amount of pivots).
	*/
	return 0;
}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
