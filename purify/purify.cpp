
#include <cstdlib>
#include <string>
#include <iostream>

#include <boost/program_options.hpp>

namespace po = boost::program_options;


extern int remove_ghosts(const std::string& path, bool verbose = false);
extern int remove_weird_sizes(const std::string& path, bool verbose = false);


int main(int argc, char** argv)
{
	std::string datasetPath;
	bool verbose;

	po::options_description actions("Actions");
	actions.add_options()
		("help,h", "Show handy manual you're reading")
		("ghosts,g", "Try and remove ghosts from motion detection stage")
		("sizes,s", "Try and remove objects with weird sizes based on statistic");
	po::options_description parameters("Parameters");
	parameters.add_options()
		("dataset-path,d", po::value<std::string>(&datasetPath)->default_value("."),
			"Path to dataset folder")
		("verbose,v", po::value<bool>(&verbose)->default_value(false));


	po::positional_options_description p;
	p.add("dataset-path", -1);

	po::options_description usage("USAGE");
	usage.add(actions).add(parameters);

	po::variables_map vm;
	try
	{
		po::store(po::command_line_parser(argc, argv).
			options(usage).positional(p).run(), vm);
		po::notify(vm);
	}
	catch(std::logic_error& err)
	{
		std::cerr << "Command-line error: " << err.what() << std::endl;
		return EXIT_FAILURE;
	}

	if(vm.count("help"))
	{
		std::cout << usage;
		return EXIT_SUCCESS;
	}
	if(vm.count("ghosts") == 0 && vm.count("sizes") == 0)
	{
		std::cerr << "Command-line error: no action specified" << std::endl;
		std::cerr << usage;
		return EXIT_FAILURE;
	}

	int r;
	if(vm.count("ghosts"))
		if((r = remove_ghosts(datasetPath, verbose)) != 0)
			return r;
	if(vm.count("sizes"))
		if((r = remove_weird_sizes(datasetPath, verbose)) != 0)
			return r;

	return EXIT_SUCCESS;
}

