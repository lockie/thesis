
#include "internal.h"
#include "datasetIO.h"


static int read_callback(void* data, int argc, char** argv, char** colNames)
{
	int r;
	char* errMsg;
	dataset_t* dataset = data;
	IplImage* img = NULL;

	const char* filename = argv[0];
	const char* id       = argv[1];
	const char* frame    = argv[2];
	const char* x        = argv[3];
	const char* y        = argv[4];
	const char* class    = argv[5] ? argv[5] : "";
	assert(argc == 6);

	if((r = _dataset_reopen_archive(dataset, 0, &errMsg)) != 0)
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
				free(buffer);
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
		fprintf(stderr,
				"READ ERROR: sample file with id=%s missing or corrupt\n", id);
		return -1;
	}
	return dataset->read_callback(img, atoi(id), atoi(frame),
		atoi(x), atoi(y), atoi(class), dataset->read_callback_parameter);
}

int dataset_read_samples(void** _dataset, const char* predicate,
		read_sample_callback callback, void* parameter, char** errMsg)
{
	int r;
	dataset_t* dataset = *_dataset;
	if(!dataset->ar_read)
	{
		*errMsg = "Dataset is not open for reading";
		return -1;
	}
	dataset->read_callback = callback;
	dataset->read_callback_parameter = parameter;

	snprintf(query, sizeof(query),
		"select filename, id, frame, x, y, class from objects where %s;", predicate);
	if((r = sqlite3_exec(dataset->db, query,
			read_callback, (void*)dataset, errMsg)) != SQLITE_OK)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		dataset->read_callback = NULL;
		return r;
	}

	dataset->read_callback = NULL;
	dataset->read_callback_parameter = NULL;
	return 0;
}

int dataset_read_sample_descriptor(void** _dataset, int id,
		float** descriptor, size_t* size, char** errMsg)
{
	int r;
	size_t oldsize = *size;
	dataset_t* dataset = *_dataset;
	sqlite3_stmt* stmt;

	if((r = sqlite3_prepare_v2(dataset->db,
			"select descriptor from objects where id = ?",
			-1, &stmt, 0)) != SQLITE_OK)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}

	if((r = sqlite3_bind_int (stmt, 1, id)) != SQLITE_OK)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}

	if((r = sqlite3_step(stmt)) != SQLITE_ROW)
	{
		/* TODO : some sort of macro ._. */
		/* TODO : store prepared statement in dataset struct to optimize? */
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}

	*size = sqlite3_column_bytes(stmt, 0);
	if(oldsize * sizeof(float) < *size)
		*descriptor = realloc(*descriptor, *size);
	memcpy(*descriptor, sqlite3_column_blob(stmt, 0), *size);
	*size /= sizeof(float);

	sqlite3_finalize(stmt);
	return 0;
}

int dataset_sample_count(void** _dataset, int* count, char** errMsg)
{
	int r;
	dataset_t* dataset = *_dataset;
	sqlite3_stmt* stmt;

	if((r = sqlite3_prepare_v2(dataset->db,
			"select count(1) from objects",
			-1, &stmt, 0)) != SQLITE_OK)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}

	if((r = sqlite3_step(stmt)) != SQLITE_ROW)
	{
		/* TODO : some sort of macro ._. */
		/* TODO : store prepared statement in dataset struct to optimize? */
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}

	*count = sqlite3_column_int(stmt, 0);

	sqlite3_finalize(stmt);
	return 0;
}

int dataset_source_size(void** _dataset, int* width, int* height, char** errMsg)
{
	int r;
	dataset_t* dataset = *_dataset;
	sqlite3_stmt* stmt = NULL;

	if(dataset->width != 0 && dataset->height != 0)
	{
		*width  = dataset->width;
		*height = dataset->height;
		return 0;
	}

	if((r = sqlite3_prepare_v2(dataset->db,
			"select width, height from metadata",
			-1, &stmt, 0)) != SQLITE_OK)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}
	if((r = sqlite3_step(stmt)) != SQLITE_ROW)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}
	dataset->width  = *width  = sqlite3_column_int(stmt, 0);
	dataset->height = *height = sqlite3_column_int(stmt, 1);
	sqlite3_finalize(stmt);
	return 0;
}

int dataset_minimum_size(void** _dataset, int* min_width,
		int* min_height, char** errMsg)
{
	int r;
	dataset_t* dataset = *_dataset;
	sqlite3_stmt* stmt = NULL;

	if(dataset->min_width != 0 && dataset->min_height != 0)
	{
		*min_width  = dataset->min_width;
		*min_height = dataset->min_height;
		return 0;
	}

	/* TODO : make 1 stmt */
	if((r = sqlite3_prepare_v2(dataset->db,
			"select min(width) from objects",
			-1, &stmt, 0)) != SQLITE_OK)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}
	if((r = sqlite3_step(stmt)) != SQLITE_ROW)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}
	dataset->min_width = *min_width = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	stmt = NULL;

	if((r = sqlite3_prepare_v2(dataset->db,
			"select min(height) from objects",
			-1, &stmt, 0)) != SQLITE_OK)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}
	if((r = sqlite3_step(stmt)) != SQLITE_ROW)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}
	dataset->min_height = *min_height = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	stmt = NULL;

	return 0;
}

