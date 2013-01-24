/**
 * Doubling suffix sort. Parallel algorithm. 
 *
 * Requires 12n memory compared to 8n for Larsson & Sadakane implementation, 
 * but finished suffix array is readily accessible without inverting ISA.
 *
 * Impelements prefix doubling as described in:
 * N. Jesper Larsson & Kunihiko Sadakane: Faster Suffix Sorting
 * LU-CS-TR:99-214 [LUNFD6/(NFCS-3140)/1{20/(1999)]
 *
 * Uses using ternary split quick sort, based on:
 * Bentley & McIlroy 1993: Engineering a Sort Function
 * SOFTWARE—PRACTICE AND EXPERIENCE, VOL. 23(11), 1249–1265 (NOVEMBER 1993)
 *
 * @author jkataja
 */

#pragma once

#include <iostream>
#include <boost/cstdint.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/threadpool.hpp>
#include <boost/thread/thread.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "numdefs.hpp"
#include "suffixsort.hpp"
#include "tqsort_task.hpp"

namespace sup {

class sortpar : public suffixsort {

private:

	sortpar(const sortpar&);
	sortpar& operator=(const sortpar&);

	boost::mutex tasks_lock;
	boost::mutex groups_lock;

	// Pooled sort operations
	boost::threadpool::pool tp;

	// Concurrent modifications to isa would alter sorting order
	// Assign new groups after doubling to this array temporarily
	uint32 * isa_assign; 

	// Number of concurrent threads to run
	const uint32 jobs;

	// Share of text length per job
	const size_t cakeslice;

	// Character count run on one thread
	void tccount(uint32, uint32, uint32 *);

	// Selection sort run on one thread
	void tcsort(uint32, uint32, uint32 *, uint32 *);

	// Parallel character count
	void init_count(uint32 *, uint32 *);

	// Parallel selection sort
	void init_sort(uint32 *, uint32 *);

	// Ternary quicksort on items in range p..p+n-1
	// Returns the count of new singleton groups
	uint32 tqsort(uint32 p, size_t n);

	// Pointers to sort tasks added too task pool
	boost::ptr_vector<tqsort_task> tasks;

	// Sort grain size range in this thread, or add new task to sort it later
	// Returns the count of new singleton groups
	inline uint32 sort_switch(uint32 p, size_t n) 
	{
		// Call tqsort in this thread
		if (n < BucketSize) return tqsort(p, n);

		// Create new task in thread pool to sort range
		tqsort_task * job = new tqsort_task(this, p, n);
		boost::threadpool::schedule(tp, boost::bind(&tqsort_task::run, job));
	
		tasks_lock.lock();
		tasks.push_back(job);
		tasks_lock.unlock();

		return 0; // Added in doubling()
	}

	// Renumber group at p..p+n-1 with matching sorting key as p+n-1
	// Group number is last index with value to keep sort keys decreasing
	// Store assignments in isa_assign due to concurrent modifications 
	// altering sort ordering
	inline void assign(uint32 p, size_t n)
	{
		uint32 g = p + n - 1;

		if (n == 1) sorted[p] = 1; // Mark as sorted singleton group

		for (size_t i = p ; i < p+n ; ++i) 
			isa_assign[ sa[i] ] = g;
	}

protected:
	virtual uint32 init();

	virtual void doubling();
	virtual void doubling(uint32, size_t);

public:

	sortpar(const char *, const uint32, const uint32, std::ostream&);
	virtual ~sortpar();
	virtual void build_lcp();

};

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab:
