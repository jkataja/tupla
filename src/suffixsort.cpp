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
	: sa(0), isa(0), lcp(0), sorted(0), h(0), text(text), len(len), err(err), 
	  finished_sa(false), finished_lcp(false)
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
	uint32 groups = 0;
	for (h = 1 ; (groups < len && h < len) ; h <<= 1) {
		groups = doubling();
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

const uint32 * const sup::suffixsort::get_sa()
{
	return (finished_sa ? sa : 0);
}

const uint32 * const sup::suffixsort::get_lcp()
{
	return (finished_lcp ? lcp : 0);
}

void sup::suffixsort::tqsort(uint32 p, size_t n)
{
/*
	// Insertion sort on smallest arrays
	if (n < 7) {
		for (uint32 i = p + 1 ; i < p + n ; ++i)
			for (uint32 j = i ; j > p && k(j-1) > k(j) ; --j)
				swap(j, j-1);
		// TODO assign
		return;
	}
*/
	
	const uint64 v = choose_pivot(p, n);

	// Partition
	uint32 a,b,c,d;
	a = b = p;
	c = d = p + (n-1);
	for (;;) {
		while (b <= c && k(b) <= v) {
			if (k(b) == v) swap(a++, b); 
			++b;
		}
		while (c >= b && k(c) >= v) {
			if (k(c) == v) swap(c, d--);
			--c;
		}
		if (b > c) break;
		swap(b++, c--);
	}

	// Move split-end group to middle
	uint32 pn = p + n;
	const uint32 s = std::min(a-p, b-a ); vecswap(p, b-s, s);
	const uint32 t = std::min(d-c, pn-1-d); vecswap(b, pn-t, t);

	const uint32 ltn = b-a;
	const uint32 gtn = d-c;
	const uint32 eqn = n - ltn - gtn;

	if (ltn > 0) sort(p, ltn);
	assign(p+ltn, eqn); 
	if (gtn > 0) sort(pn-gtn, gtn);
}

bool sup::suffixsort::is_xvalid()
{
	for (size_t i = 0 ; i<len ; ++i)
		if (isa[ sa[i] ] != i) return false;
	return true;
}

bool sup::suffixsort::out_incorrect_order()
{
	uint32 wrongorder = 0;
	for (size_t i = 1 ; i<len ; ++i) {
		std::string a( (text + sa[i]) );
		std::string b( (text + sa[i-1]) );
		if (a < b) {
			err << (i-1) << ": "<< a << std::endl;
			err << (i) << ": "<< b << std::endl << std::endl;
			++wrongorder;
		}
	}
	err << SELF << ": incorrect orders " << wrongorder << std::endl;
	return (wrongorder > 0);
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
	const uint64 v = choose_pivot(p, n);
	err << "i\tsa[i]\torder            suffix" << std::endl;
	for (size_t i = p ; i<p+n ; ++i)  {
		std::string str( (text + sa[i]) );
		str.append("$");
		if (str.length() > 34) str.erase(34);
		std::replace( str.begin(), str.end(), '\n', '#');
		std::replace( str.begin(), str.end(), '\t', '#');
		err << std::hex << i << "\t"  << sa[i] << "\t"  
			<< std::setw(16) << std::setfill('0') << k(i) << std::dec;
		if (k(i) < v) err << " <";
		if (k(i) == v) err << " =";
		if (k(i) > v) err << " >";
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
	err << SELF << ": ISA duplicates " << dupes << std::endl;

	bool eq = is_xvalid();
	if (!eq) err << SELF << ": SA and ISA are not equal" << std::endl;

	err << SELF << ": comparing neighbouring ISA index positions" << std::endl;
	out_incorrect_order();

	if (finished_lcp) {
		// TODO LCP vs strncmp
	}

	return (dupes == 0 && eq);
}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
