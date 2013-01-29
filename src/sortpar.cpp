#include <boost/threadpool.hpp>
#include <boost/bind.hpp>

#include "sortpar.hpp"
#include "sup.hpp"
#include "tqsort_task.hpp"
#include "doubling_task.hpp"

using namespace sup;

sup::sortpar::sortpar(const char * text, const uint32 len, const uint32 jobs,
		std::ostream& err)
	: suffixsort(text, len, err), isa_assign(0), jobs(jobs), 
	  chunk( std::min( std::max(BucketSize, (len/jobs) + 1) , len) )
{
}

sup::sortpar::~sortpar()
{
	delete [] isa_assign;
}

uint32 sup::sortpar::tqsort(uint32 p, size_t n)
{
	uint32 a,b,c,d;
	uint32 pn = p + n;

	if (n < 7) return sort_small(p, n);

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

uint32 sup::sortpar::tqsort_grainsize(uint32 p, size_t n)
{
	uint32 a,b,c,d;
	uint32 pn = p + n;

	if (n < 7) return sort_small(p, n);

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

	if (ltn > 0) lts = tqsort_grainsize(p, ltn);
	assign(p+ltn, eqn); 
	if (gtn > 0) gts = tqsort_grainsize(pn-gtn, gtn);

	return (lts + (eqn == 1) + gts);
}

void sup::sortpar::build_lcp()
{
	if (!finished_sa) {
		throw std::runtime_error("suffix array not complete");
	}

	err << SELF << ": building longest common prefix array" << std::endl;
	
	lcp = new uint32[len];
	memset(lcp, 0, (len * sizeof(uint32)) );

	parallel_chunk( boost::bind(&sup::sortpar::lcp_range, this, _1, _2) );
	
	finished_lcp = true;

}

uint32 sup::sortpar::init()
{
	uint32 group[Alpha] = { Z256 };
	uint32 count[Alpha] = { Z256 };
	uint8 sorted[Alpha] = { Z256 };

	sa = new uint32[len];
	isa = new uint32[len];
	isa_assign = new uint32[len];

	// Thread specific character counts
	uint32 * range_count = new uint32[Alpha * jobs];

	memset(sa, 0, (len * sizeof(uint32)) );
	memset(isa, 0, (len * sizeof(uint32)) );
	memset(range_count, 0, (Alpha * jobs * sizeof(uint32)) );

	// Count characters and merge
	parallel_chunk( boost::bind(&sup::sortpar::count_range, 
			this, _1, _2, range_count, _3) );
	for (size_t i = 0 ; i < (jobs * Alpha) ; ++i)
		count[i & 0xFF] += range_count[i];

	// Multiple nulls in input
	if (count[0] != 1) 
		throw std::runtime_error("input contains multiple nulls");

	// Assign initial sorting groups and build prefix sums
	uint32 alphasize = build_prefix(count, range_count, group, sorted, jobs);

	// Counting sort on first character of suffix
	parallel_chunk( boost::bind(&sup::sortpar::sort_range, 
			this, _1, _2, range_count, group, sorted, _3) ); 

	delete [] range_count;

	return alphasize;
}

void sup::sortpar::doubling_range(uint32 p, size_t n) {
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

void sup::sortpar::invert()
{
	parallel_chunk( boost::bind(&sup::sortpar::invert_range, this, _1, _2) ); 
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
