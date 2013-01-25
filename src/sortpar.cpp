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
	: suffixsort(text, len, err), jobs(jobs), 
	  cakeslice( std::min( std::max(BucketSize, (len/jobs) + 1) , len) )
{
}

sup::sortpar::~sortpar()
{
}

uint32 sup::sortpar::tqsort(uint32 p, size_t n)
{
	uint32 a,b,c,d;
	uint32 pn = p + n;

	// Sort small tables with bingo sort 
	// Supposedly more efficient than selection sort with duplicate values
	// Adapted from:
	// @see http://en.wikipedia.org/wiki/Selection_sort#Variants
	if (n < 7) {
		a = pn-1;
		uint32 eqn = 0; // Count of equal items
		uint32 ns = 0; // Count of assigned singleton groups

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
			assign(a + 1, eqn); ns += (eqn == 1);
			eqn = 0;
			while ((a > p) && (k(a) == v)) { --a; ++eqn; }
		}
		// First index also contained highest value
		if (k(p) == v) {
			assign(p, eqn + 1); ns += (eqn == 0);
		}
		else {
			assign(a + 1, eqn); ns += (eqn == 1);
			assign(p, 1);  ++ns;
		}
		return ns;
	}

	const uint64 v = choose_pivot(p, n);

	// Partition
	a = b = p;
	c = d = p + (n-1);

	// Assume on range i=f..g value ISA_h[ SA_h[i] ] is equal
	// Only use the doubling part  ISA_h[ SA_h[i] + h ] in comparison
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

	// New singleton groups in ranges less than and greater than pivot
	uint32 lts = 0;
	uint32 gts = 0;

	if (ltn > 0) lts = sort_switch(p, ltn);
	assign(p+ltn, eqn); 
	if (gtn > 0) gts = sort_switch(pn-gtn, gtn);

	return (lts + (eqn == 1) + gts);
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
		uint32 * group, uint8 * sorted)
{
	uint32 p = 0; // Starting index of sorted group
	uint32 sl = 0; // Length of sorted groups following p

	for (size_t i = ptext ; i < ptext+n ; ++i) {
		uint8 c = (uint8)*(text + i);
		uint32 j = pcount[c]++;
		// Counting sort f..g
		sa[j] = i;
		// Sorting key for range f..g is g
		isa[i] = group[c];
		// Increase sorted group length
		if (sorted[c]) set_sorted(j, 1);
		if (uint32 s = get_sorted(j)) {
			sl += s; 
			continue;
		} 
		// Combine sorted group before i
		if (sl > 0) {
			set_sorted(p, sl);
			sl = 0;
		}
		p = i + 1;
	}
}

void sup::sortpar::init_count(uint32 * tcount, uint32 * count)
{
	boost::thread_group tccount_group;
	size_t p = 0; 
	size_t n = len; 

	for (size_t j = 0 ; j < jobs ; ++j) {
		uint32 * pcount = (tcount + (Alpha * j)); 

		tccount_group.create_thread( boost::bind(&sup::sortpar::tccount, 
				this, p, std::min(n, cakeslice), pcount) );

		if (n <= cakeslice) break;
		n -= cakeslice; p += cakeslice;
	}
	tccount_group.join_all();

	// Merge
	for (size_t i = 0 ; i < (jobs * Alpha) ; ++i)
		count[i & 0xFF] += tcount[i];
}

void sup::sortpar::init_sort(uint32 * tcount, uint32 * group, uint8 * sorted)
{
	boost::thread_group tcsort_group;
	size_t p = 0;
	size_t n = len;

	for (size_t j = 0 ; j < jobs ; ++j) {
		uint32 * pcount = (tcount + (Alpha * j));

		tcsort_group.create_thread(
				boost::bind(&sup::sortpar::tcsort, this,
					p, std::min(n, cakeslice), pcount, group, sorted) );

		if (n <= cakeslice) break;
		n -= cakeslice; p += cakeslice;
	}
	tcsort_group.join_all();
}

uint32 sup::sortpar::init()
{
	uint32 alphasize = 0;
	uint32 group[Alpha] = { Z256 };
	uint32 count[Alpha] = { Z256 };
	uint8 sorted[Alpha] = { Z256 };

	sa = new uint32[len];
	isa = new uint32[len];
	isa_assign = new uint32[len];

	// Per thread character counts
	uint32 * tcount = new uint32[Alpha * jobs];

	memset(sa, 0, (len * sizeof(uint32)) );
	memset(isa, 0, (len * sizeof(uint32)) );
	memset(tcount, 0, (Alpha * jobs * sizeof(uint32)) );

	// Count characters
	init_count(tcount, count);

	// Multiple nulls in input
	if (count[0] != 1) {
		throw std::runtime_error("input contains multiple nulls");
	}

	// Assign initial group of each character and build counting sort tables
	uint32 f = 0; // First index in group
	for (size_t i = 0 ; i < Alpha ; ++i) {
		uint32 n = count[i];
		uint32 tn = tcount[i]; // Count in thread segment
		uint32 g = f + n - 1; // Last position in group f..g

		group[i] = g; // Assign group sorting key to last index in f..g
		groups += sorted[i] = (n == 1); // Singleton group is sorted

		// Counting sort starts from f for first and f+tn for following threads
		tcount[i] = f; 
		for (size_t j = 1 ; j < jobs ; ++j) {
			uint32 tin = tcount[(j * Alpha) + i];
			tcount[(j * Alpha) + i] = f + tn;
			tn += tin;
		}

		f += n;

		alphasize += (n > 0);
	}

	// Counting sort
	init_sort(tcount, group, sorted);

	// TODO Merge sorted[] at border

	delete [] tcount;

	return alphasize;
}

void sup::sortpar::doubling(uint32 p, size_t n) {
	uint32 sp = p; // Sorted group start
	uint32 sl = 0; // Sorted groups length following start
	uint32 ns = 0; // New singleton groups g
	for (size_t i = p ; i < p+n ; ) {
		// Skip sorted group
		if (uint32 s = get_sorted(i)) {
			i += s; sl += s;
			continue;
		} 
		// Combine sorted group before i
		if (sl > 0) {
			set_sorted(sp, sl);
			sl = 0;
		}
		// Sort unsorted group i..g
		uint32 g = isa[ sa[i] ] + 1;

		ns += sort_switch(i, g-i);

		sp = i = g;
	}
	// Combine sorted group at end
	if (sl > 0) set_sorted(sp, sl);

	groups_lock.lock();
	groups += ns;
	groups_lock.unlock();
}

void sup::sortpar::doubling()
{
	memcpy(isa_assign, isa, sizeof(uint32) * len);

	// Thread pool
	tp.size_controller().resize(jobs);

	uint32 p = 0; // Starting index of bucket

	// Buckets p..pn
	for (uint32 pn = BucketSize ; p < len ; p = pn , pn += BucketSize) {
		if (pn > len - h) pn = len; // End of file
		else if (!get_sorted(pn)) pn = isa[ sa[pn] ] + 1; // Last in group

		boost::shared_ptr<doubling_task> job(
				new doubling_task(this, p, (pn-p)));
		boost::threadpool::schedule(tp, 
				boost::bind(&doubling_task::run, job));

	}
	tp.wait();

	// Keep count of assigned singletons
	for (auto it : tasks) groups += it.groups;
	tasks.clear();

	std::swap( isa, isa_assign );
}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
