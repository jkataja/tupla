/**
 * Print lines in file matching to pattern using pre-generated suffix array.
 *
 * @author jkataja
 */

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include "sup.hpp"

namespace po = boost::program_options;

using namespace sup;

#define USAGE "Usage: supgrep pattern text-file\n" \
	"Print lines in file matching to pattern using pre-generated suffix array.\n" \
	"\n"

int main(int argc, char** argv) {

	uint32 found = 0;

	try {
		po::options_description opts;
		opts.add_options()
			( "pattern", po::value<std::string>(), "pattern")
			( "text-file", po::value<std::string>(), "input file")
			;
		
		po::positional_options_description p;
		p.add("pattern", 1);
		p.add("text-file", 1);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
				options(opts).positional(p).run(), vm);
		po::notify(vm);

		// TODO Pattern does not contain linefeed, null, etc.

		// no input
		if (!vm.count("pattern") || !vm.count("text-file")) {
			std::cerr << USAGE << std::endl << std::flush;
			return EXIT_FAILURE;
		}

		// map input text file
		std::string text_name( vm["input-file"].as<std::string>() );
		long text_filesize = stat_filesize(text_name);
		if ((text_filesize + sizeof(uint32)) >= std::numeric_limits<uint32>::max() ) {
			std::cerr << SELF << ": input file too large (max 4 GiB)"
				<< std::endl << std::flush;
			return EXIT_FAILURE;
		}

		uint32 textlen = (uint32)text_filesize;
		uint32 textlen_eof = textlen + 1;
		char * text_eof = (char *)read_byte_string(text_name);

		if (has_null(text_eof, textlen)) {
			std::cerr << SELF << ": input contains nulls"
					<< std::endl << std::flush;
			return EXIT_FAILURE;
		}

		// map input rank file
		std::string rank_filename( boost::str( boost::format("%1%.%2%") 
				% text_name % RankFileSuffix ) );
		long rank_filesize = stat_filesize(rank_filename);
		if (rank_filesize == -1) {
			std::cerr << SELF << ": could not stat rank file '" 
					<< rank_filename << "'" << std::endl << std::flush;
			std::cerr << SELF << ": run 'sup " << text_name
					<< "' to create the rank file" << std::endl << std::flush;
			return EXIT_FAILURE;
		}
		if ((rank_filesize >> 2) != textlen) {
			std::cerr << SELF << ": rank size does not match text length" 
					<< std::endl << std::flush;
			return EXIT_FAILURE;
		}
		// TODO slurp rank_filename  to rank

		// TODO grep
	}
	catch (std::exception& e) {
		std::cerr << SELF << ": " << e.what() << std::endl << std::flush;
		return EXIT_FAILURE;
	}
	catch (...) {
		std::cerr << SELF << ": caught unknown exception" 
			<< std::endl << std::flush;
		return EXIT_FAILURE;
	}

	std::cout << std::flush;
	std::cerr << std::flush;

	return (found > 0 ? EXIT_SUCCESS : EXIT_FAILURE);

}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
