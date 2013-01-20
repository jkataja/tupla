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

bool sup::suffixsort::out_incorrect_order()
{
	uint32 wrongorder = 0;
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
			++wrongorder;
		}
	}
	err << SELF << ": found " << wrongorder << " pairs of suffixes in incorrect lexicographical order" << std::endl;
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

	err << SELF << ": strcmp'ing neighboring suffixes in ISA" << std::endl;
	out_incorrect_order();

	if (finished_lcp) {
		// TODO LCP vs strncmp
	}

	return (dupes == 0 && eq);
}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
