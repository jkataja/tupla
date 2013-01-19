/**
 * Common defines and functions for suffix sort.
 *
 * @author jkataja
 */

#pragma once

#include <iostream>
#include <boost/cstdint.hpp>

#include "numdefs.hpp"

#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 
#define Z64 Z16,Z16,Z16,Z16
#define Z256 Z64,Z64,Z64,Z64

namespace sup {

// id for error messages
static const char SELF[] = "sup";

// Output rank table file suffix 
static const char RankFileSuffix[] = "rank";

// Output LCP table file suffix 
static const char LCPFileSuffix[] = "lcp";

// Letters in alphabet
static const uint32 Alpha = 256;

// Concurrency level 
static const uint32 JobsMin = 1;
static const uint32 JobsMax = 64;

// Get file size
long stat_filesize(const std::string&);

// Read byte string from file, returns pointer to allocated memory
// plus one additional byte for null terminator 
void * read_byte_string(const std::string&);

// Write byte string to file
void write_byte_string(const void * const, const size_t, const std::string&);

// String length matches expected length
bool has_null(const char *, const size_t);

} // namespace

// vim:set ts=4 sts=4 sw=4 noexpandtab: