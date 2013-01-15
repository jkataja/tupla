#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test_framework.hpp>

#include <boost/iostreams/device/mapped_file.hpp>
#include <cstdio>
#include <cstring>

#include "sup.hpp"
#include "suffixsort.hpp"

using namespace sup;

static uint32 banana_sa[] = { 6, 5, 3, 1, 0, 4, 2 };

// First element in suffix array is terminator
bool has_sa_terminator(const uint32 * const sa, uint32 len)
{
	return ( sa[0] == len-1 );
}

// Suffix array contents for a and b are equal 
bool has_sa_equal(const uint32 * const a, const uint32 * const b, 
		const uint32 len)
{
	return ( memcmp(a, b, sizeof(uint32) * len) == 0 );
}

// All indices in suffix array are under text length
bool has_sa_range(const uint32 * const sa, const uint32 len)
{
	for (uint32 i = 0 ; i<len ; ++i) {
		if (sa[i] >= len)
			return false;
	}
	return true;
}

// All indices in suffix array are unique
bool has_sa_unique(const uint32 * const sa, uint32 len)
{
	uint32 * match = new uint32[len]();
	uint32 dupes  = 0;
	for (size_t i = 0 ; i<len ; ++i) {
		dupes += ( ++match[ sa[i] ] > 1 );
	}
	delete [] match;
	return ( dupes == 0 );
}

// Run sequential algorithm for input and compare result to expected
void run_sequential(const std::string& in_name, uint32 * expect)
{
	long in_filesize = stat_filesize(in_name);
	BOOST_CHECK( in_filesize != -1 );

	uint32 len = (uint32)in_filesize;
	uint32 len_eof = len + 1;
	char * text_eof = (char *)read_byte_string(in_name);

	BOOST_CHECK( !has_null(text_eof, len) );

	suffixsort sorter(text_eof, len_eof);
	sorter.run_sequential();
	const uint32 * const sa = sorter.get_sa();

	BOOST_CHECK( has_sa_range(sa, len_eof) );
	BOOST_CHECK( has_sa_unique(sa, len_eof) );
	BOOST_CHECK( has_sa_terminator(sa, len_eof) );
	BOOST_CHECK( has_sa_equal(sa, expect, len_eof) );

	delete [] text_eof;
}

// Run parallel algorithm for input and compare result to expected
void run_parallel(std::string& in_name, uint32 * expect,
		uint32 jobs)
{
	long in_filesize = stat_filesize(in_name);
	BOOST_CHECK( in_filesize != -1 );

	uint32 len = (uint32)in_filesize;
	uint32 len_eof = len + 1;
	char * text_eof = (char *)read_byte_string(in_name);

	BOOST_CHECK( !has_null(text_eof, len) );

	suffixsort sorter(text_eof, len);
	sorter.run_parallel(jobs);
	const uint32 * const sa = sorter.get_sa();

	BOOST_CHECK( has_sa_range(sa, len_eof) );
	BOOST_CHECK( has_sa_unique(sa, len_eof) );
	BOOST_CHECK( has_sa_terminator(sa, len_eof) );
	BOOST_CHECK( has_sa_equal(sa, expect, len_eof) );

	delete [] text_eof;
}

BOOST_AUTO_TEST_CASE( read_text ) 
{
	// test/empty
	{
		char * text = (char *)read_byte_string("test/empty");
		BOOST_CHECK( strcmp("", text) == 0 );
		delete [] text;
	}
	// test/banana
	{
		char * text = (char *)read_byte_string("test/banana");
		BOOST_CHECK( strcmp("banana", text) == 0 );
		delete [] text;
	}
}

BOOST_AUTO_TEST_CASE( write_binary ) 
{
	// test/cafebabe
	{
		uint32 data = 0xCAFEBABE;
		write_byte_string(&data, 4, "test/cafebabe");
	}
}

BOOST_AUTO_TEST_CASE( null_str )
{
	BOOST_CHECK( !has_null("can't has null", 14) );
	BOOST_CHECK( has_null("can\0has\0null", 14) );
}

BOOST_AUTO_TEST_CASE( read_binary ) 
{
	// test/cafebabe
	{
		uint32 * data = (uint32 *)read_byte_string("test/cafebabe");
		BOOST_CHECK( *data == 0xCAFEBABE );
		delete [] data;
	}
}

BOOST_AUTO_TEST_CASE( sequential ) 
{
	run_sequential("test/banana", banana_sa);
	// TODO more complex input
}

/*
BOOST_AUTO_TEST_CASE( parallel ) 
{
	for (int jobs = 1 ; jobs <= 32 ; ++jobs) {
		// TODO 
	}
}
*/

// vim:set ts=4 sts=4 sw=4 noexpandtab:
