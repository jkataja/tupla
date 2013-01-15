/**
 * Doubling suffix sort. 
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

#include "numdefs.hpp"

namespace sup {

class suffixsort {

	uint32 * sa;  // Suffixes sorted in h-order
	uint32 * isa; // Sorting keys for suffix starting from index
	uint32 * lcp; // LCP array built from completed ISA
	uint32 * sorted; // Length of sorted group starting from index

	uint64 h; // Current suffix doubling distance

	const char * const text; // Input
	const uint32 len; // Length of input

	std::ostream& err; // Error stream

	bool finished_sa;
	bool finished_lcp;

	suffixsort(const suffixsort&);
	suffixsort& operator=(const suffixsort&);

	// Debugging and testing
	void out_sa(uint32, size_t);
	bool out_incorrect_order();
	uint32 count_dupes();
	bool is_xvalid();
	
	// Allocate and initialize suffix array and inverse suffix array
	// Sort first round using counting sort on first character
	void init();

	// Sort range using ternary split quick sort
	// Based on Bentley-McIlroy 1993: Engineering a Sort Function
	void tqsort(uint32, size_t);

	// Comparison key for index p in suffix array
	// Contains doubling pair ( ISA_h[p] , ISA_h[p+h] ) packed in long
	inline uint64 k(const uint32 p)
	{
		uint32 sap = sa[p];
		return (sap + h < len 
				? (((uint64)isa[ sap ] << 32) | isa[ sap + h ] )
				:  ((uint64)isa[ sap ] << 32) ); 
	}

	// Determine median value of three suffix array elements
	// Returns index to position where median was found
	// Based on Bentley-McIlroy 1993: Engineering a Sort Function
	inline uint32 med3(const uint32 a, const uint32 b, const uint32 c)
	{
		const uint64 ka = k(a);
		const uint64 kb = k(b);
		const uint64 kc = k(c);
		// abc acb cab
		// cba bac bca
		return (ka < kb ? (kb < kc ? b : (ka < kc ? c : a) )
		                : (kb > kc ? b : (ka < kc ? a : c) ) ); 
	}

	// Choose pivot value from n elements starting at p using pseudomedian
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
	inline void swap(const uint32& a, const uint32& b)
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
		else tqsort(p, n);
	}

	// Renumber group at p..g with matching sorting key as g 
	// Group number is last index with value to keep sort keys decreasing
	inline void assign(uint32 p, size_t n)
	{
		uint32 g = p + n - 1; 
		if (n == 1) sorted[p] = 1;
		for (size_t i = p ; i < p+n ; ++i) 
			isa[ sa[i] ] = g;
	}

public:

	suffixsort(const char * text, const uint32 len, std::ostream& err)
			: sa(0), isa(0), lcp(0), sorted(0), h(0),
			  text(text), len(len), err(err),
			  finished_sa(false), finished_lcp(false) { }

	~suffixsort() {
		delete [] sa;
		delete [] isa;
		delete [] lcp;
		delete [] sorted;
	}

	// Sequential suffix sort
	void run_sequential();

	// Parallel suffix sort
	void run_parallel(uint32);

	// Compute longest common prefix data from completed suffix array
	void build_lcp();

	// Output generated SA
	void out_sa();

	// Run cross-validation on SA vs ISA and strcmp all following suffixes
	bool out_validate();


	// Access the class internal SA
	const uint32 * const get_sa();

	// Access the class internal LCP array
	const uint32 * const get_lcp();

};

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab:
