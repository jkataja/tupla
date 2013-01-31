#pragma once

#include "tupla.hpp"
#include "sortpar.hpp"

namespace tupla {

class doubling_task
{
public:
	doubling_task(suffixsort * sorter, uint32 p, size_t n)
		: sorter(sorter), p(p), n(n) 
	{
	}

	void run()
	{
		sorter->doubling_range(p, n);
	}

protected:
	suffixsort * sorter;
	uint32 p;
	size_t n;
};

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab:
