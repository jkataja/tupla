#include "sortseq.hpp"
#include "sup.hpp"

#include <stdexcept>
#include <iomanip>
#include <algorithm>
#include <cstring>

using namespace sup;

sup::sortseq::sortseq(const char * text, const uint32 len, std::ostream& err)
	: suffixsort(text, len, err)
{
}

sup::sortseq::~sortseq()
{
}

void sup::sortseq::build_lcp()
{
	if (!finished_sa) {
		throw std::runtime_error("suffix array not complete");
	}
	
	lcp = new uint32[len];
	memset(lcp, 0, (len * sizeof(uint32)) );

	// XXX spa lecture10 luentokalvoista
	/*
	uint32 l = 0;
	for (size_t i = 0 ; i < len - 1 ; ++i) {
		uint32 j = sa[k - 1]; // XXX
		while (*(text + i + l) == *(text + j + l)) // XXX
			++l;
		lcp[ isa[i] ] = l;
		if (l > 0) --l;
	}
	finished_lcp = true;
	*/

}

void sup::sortseq::init()
{
	uint32 group[Alpha] = { Z256 };
	uint32 count[Alpha] = { Z256 };
	
	sa = new uint32[len];
	isa = new uint32[len];
	sorted = new uint32[len];

	memset(sa, 0, (len * sizeof(uint32)) );
	memset(isa, 0, (len * sizeof(uint32)) );
	memset(sorted, 0, (len * sizeof(uint32)) );

	// Count character occurences
	for (size_t i = 0 ; i < len ; ++i) 
		++count[ (uint8)*(text + i) ];

	// Multiple nulls in input
	if (count[0] != 1) 
		throw std::runtime_error("input contains multiple nulls");

	// Starting index of each sorting group f..g
	uint32 f = 0; // First index in group
	for (size_t i = 0 ; i < Alpha ; ++i) {
		uint32 n = count[i]; 
		uint32 g = f + n - 1; // Last position in group f..g
		count[i] = f; // Starting counting sort from f
		group[i] = g; // Sorting group key 
		groups += sorted[f] = (n == 1); // Singleton group is sorted
		f += n;
	}
	uint32 p = 0; // Starting index of sorted group
	uint32 sl = 0; // Length of sorted groups following p
	for (size_t i = 0 ; i < len ; ++i) {
		// Counting sort f..g
		sa[ count[  (uint8)*(text + i) ]++ ] = i;
		// Sorting key for range f..g is g
		isa[i] = group[ (uint8)*(text + i) ];
		// Increase sorted group length
		if (uint32 s = sorted[i]) {
			sl += s; 
			continue;
		} 
		// Combine sorted group before i
		if (sl > 0) {
			sorted[p] = sl; 
			sl = 0;
		}
		p = i + 1;
	}
}

void sup::sortseq::doubling()
{
	uint32 p = 0; // Starting index of sorted group
	uint32 sl = 0; // Sorted groups length following start
	for (size_t i = 0 ; i < len ; ) {
		// Skip sorted group
		if (uint32 s = sorted[i]) {
			i += s; sl += s;
			continue;
		} 
		// Combine sorted group before i
		if (sl > 0) {
			sorted[p] = sl; 
			sl = 0;
		}
		// Sort unsorted group i..g
		uint32 g = isa[ sa[i] ] + 1;
		sort(i, g-i);
		p = i = g;
	}
	// Combine sorted group at end
	if (sl > 0) sorted[p] = sl;
}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
