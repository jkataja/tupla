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
 * Implements LCP array construction as described in:
 * J. Kärkkäinen, G. Manzini & S.J. Puglisi 2009 
 * Permuted Longest-Common-Prefix Array
 *
 * @author jkataja
 */

#pragma once

#include <iostream>
#include <boost/cstdint.hpp>
#include <boost/thread/mutex.hpp>
#ifdef __SSE4_2__
#include <nmmintrin.h>
#endif

#include "numdefs.hpp"

// Flags for _mm_cmpistri intrisic in SSE4.2 optimized lcplen:
// Unsigned bytes source
// Equal each compare: match a[i] with b[i]
// Negative polarity gives matching elements
//
// Intel SSE4 Programming Reference
// @see http://software.intel.com/sites/default/files/m/0/3/c/d/4/18187-d9156103.pdf 
#ifdef __SSE4_2__
#define LCPLEN_FLAGS (_SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_NEGATIVE_POLARITY )
#endif

namespace tupla {

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
	virtual void doubling_range(uint32, size_t) = 0;

	// Longest common prefix for range
	void lcp_range(uint32 p, uint32 n);

	// Character count for range
	void count_range(uint32, uint32, uint32 *, uint32);

	// Build prefix sums for counting sort
	const uint32 build_prefix(uint32 *, uint32 *, uint32 *, uint8 *, uint32);

	// Counting sort for range
	void sort_range(uint32, uint32, uint32 *, uint32 *, uint8 *, uint32);

	// Reconstruct suffix array from inverse suffix array
	void invert_range(uint32, uint32);

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
	 __attribute__((always_inline))
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
		uint32 a = p;
		uint32 b = p + (n/2);
		uint32 c = p + n - 1;
		if (n > 40) { // Big arrays, pseudomedian of 9
			uint32 s = (n/8);
			a = med3( a, a+s, a+2*s );
			b = med3( b-s, b, b+s );
			c = med3( c-2*s, c-s, c );
		}
		b = med3( a, b, c ); // Mid-size, med of 3

		return k(b);
	}

	// Comparison key for index p in suffix array
	// Returns pair ( ISA_h[ SA_h[p] ] , ISA_h[ SA_h[p] + h ] ) packed in long
	inline uint64 k(const uint32 p) 
	__attribute__((always_inline))
	{
		uint32 v = sa[p];

		return (v + h < len 
				? (((uint64)isa[ v ] << 32) | isa[ v + h ] )
				:  ((uint64)isa[ v ] << 32) ); 
	}

	// Find longest common prefix of positions a and b in text.
	// SSE4.2 version uses _mm_cmpistri intrisic to compare 16 characters 
	// at a time, matching also nulls (if a==b until segfault)
	inline uint32 lcplen(const uint32 a, const uint32 b)
	__attribute__((always_inline))
	{
		const char * pa = (text + a);
		const char * pb = (text + b);
#ifdef __SSE4_2__
		uint32 l = 1;
		uint32 n;
		if (*pa++ != *pb++) return 0;
		__m128i xa, xb;
		do {
			xa = _mm_loadu_si128((__m128i *)pa);
			xb = _mm_loadu_si128((__m128i *)pb);
		} while ( ((n = _mm_cmpistri(xa, xb, LCPLEN_FLAGS)) == 16) 
				&& (l += n) && (pa += 16) && (pb += 16) );
		return l + n;
#else
		uint32 l = 0;
		while (*pa++ == *pb++) ++l;
		return l;
#endif
	}

	// Swap suffix array elements at indices
	inline void swap(const uint32& a, const uint32& b)
	__attribute__((always_inline))
	{
		std::swap( sa[a], sa[b] );
	}

	// Swap n suffix array elements starting from indices a and b
	inline void vecswap(uint32 a, uint32 b, size_t n)
	__attribute__((always_inline))
	{
		for ( ; n ; --n) swap(a++, b++);
	}

	// Renumber group at p..g with matching sorting key as g 
	// Group number is last index with value to keep sort keys decreasing
	inline void assign(uint32 p, size_t n)
	__attribute__((always_inline))
	{
		uint32 g = p + n - 1;

		for (size_t i = p ; i < p+n ; ++i) 
			isa[ sa[i] ] = g;
		
		if (n == 1) set_sorted(p, 1); // Mark as sorted singleton group

	}

	// First byte set in suffix array sets sorted flag
	inline void set_sorted(uint32 p, uint32 n)
	__attribute__((always_inline))
	{
		sa[p] = (0x80000000U ^ n);
		
	}

	// Length of sorted group starting from p
	inline uint32 get_sorted(uint32 p)
	__attribute__((always_inline))
	{
		uint32 v = sa[p];
		return ((v >> 31) ? (0x7FFFFFFFU & v) : 0); 
		
	}

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
	

public:

	static suffixsort * instance(const char *, const uint32, const uint32, 
			std::ostream&);
	~suffixsort();

	// Build Suffix Array
	virtual void build_sa();

	// Compute Longest Common Prefix array
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

	// Access the class internal LCP array
	virtual const uint32 * const get_lcp();

};

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab:
