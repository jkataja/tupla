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
	/*
		sorter->err_lock.lock();
		sorter->err << "doubling_task::run(" << p << "," << n << ")" 
				<< std::endl;
		sorter->err_lock.unlock();
		*/
		sorter->doubling(p, n);
	}

protected:
	suffixsort * sorter;
	uint32 p;
	size_t n;
};

} // namespace
