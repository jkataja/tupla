/**
 * Doubling suffix sort. Parallel algorithm. 
 *
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

class sortpar : public suffixsort {
private:
	uint32 jobs;

	sortpar(const sortpar&);
	sortpar& operator=(const sortpar&);

	// Parallel initialization subroutines
	void tccount(uint32, uint32, uint32 *);
	void tcsort(uint32, uint32, uint32 *, uint32 *);

protected:
	virtual void init();
	virtual uint32 doubling();

public:

	sortpar(const char *, const uint32, const uint32, std::ostream&);
	virtual ~sortpar();
	virtual void build_lcp();

};

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab:
