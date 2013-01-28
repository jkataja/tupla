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

	lcp_range(0, len);

	finished_lcp = true;
}

uint32 sup::sortseq::init()
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

void sup::sortseq::doubling(uint32 p, size_t n) {
}

void sup::sortseq::invert()
{
	invert_range(0, len);
}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
