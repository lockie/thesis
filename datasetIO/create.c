
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "internal.h"


static char db_filename[PATH_MAX];

int dataset_create(void** _dataset, const char* path,
		const char* title, char** errorMessage)
{
	int r;
	dataset_t* dataset = malloc(sizeof(dataset_t));
	strncpy(db_filename, path, sizeof(db_filename));
	strncat(db_filename, "index.sqlite3", sizeof(db_filename) - strlen(path));
	memset(dataset, 0, sizeof(dataset_t));

	if((r = sqlite3_open(db_filename, &dataset->db)) != SQLITE_OK)
	{
		*errorMessage = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}
	if((r = sqlite3_exec(dataset->db,
			"create table objects("
				"id integer primary key autoincrement not null, "
				"frame integer not null, "
				"x integer not null, y integer not null, "
				"width integer not null, height integer not null, "
				"descriptor blob, "
				"filename text unique not null);",
			NULL, NULL, errorMessage)) != SQLITE_OK)
		return r;

	dataset->title = title;
	dataset->path  = path;

	*_dataset = dataset;
	return 0;
}

void dataset_close(void** _dataset)
{
	dataset_t* dataset = *_dataset;
	*_dataset = NULL;

	sqlite3_close(dataset->db);

	free(dataset);
}

static char outFileName[PATH_MAX];
static char query[8192];  /* 640k should be enough for everyone */
static IplImage* tmp = NULL;

int dataset_create_sample(void** _dataset, int frame, IplImage* image,
		const CvRect* b, char** errMsg)
{
	int r;
	dataset_t* dataset = *_dataset;
	if(dataset->lastFrame != frame)
	{
		dataset->lastFrame = frame;
		dataset->frameObjectCounter = 0;
	}

	if(tmp)
		cvReleaseImage(&tmp);
	tmp = cvCreateImage(cvSize(b->width, b->height),
		image->depth, image->nChannels);
	cvSetImageROI(image, *b);
	cvCopyImage(image, tmp);

	// TODO : compress in tar archive via libtar/libarchive ?
	snprintf(outFileName, sizeof(outFileName), "%s/%s_%d_%d.png",
		dataset->path, dataset->title, frame, ++dataset->frameObjectCounter);
	cvSaveImage(outFileName, tmp, 0);

	cvResetImageROI(image);
	cvReleaseImage(&tmp);

	snprintf(query, sizeof(query),
		"insert into objects (frame, x, y, width, height, filename)"
		"values (%d, %d, %d, %d, %d, '%s');",
		frame, b->x, b->y, b->width, b->height, outFileName
	);
	if((r = sqlite3_exec(dataset->db, query, NULL, NULL, errMsg)) != SQLITE_OK)
		return r;

	return 0;
}

