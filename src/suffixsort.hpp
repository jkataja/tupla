/**
 * Doubling suffix sort. 
 *
 * @author jkataja
 */

#pragma once

#include <iostream>
#include <boost/cstdint.hpp>

#include "numdefs.hpp"

namespace sup {

class suffixsort {

private:
	suffixsort(const suffixsort&);
	suffixsort& operator=(const suffixsort&);

protected:
	uint32 * sa;  // Suffixes sorted in h-order
	uint32 * isa; // Sorting keys for suffix starting from index
	uint32 * lcp; // LCP array built from completed ISA
	uint32 * sorted; // Length of sorted group starting from index

	uint64 h; // Current suffix doubling distance

	const char * const text; // Input
	const uint32 len; // Length of input
	uint32 groups; // Singleton groups

	std::ostream& err; // Error stream

	bool finished_sa;
	bool finished_lcp;

	// Constructor called from instance()
	suffixsort(const char * text, const uint32 len, std::ostream& err);

	// Allocate and initialize suffix array and inverse suffix array
	// Sort first round using counting sort on first character
	virtual void init() = 0;

	// Doubling step
	virtual void doubling() = 0;

	// Debugging and testing
	void out_sa(uint32, size_t);
	bool out_incorrect_order();
	uint32 count_dupes();
	bool is_xvalid();

	// Comparison key for index p in suffix array
	// Contains doubling pair ( ISA_h[p] , ISA_h[p+h] ) packed in long
	inline uint64 k(const uint32 p)
	{
		uint32 sap = sa[p];
		return (sap + h < len 
				? (((uint64)isa[ sap ] << 32) | isa[ sap + h ] )
				:  ((uint64)isa[ sap ] << 32) ); 
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

	// Renumber group at p..g with matching sorting key as g 
	// Group number is last index with value to keep sort keys decreasing
	inline void assign(uint32 p, size_t n)
	{
		uint32 g = p + n - 1;
		if (n == 1) { ++groups; sorted[p] = 1; }
		for (size_t i = p ; i < p+n ; ++i) 
			isa[ sa[i] ] = g;
	}

public:

	static suffixsort * instance(const char *, const uint32, const uint32, 
			std::ostream&);
	~suffixsort();

	// Run suffix sort
	virtual void run();

	// Compute longest common prefix data from completed suffix array
	virtual void build_lcp() = 0;

	// Output generated SA
	virtual void out_sa();

	// Run cross-validation on SA vs ISA and strcmp all following suffixes
	virtual bool out_validate();

	// Access the class internal SA
	virtual const uint32 * const get_sa();

	// Access the class internal LCP array
	virtual const uint32 * const get_lcp();

};

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab:
