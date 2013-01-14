#include "suffixsort.hpp"
#include "sup.hpp"
#include <cassert> // DEBUG
#include <stdexcept>
#include <iomanip>

using namespace sup;

bool sup::suffixsort::eq_sa_isa()
{
	for (size_t i = 0 ; i<len ; ++i)
		if (isa[ sa[i] ] != i) return false;
	return true;
}

bool sup::suffixsort::out_incorrect_order(std::ostream& out)
{
	uint32 wrongorder = 0;
	for (size_t i = 1 ; i<len ; ++i) {
		std::string a( (text + sa[i]) );
		std::string b( (text + sa[i-1]) );
		if (a < b) {
			out << (i-1) << ": "<< a << std::endl;
			out << (i) << ": "<< b << std::endl << std::endl;
			++wrongorder;
		}
	}
	out << SELF << ": incorrect orders " << wrongorder << std::endl;
	return (wrongorder > 0);
}

uint32 sup::suffixsort::has_dupes()
{
	uint32 * match = new uint32[len]();
	uint32 dupes  = 0;
	for (size_t i = 0 ; i<len ; ++i) {
		dupes += ( ++match[ sa[i] ] > 1 );
	}
	delete [] match;
	return dupes;
}

void sup::suffixsort::out_sa(uint32 p, size_t n, std::ostream& out)
{
	const uint64 v = choose_pivot(p, n);
	out << "i\tsa[i]\torder            suffix" << std::endl;
	for (size_t i = p ; i<p+n ; ++i)  {
		std::string str( (text + sa[i]) );
		str.append("$");
		if (str.length() > 34) str.erase(34);
		std::replace( str.begin(), str.end(), '\n', '#');
		std::replace( str.begin(), str.end(), '\t', '#');
		out << std::hex << i << "\t"  << sa[i] << "\t"  
			<< std::setw(16) << std::setfill('0') << k(i) << std::dec;
		if (k(i) < v) out << " <";
		if (k(i) == v) out << " =";
		if (k(i) > v) out << " >";
		out << " '" << str << "'" << std::endl;
	}
}

void sup::suffixsort::out_sa(std::ostream& out)
{
	out << "i\tsa[i]\tsorted\torder            suffix" << std::endl;
	for (size_t i = 0 ; i<len ; ++i)  {
		std::string str( (text + sa[i]) );
		str.append("$");
		if (str.length() > 36) str.erase(36);
		std::replace( str.begin(), str.end(), '\n', '#');
		std::replace( str.begin(), str.end(), '\t', '#');
		out << std::hex << i << "\t"  << sa[i] << "\t"  
			<< std::dec << sorted[i] << "\t" 
			<< std::hex << std::setw(16) << std::setfill('0') << k(i) 
			<< std::dec << " '" << str << "'" << std::endl;
	}
}

void sup::suffixsort::out_isa(std::ostream& out)
{
	out << "i\torder            suffix" << std::endl;
	for (size_t i = 0 ; i < len ; ++i) {
		std::string str( (text + i) );
		str.append("$");
		if (str.length() > 52) str.erase(52);
		std::replace( str.begin(), str.end(), '\n', '#');
		std::replace( str.begin(), str.end(), '\t', '#');
		out << std::hex << i << "\t" 
			<< std::setw(8) << std::setfill('0') << isa[i]
			<< std::setw(8) << std::setfill('0') << (i+h < len ? isa[i+h] : 0)
			<< std::dec << " '" << str << "'" << std::endl;
	}
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
	bzero(lcp, (len * sizeof(uint32)) );

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

	bzero(sa, (len * sizeof(uint32)) );
	bzero(isa, (len * sizeof(uint32)) );
	bzero(sorted, (len * sizeof(uint32)) );

	// Count character occurences
	for (size_t i = 0 ; i < len ; ++i) 
		++group[ (uint8)*(text + i) ];

	// Starting index of each sorting group
	uint32 f = 0; // First index in group
	for (size_t i = 0 ; i < Alpha ; ++i) {
		uint32 n = group[i]; 
		uint32 g = f + n - 1; // Last character in group
		count[i] = f;
		group[i] = g;
		sorted[f] = (n == 1);
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

		std::cerr << SELF << ": doubling " << ffs(h) 
				<< " with " << groups << " groups" << std::endl;
	}

#ifndef NDEBUG
	out_incorrect_order(std::cerr);

	uint32 dupes = has_dupes();
	std::cerr << SELF << ": duplicates " << dupes << std::endl;

	if (!eq_sa_isa())
		std::cerr << SELF << ": SA and ISA are not consistent" << std::endl;
#endif

	if (groups != len) {
		throw std::runtime_error("unable to find unique values for all suffixes");
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
