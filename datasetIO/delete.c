
#include "internal.h"


static int copy_callback(void* data, int argc, char** argv, char** colNames)
{
	int r;
	char* errMsg;
	dataset_t* dataset = data;

	const char* filename = argv[0];
	assert(argc == 1);

	if((r = _dataset_reopen_archive(dataset, 0, &errMsg)) != 0)
	{
		fprintf(stderr, "READ ERROR: %s\n", errMsg);
		return r;
	}

	while(archive_read_next_header(dataset->ar_read, &dataset->ent) == ARCHIVE_OK)
	{
		if(strcmp(archive_entry_pathname(dataset->ent), filename) == 0)
		{
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

			if((r = archive_write_header(dataset->ar_write, dataset->ent))
					!= ARCHIVE_OK)
			{
				fprintf(stderr, "WRITE ERROR: %s\n",
					/*TODO:leak*/
					archive_error_string(dataset->ar_write));
				free(buffer);
				return r;
			}

			if(archive_write_data(dataset->ar_write, buffer, size) < 0)
			{
				fprintf(stderr, "WRITE ERROR: %s\n",
					/*TODO:leak*/
					archive_error_string(dataset->ar_write));
				free(buffer);
				return r;
			}
			free(buffer);
			break;
		}
		archive_read_data_skip(dataset->ar_read);
	}

	return 0;
}

int dataset_delete_samples(void** _dataset, const char* predicate,
		char** errMsg)
{
	int r;
	dataset_t* dataset = *_dataset;

	if(!dataset->ar_read)
	{
		*errMsg = "Dataset is not open for reading";
		return -1;
	}
	if(!dataset->ar_write)
	{
		*errMsg = "Dataset is not open for writing";
		return -1;
	}

	snprintf(query, sizeof(query),
		"select filename from objects where not (%s);", predicate);
	if((r = sqlite3_exec(dataset->db, query,
			copy_callback, (void*)dataset, errMsg)) != SQLITE_OK)
		return r;

	snprintf(query, sizeof(query),
		"delete from objects where %s;", predicate);
	if((r = sqlite3_exec(dataset->db, query, NULL, NULL, errMsg)) != SQLITE_OK)
		return r;

	archive_write_close(dataset->ar_write);

	snprintf(filename, PATH_MAX, "/bin/cp -fp \'%s\' \'%s\'",
		tempFileName, dataset->archive_path);
	system(filename);

	_dataset_reopen_archive(dataset, 1, errMsg);

	return 0;
}

