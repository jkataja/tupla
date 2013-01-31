#pragma once

#include "tupla.hpp"
#include "sortpar.hpp"

namespace tupla {

class tqsort_task
{
public:

	tqsort_task(suffixsort * sorter, uint32 p, size_t n)
		: groups(0), sorter(sorter), p(p), n(n)
	{
	}


	void run()
	{
		groups = sorter->tqsort(p, n);
	}

	uint32 groups;

protected:
	suffixsort * sorter;
	uint32 p;
	size_t n;
};

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab:
