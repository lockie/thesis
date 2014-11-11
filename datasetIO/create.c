
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"


const char* tempFileName = "/tmp/dataset.tar";

char filename[PATH_MAX];

int dataset_create(void** _dataset, const char* path, char** errorMessage)
{
	int r;
	dataset_t* dataset = malloc(sizeof(dataset_t));
	strncpy(filename, path, sizeof(filename));
	strncat(filename, "/", sizeof(filename) - strlen(path));
	strncat(filename, "index.sqlite3", sizeof(filename) - strlen(path) - 1);
	memset(dataset, 0, sizeof(dataset_t));
	*_dataset = dataset;

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
	strncat(filename, "/", sizeof(filename) - strlen(path));
	strncat(filename, "samples.tar", sizeof(filename) - strlen(path) - 1);

	dataset->ar_write = archive_write_new();
	archive_write_set_format_pax_restricted(dataset->ar_write);
	if((r = archive_write_open_filename(dataset->ar_write,
			filename)) != ARCHIVE_OK)
	{
		*errorMessage = strdup(archive_error_string(dataset->ar_write)); /*TODO:leak*/
		return r;
	}
	dataset->ent = archive_entry_new();

	dataset->path  = path;

	return 0;
}

/* internal use only */
int _dataset_reopen_archive(dataset_t* dataset, int write, char** errMsg)
{
	int r;

	if(write)
	{
		/* in this case file opened R/W, so open empty tar file for writing
		 *  for further copying it back */
		if(dataset->ar_write)
			archive_write_free(dataset->ar_write);
		dataset->ar_write = archive_write_new();
		archive_write_set_format_pax_restricted(dataset->ar_write);
		if((r = archive_write_open_filename(dataset->ar_write,
			tempFileName)) != ARCHIVE_OK)
		{
			*errMsg = strdup(archive_error_string(dataset->ar_write));
			/*TODO:leak*/
			return r;
		}
	}

	if(dataset->ar_read)
		archive_read_free(dataset->ar_read);
	dataset->ar_read = archive_read_new();
	archive_read_support_format_tar(dataset->ar_read);
	if ((r = archive_read_open_filename(dataset->ar_read,
			dataset->archive_path, 16384)) != ARCHIVE_OK)
	{
		*errMsg = strdup(archive_error_string(dataset->ar_read)); /*TODO:leak*/
		return r;
	}
	return 0;
}

int dataset_open(void** _dataset, const char* path, int write, char** errMsg)
{
	int r;
	dataset_t* dataset = malloc(sizeof(dataset_t));
	strncpy(filename, path, sizeof(filename));
	strncat(filename, "/", sizeof(filename) - strlen(path));
	strncat(filename, "index.sqlite3", sizeof(filename) - strlen(path) - 1);
	memset(dataset, 0, sizeof(dataset_t));
	*_dataset = dataset;

	if((r = sqlite3_open(filename, &dataset->db)) != SQLITE_OK)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}

	strncpy(filename, path, sizeof(filename));
	strncat(filename, "/", sizeof(filename) - strlen(path));
	strncat(filename, "samples.tar", sizeof(filename) - strlen(path) - 1);
	dataset->archive_path = strdup(filename);

	if((r = _dataset_reopen_archive(dataset, write, errMsg)) != 0)
		return r;

	dataset->path  = path;

	return 0;
}

void dataset_close(void** _dataset)
{
	dataset_t* dataset = *_dataset;
	*_dataset = NULL;

	if(dataset->ar_write)
	{
		archive_write_close(dataset->ar_write);
		archive_write_free(dataset->ar_write);
	}
	if(dataset->ar_read)
	{
		archive_read_free(dataset->ar_read);
	}
	free(dataset->archive_path);

	sqlite3_close(dataset->db);

	free(dataset);
}

static char outFileName[PATH_MAX];
/* TODO : replace query string with sqlite_exec_printf */
char query[8192];  /* 640k should be enough for everyone */
static IplImage* tmp = NULL;
static int compression[2] = {CV_IMWRITE_PNG_COMPRESSION, 9};

int dataset_create_sample(void** _dataset, int frame, IplImage* image,
		const CvRect* b, char** errMsg)
{
	int r;
	CvMat* mat;
	dataset_t* dataset = *_dataset;
	if(!dataset->ar_write)
	{
		*errMsg = "Dataset is not open for writing";
		return -1;
	}
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
	snprintf(outFileName, sizeof(outFileName), "sample_%d_%d.png",
		frame, ++dataset->frameObjectCounter);
	archive_entry_set_pathname(dataset->ent, outFileName);
	archive_entry_set_size(dataset->ent, mat->step);
	archive_entry_set_filetype(dataset->ent, AE_IFREG);
	archive_entry_set_perm(dataset->ent, 0644);
	if((r = archive_write_header(dataset->ar_write, dataset->ent)) != ARCHIVE_OK)
	{
		*errMsg = strdup(archive_error_string(dataset->ar_write)); /*TODO:leak*/
		return r;
	}
	if(archive_write_data(dataset->ar_write, mat->data.ptr, mat->step) < 0)
	{
		*errMsg = strdup(archive_error_string(dataset->ar_write)); /*TODO:leak*/
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

