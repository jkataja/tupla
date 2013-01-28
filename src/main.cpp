/**
 * Main program for suffix sorting.
 *
 * @author jkataja
 */

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <memory>
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

	// Hardware threads available
	int hardware_jobs = boost::thread::hardware_concurrency();

	try {
		std::string jobs_str( boost::str( boost::format(
			"Allow arg sorting threads to run simultaneously [%1%,%2%]") % JobsMin % JobsMax));

		po::options_description visible_opts("Options");
		visible_opts.add_options()
			( "benchmark,b", "Do not output file(s)" )
			( "force,f", "Force overwrite of existing output" )
			( "help,h", "Show this help and exit" )
			( "jobs,j",
			  po::value<uint32>()->default_value(hardware_jobs),
			  jobs_str.c_str()
			)
			( "lcp,l", "Compute LCP array as well" )
			( "count,n", 
			  po::value<uint32>()->default_value(MaxInput, ""),
			  "Stop processing input after arg bytes" 
			)
			( "output,o", "Print generated suffix array to stderr" )
			( "validate,v", "Validate generated suffix array (slow)" )
			;

		// TODO accept k and M in count

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

		// Jobs range
		if (vm["jobs"].as<uint32>() < JobsMin 
				|| vm["jobs"].as<uint32>() > JobsMax) {
			std::cerr << SELF << ": concurrency level not in accepted range "
				<< "[" << JobsMin << "," << JobsMax << "]" << std::endl;
			return EXIT_FAILURE;
		}

		// No input
		if (!vm.count("input-file")) {
			std::cerr << SELF << ": no input" << std::endl;
		}

		// Help
		if (vm.count("help") || !vm.count("input-file")) {
			std::cerr << USAGE << visible_opts << std::endl << std::flush;
			return EXIT_FAILURE;
		}

		// Output filename
		std::string out_name(boost::str( boost::format("%1%.%2%") 
				% vm["input-file"].as<std::string>() % RankFileSuffix ) );

		// Output already found
		if (!vm.count("benchmark") && !vm.count("force")
				&& std::ifstream( out_name.c_str() ).is_open())  {
			std::cerr << SELF << ": output suffix array file '" << out_name 
					<< "' already found" << std::endl;
			std::cerr << SELF << ": use option -f to force overwrite" 
					<< std::endl << std::flush;
			return EXIT_FAILURE;
		}
		
		// Read input text file
		std::string in_name( vm["input-file"].as<std::string>() );
		// Type limits
		long in_filesize = stat_filesize(in_name);
		if (in_filesize > MaxInput) {
			std::cerr << SELF << ": input file too large (max 2 GiB)" 
					<< std::endl << std::flush;
			return EXIT_FAILURE;
		}
		uint32 len = (uint32)in_filesize;

		// Limit input bytes to read
		if (vm.count("count")) {
			uint32 maxcount = vm["count"].as<uint32>();
			if (maxcount < len) len = std::min(len, maxcount);
		}
		uint32 len_eof = len + 1;

		// TODO read from stdin
		char * text_eof = (char *)read_byte_string(in_name, len);

		std::unique_ptr<suffixsort> sorter( suffixsort::instance( text_eof,
				len_eof, vm["jobs"].as<uint32>(), std::cerr) );

		sorter->build_sa();

		// Compute LCP array from completed SA
		if (vm.count("lcp")) {
			sorter->build_lcp();
		}

		// Run cross-validation test
		if (vm.count("validate")) {
			sorter->out_validate();
		}

		// Output completed suffix array
		if (vm.count("output")) {
			if (vm.count("lcp")) sorter->out_lcp();
			else sorter->out_sa();
		}

		// Output suffix array to file
		if (!vm.count("benchmark")) {
			const uint32 * const sa = sorter->get_sa();
			write_byte_string(sa, ((len_eof) * sizeof(uint32)), out_name);
		}

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

	return EXIT_SUCCESS;

}

// vim:set ts=4 sts=4 sw=4 noexpandtab:
