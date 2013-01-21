#pragma once

#include "sup.hpp"
#include "sortpar.hpp"

namespace sup {

class tqsort_task
{
public:
	tqsort_task(suffixsort * sorter, uint32 p, size_t n)
		: sorter(sorter), p(p), n(n) 
	{
	}

	void run()
	{
	/*
		sorter->err_lock.lock();
		sorter->err << "tqsort_task::run(" << p << "," << n << ")" 
				<< std::endl;
		sorter->err_lock.unlock();
		*/
		sorter->tqsort(p, n);
	}

protected:
	suffixsort * sorter;
	uint32 p;
	size_t n;
};

} // namespace
