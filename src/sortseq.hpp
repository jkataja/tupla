/**
 * Doubling suffix sort. Sequential implementation requires 8n memory.
 *
 * @author jkataja
 */

#pragma once

#include <iostream>
#include <boost/cstdint.hpp>

#include "numdefs.hpp"
#include "suffixsort.hpp"

namespace tupla {

class sortseq : public suffixsort {
private:
	sortseq(const sortseq&);
	sortseq& operator=(const sortseq&);

protected:

	virtual uint32 init();
	virtual void doubling();
	virtual void doubling_range(uint32, size_t);
	virtual void invert();

public:
	sortseq(const char *, const uint32, std::ostream&);
	virtual ~sortseq();
	virtual void build_lcp();

};

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab:
