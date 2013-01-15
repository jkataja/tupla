#include "suffixsort.hpp"
#include "sup.hpp"

#include <stdexcept>
#include <iomanip>
#include <algorithm>
#include <cstring>

using namespace sup;

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

const uint32 * const sup::suffixsort::get_sa()
{
	return (finished_sa ? sa : 0);
}

const uint32 * const sup::suffixsort::get_lcp()
{
	return (finished_lcp ? lcp : 0);
}

void sup::suffixsort::build_lcp()
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

void sup::suffixsort::init()
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
		++group[ (uint8)*(text + i) ];

	// Starting index of each sorting group f..g
	uint32 f = 0; // First index in group
	for (size_t i = 0 ; i < Alpha ; ++i) {
		uint32 n = group[i]; 
		uint32 g = f + n - 1; // Last character in group f..g
		count[i] = f; // Starting counting sort from f
		group[i] = g; // Assign group sorting key to g
		sorted[f] = (n == 1); // Sorted group length can be 1 element
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

void sup::suffixsort::run_sequential()
{
	// Allocate and initialize with counting sort
	init();

	// Doubling steps until number of sorting groups matches length
	uint32 groups = 0;
	for (h = 1 ; (groups < len && h < len) ; h <<= 1) {
		uint32 p = 0; // Starting index of sorted group
		uint32 sl = 0; // Sorted groups length following start
		groups = 0;
		for (size_t i = 0 ; i < len ; ) {
			// Skip sorted group
			if (uint32 s = sorted[i]) {
				i += s; sl += s; groups += s;
			} 
			else {
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
		}
		// Combine sorted group at end
		if (sl > 0) sorted[p] = sl;

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

void sup::suffixsort::run_parallel(uint32 jobs)
{
	std::cerr << SELF << ": not implemented" << std::endl;
	/*
The next ((j + 1)-th) doubling step for prosessor i works as follows. Set 
Ai[k] = ((R[xik], R[xik + 2(j+1) - 1]), xik) 
and sort the array by the values (R[xik], R[xik + 2^(j+1) - 1]), 
and replace the values with the ranks of the pairs. Here omitting the
special cases of indexes exceeding |T|. 
Once the array is sorted, the head of the
array where ranks are below the pivot S^(i−1)
and tail of the array where ranks are
above S^i are cut oﬀ.
This process is repeated on all processors i at most O(log n) times
until all ranks are unique. With perfect choice of pivots, the
running time of distributed doubling algorithm is O(n + N log 2
n/p), where n ≤ N ≤ np is the size of the global rank table after
ﬁrst round (again T = c n is the worst case). This bound assumes
comparison sort is used for sorting the pairs; better bounds can
be achieved using other models of computation. When assuming T is
sampled from uniform distribution, we have N ≤ n⌈p/σ⌉ (for each
character there is the same amount of pivots).
	*/
	return;
}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
