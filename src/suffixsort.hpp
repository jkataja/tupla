/**
 * Doubling suffix sort. 
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

#include "numdefs.hpp"

namespace sup {

class suffixsort {

	uint32 * sa;  // Suffixes sorted in h-order
	uint32 * isa; // Sorting groups for suffix starting from index
	uint32 * lcp; // LCP array computed from finished ISA
	uint32 * elem; // Membership counts for sorting groups

	uint64 h; // Current suffix doubling distance 
	uint32 groups; // Count of groups with order assigned 

	const char * const text; // Input
	const uint32 len; // Length of input

	bool finished_sa;
	bool finished_lcp;

	suffixsort(const suffixsort&);
	suffixsort& operator=(const suffixsort&);

	// Debugging
	void out_sa(std::ostream&);
	void out_sa(uint32, size_t, std::ostream&);
	void out_isa(std::ostream&);
	bool out_incorrect_order(std::ostream&);
	uint32 has_dupes();
	
	// Allocate and initialize suffix array and inverse suffix array
	// Sort first round using counting sort on first character
	void init();

	// Sort range using ternary split quick sort
	// Based on Bentley-McIlroy 1993: Engineering a Sort Function
	void tqsort(uint32, size_t);

	// Comparison key for sorting groups
	// Contains doubling pair ( ISA_h[i] , ISA_h[i+h] ) packed in long
	inline uint64 k(const uint32 p)
	{
		return (sa[p] + h < len 
				? (((uint64)isa[ sa[p] ] << 32) | isa[ sa[p] + h ] )
				:  ((uint64)isa[ sa[p] ] << 32) ); 
	}

	// Determine median value of 3 ISA indexes
	// Returns index to position where median was found
	// Based on Bentley-McIlroy 1993: Engineering a Sort Function
	inline uint32 med3(const uint32 a, const uint32 b, const uint32 c)
	{
		// abc acb cab
		// cba bac bca
		return (k(a) < k(b) ? (k(b) < k(c) ? b : (k(a) < k(c) ? c : a) )
		                    : (k(b) > k(c) ? b : (k(a) < k(c) ? a : c) ) ); 
	}

	// Choose pivot value starting from p using pseudomedian
	// Returns pair ( ISA_h[i] , ISA_h[i+h] ) packed in long
	// Based on Bentley-McIlroy 1993: Engineering a Sort Function
	inline uint64 choose_pivot(uint32 p, size_t n)
	{
		uint32 b = p + (n/2); // Small arrays, middle element
		if (n > 7) {
			uint32 a = p;
			uint32 c = p + n - 1;
			if (n > 40) { // Big arrays, pseudomedian of 9
				uint32 s = (n/8);
				a = med3( a, a+s, a+2*s );
				b = med3( b-s, b, b+s );
				c = med3( c-2*s, c-s, c );
			}
			b = med3( a, b, c ); // Mid-size, med of 3
		}
		return k(b);
	}

	// Swap suffix array elements at indices
	// TODO try pre-assigned tmp variable
	inline void swap(const uint32 a, const uint32 b)
	{
		std::swap( sa[a], sa[b] );
	}

	// Swap n suffix array elements starting from indices a and b
	inline void vecswap(uint32 a, uint32 b, size_t n)
	{
		for ( ; n ; --n) swap(a++, b++);
	}

	// Sort sorting group starting from p
	// Reduce function calls to tqsort
	// TODO inline insertion sort for small arrays
	inline void sort(uint32 p, size_t n)
	{
		if (n == 0) return;

		if (n == 1 && isa[ sa[p] ] != p) assign(p, 1);
		else tqsort(p, n);
	}

	// Assign order for sorting group starting from p
	inline void assign(uint32 p, size_t n)
	{
		if (n == 1) ++groups;

		for (size_t i = p ; i < p+n ; ++i) isa[ sa[i] ] = p;
	}

public:

	suffixsort(const char * text, const uint32 len) 
			: sa(0), isa(0), lcp(0), elem(0), 
			  h(0), groups(0), text(text), len(len), 
			  finished_sa(false), finished_lcp(false) { }

	~suffixsort() { delete [] sa; delete [] isa; delete [] lcp; }

	// Sequential suffix sort
	void run_sequential();

	// Parallel suffix sort
	void run_parallel(uint32);

	// Compute longest common prefix data from completed suffix array
	void build_lcp();

	// Access the class internal SA
	const uint32 * const get_sa();

	// Access the class internal LCP array
	const uint32 * const get_lcp();

};

} // namespace
