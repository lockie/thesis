/*  See <<Representations and Techniques for 3D Object Recognition and Scene
 *  Interpretation>> by Derek Hoiem & Silvio Savarese, Morgan & Claypool
 *  Publishers, 2011, p. 36:
 *  "In most cases, simple k-means clustering of densely sampled SIFT or HOG
 *  descriptors will work well.
 * */

#include <cstdlib>
#include <iostream>
#include <iomanip>

#include <boost/program_options.hpp>

#include <opencv2/core/core.hpp>

#include "datasetIO.h"

using namespace std;
namespace po = boost::program_options;


static int _verbose = 0;

int do_cluster(const string& datasetPath);

int main(int argc, char** argv)
{
	string datasetPath;

	po::options_description usage("Usage");
	usage.add_options()
		("help,h", "Show handy manual you're reading")
		("verbose,v", "Be verbose about what is happening")
		("dataset-path,d", po::value<string>(&datasetPath)->default_value("."),
			"Path to dataset folder")
	;
	po::positional_options_description p;
	p.add("dataset-path", -1);

	po::variables_map vm;
	try
	{
		po::store(po::command_line_parser(argc, argv).
			options(usage).positional(p).run(), vm);
		po::notify(vm);
	}
	catch(logic_error& err)
	{
		cerr << "Command-line error: " << err.what() << endl;
		return EXIT_FAILURE;
	}

	if(vm.count("help"))
	{
		cout << usage;
		return EXIT_SUCCESS;
	}
	if(vm.count("verbose"))
		_verbose = 1;

	return do_cluster(datasetPath);
}

// XXX : even 10 is way over for video with people, cars & other shit
static const int _maxClasses = 50;

int determine_cluster_count(map<int, double>& kmeansCompactness)
{
	// see http://stackoverflow.com/a/4473065/1336774
	map<int, double> secondDerivative;
	for(int i = 2; i < _maxClasses-1; i++)
	{
		secondDerivative[i] = kmeansCompactness[i+1] + kmeansCompactness[i-1] -
			2 * kmeansCompactness[i];
	}
	secondDerivative.erase(2);  // XXX do not include boundary; 2 classes wont make much sense anyways

	auto it = max_element(secondDerivative.begin(), secondDerivative.end(),
		[](const pair<int, double>& p1, const pair<int, double>& p2) {
			return abs(p1.second) < abs(p2.second);
		}
	);

	if(_verbose)
		cout << "Found # of clusters to be " << it->first << endl;

	// TODO : this is always 3 !!!!!!!!!!!!!!!!!!!!!!!!!!
	return it->first;
}

typedef struct
{
	void* dataset;
	cv::Mat* input;
} callback_data_t;

int buildKmeansInput_callback(const IplImage*, int id, int frame,
		int x, int y, int, void* p)
{
	int r;
	char* errMsg = NULL;
	callback_data_t* data = (callback_data_t*)p;

	float* descriptor = NULL;
	size_t size = 0;
	if((r = dataset_read_sample_descriptor(&data->dataset, id,
		&descriptor, &size, &errMsg)) != 0)
	{
		cout << "Error: " << errMsg << endl;
		return r;
	}

	cv::Mat row(1, size+3, CV_32F);
	memcpy(row.ptr(), descriptor, size * sizeof(float));  // aw jee
	row.at<float>(0, size) = frame; // TODO : normalize somehow!!!
	row.at<float>(0, size+1) = x;
	row.at<float>(0, size+2) = y;

	data->input->push_back(row);

	free(descriptor);

	return 0;
}

int do_cluster(const string& datasetPath)
{
	void* dataset;
	char* errMsg;
	int r;
	if((r = dataset_open(&dataset, datasetPath.c_str(), 0, &errMsg)) != 0)
	{
		cerr << "Error: " << errMsg << endl;
		dataset_close(&dataset);
		return r;
	}

	cv::Mat input;  // for k-means
	callback_data_t data = { dataset, &input };
	if((r = dataset_read_samples(&dataset, "1=1", buildKmeansInput_callback,
			&data, &errMsg)) != 0)
	{
		cerr << "Error: " << errMsg << endl;
		dataset_close(&dataset);
		return r;
	}

	// 100 is hardcoded iterations maximum in OpenCV anyways
	const int    _maxIterations = 100;
	// TODO : 0.1???
	const double _maxAccuracy   = 0.1;

	int sampleCount = 0;
	if((r = dataset_sample_count(&dataset, &sampleCount, NULL, &errMsg)) != 0)
	{
		cerr << "Error: " << errMsg << endl;
		dataset_close(&dataset);
		return r;
	}
	if(sampleCount < _maxClasses)
	{
		cerr << "Error: there is too few samples to properly classify." << endl;
		return EXIT_FAILURE;
	}

	map<int, double> compactness;  // cluster count -> compactness
	for(int k = 1; k < _maxClasses; k++)
	{
		if(_verbose)
			cout << "\rApplying k-means to " << k << "/" <<
				_maxClasses << " clusters";
		cv::Mat output;
		compactness[k] = cv::kmeans(input, k, output, cv::TermCriteria(3,
			_maxIterations, _maxAccuracy), 1, cv::KMEANS_PP_CENTERS);
	}
	if(_verbose)
		cout << endl;

	cv::Mat output;
	cv::kmeans(input, determine_cluster_count(compactness), output,
			cv::TermCriteria(3, _maxIterations, _maxAccuracy),
			1, cv::KMEANS_PP_CENTERS);

	for(int i = 0; i < output.rows; i++)
	{
		if((r = dataset_update_sample_class(&dataset,
				//  both objects & classes are zero-based in OpenCV
				i + 1, output.at<int>(i, 0) + 1, &errMsg)) != 0)
		{
			cerr << "Error: " << errMsg << endl;
			dataset_close(&dataset);
			return r;
		}
	}

	dataset_close(&dataset);
	return 0;
}

