#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test_framework.hpp>

#include <boost/iostreams/device/mapped_file.hpp>
#include <cstdio>
#include <cstring>

#include "sup.hpp"
#include "suffixsort.hpp"

using namespace sup;

std::vector<std::string> test_files = { 
	"data/trivial/banana",
	"data/artificial/fib41", 
	"data/largetext/enwik8", 
	"data/pseudo-real/dblp.xml.50MB", 
};


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

// SA and ISA match
bool is_xvalid(const uint32 * const sa, const uint32 * const isa, uint32 len)
{
	for (size_t i = 0 ; i<len ; ++i)
		if (isa[ sa[i] ] != i) return false;
	return true;
}

// Suffixes in ascending order
bool is_ascending(const uint32 * const sa, const char * text, uint32 len)
{
	for (size_t i = 1 ; i<len ; ++i) {
		if (memcmp((text+sa[i]), (text + sa[i-1]), len-sa[i]) < 0) {
			std::cerr << "SA not in ascending order at " << i << std::endl;
			return false;
		}
	}
	return true;
}

// Common prefix part matches and first character after is different
bool has_correct_lcp(const uint32 * const sa, const uint32 * const lcp, 
		const char * text, uint32 len)
{
	for (size_t i = 1 ; i<len ; ++i) {
		if (memcmp((text+sa[i]), (text + sa[i-1]), lcp[i]) != 0) {
			std::cerr << "LCP does not match text at " << i << std::endl;
			return false;
		}
	}
	return true;
}

// Run parallel algorithm for input and compare result to expected
void run_sorter(std::string& in_name, uint32 jobs, 
		const uint32 cap = 0x7FFFFFFE)
{
	long in_filesize = stat_filesize(in_name);
	BOOST_CHECK( in_filesize != -1 );

	uint32 len = (uint32)in_filesize;
	len = std::min(len, cap);
	uint32 len_eof = len + 1;
	char * text_eof = (char *)read_byte_string(in_name, len);

	std::unique_ptr<suffixsort> sorter( suffixsort::instance( text_eof,
			len_eof, jobs, std::cerr) );

	sorter->build_sa();

	const uint32 * const sa = sorter->get_sa();
	const uint32 * const isa = sorter->get_isa();

	BOOST_CHECK( has_sa_range(sa, len_eof) );
	BOOST_CHECK( has_sa_unique(sa, len_eof) );
	BOOST_CHECK( has_sa_terminator(sa, len_eof) );
	BOOST_CHECK( is_ascending(sa, text_eof, len_eof) );
	BOOST_CHECK( is_xvalid(sa, isa, len_eof) );

	sorter->build_lcp();
	
	const uint32 * const lcp = sorter->get_lcp();
	
	BOOST_CHECK( has_correct_lcp(sa, lcp, text_eof, len_eof) );

	delete [] text_eof;
}

BOOST_AUTO_TEST_CASE( read_text ) 
{
	// test/banana
	{
		char * text = (char *)read_byte_string("data/trivial/banana", 6);
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

BOOST_AUTO_TEST_CASE( read_binary ) 
{
	// test/cafebabe
	{
		uint32 * data = (uint32 *)read_byte_string("test/cafebabe", 4);
		BOOST_CHECK( *data == 0xCAFEBABE );
		delete [] data;
	}
}

BOOST_AUTO_TEST_CASE( run_test_files_limited ) 
{
	for (auto filename : test_files) {
		for (int jobs = 1 ; jobs <= 8 ; jobs <<= 1) {
			std::cerr << "Running test with '" << filename 
					<< "' (1 MB) " << jobs << " threads" << std::endl;
			run_sorter(filename, jobs, (1 << 20));
		}
	}
}

BOOST_AUTO_TEST_CASE( run_largetext ) 
{
	std::string filename("data/largetext/enwik8");
	for (int jobs = 1 ; jobs <= 8 ; jobs <<= 1) {
		std::cerr << "Running test with '" << filename 
			<< " (95 MB) with " << jobs << " threads" << std::endl;
		run_sorter(filename, jobs);
	}
}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
