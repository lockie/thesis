
#include <algorithm>
#include <string>
#include <iostream>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "datasetIO.h"

using namespace std;
using namespace cv;




int remove_weird_sizes(const std::string& path, bool verbose = false)
{
	void* dataset;
	char* errMsg;
	int r;
	if((r = dataset_open(&dataset, path.c_str(), 1, &errMsg)) != 0)
	{
		cerr << "Error: " << errMsg << endl;
		dataset_close(&dataset);
		return r;
	}

	int width, height;
	if((r = dataset_source_size(&dataset, &width, &height, &errMsg)) != 0)
	{
		cerr << "Error: " << errMsg << endl;
		dataset_close(&dataset);
		return r;
	}


	// TODO : TODO!

	return 0;
}

