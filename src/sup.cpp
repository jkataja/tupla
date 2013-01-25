#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdexcept>
#include <boost/iostreams/device/mapped_file.hpp>

#include "sup.hpp"

using namespace sup;

long sup::stat_filesize(const std::string& filename) {
	struct stat stat_buf;
	int rc = stat(filename.c_str(), &stat_buf);
	if (rc != 0) {
		throw std::runtime_error("could not stat file");
	}
	return (rc == 0 ? stat_buf.st_size : -1);
}

void * sup::read_byte_string(const std::string& filename, const uint32 len) {
	long len_eof = len + 1;

	if (len > stat_filesize(filename)) {
		throw std::runtime_error("attempt to read past input");
	}

	// empty file
	if (len == 0) return new char[1]();

	boost::iostreams::mapped_file_source in;
	try {
		in.open(filename, len);
	} catch (...) { }
	if (!in.is_open()) {
		throw std::runtime_error("could not map input file");
	}

	// copy data and pad with terminator
	char * data = (char * )in.data();
	char * data_eof = new char[len_eof];
	if (data_eof == 0) {
		in.close();
		throw std::runtime_error("could not allocate enough memory to fit file");
	}
	memcpy((void *)data_eof, (void *)data, len);
	data_eof[len] = 0;

	in.close();

	return data_eof;
}

void sup::write_byte_string(const void * const data, const size_t len, 
		const std::string& filename) {
	boost::iostreams::mapped_file_sink out(filename);
	boost::iostreams::mapped_file_params out_params;
	out_params.path = filename;
	out_params.length = len;
	out_params.new_file_size = len;
	try {
		out.open(out_params);
	} catch (...) { }
	if (!out.is_open()) {
		throw std::runtime_error("could not map output file");
	}

	if (len > 0) {
		uint32 * out_data = (uint32 * )out.data();
		memcpy((void *)out_data, (void *)data, len); 
	}

	out.close();
}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
