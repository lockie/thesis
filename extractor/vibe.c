
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cv.h"
#include "highgui.h"

typedef struct
{
	int  nbSamples;
	int  reqMatches;
	unsigned radius;
	int  subsamplingFactor;

	IplImage** samples;  /* background model */
	IplImage* segmentationMap; /* foreground detection map */

	float* rand_cache;
	int rand_cache_size;
	int rand_cache_ptr;

	int width, height;

	/* postprocessing */
	IplConvKernel* morphology_kernel;
	CvMemStorage* storage;  /* for contours */
	void* result_buffer;
	size_t result_buffer_size;
} vibe_t;


#define SQUARE(x) ((x)*(x))

inline static unsigned fast_abs(const int8_t x)
{
	const int8_t mask = x >> (sizeof(int8_t) * CHAR_BIT - 1);
	return (x + mask) ^ mask;
}

inline static unsigned getEuclideanDist(const uint8_t* im1, const uint8_t* im2)
{
	return fast_abs((int8_t)*im1 - *im2);
}

inline static void setPixelBackground(vibe_t* p, int x, int y)
{
	CV_IMAGE_ELEM(p->segmentationMap, uint8_t, y, x) = 0;
}

inline static void setPixelForeground(vibe_t* p, int x, int y)
{
	CV_IMAGE_ELEM(p->segmentationMap, uint8_t, y, x) = UINT8_MAX;
}

inline static int getRandomNumber(vibe_t* p, int start, int end)
{
	float r = p->rand_cache[p->rand_cache_ptr++];
	if(p->rand_cache_ptr == p->rand_cache_size)
	{
#ifndef NDEBUG
		fprintf(stderr, "Random cache empty\n");
#endif  /* NDEBUG */
		p->rand_cache_ptr = 0;
	}
	return round(start + (end-start) * r);
}

static void chooseRandomNeighbor(vibe_t* p, int x, int y, int* neighborX, int* neighborY)
{
	int deltax = 0, deltay = 0;
	while(deltax == 0 && deltay == 0)
	{
		deltax = getRandomNumber(p, x == 0 ? 0 : -1, x == p->width-1  ? 0 : 1);
		deltay = getRandomNumber(p, y == 0 ? 0 : -1, y == p->height-1 ? 0 : 1);
	}
	*neighborX = x + deltax;
	*neighborY = y + deltay;
	if(*neighborX < 0)
		*neighborX = 0;
	if(*neighborY < 0)
		*neighborY = 0;
}

vibe_t* init_vibe(const IplImage* image)
{
	int i, size;
	vibe_t* res = malloc(sizeof(vibe_t));

	res->nbSamples = 20;
	res->reqMatches = 2;
	res->radius = 20;
	res->subsamplingFactor = 16;

	res->width  = image->width;
	res->height = image->height;

	res->samples = malloc(sizeof(IplImage) * res->nbSamples);
	for(i = 0; i < res->nbSamples; i++)
	{
		res->samples[i] = cvCloneImage(image);
		cvSmooth(res->samples[i], res->samples[i], CV_MEDIAN, 3, 3, 0, 0);
	}

	res->segmentationMap = cvCreateImage(cvGetSize(image), 8, 1);
	cvZero(res->segmentationMap);

	srand(time(NULL));
	res->rand_cache_size = 65535 * 1024;
	res->rand_cache_ptr = 0;
	res->rand_cache = malloc(res->rand_cache_size*sizeof(res->rand_cache[0]));
	for(i = 0; i < res->rand_cache_size; i++)
		res->rand_cache[i] = (float)rand() / RAND_MAX;

	/* TODO : some smarter kernel size heuristic */
	size = res->width > 600 ? 5 : 3;
	res->morphology_kernel =
		cvCreateStructuringElementEx(size, size, size/2, size/2,
			0/*MORPH_RECT*/, NULL);
	res->storage = cvCreateMemStorage(0);

	res->result_buffer_size = 0;
	res->result_buffer = NULL;

#ifndef NDEBUG
	cvNamedWindow("raw vibe result", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("background model", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("post-processed vibe result", CV_WINDOW_AUTOSIZE);
#endif  /* !NDEBUG */

	return res;
}

/* see O. Barnich, M. van Droogenbroeck. ViBe: A universal background
 * subtraction algorithm for video sequences
 */
void do_vibe(vibe_t** vibestruct, const IplImage* image)
{
	int x, y, index, randomNumber;
	vibe_t* p;

	assert(image->nChannels == 1);
	assert(image->depth == 8);

	if(*vibestruct == NULL)
		*vibestruct = init_vibe(image);
	p = *vibestruct;

	assert(p->width == image->width && p->height == image->height);

	for(x = 0; x < image->width; x++)
	{
		for(y = 0; y < image->height; y++)
		{
			/* comparison with the model */
			const uint8_t* imgptr = &CV_IMAGE_ELEM(image, uint8_t, y, x);
			int count = 0;
			index = 0;
			while((count < p->reqMatches) && (index < p->nbSamples))
			{
				uint8_t* sptr = &CV_IMAGE_ELEM(p->samples[index], uint8_t, y, x);
				unsigned distance = getEuclideanDist(imgptr, sptr);
				if(distance < p->radius)
					count++;
				index++;
			}

			/* pixel classification according to reqMatches */
			if(count >= p->reqMatches)
			{
				/* the pixel belongs to the background
				 * stores the result in the segmentation map */
				setPixelBackground(p, x, y);
				randomNumber = getRandomNumber(p, 0, p->subsamplingFactor-1);
				/* update of the current pixel model */
				if(randomNumber == 0)
				{
					/* random subsampling
					 * other random values are ignored */
					randomNumber = getRandomNumber(p, 0, p->nbSamples-1);
					CV_IMAGE_ELEM(p->samples[randomNumber], uint8_t, y, x) =
						CV_IMAGE_ELEM(image, uint8_t, y, x);
				}
				/* update of a neighboring pixel model */
				randomNumber = getRandomNumber(p, 0, p->subsamplingFactor-1);
				if(randomNumber == 0)
				{
					/* random subsampling
					 * chooses a neighboring pixel randomly */
					int neighborX; int neighborY;
					chooseRandomNeighbor(p, x, y, &neighborX, &neighborY);
					/* chooses the value to be replaced randomly */
					randomNumber = getRandomNumber(p, 0, p->nbSamples-1);
					CV_IMAGE_ELEM(p->samples[randomNumber], uint8_t, neighborY, neighborX) =
						CV_IMAGE_ELEM(image, uint8_t, y, x);
				}
			}
			else /* the pixel belongs to the foreground
				  * stores the result in the segmentation map */
				setPixelForeground(p, x, y);
		}
	}

#ifndef NDEBUG
	cvShowImage("raw vibe result", p->segmentationMap);
	cvShowImage("background model", p->samples[5]);
#endif  /* !NDEBUG */
}

/* see N. Lu et al. An Improved Motion Detection Method for
 * Real-Time Surveillance
 *
 * see A. Amer. New binary morphological operations for effective low-cost
 * boundary detection.
 */
CvRect* postprocess_vibe(vibe_t* p)
{
	IplImage* segMap = cvCloneImage(p->segmentationMap);
	CvSeq* contour = NULL;
	CvRect* res;
	int i = 0, n;

	cvMorphologyEx(segMap, segMap, NULL, p->morphology_kernel, CV_MOP_ERODE, 1);
	cvMorphologyEx(segMap, segMap, NULL, p->morphology_kernel, CV_MOP_GRADIENT, 1);
	cvMorphologyEx(segMap, segMap, NULL, p->morphology_kernel, CV_MOP_DILATE, 4);


#ifndef NDEBUG
	cvShowImage("post-processed vibe result", segMap);
#endif  /* !NDEBUG */

	cvClearMemStorage(p->storage);
	n = cvFindContours(segMap, p->storage, &contour, sizeof(CvContour), CV_RETR_TREE,
		CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

	cvReleaseImage(&segMap);

	if(!contour)
		return NULL;

	if((n+1) * sizeof(CvRect) > p->result_buffer_size)
	{
		p->result_buffer_size = (n+1) * sizeof(CvRect);
		p->result_buffer = realloc(p->result_buffer, p->result_buffer_size);
	}
	memset(p->result_buffer, 0, p->result_buffer_size);

	res = p->result_buffer;
	for(; contour; contour = contour->h_next)
	{
		CvRect r = cvBoundingRect(contour, 0);
		/* border effects :3 */
		r.x -= 5; if(r.x < 0) r.x = 0;
		r.y -= 5; if(r.y < 0) r.y = 0;
		r.width  += 5; if(r.x+r.width  > p->width)  r.width  = p->width-r.x;
		r.height += 5; if(r.y+r.height > p->height) r.height = p->height-r.y;

		if(r.width != 0 && r.height != 0)
			res[i++] = r;
	}
	return res;
}

void finalize_vibe(vibe_t** handle)
{
	int i;
	vibe_t* p = *handle;
	*handle = NULL;

	cvDestroyWindow("post-processed vibe result");
	cvDestroyWindow("background model");
	cvDestroyWindow("raw vibe result");

	p->result_buffer_size = 0;
	free(p->result_buffer);
	p->result_buffer = NULL;

	cvReleaseMemStorage(&p->storage);
	cvReleaseStructuringElement(&p->morphology_kernel);

	p->rand_cache_size = p->rand_cache_ptr = 0;
	free(p->rand_cache);
	p->rand_cache = NULL;

	cvReleaseImage(&p->segmentationMap);

	for(i = 0; i < p->nbSamples; i++)
		cvReleaseImage(&p->samples[i]);
	free(p->samples);
	p->samples = NULL;

	p->width = p->height = 0;

	free(p);
}

