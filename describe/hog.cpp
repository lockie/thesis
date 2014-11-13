
#include <iostream>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include "datasetIO.h"

using namespace std;


int descr_callback(const IplImage* img, int id, int frame,
		int x, int y, int _class, void* dataset)
{
	// TODO : show progress
	// TODO : write own implementation?..
	cv::HOGDescriptor hog;

	cv::Mat ii;
	resize(cv::Mat(img), ii, cvSize(64, 128), 0, 0, CV_INTER_LANCZOS4);

	cv::Mat oo(64, 128, CV_8UC1);
	cv::cvtColor(ii, oo, CV_RGB2GRAY);

	vector<float> d;
	hog.compute(oo, d);

	char* errMsg;
	int r;
	if((r = dataset_update_sample_descriptor(&dataset, id,
			d.data(), d.size(), &errMsg)) != 0)
	{
		cerr << "Error: " << errMsg << endl;
		return r;
	}

	return 0;
}

extern "C" int describe_HOG(const char* dataset_path)
{
	int r;
	void* dataset = NULL;
	char* errMsg;
	if((r = dataset_open(&dataset, dataset_path, 0, &errMsg)) != 0)
	{
		cerr << "Error: " << errMsg << endl;
		dataset_close(&dataset);
		return r;
	}
	if((r = dataset_read_samples(&dataset, "1=1",
			descr_callback, dataset, &errMsg)) != 0)
	{
		cerr << "Error: " << errMsg << endl;
		dataset_close(&dataset);
		return r;
	}
	dataset_close(&dataset);
	return 0;
}

