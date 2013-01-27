/**
 * Doubling suffix sort. Parallel implementation. Requires 12n memory.
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

#include <boost/thread/mutex.hpp>
#include <boost/threadpool.hpp>
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
	const size_t chunk;

	// Longest common prefix for range
	void lcp_range(uint32 p, uint32 n);

	// Character count for range
	void count_range(uint32, uint32, uint32 *, uint32);

	// Counting sort for range
	void sort_range(uint32, uint32, uint32 *, uint32 *, uint8 *, uint32);

	// Reconstruct suffix array from inverse suffix array
	void invert_range(uint32, uint32);

	// Invoke function parallel for each thread's range in input
	template <class F>
	void parallel_chunk(F fun_range)
	{
		boost::thread_group threads;
		size_t p = 0;
		size_t n = len;

		for (size_t j = 0 ; j < jobs ; ++j) {
			threads.create_thread( 
					boost::bind(fun_range, p, std::min(n, chunk), j) );

			if (n <= chunk) break;
			n -= chunk; p += chunk;
		}
		threads.join_all();
	}

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

		for (size_t i = p ; i < p+n ; ++i) 
			isa_assign[ sa[i] ] = g;
		
		if (n == 1) set_sorted(p, 1); // Mark as sorted singleton group
	}

protected:
	virtual uint32 init();

	virtual void invert();
	virtual void doubling();
	virtual void doubling(uint32, size_t);

public:

	sortpar(const char *, const uint32, const uint32, std::ostream&);
	virtual ~sortpar();
	virtual void build_lcp();

};

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab:
