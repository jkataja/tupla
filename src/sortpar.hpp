/**
 * Doubling suffix sort. Parallel implementation. Requires 12n memory.
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

namespace tupla {

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
	// Recurse to sort_switch
	// Returns the count of new singleton groups
	uint32 tqsort(uint32 p, size_t n);

	// Ternary quicksort on items in range p..p+n-1
	// Recurse to tqsort_grainsize
	// Returns the count of new singleton groups
	uint32 tqsort_grainsize(uint32 p, size_t n);

	// Pointers to sort tasks added too task pool
	boost::ptr_vector<tqsort_task> tasks;

	// Sort small range using variation of selection sort
	// Based on N. Jesper Larsson & Kunihiko Sadakane: Faster Suffix Sorting
	inline uint32 sort_small(uint32 p, uint32 n)
	__attribute__((always_inline))
	{
		uint32 a = p; // Start of current sorting range and minimum group
		uint32 b = p; // End of minimum group
		uint32 d = p+n-1; // End of sorting range
		uint32 ns = 0; // Count of assigned singleton groups
		uint32 tv; // Comparison element

		while (a < d) {
			// Move minimum group to range a..b-1
			for (uint32 i = b = a+1 , min = isa[ sa[a] + h ] ; i <= d ; ++i) {
				if ((tv = isa[ sa[i] + h ]) < min) {
					min = tv;
					swap(i, a);
					b = a+1;
				}
				else if (tv == min) {
					swap(i, b++);
				}
			}
			// Renumber minimum group 
			assign(a, b-a);
			if (b - a == 1) ++ns;
			a = b;
		}
		// Last element contains a singleton group
		if (a == d) {
			assign(a, 1); ++ns;
		}

		return ns;
	}
	

	// Sort grain size range in this thread, or add new task to sort it later
	// Returns the count of new singleton groups
	inline uint32 sort_switch(uint32 p, size_t n) 
	{
		// Call tqsort in this thread
		if (n < BucketSize) return tqsort_grainsize(p, n);

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
	__attribute__((always_inline))
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
	virtual void doubling_range(uint32, size_t);

public:

	sortpar(const char *, const uint32, const uint32, std::ostream&);
	virtual ~sortpar();
	virtual void build_lcp();

};

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab:
