
#include "internal.h"
#include "datasetIO.h"


static char query[8192];  /* 640k should be enough for everyone */

static int read_callback(void* data, int argc, char** argv, char** colNames)
{
	int r;
	char* errMsg;
	dataset_t* dataset = data;
	IplImage* img = NULL;

	const char* filename = argv[0];
	const char* frame    = argv[1];
	const char* x        = argv[2];
	const char* y        = argv[3];
	assert(argc == 4);

	if((r = _dataset_reopen_archive(dataset, &errMsg)) != 0)
	{
		fprintf(stderr, "READ ERROR: %s\n", errMsg);
		return r;
	}

	while(archive_read_next_header(dataset->ar_read, &dataset->ent) == ARCHIVE_OK)
	{
		if(strcmp(archive_entry_pathname(dataset->ent), filename) == 0)
		{
			CvMat* mat;
			int64_t size = archive_entry_size(dataset->ent);
			uchar* buffer = malloc(size);
			r = archive_read_data(dataset->ar_read, buffer, size);
			if(r < 0)
			{
				fprintf(stderr, "READ ERROR: %s\n",
					archive_error_string(dataset->ar_read));
				return r;
			}
			assert(r == size);

			mat = cvCreateMat(1, size, CV_8UC1);
			memcpy(mat->data.ptr, buffer, size);
			free(buffer);
			img = cvDecodeImage(mat, CV_LOAD_IMAGE_COLOR);
			cvReleaseMat(&mat);
			break;
		}
		archive_read_data_skip(dataset->ar_read);
	}
	if(!img)
	{
		/* inconsistent DB :E */
		fprintf(stderr, "READ ERROR: sample file missing\n");
		return -1;
	}
	return dataset->read_callback(img, atoi(frame), atoi(x), atoi(y));
}

int dataset_read_samples(void** _dataset, const char* predicate,
		read_sample_callback callback, char** errMsg)
{
	int r;
	dataset_t* dataset = *_dataset;
	if(!dataset->ar_read)
	{
		*errMsg = "Dataset is not open for reading";
		return -1;
	}
	dataset->read_callback = callback;

	snprintf(query, sizeof(query),
		"select filename, frame, x, y from objects where %s;", predicate);
	if((r = sqlite3_exec(dataset->db, query,
			read_callback, (void*)dataset, errMsg)) != SQLITE_OK)
	{
		dataset->read_callback = NULL;
		return r;
	}

	dataset->read_callback = NULL;
	return 0;
}

