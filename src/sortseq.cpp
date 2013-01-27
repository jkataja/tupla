#include "sortseq.hpp"
#include "sup.hpp"

#include <stdexcept>

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

	err << SELF << ": building longest common prefix array" << std::endl;
	
	lcp = new uint32[len];
	memset(lcp, 0, (len * sizeof(uint32)) );

	// SPA lecture 10
	uint32 l = 0;
	for (size_t i = 0 ; i < len - 1 ; ++i) {
		uint32 k = isa[i]; 
		uint32 j = sa[k - 1]; 
		while (*(text + i + l) == *(text + j + l)) ++l;
		lcp[k] = l;
		if (l > 0) --l;
	}

	finished_lcp = true;
}

uint32 sup::sortseq::init()
{
	uint32 alphasize = 0;
	uint32 group[Alpha] = { Z256 };
	uint32 count[Alpha] = { Z256 };
	uint8 sorted[Alpha] = { Z256 };
	
	sa = new uint32[len];
	isa = new uint32[len];

	memset(sa, 0, (len * sizeof(uint32)) );
	memset(isa, 0, (len * sizeof(uint32)) );

	// Count character occurences
	for (size_t i = 0 ; i < len ; ++i) 
		++count[ (uint8)*(text + i) ];

	// Multiple nulls in input
	if (count[0] != 1) 
		throw std::runtime_error("input contains multiple nulls");

	// Assign prefix sums for each character
	uint32 f = 0; // First index in group
	for (size_t i = 0 ; i < Alpha ; ++i) {
		uint32 n = count[i]; 
		uint32 g = f + n - 1; // Last position in group f..g
		count[i] = f; // Prefix starts from f
		group[i] = g; // Group sorting key is last index in f..g
		groups += sorted[i] = (n == 1); // Singleton group is sorted
		f += n;
		alphasize += (n > 0);
	}
	// Assign initial sorting groups and build prefix sums
	for (size_t i = 0 ; i < len ; ++i) {
		uint8 c = (uint8)*(text + i);
		uint32 j = count[ c ]++;
		// Initialize suffix array with counting sort 
		sa[j] = i;
		// Initialize inverse suffix array with group sorting key
		isa[i] = group[c];
		// Mark singleton group as sorted
		if (sorted[c]) set_sorted(j, 1);
	}
	return alphasize;
}

void sup::sortseq::doubling()
{
	uint32 p = 0; // Starting index of sorted group
	uint32 sl = 0; // Sorted groups length following start
	for (size_t i = 0 ; i < len ; ) {
		// Skip sorted group
		if (uint32 s = get_sorted(i)) {
			i += s; sl += s;
			continue;
		} 
		// Combine sorted group before i
		if (sl > 0) {
			set_sorted(p, sl);
			sl = 0;
		}
		// Sort unsorted group i..g
		uint32 g = isa[ sa[i] ] + 1;
		groups += tqsort(i, g-i);
		p = i = g;
	}
	// Combine sorted group at end
	if (sl > 0) set_sorted(p, sl);
}

void sup::sortseq::doubling(uint32 p, size_t n)
{
	throw std::runtime_error("not implemented");
}

void sup::sortseq::invert()
{
	for (size_t i = 0 ; i<len ; ++i)
		sa[ isa[i] ] = i;
}


// vim:set ts=4 sts=4 sw=4 noexpandtab:
