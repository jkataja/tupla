#pragma once

#include "sup.hpp"
#include "sortpar.hpp"

namespace sup {

class doubling_task
{
public:
	doubling_task(suffixsort * sorter, uint32 p, size_t n)
		: sorter(sorter), p(p), n(n) 
	{
	}

	void run()
	{
		sorter->doubling(p, n);
	}

protected:
	suffixsort * sorter;
	uint32 p;
	size_t n;
};

} // namespace
