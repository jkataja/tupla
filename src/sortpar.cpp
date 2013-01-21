#include <stdexcept>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>
#include <boost/threadpool.hpp>
#include <boost/thread/thread.hpp>

#include "suffixsort.hpp"
#include "sortpar.hpp"
#include "sup.hpp"
#include "tqsort_task.hpp"
#include "doubling_task.hpp"

using namespace sup;

sup::sortpar::sortpar(const char * text, const uint32 len, const uint32 jobs,
		std::ostream& err)
	: suffixsort(text, len, err), jobs(jobs)
{
}

sup::sortpar::~sortpar()
{
}

void sup::sortpar::tqsort(uint32 p, size_t n)
{
	uint32 a,b,c,d;
	uint32 pn = p + n;

	// Sort small tables with bingo sort 
	// Supposedly more efficient than selection sort with duplicate values
	// Adapted from:
	// @see http://en.wikipedia.org/wiki/Selection_sort#Variants
	if (n < 7) {
		a = pn-1;
		uint32 eqn = 0;

		// Find the highest value
		uint64 v = k(a);
		for (uint32 i = a ; i >= p ; --i)
			if (k(i) > v) v = k(i);
		while ((a > p) && (k(a) == v)) { --a; ++eqn; }

		// Move every item with highest values to end of list
		// One pass for each value in list
		while (a > p) {
			uint64 f = v;
			v = k(a);
			for (uint32 i = a - 1 ; i >= p ; --i) {
				uint64 ki = k(i);
				if (ki == f) { swap(i, a--); ++eqn; }
				else if (ki > v) v = ki;
			}
			// Assign items sharing highest value to new group
			assign(a + 1, eqn);
			eqn = 0;
			while ((a > p) && (k(a) == v)) { --a; ++eqn; }
		}
		// First index also contained highest value
		if (k(p) == v) {
			assign(p, eqn + 1);
		}
		else {
			assign(a + 1, eqn);
			assign(p, 1);
		}
		return;
	}
	
	const uint64 v = choose_pivot(p, n);

	// Partition
	a = b = p;
	c = d = p + (n-1);

	// Assume on range i=f..g value ISA_h[ SA_h[i] ] is equal
	// Only use the doubling part  ISA_h[ SA_h[i] + h ] as comparison key
	uint32 sv = (v & 0xFFFFFFFF);
	uint32 tv;
	for (;;) {
		while (b <= c && (tv = isa[ sa[b] + h ]) <= sv) {
			if (tv == sv) swap(a++, b); 
			++b;
		}
		while (c >= b && (tv = isa[ sa[c] + h ]) >= sv) {
			if (tv == sv) swap(c, d--);
			--c;
		}
		if (b > c) break;
		swap(b++, c--);
	}

	// Move split-end group to middle
	const uint32 s = std::min(a-p, b-a ); vecswap(p, b-s, s);
	const uint32 t = std::min(d-c, pn-1-d); vecswap(b, pn-t, t);

	const uint32 ltn = b-a;
	const uint32 gtn = d-c;
	const uint32 eqn = n - ltn - gtn;

	if (ltn > 0) sort_switch(p, ltn);
	assign(p+ltn, eqn); 
	if (gtn > 0) tqsort(pn-gtn, gtn);
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
	isa_assign = new uint32[len];
	sorted = new uint32[len];

	uint32 * tcount = new uint32[Alpha * jobs];

	memset(sa, 0, (len * sizeof(uint32)) );
	memset(isa, 0, (len * sizeof(uint32)) );
	memset(sorted, 0, (len * sizeof(uint32)) );
	memset(tcount, 0, (Alpha * jobs * sizeof(uint32)) );

	size_t tstep = std::max(GrainSize, (len/jobs) + 1); 

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
		groups += sorted[f] = (n == 1); // Singleton group is sorted

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
	// Thread pool
	tp.size_controller().resize(jobs);


	delete [] tcount;
}

void sup::sortpar::doubling()
{
	memcpy(isa_assign, isa, sizeof(uint32) * len);

	uint32 p = 0; // Starting index of sorted group
	uint32 sl = 0; // Sorted groups length following start
	for (size_t i = 0 ; i < len ; ) {
		// Skip sorted group
		if (uint32 s = sorted[i]) {
			i += s; sl += s;
			continue;
		} 
		// Combine sorted group before i
		if (sl > 0) {
			assign_lock.lock(); sorted[p] = sl; assign_lock.unlock(); 
			sl = 0;
		}
		// Sort unsorted group i..g
		uint32 g = isa[ sa[i] ] + 1;
		sort_switch(i, g-i);
		p = i = g;
	}
	tp.wait();

	// Combine sorted group at end
	if (sl > 0) sorted[p] = sl;

	std::swap( isa, isa_assign );
}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
