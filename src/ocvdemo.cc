/* Copyright (C) 2012  Politecnico di Milano
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdio>
#include <iostream>
#include <random>
#include <cstring>
#include <memory>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "version.h"
#include "ocvdemo_exc.h"
#include <bbque/utils/utility.h>

#define EXC_BASENAME "OCVDemo"
#define RCP_BASENAME "ocvdemo"

// Setup logging
#undef  BBQUE_LOG_MODULE
#define BBQUE_LOG_MODULE "ocvdemo"

namespace po = boost::program_options;

/**
 * @brief A pointer to an EXC
 */
typedef std::shared_ptr<BbqueEXC> pBbqueEXC_t;

/**
 * The RNG used for testcase initialization.
 */
std::mt19937 rng_engine(time(0));

/**
 * The decription of each testapp parameters
 */
po::options_description opts_desc("BBQ-OpenCV Demo Options");

/**
 * The map of all testapp parameters values
 */
po::variables_map opts_vm;

/**
 * The services exported by the RTLib
 */
RTLIB_Services_t *rtlib;

/**
 * @brief The recipe to use for all the EXCs
 */
std::string recipe;

/**
 * @brief The maximum framerate
 *
 * This is used to identify the minimum cycle duration
 */
unsigned short fps_max;

/**
 * @brief The maximum number of frames
 *
 * This is used to identify the maximum number of frames to decode, in case the
 * input video has more then this number. If "0" all frames must be decoded.
 */
unsigned num_frames;

/**
 * @brief The ID of the V4L2 controlled webcam
 */
unsigned short cam_id;

/**
 * @brief The path of the .AVI video to use
 */
std::string video_path;

void ParseCommandLine(int argc, char *argv[]) {
	// Parse command line params
	try {
	po::store(po::parse_command_line(argc, argv, opts_desc), opts_vm);
	} catch(...) {
		std::cout << "Usage: " << argv[0] << " [options]\n";
		std::cout << opts_desc << std::endl;
		::exit(EXIT_FAILURE);
	}
	po::notify(opts_vm);

	// Check for help request
	if (opts_vm.count("help")) {
		std::cout << "Usage: " << argv[0] << " [options]\n";
		std::cout << opts_desc << std::endl;
		::exit(EXIT_SUCCESS);
	}

	// Check for version request
	if (opts_vm.count("version")) {
		std::cout << "Barbeque OpenCV Demo\n";
		std::cout << "Copyright (C) 2012 Politecnico di Milano\n";
		std::cout << "\n";
		std::cout << "Built on " <<
			__DATE__ << " " <<
			__TIME__ << "\n";
		std::cout << "\n";
		std::cout << "This is free software; see the source for "
			"copying conditions.  There is NO\n";
		std::cout << "warranty; not even for MERCHANTABILITY or "
			"FITNESS FOR A PARTICULAR PURPOSE.";
		std::cout << "\n" << std::endl;
		::exit(EXIT_SUCCESS);
	}
}

/**
 * @brief Register the required EXCs and enable them
 *
 * @param name the recipe to use
 */
pBbqueEXC_t SetupEXC(uint8_t cam_id,
		std::string const &video,
		std::string const &recipe) {
	char exc_name[] = EXC_BASENAME "_99";
	pBbqueEXC_t pexc;

	// Setup EXC name and recipe name
	::snprintf(exc_name+sizeof(exc_name)-3, 3, "%02d", cam_id);

	// Build a new EXC (without enabling it yet)
	assert(rtlib);
	pexc = pBbqueEXC_t(new OCVDemo(exc_name, recipe, rtlib,
				video, cam_id, fps_max, num_frames));

	// Saving the EXC (if registration to BBQ was successfull)
	if (!pexc->isRegistered())
		return pBbqueEXC_t();

	// Starting the control thread for the specified EXC
	assert(pexc);
	pexc->Start();

	return pexc;
}

int main(int argc, char *argv[]) {
	pBbqueEXC_t pexc;

	opts_desc.add_options()
		("help,h", "print this help message")
		("version,v", "print program version")

		("recipe,r", po::value<std::string>(&recipe)->
			default_value("OCVDemo"),
			"the recipe to use for the OpenCV demo")

		("cam,c", po::value<unsigned short>(&cam_id)->
			default_value(0),
			"the ID of the V4L2 webcam to use")
		("input,i", po::value<std::string>(&video_path)->
			default_value(""),
			"the path of the .AVI video to use")
		("fps_max,f", po::value<unsigned short>(&fps_max)->
			default_value(25),
			"the maximum framerate required")
		("num,n", po::value<unsigned>(&num_frames)->
			default_value(0),
			"the maximum number of frames to decode")
	;

	ParseCommandLine(argc, argv);

	// Welcome screen
	fprintf(stdout, FI(".:: BBQ OpenCV Demo Application (ver. %s)::.\n"),
			g_git_version);
	fprintf(stdout, FI("Built: " __DATE__  " " __TIME__ "\n\n"));

	// Init  RTLib library and setup the BBQ communication channel
	RTLIB_Init(::basename(argv[0]), &rtlib);
	assert(rtlib);

	// Configuring required Execution Contexts
	pexc = SetupEXC(cam_id, video_path, recipe);
	if (!pexc)
		return EXIT_FAILURE;

	// Wait for the demo to complete
	pexc->WaitCompletion();

	fprintf(stderr, FI("===== BBQ OpenCV Demo DONE! =====\n"));
	return EXIT_SUCCESS;
}

