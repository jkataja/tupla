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
	: sa(0), isa(0), lcp(0), h(0), text(text), len(len), groups(0),
	  err(err), finished_sa(false), finished_lcp(false)
{
}

suffixsort::~suffixsort()
{
	delete [] sa;
	delete [] isa;
	delete [] lcp;
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

void sup::suffixsort::build_sa()
{
	// Allocate and initialize with counting sort
	uint32 alphasize = init();
	err << SELF << ": alphabet size " << alphasize << std::endl;

	// Doubling steps until number of sorting groups matches length
	uint32 precision = 1;
	for (h = 1 ; (groups < len && h < len) ; h <<= 1) {
		doubling();

		double done = groups/(double)len;
		if (groups == len) precision = 1;
		else if (done >= 0.9995) ++precision;

		err << SELF << ": doubling " << ffsl(h) 
				<< " with " << groups << " singleton groups ("
				<< std::fixed << std::setprecision(precision) << (done * 100) 
				<< "% complete)" << std::endl;
	}

	if (groups != len) {
		throw std::runtime_error("could not find singleton groups for all suffixes");
	}

	// Invert inverse suffix array
	err << SELF << ": inverting inverse suffix array" << std::endl;
	invert();

	finished_sa = true;
}


uint32 sup::suffixsort::tqsort(uint32 p, size_t n)
{
	uint32 a,b,c,d;
	uint32 pn = p + n;

	// Sort small tables with bingo sort 
	// Supposedly more efficient than selection sort with duplicate values
	// Adapted from:
	// @see http://en.wikipedia.org/wiki/Selection_sort#Variants
	if (n < 7) {
		a = pn-1;
		uint32 eqn = 0; // Count of equal items
		uint32 ns = 0; // Count of assigned singleton groups

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
			assign(a + 1, eqn); ns += (eqn == 1);
			eqn = 0;
			while ((a > p) && (k(a) == v)) { --a; ++eqn; }
		}
		// First index also contained highest value
		if (k(p) == v) {
			assign(p, eqn + 1); ns += (eqn == 0);
		}
		else {
			assign(a + 1, eqn); ns += (eqn == 1);
			assign(p, 1);  ++ns;
		}
		return ns;
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

	// New singleton groups in ranges less than and greater than pivot
	uint32 lts = 0;
	uint32 gts = 0;

	if (ltn > 0) lts = tqsort(p, ltn);
	assign(p+ltn, eqn); 
	if (gtn > 0) gts = tqsort(pn-gtn, gtn);

	return (lts + (eqn == 1) + gts);
}

void sup::suffixsort::lcp_range(uint32 p, uint32 n)
{
	uint32 l = 0;
	for (size_t i = p ; i < p+n-1 ; ++i) {
		uint32 k = isa[i];
		uint32 j = sa[k - 1];
		while (*(text + i + l) == *(text + j + l)) ++l;
		lcp[k] = l;
		if (l > 0) --l;
	}
}

void sup::suffixsort::count_range(uint32 p, uint32 n, uint32 * range_count, 
		uint32 j)
{
	uint32 * task_count = (range_count + (j * Alpha));
	for (size_t i = p ; i < p+n ; ++i) 
		++task_count[  (uint8)*(text + i) ];
}

const uint32 sup::suffixsort::build_prefix(uint32 * count, 
	uint32 * range_count, uint32 * group, uint8 * sorted, uint32 jobs)
{
	uint32 alphasize = 0;

	// Assign initial sorting groups and build prefix sums
	uint32 f = 0; // First index in group
	for (size_t i = 0 ; i < Alpha ; ++i) {
		uint32 n = count[i];
		uint32 tn = range_count[i]; // Count in thread range of text
		uint32 g = f + n - 1; // Last position in group f..g

		group[i] = g; // Assign group sorting key to last index in f..g
		groups += sorted[i] = (n == 1); // Singleton group is sorted

		// Add offset to count array after each thread
		// Prefix sum starts from f for first and f+tn for following threads
		range_count[i] = f; 
		for (size_t j = 1 ; j < jobs ; ++j) {
			uint32 tin = range_count[(j * Alpha) + i];
			range_count[(j * Alpha) + i] = f + tn;
			tn += tin;
		}

		f += n;

		alphasize += (n > 0);
	}
	return alphasize;
}

void sup::suffixsort::sort_range(uint32 p, uint32 n, uint32 * range_count,
		uint32 * group, uint8 * sorted, uint32 j)
{
	uint32 * task_count = (range_count + (j * Alpha));

	for (size_t i = p ; i < p+n ; ++i) {
		uint8 c = (uint8)*(text + i);
		uint32 j = task_count[c]++;
		// Initialize suffix array with counting sort 
		sa[j] = i;
		// Initialize inverse suffix array with group sorting key
		isa[i] = group[c];
		// Set singleton group as sorted (overwriting sa[j])
		if (sorted[c]) set_sorted(j, 1);
	}
}

void sup::suffixsort::invert_range(uint32 p, uint32 n)
{
	for (size_t i = p ; i<p+n ; ++i)
		sa[ isa[i] ] = i;
}

const uint32 * const sup::suffixsort::get_sa()
{
	return (finished_sa ? sa : 0);
}

const uint32 * const sup::suffixsort::get_lcp()
{
	return (finished_lcp ? lcp : 0);
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
			err << SELF << ": at " << (i-1) << ": "<< a << std::endl;
			err << SELF << ": at " << (i) << ": "<< b << std::endl;
			++descending;
		}
	}
	err << SELF << ": found " << descending << " text positions with following suffix in descending order" << std::endl;
	return (descending > 0);
}

bool sup::suffixsort::out_incorrect_lcp()
{
	uint32 nomatch = 0;
	for (size_t i = 1 ; i<len ; ++i) {
		if ( (strncmp((text+sa[i]), (text + sa[i-1]), lcp[i]) != 0)
					&& *(text + sa[i] + lcp[i] + 1) == 
					   *(text + sa[i-1] + lcp[i] + 1) ) {
			std::string a( (text + sa[i]) );
			std::string b( (text + sa[i-1]) );
			a.append("$"); b.append("$");
			if (a.length() > 72) a.erase(72);
			if (b.length() > 72) b.erase(72);
			std::replace( a.begin(), a.end(), '\n', '#');
			std::replace( a.begin(), a.end(), '\t', '#');
			std::replace( b.begin(), b.end(), '\n', '#');
			std::replace( b.begin(), b.end(), '\t', '#');
			err << SELF << ": at "<< (i) << " lcp " << lcp[i] << std::endl;
			err << SELF << ": '" << a << "'" << std::endl;
			err << SELF << ": '" << b << "'" << std::endl;
			++nomatch;
		}
	}
	err << SELF << ": found " << nomatch << " text positions where longest common prefix is not matching text" << std::endl;
	return (nomatch > 0);
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

void sup::suffixsort::out_sa()
{
	err << "i       sa[i]   order            suffix" << std::endl;
	for (size_t i = 0 ; i<len ; ++i)  {
		if (get_sorted(i)) {
			err << std::hex << i << std::dec << "\t"  
					<< get_sorted(i) << std::endl;
		}
		else {
			std::string str( (text + sa[i]) );
			str.append("$");
			if (str.length() > 44) str.erase(44);
			std::replace( str.begin(), str.end(), '\n', '#');
			std::replace( str.begin(), str.end(), '\t', '#');
			err << std::hex << i << "\t"  << sa[i] << "\t"  
				<< std::hex << std::setw(16) << std::setfill('0') << k(i) 
				<< std::dec << " '" << str << "'" << std::endl;
		}
	}
}

void sup::suffixsort::out_lcp()
{
	if (!finished_lcp) return;
	err << "i       sa[i]   order            lcp[i] suffix" << std::endl;
	for (size_t i = 0 ; i<len ; ++i)  {
		std::string str( (text + sa[i]) );
		str.append("$");
		if (str.length() > 37) str.erase(37);
		std::replace( str.begin(), str.end(), '\n', '#');
		std::replace( str.begin(), str.end(), '\t', '#');
		err << std::hex << i << "\t"  << sa[i] << "\t"  
			<< std::hex << std::setw(16) << std::setfill('0') << k(i) 
			<< std::dec << " " << std::setw(6) << std::setfill(' ') << lcp[i] 
			<< " '" << str << "'" << std::endl;
	}
}

bool sup::suffixsort::out_validate()
{
	if (!finished_sa) {
		err << SELF << ": suffix array not complete" << std::endl;
		return false;
	}

	uint32 dupes = count_dupes();

	out_descending();

	uint32 nomatch = 0;
	if (finished_lcp) nomatch = out_incorrect_lcp();

	return (dupes == 0 && nomatch == 0);
}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
