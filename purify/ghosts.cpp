
#include <cstdio>
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


static const int GHOST_CLASS = 666;  // actually makes sense in some weird way
static const int FILTER_SIZE = 7;

struct fill_coords_callback_param_t
{
	Mat* m;
};

int fill_coords_callback(int, int, int x, int y, int, int, int, void* _param)
{
	fill_coords_callback_param_t* param = (fill_coords_callback_param_t*)_param;
	param->m->at<float>(y, x) += 1;
	return 0;
}

struct set_class_callback_param_t
{
	void* dataset;
};

int set_class_callback(int id, int, int, int, int, int,
		int _class, void* _param)
{
	char* errMsg;
	int r;
	set_class_callback_param_t* param = (set_class_callback_param_t*)_param;

	if(_class != GHOST_CLASS)
		if((r = dataset_update_sample_class(&param->dataset, id,
				GHOST_CLASS, &errMsg)) != 0)
		{
			cerr << "Error: " << errMsg << endl;
			return r;
		}

	return 0;
}

struct show_callback_param_t
{
	int frame_count;  // fuck opencv and its CV_CAP_PROP_POS_FRAMES
	VideoCapture* cap;
	Mat frame;
};

int show_callback(int id, int frame, int x, int y,
		int width, int height, int _class, void* _param)
{
	show_callback_param_t* param = (show_callback_param_t*)_param;

	Mat output;
	if(param->frame_count >= frame)
		output = param->frame.clone();
	while(param->frame_count <= frame)
	{
		*param->cap >> output;
		param->frame_count++;
	}
	param->frame = output.clone();

	Rect bound(x, y, width, height);
	rectangle(output, bound, CV_RGB(255, 0, 0));

	imshow("bad sample", output);

	printf("frame %d, %d x %d @ (%d, %d)\n",
		frame, width, height, x, y);

	if((cvWaitKey(0) & 0xff) == 27)
		return -1;
	return 0;
}

static char predicate[1024];

int remove_ghosts(const string& path, bool verbose = false)
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

	Mat samples(height, width, CV_32F, Scalar(0));
	fill_coords_callback_param_t param = { &samples };
	if((r = dataset_read_samples_metadata(&dataset, NULL,
			fill_coords_callback, &param, &errMsg)) != 0)
	{
		std::cerr << "Error: " << errMsg << std::endl;
		dataset_close(&dataset);
		return r;
	}

	int Nsamples;
	if((r = dataset_sample_count(&dataset, &Nsamples, NULL, &errMsg)) != 0)
	{
		std::cerr << "Error: " << errMsg << std::endl;
		dataset_close(&dataset);
		return r;
	}

	Mat ghosts;
	//medianblur(samples, ghosts, 5);  // XXX: 3? inverse median? gaussian blur? bilateral? SOBEL! laplace?

//	GaussianBlur(samples, ghosts, Size(FILTER_SIZE, FILTER_SIZE), 0);
//	threshold(ghosts, ghosts, 1, 1, thresh_binary);

	/*
	double gmin, gmax;
	minMaxLoc(ghosts, &gmin, &gmax);
	Mat tmp;
	ghosts.convertTo(tmp, CV_8UC1, 255. / gmax);
	threshold(tmp, tmp, 0, 255, THRESH_OTSU);
	ghosts = tmp;
	*/

	//medianBlur(samples, ghosts, 5);

/*	GaussianBlur(samples, ghosts, Size(3, 3), 0);
	ghosts = ghosts.mul(samples);
	threshold(ghosts, ghosts, 1, 1, THRESH_BINARY);
	*/
	bilateralFilter(samples, ghosts, 3, 10., 0.);
	threshold(ghosts, ghosts, 1, 1, THRESH_BINARY);


#if 0
	int size = 1;
	Mat element = getStructuringElement(MORPH_RECT, /*MORPH_CROSS*/ /*MORPH_ELLIPSE*/
		Size( 2*size + 1, 2*size+1 ),
		Point( size, size ) );
	dilate(ghosts, ghosts, element);
#endif // 0


#if 0
	GaussianBlur(samples, ghosts, Size(3, 3), 0);
	int scale = 1;
	int delta = 0;
	Mat grad_x, grad_y;
	// Gradient X
	Sobel(ghosts, grad_x, -1, 1, 0, 3, scale, delta, BORDER_DEFAULT);
	// Gradient Y
	Sobel(ghosts, grad_y, -1, 0, 1, 3, scale, delta, BORDER_DEFAULT);
	Mat res;
	addWeighted(grad_x, 0.5, grad_y, 0.5, 0, res);
	ghosts = res.clone();
	threshold(ghosts, ghosts, 1, 1, THRESH_BINARY);
#endif  // 0

	//subtract(Scalar::all(float(width*height) / Nsamples), samples, ghosts);
	
	//Sobel(ghosts, ghosts, -1, 1, 1/*, CV_SCHARR*/);
#ifndef NDEBUG
	imshow("sample distribution", samples);
	imshow("ghosts distribution", ghosts);
	cvWaitKey(0);
#endif  // NDEBUG

	set_class_callback_param_t param2 = { dataset };
	if((r = dataset_begin_update(&dataset, &errMsg)) != 0)
	{
		cerr << "Error: " << errMsg << endl;
		dataset_close(&dataset);
		return r;
	}
	for(int y = FILTER_SIZE; y < height-FILTER_SIZE; y++)
		for(int x = FILTER_SIZE; x < width-FILTER_SIZE; x++)
		/* border effects again :3 */
		{
			if(ghosts.at<float>(y, x) > 0)
			{
				snprintf(predicate, sizeof(predicate), "x=%d and y=%d", x, y);
				if((r = dataset_read_samples_metadata(&dataset, predicate,
						set_class_callback, &param2, &errMsg)) != 0)
				{
					cerr << "Error: " << errMsg << endl;
					dataset_close(&dataset);
					return r;
				}
			}
		}
	if((r = dataset_end_update(&dataset, &errMsg)) != 0)
	{
		cerr << "Error: " << errMsg << endl;
		dataset_close(&dataset);
		return r;
	}


	int Nghosts;
	snprintf(predicate, sizeof(predicate), "class=%d", GHOST_CLASS);
	if((r = dataset_sample_count(&dataset, &Nghosts, predicate, &errMsg)) != 0)
	{
		std::cerr << "Error: " << errMsg << std::endl;
		dataset_close(&dataset);
		return r;
	}
	if(verbose)
		cout << Nghosts << " / " << Nsamples << " ghosts found" << endl;

	if(verbose)
	{
		snprintf(predicate, sizeof(predicate), "class=%d order by frame",
				// little SQL injection :3
				GHOST_CLASS);

		VideoCapture c(path + "/video");
		assert(c.isOpened());
		show_callback_param_t param3 = { 0, &c };
		if((r = dataset_read_samples_metadata(&dataset, predicate,
				show_callback, &param3, &errMsg)) != 0 && r != QUERY_ABORT)
		{
			cerr << "Error: " << errMsg << endl;
			dataset_close(&dataset);
			return r;
		}
	}

	cout << "ACTUALLY DELETING!!!1" <<endl;

	snprintf(predicate, sizeof(predicate), "class is %d", GHOST_CLASS);
	if((r = dataset_delete_samples(&dataset, predicate, &errMsg)) != 0)
	{
		cerr << "Error: " << errMsg << endl;
		dataset_close(&dataset);
		return r;
	}

	dataset_close(&dataset);
	return 0;
}

