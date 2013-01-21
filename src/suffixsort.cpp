#include "suffixsort.hpp"
#include "sup.hpp"
#include "sortseq.hpp"
#include "sortpar.hpp"

#include <stdexcept>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <boost/thread/thread.hpp>

using namespace sup;

suffixsort::suffixsort(const char * text, const uint32 len, std::ostream& err)
	: sa(0), isa(0), lcp(0), sorted(0), h(0), text(text), len(len), groups(0),
	  err(err), finished_sa(false), finished_lcp(false)
{
}

suffixsort::~suffixsort()
{
	delete [] sa;
	delete [] isa;
	delete [] lcp;
	delete [] sorted;
}

suffixsort * sup::suffixsort::instance(const char * text, 
		const uint32 len, const uint32 jobs, std::ostream& err)
{
	if (jobs > 1) {
		err << SELF << ": using parallel algorithm with " << jobs 
				<< " jobs" << std::endl;
		return new sortpar(text, len, jobs, err);
	}
	else {
		err << SELF << ": using sequential algorithm" << std::endl;
		return new sortseq(text, len, err);
	}
}

void sup::suffixsort::run()
{
	// Allocate and initialize with counting sort
	init();

	// Doubling steps until number of sorting groups matches length
	for (h = 1 ; (groups < len && h < len) ; h <<= 1) {
		doubling();
		std::cerr << SELF << ": doubling " << ffsl(h) 
				<< " with " << groups << " singleton groups ("
				<< std::fixed << std::setprecision(1) 
				<< ((groups/(double)(len)) * 100) 
				<< "% complete)" << std::endl;
	}

	if (groups != len) {
		throw std::runtime_error("could not find singleton groups for all suffixes");
	}

	finished_sa = true;
}


void sup::suffixsort::tqsort(uint32 p, size_t n)
{
	uint32 a,b,c,d;
	uint32 pn = p + n;

	// Sort small tables with bingo sort 
	// Supposedly more efficient than selection sort with duplicate values
	// Adapted from:
	// @see http://en.wikipedia.org/wiki/Selection_sort#Variants
	if (n < 7) {
		a = pn-1;
		uint32 eqn = 0;

		// Find the highest value
		uint64 v = k(a);
		for (uint32 i = a ; i >= p ; --i)
			if (k(i) > v) v = k(i);
		while ((a > p) && (k(a) == v)) { --a; ++eqn; }

		// Move every item with highest values to end of list
		// One pass for each value in list
		while (a > p) {
			uint64 f = v;
			v = k(a);
			for (uint32 i = a - 1 ; i >= p ; --i) {
				uint64 ki = k(i);
				if (ki == f) { swap(i, a--); ++eqn; }
				else if (ki > v) v = ki;
			}
			// Assign items sharing highest value to new group
			assign(a + 1, eqn);
			eqn = 0;
			while ((a > p) && (k(a) == v)) { --a; ++eqn; }
		}
		// First index also contained highest value
		if (k(p) == v) {
			assign(p, eqn + 1);
		}
		else {
			assign(a + 1, eqn);
			assign(p, 1);
		}
		return;
	}
	
	const uint64 v = choose_pivot(p, n);

	// Partition
	a = b = p;
	c = d = p + (n-1);

	// Assume on range i=f..g value ISA_h[ SA_h[i] ] is equal
	// Only use the doubling part  ISA_h[ SA_h[i] + h ] as comparison key
	uint32 sv = (v & 0xFFFFFFFF);
	uint32 tv;
	for (;;) {
		while (b <= c && (tv = isa[ sa[b] + h ]) <= sv) {
			if (tv == sv) swap(a++, b); 
			++b;
		}
		while (c >= b && (tv = isa[ sa[c] + h ]) >= sv) {
			if (tv == sv) swap(c, d--);
			--c;
		}
		if (b > c) break;
		swap(b++, c--);
	}

	// Move split-end group to middle
	const uint32 s = std::min(a-p, b-a ); vecswap(p, b-s, s);
	const uint32 t = std::min(d-c, pn-1-d); vecswap(b, pn-t, t);

	const uint32 ltn = b-a;
	const uint32 gtn = d-c;
	const uint32 eqn = n - ltn - gtn;

	if (ltn > 0) tqsort(p, ltn);
	assign(p+ltn, eqn); 
	if (gtn > 0) tqsort(pn-gtn, gtn);
}

const uint32 * const sup::suffixsort::get_sa()
{
	return (finished_sa ? sa : 0);
}

const uint32 * const sup::suffixsort::get_lcp()
{
	return (finished_lcp ? lcp : 0);
}

bool sup::suffixsort::is_xvalid()
{
	for (size_t i = 0 ; i<len ; ++i)
		if (isa[ sa[i] ] != i) return false;
	return true;
}

bool sup::suffixsort::out_descending()
{
	uint32 descending = 0;
	for (size_t i = 1 ; i<len ; ++i) {
		if (strcmp((text+sa[i]), (text + sa[i-1])) < 0) {
			std::string a( (text + sa[i]) );
			std::string b( (text + sa[i-1]) );
			a.append("$"); b.append("$");
			if (a.length() > 34) a.erase(34);
			if (b.length() > 34) b.erase(34);
			std::replace( a.begin(), a.end(), '\n', '#');
			std::replace( a.begin(), a.end(), '\t', '#');
			std::replace( b.begin(), b.end(), '\n', '#');
			std::replace( b.begin(), b.end(), '\t', '#');
			err << SELF << ": line " << (i-1) << ": "<< a << std::endl;
			err << SELF << ": line " << (i) << ": "<< b << std::endl;
			++descending;
		}
	}
	err << SELF << ": found " << descending << " in descending order" << std::endl;
	return (descending > 0);
}

uint32 sup::suffixsort::count_dupes()
{
	uint32 * match = new uint32[len]();
	uint32 dupes  = 0;
	for (size_t i = 0 ; i<len ; ++i) {
		dupes += ( ++match[ sa[i] ] > 1 );
	}
	delete [] match;
	return dupes;
}

void sup::suffixsort::out_sa(uint32 p, size_t n)
{
	err << "i\tsa[i]\torder            suffix" << std::endl;
	for (size_t i = p ; i<p+n ; ++i)  {
		std::string str( (text + sa[i]) );
		str.append("$");
		if (str.length() > 36) str.erase(36);
		std::replace( str.begin(), str.end(), '\n', '#');
		std::replace( str.begin(), str.end(), '\t', '#');
		err << std::hex << i << "\t"  << sa[i] << "\t"  
			<< std::setw(16) << std::setfill('0') << k(i) << std::dec;
		err << " '" << str << "'" << std::endl;
	}
}

void sup::suffixsort::out_sa()
{
	err << "i\tsa[i]\tsorted\torder            suffix" << std::endl;
	for (size_t i = 0 ; i<len ; ++i)  {
		std::string str( (text + sa[i]) );
		str.append("$");
		if (str.length() > 36) str.erase(36);
		std::replace( str.begin(), str.end(), '\n', '#');
		std::replace( str.begin(), str.end(), '\t', '#');
		err << std::hex << i << "\t"  << sa[i] << "\t"  
			<< std::dec << sorted[i] << "\t" 
			<< std::hex << std::setw(16) << std::setfill('0') << k(i) 
			<< std::dec << " '" << str << "'" << std::endl;
	}
}

bool sup::suffixsort::out_validate()
{
	if (!finished_sa) {
		err << SELF << ": suffix array not complete" << std::endl;
		return false;
	}

	uint32 dupes = count_dupes();
	err << SELF << ": found " << dupes << " duplicates in ISA" << std::endl;

	bool eq = is_xvalid();
	if (!eq) err << SELF << ": final SA and ISA __DO_NOT__ match" << std::endl;
	else err << SELF << ": final SA and ISA match" << std::endl;

	err << SELF << ": checking if ISA is in ascending order" << std::endl;
	out_descending();

	if (finished_lcp) {
		// TODO LCP vs strncmp
	}

	return (dupes == 0 && eq);
}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
