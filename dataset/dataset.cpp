
#include <cstdlib>
#include <string>
#include <iostream>

#include <boost/program_options.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "datasetIO.h"

namespace po = boost::program_options;


int show_callback(const IplImage* img, int frame, int x, int y)
{
	cvShowImage("sample", img);
	if((cvWaitKey(0) & 0xff) == 27)
		return -1;
	return 0;
}

int static show(const char* path, const char* predicate)
{
	void* dataset;
	char* errMsg;
	int r;
	if((r = dataset_open(&dataset, path, 0, &errMsg)) != 0)
	{
		std::cout << "Error: " << errMsg << std::endl;
		dataset_close(&dataset);
		return r;
	}
	if((r = dataset_read_samples(&dataset, predicate,
			show_callback, &errMsg)) != 0 && r != QUERY_ABORT)
	{
		std::cout << "Error: " << errMsg << std::endl;
		dataset_close(&dataset);
		return r;
	}
	dataset_close(&dataset);
	return 0;
}

int static remove(const char* path, const char* predicate)
{
	void* dataset;
	char* errMsg;
	int r;
	if((r = dataset_open(&dataset, path, 1, &errMsg)) != 0)
	{
		std::cout << "Error: " << errMsg << std::endl;
		dataset_close(&dataset);
		return r;
	}
	if((r = dataset_delete_samples(&dataset, predicate, &errMsg)) != 0)
	{
		std::cout << "Error: " << errMsg << std::endl;
		dataset_close(&dataset);
		return r;
	}
	dataset_close(&dataset);
	return 0;
}

int static calcDescriptors(const char* dataset, int algorithm) // ?
{

}

void static conflicting_options(const po::variables_map& vm,
		const char* opt1, const char* opt2)
{
	if(vm.count(opt1) && !vm[opt1].defaulted()
		&& vm.count(opt2) && !vm[opt2].defaulted())
			throw std::logic_error(std::string("Conflicting options '")
				+ opt1 + "' and '" + opt2 + "'.");
}

void static option_dependency(const po::variables_map& vm,
		const char* for_what, const char* required_option)
{
	if(vm.count(for_what) && !vm[for_what].defaulted())
		if (vm.count(required_option) == 0 || vm[required_option].defaulted())
			throw std::logic_error(std::string("Option '") + for_what
				+ "' requires option '" + required_option + "'.");
}

int main(int argc, char** argv)
{
	// TODO : limit?
	std::string datasetPath;
	std::string predicate;

	po::options_description actions("Actions");
	actions.add_options()
		("help,h", "Show handy manual you're reading")
		("show,s", "Show samples in dataset (predicate is optional)")
		("remove,r", "Remove samples from dataset by (mandatory) predicate ")
		// TODO : descriptor algo?
		("calc-descriptors,c", "Calculate descriptors");
	po::options_description parameters("Parameters");
	parameters.add_options()
		("dataset-path,d", po::value<std::string>(&datasetPath)->default_value("."),
			"Path to dataset folder")
		("predicate,p", po::value<std::string>(&predicate),
			"Predicate for samples (SQL syntax)");

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

		conflicting_options(vm, "help", "show");
		conflicting_options(vm, "help", "remove");
		conflicting_options(vm, "help", "calc-descriptors");
		conflicting_options(vm, "show", "remove");
		conflicting_options(vm, "show", "calc-descriptors");
		conflicting_options(vm, "remove", "calc-descriptors");

		option_dependency(vm, "remove", "predicate");
	}
	catch(std::logic_error& err)
	{
		std::cout << "Command-line error: " << err.what() << std::endl;
		return EXIT_FAILURE;
	}

	if(vm.count("help"))
	{
		std::cout << usage;
		return EXIT_SUCCESS;
	}

	const char* pred = predicate.empty() ? "1=1" : predicate.c_str();

	if(vm.count("show"))
		return show(datasetPath.c_str(), pred);
	else if(vm.count("remove"))
		return remove(datasetPath.c_str(), pred);
	else if(vm.count("calc-descriptors"))
		return calcDescriptors(datasetPath.c_str(), 0);
	else
	{
		std::cout << "Command-line error: no action specified" << std::endl;
		return EXIT_FAILURE;
	}
}

