/**
 * Main program for suffix sorting.
 *
 * @author jkataja
 */

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/thread.hpp>

#include "sup.hpp"
#include "suffixsort.hpp"

namespace po = boost::program_options;

using namespace sup;

#define USAGE "Usage: sup [option]... input-file\n" \
	"Parallel suffix sorting in shared memory.\n" \
	"\n"

int main(int argc, char** argv) {

	// hardware threads available
	int hardware_jobs = boost::thread::hardware_concurrency();

	try {
		std::string jobs_str( boost::str( boost::format(
			"concurrency level [%1%,%2%]") % JobsMin % JobsMax));

		po::options_description visible_opts("Options");
		visible_opts.add_options()
			( "jobs,j",
			  po::value<uint32>()->default_value(hardware_jobs),
			  jobs_str.c_str()
			)
			( "help,h", "show this help" )
			( "lcp,l", "compute LCP array as well" )
			( "force,f", "force overwrite of existing output" )
			;

		po::options_description hidden_opts;
		hidden_opts.add_options()
			( "input-file", po::value<std::string>(), "input file")
			;
		
		po::positional_options_description p;
		p.add("input-file", 1);

		po::options_description opts;
		opts.add(visible_opts).add(hidden_opts);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
				options(opts).positional(p).run(), vm);
		po::notify(vm);

		// jobs range
		if (vm["jobs"].as<uint32>() < JobsMin 
				|| vm["jobs"].as<uint32>() > JobsMax) {
			std::cerr << SELF << ": concurrency level not in accepted range "
				<< "[" << JobsMin << "," << JobsMax << "]" << std::endl;
			return EXIT_FAILURE;
		}

		// no input
		if (!vm.count("input-file")) {
			std::cerr << SELF << ": no input" << std::endl;
		}

		// help
		if (vm.count("help") || !vm.count("input-file")) {
			std::cerr << USAGE << visible_opts << std::endl << std::flush;
			return EXIT_FAILURE;
		}

		// output filename
		std::string out_name(boost::str( boost::format("%1%.%2%") 
				% vm["input-file"].as<std::string>() % RankFileSuffix ) );

		// output already found
		if (std::ifstream( out_name.c_str() ).is_open() 
				&& !vm.count("force")) {
			std::cerr << SELF << ": output suffix array file '" << out_name 
					<< "' already found" << std::endl;
			std::cerr << SELF << ": use option -f to force overwrite" 
					<< std::endl << std::flush;
			return EXIT_FAILURE;
		}
		
		// read input text file
		std::string in_name( vm["input-file"].as<std::string>() );
		// type limits
		long in_filesize = stat_filesize(in_name);
		if ((in_filesize + sizeof(uint32)) >= UINT32_MAX) {
			std::cerr << SELF << ": input file too large (max 4 GiB)" 
					<< std::endl << std::flush;
			return EXIT_FAILURE;
		}
		uint32 len = (uint32)in_filesize;
		uint32 len_eof = len + 1;
		char * text_eof = (char *)read_byte_string(in_name);

		if (has_null(text_eof, len)) {
			std::cerr << SELF << ": input text contains null"
					<< std::endl << std::flush;
			return EXIT_FAILURE;
		}

		suffixsort sorter(text_eof, len_eof);

		// sequential 
		if (vm["jobs"].as<uint32>() == 1) {
			std::cerr << SELF << ": using sequential algorithm" << std::endl;
			sorter.run_sequential();
		}
		// parallel
		else {
			std::cerr << SELF << ": using parallel algorithm" << std::endl;
			sorter.run_parallel(vm["jobs"].as<uint32>());
		}
		
		// compute lcp array
		if (vm.count("lcp") != 0) {
			// TODO
			std::cerr << SELF << ": computing of LCP array not implemented" 
					<< std::endl << std::flush;
		}

		// output suffix array to file
		const uint32 * const sa = sorter.get_sa();
		write_byte_string(sa, ((len_eof) * sizeof(uint32)), out_name);

		delete [] text_eof;
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

	//return (len >= 0 ? EXIT_SUCCESS : EXIT_FAILURE);
	return EXIT_SUCCESS;

}
