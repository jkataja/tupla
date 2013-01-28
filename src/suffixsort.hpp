/**
 * Doubling suffix sort using ternary split quick sort. 
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

#include "numdefs.hpp"

namespace sup {

class suffixsort {

	friend class tqsort_task;
	friend class doubling_task;

private:
	suffixsort(const suffixsort&);
	suffixsort& operator=(const suffixsort&);

protected:
	uint32 * sa;  // Suffixes sorted in lexicographical h-order
	uint32 * isa; // Sorting h-order of the suffixes
	uint32 * lcp; // LCP built from completed suffix array

	size_t h; // Current suffix doubling distance

	const char * const text; // Input
	const uint32 len; // Length of input
	uint32 groups; // Count of singleton groups

	std::ostream& err; // Error stream

	bool finished_sa;
	bool finished_lcp;

	// Constructor called from instance()
	suffixsort(const char * text, const uint32 len, std::ostream& err);

	// Allocate and initialize suffix array and inverse suffix array
	// Sort first round using counting sort on first character
	virtual uint32 init() = 0;

	// Doubling step
	virtual void doubling() = 0;
	virtual void doubling(uint32, size_t) = 0;

	// Reconstruct suffix array from inverse suffix array
	virtual void invert() = 0;

	// Debugging and testing
	bool out_descending();
	uint32 count_dupes();

	// Sort range using ternary split quick sort
	// Based on Bentley-McIlroy 1993: Engineering a Sort Function
	virtual uint32 tqsort(uint32, size_t);

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
	// Returns value ( ISA_h[ SA_h[pivot] ] , ISA_h[ SA_h[pivot] + h ] ) 
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

	// Comparison key for index p in suffix array
	// Returns pair ( ISA_h[ SA_h[p] ] , ISA_h[ SA_h[p] + h ] ) packed in long
	inline uint64 k(const uint32 p)
	{
		uint32 v = sa[p];

		return (v + h < len 
				? (((uint64)isa[ v ] << 32) | isa[ v + h ] )
				:  ((uint64)isa[ v ] << 32) ); 
	}

	// Swap suffix array elements at indices
	inline void swap(const uint32& a, const uint32& b)
	{
		std::swap( sa[a], sa[b] );
	}

	// Swap n suffix array elements starting from indices a and b
	inline void vecswap(uint32 a, uint32 b, size_t n)
	{
		for ( ; n ; --n) swap(a++, b++);
	}

	// Renumber group at p..g with matching sorting key as g 
	// Group number is last index with value to keep sort keys decreasing
	inline void assign(uint32 p, size_t n)
	{
		uint32 g = p + n - 1;

		for (size_t i = p ; i < p+n ; ++i) 
			isa[ sa[i] ] = g;
		
		if (n == 1) set_sorted(p, 1); // Mark as sorted singleton group

	}

	// First byte set in suffix array sets sorted flag
	inline void set_sorted(uint32 p, uint32 n)
	{
		sa[p] = (0x80000000U ^ n);
		
	}

	// Length of sorted group starting from p
	inline uint32 get_sorted(uint32 p)
	{
		uint32 v = sa[p];
		return ((v >> 31) ? (0x7FFFFFFFU & v) : 0); 
		
	}

public:

	static suffixsort * instance(const char *, const uint32, const uint32, 
			std::ostream&);
	~suffixsort();

	// Build suffix array
	virtual void build_sa();

	// Compute longest common prefix data from completed suffix array
	virtual void build_lcp() = 0;

	// Output generated SA
	virtual void out_sa();
	
	// Output generated LCP
	virtual void out_lcp();

	// Compare strings on all following suffixes
	virtual bool out_validate();

	// Compare all LCP matching to text
	virtual bool out_incorrect_lcp();

	// Access the class internal SA
	virtual const uint32 * const get_sa();

	// Access the class internal ISA
	virtual const uint32 * const get_isa();

	// Access the class internal LCP array
	virtual const uint32 * const get_lcp();

};

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab:
