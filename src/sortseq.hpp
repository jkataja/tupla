/**
 * Doubling suffix sort. Sequential algorithm using ternary split quick sort. 
 * Requires 12n memory compared to 8n for Larsson & Sadakane implementation, 
 * but finished suffix array is readily accessible without inverting ISA.
 *
 * Impelements prefix doubling as described in:
 * N. Jesper Larsson & Kunihiko Sadakane: Faster Suffix Sorting
 * LU-CS-TR:99-214 [LUNFD6/(NFCS-3140)/1{20/(1999)]
 *
 * Uses using ternary split quick sort, based on:
 * Bentley & McIlroy 1993: Engineering a Sort Function
 * SOFTWARE—PRACTICE AND EXPERIENCE, VOL. 23(11), 1249–1265 (NOVEMBER 1993)
 *
 * @author jkataja
 */

#pragma once

#include <iostream>
#include <boost/cstdint.hpp>

#include "numdefs.hpp"
#include "suffixsort.hpp"

namespace sup {

class sortseq : public suffixsort {
private:
	sortseq(const sortseq&);
	sortseq& operator=(const sortseq&);

	// Sort range using ternary split quick sort
	// Based on Bentley-McIlroy 1993: Engineering a Sort Function
	void tqsort(uint32, size_t);

	// Determine median value of three suffix array elements
	// Returns index to position where median was found
	// Based on Bentley-McIlroy 1993: Engineering a Sort Function
	inline uint32 med3(const uint32 a, const uint32 b, const uint32 c)
	{
		const uint64 ka = k(a);
		const uint64 kb = k(b);
		const uint64 kc = k(c);
		// abc acb cab
		// cba bac bca
		return (ka < kb ? (kb < kc ? b : (ka < kc ? c : a) )
		                : (kb > kc ? b : (ka < kc ? a : c) ) ); 
	}

	// Choose pivot value from n elements starting at p using pseudomedian
	// Returns pair ( ISA_h[i] , ISA_h[i+h] ) packed in long
	// Based on Bentley-McIlroy 1993: Engineering a Sort Function
	inline uint64 choose_pivot(uint32 p, size_t n)
	{
		uint32 b = p + (n/2); // Small arrays, middle element
		if (n > 7) {
			uint32 a = p;
			uint32 c = p + n - 1;
			if (n > 40) { // Big arrays, pseudomedian of 9
				uint32 s = (n/8);
				a = med3( a, a+s, a+2*s );
				b = med3( b-s, b, b+s );
				c = med3( c-2*s, c-s, c );
			}
			b = med3( a, b, c ); // Mid-size, med of 3
		}
		return k(b);
	}

protected:

	virtual void init();
	virtual void doubling();

public:
	sortseq(const char *, const uint32, std::ostream&);
	virtual ~sortseq();
	virtual void build_lcp();

};

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab:
