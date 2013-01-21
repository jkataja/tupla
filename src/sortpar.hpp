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
#include <boost/signals2/mutex.hpp>
#include <boost/threadpool.hpp>
#include <boost/thread/thread.hpp>

#include "numdefs.hpp"
#include "suffixsort.hpp"
#include "tqsort_task.hpp"

namespace sup {

class sortpar : public suffixsort {

private:

	sortpar(const sortpar&);
	sortpar& operator=(const sortpar&);

	boost::signals2::mutex assign_lock;

	boost::threadpool::pool tp;

	uint32 * isa_assign; // Doubling assignments

	const uint32 jobs;

	// Parallel initialization subroutines
	void tccount(uint32, uint32, uint32 *);
	void tcsort(uint32, uint32, uint32 *, uint32 *);
	void sort_range(uint32 p, size_t n);

	void tqsort(uint32 p, size_t n);
	

	inline void sort_switch(uint32 p, size_t n) 
	{
		// Inline
		if (n < GrainSize) {
			tqsort(p, n);
		}
		// New task for thread
		// TODO assign on same thread
		else {
			boost::shared_ptr<tqsort_task> job(new tqsort_task(this, p, n));
			boost::threadpool::schedule(tp, 
					boost::bind(&tqsort_task::run, job));
		}
	}

	// Renumber group at p..g with matching sorting key as g 
	// Group number is last index with value to keep sort keys decreasing
	inline void assign(uint32 p, size_t n)
	{
		uint32 g = p + n - 1;
		
		if (n == 1) { 
			assign_lock.lock(); ++groups; assign_lock.unlock();
			sorted[p] = 1; 
		}
		
		for (size_t i = p ; i < p+n ; ++i) 
			isa_assign[ sa[i] ] = g;
	}

protected:
	virtual void init();
	virtual void doubling();
	virtual void doubling(uint32, size_t);
	

public:

	sortpar(const char *, const uint32, const uint32, std::ostream&);
	virtual ~sortpar();
	virtual void build_lcp();

};

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab:
