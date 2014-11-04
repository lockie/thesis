
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

#include <cv.h>
#include <highgui.h>

#include "datasetIO.h"
#include "vibe.h"


int main(int argc, char** argv)
{
	const char* fileName = argc == 2 ? argv[1] : "video";
	char* fn = strdup(fileName);
	/*const char* baseName = basename(fn);*/

	// TODO : image ROI parameter for that Kristiinankaupunki video!

	unsigned frameCntr = 0, objectCntr = 0;
	IplImage* img = NULL;
	void* vibe = NULL;

	void* dataset = NULL;
	char* errMsg = NULL;

	CvCapture* capture = cvCreateFileCapture(fileName);
	if(!capture)
	{
		fprintf(stderr, "ERROR: failed to open videofile\n");
		return EXIT_FAILURE;
	}

	/* TODO: command-line parameter for output dir  */
	if(dataset_create(&dataset, "", &errMsg) != 0)
	{
		fprintf(stderr, "ERROR: %s\n", errMsg);
		cvReleaseCapture(&capture);
		return EXIT_FAILURE;
	}

#ifndef NDEBUG
	cvNamedWindow("video", CV_WINDOW_AUTOSIZE);
#endif  /* !NDEBUG */

	while(1)
	{
		CvRect *result, *r;
		IplImage* frame = cvQueryFrame(capture);
		if(!frame) break;
		if(!img) img  = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);

		cvCvtColor(frame, img, CV_RGB2GRAY);

		do_vibe(&vibe, img);
		r = result = postprocess_vibe(vibe);

		for(; r && r->height != 0; r++)
		{
			objectCntr++;
			if(dataset_create_sample(&dataset, frameCntr, frame, r, &errMsg) != 0)
			{
				fprintf(stderr, "ERROR: %s\n", errMsg);
				break;
			}
		}

#ifndef NDEBUG
		for(; result && result->width != 0; result++)
			cvRectangle(frame,
				cvPoint(result->x, result->y),
				cvPoint(result->x+result->width, result->y+result->height),
				CV_RGB(128, 255, 0),
				1, CV_AA, 0
			);
		cvShowImage("video", frame);
#endif  /* !NDEBUG */

#ifdef NDEBUG
# define WAIT_DELAY 10
#else
# define WAIT_DELAY 0
#endif  /* NDEBUG */
		if((cvWaitKey(WAIT_DELAY) & 0xff) == 27) break;

		fprintf(stdout, "\r%u frames processed, %u objects extracted",
			frameCntr++, objectCntr);
	}

	finalize_vibe(&vibe);

	dataset_close(&dataset);

	cvReleaseCapture(&capture);
#ifndef NDEBUG
	cvDestroyAllWindows();
#endif  /* !NDEBUG */

	free(fn);

	return EXIT_SUCCESS;
}

