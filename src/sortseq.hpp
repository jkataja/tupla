/**
 * Doubling suffix sort. Sequential implementation. Requires 8n memory.
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

protected:

	virtual uint32 init();
	virtual void doubling();
	virtual void doubling(uint32, size_t);
	virtual void invert();

public:
	sortseq(const char *, const uint32, std::ostream&);
	virtual ~sortseq();
	virtual void build_lcp();

};

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab:
