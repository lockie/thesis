
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
		fprintf(stderr, "READ ERROR: sample file missing\n");
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

