#include "sortseq.hpp"
#include "tupla.hpp"
#include <stdexcept>

using namespace tupla;

tupla::sortseq::sortseq(const char * text, const uint32 len, std::ostream& err)
	: suffixsort(text, len, err)
{
}

tupla::sortseq::~sortseq()
{
}

void tupla::sortseq::build_lcp()
{
	if (!finished_sa) {
		throw std::runtime_error("suffix array not complete");
	}

	if (finished_lcp) return;

	err << SELF << ": building longest common prefix array via permuted" << std::endl;

	lcp = new uint32[len];
	memset(lcp, 0, (len * sizeof(uint32)) );

	// Use throwaway inverse suffix array table for PLCP
	uint32 * plcp = isa;
	
	// Use allocated LCP temorarily for phi
	uint32 * phi = lcp;

	// Compute phi for q=1
	for (size_t i = 1 ; i < len ; ++i)
		phi[ sa[i] ] = sa[i-1];
	
	// Turn phi into PLCP 
	uint32 l = 0;
	for (size_t i = 0 ; i < len-1 ; ++i) {
		uint32 j = phi[i];
		plcp[i] = (l += lcplen(i+l, j+l));
		l = ( l >= 1 ? l - 1 : 0);
	}

	// Compute irreducible LCP values
	for (size_t i = 1 ; i < len ; ++i) {
		uint32 j = sa[i - 1];
		uint32 k = sa[i];
		// Wrap around t[-1]
		if ( j == 0 || k == 0  || *(text + j - 1) != *(text + k - 1) ) {
			l = lcplen(j, k);
			if (plcp[k] < l) 
				plcp[k] = l;
		}
	}

	// Fill in the other values
	for (size_t i = 1 ; i < len-1 ; ++i) 
		if (plcp[i] + 1 < plcp[i-1])
			plcp[i] = plcp[i-1] - 1;
	
	// Build LCP from PLCP permutation
	for (size_t i = 0 ; i < len ; ++i)
		lcp[i] = plcp[ sa[i] ];

	finished_lcp = true;
}

uint32 tupla::sortseq::init()
{
	uint32 group[Alpha] = { Z256 };
	uint32 count[Alpha] = { Z256 };
	uint8 sorted[Alpha] = { Z256 };
	
	sa = new uint32[len];
	isa = new uint32[len];

	memset(sa, 0, (len * sizeof(uint32)) );
	memset(isa, 0, (len * sizeof(uint32)) );

	// Count character occurences
	count_range(0, len, count, 0);

	// Multiple nulls in input
	if (count[0] != 1) 
		throw std::runtime_error("input contains multiple nulls");

	// Assign prefix sums for each character
	uint32 alphasize = build_prefix(count, count, group, sorted, 0);

	// Counting sort on first character of suffix
	sort_range(0, len, count, group, sorted, 0);

	return alphasize;
}

void tupla::sortseq::doubling()
{
	doubling_range(0, len);
}

void tupla::sortseq::doubling_range(uint32 p, size_t n) 
{
	uint32 sp = 0; // Starting index of sorted group
	uint32 sl = 0; // Sorted groups length following start
	for (size_t i = p ; i < p+n-1 ; ) {
		// Skip sorted group
		if (uint32 s = get_sorted(i)) {
			i += s; sl += s;
			continue;
		} 
		// Combine sorted group before i
		if (sl > 0) {
			set_sorted(sp, sl);
			sl = 0;
		}
		// Sort unsorted group i..g
		uint32 g = isa[ sa[i] ] + 1;
		groups += tqsort(i, g-i);
		sp = i = g;
	}
	// Combine sorted group at end
	if (sl > 0) set_sorted(sp, sl);
}

void tupla::sortseq::invert()
{
	invert_range(0, len);
}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
