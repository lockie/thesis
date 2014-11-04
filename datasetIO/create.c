
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "datasetIO.h"
#include "internal.h"


static char filename[PATH_MAX];

int dataset_create(void** _dataset, const char* path,
		const char* title, char** errorMessage)
{
	int r;
	dataset_t* dataset = malloc(sizeof(dataset_t));
	strncpy(filename, path, sizeof(filename));
	strncat(filename, "index.sqlite3", sizeof(filename) - strlen(path));
	memset(dataset, 0, sizeof(dataset_t));

	if((r = sqlite3_open(filename, &dataset->db)) != SQLITE_OK)
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

	strncpy(filename, path, sizeof(filename));
	strncat(filename, "samples.tar", sizeof(filename) - strlen(path));

	dataset->ar = archive_write_new();
	archive_write_set_format_pax_restricted(dataset->ar);
	if((r = archive_write_open_filename(dataset->ar, filename)) != ARCHIVE_OK)
	{
		*errorMessage = strdup(archive_error_string(dataset->ar)); /*TODO:leak*/
		dataset_close((void**)&dataset);
		return r;
	}
	dataset->ent = archive_entry_new();

	dataset->title = title;
	dataset->path  = path;

	*_dataset = dataset;
	return 0;
}

// TODO : dataset_open fn?

void dataset_close(void** _dataset)
{
	dataset_t* dataset = *_dataset;
	*_dataset = NULL;

	archive_write_close(dataset->ar);
	archive_write_free(dataset->ar);

	sqlite3_close(dataset->db);

	free(dataset);
}

static char outFileName[PATH_MAX];
static char query[8192];  /* 640k should be enough for everyone */
static IplImage* tmp = NULL;
static int compression[2] = {CV_IMWRITE_PNG_COMPRESSION, 9};

int dataset_create_sample(void** _dataset, int frame, IplImage* image,
		const CvRect* b, char** errMsg)
{
	int r;
	CvMat* mat;
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

	mat = cvEncodeImage(".png", tmp, &compression[0]);
	archive_entry_clear(dataset->ent);
	snprintf(outFileName, sizeof(outFileName), "%s_%d_%d.png",
		dataset->title, frame, ++dataset->frameObjectCounter);
	archive_entry_set_pathname(dataset->ent, outFileName);
	archive_entry_set_size(dataset->ent, mat->step);
	archive_entry_set_filetype(dataset->ent, AE_IFREG);
	archive_entry_set_perm(dataset->ent, 0644);
	if((r = archive_write_header(dataset->ar, dataset->ent)) != ARCHIVE_OK)
	{
		*errMsg = strdup(archive_error_string(dataset->ar)); /*TODO:leak*/
		return r;
	}
	if(archive_write_data(dataset->ar, mat->data.ptr, mat->step) < 0)
	{
		*errMsg = strdup(archive_error_string(dataset->ar)); /*TODO:leak*/
		return r;
	}

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

