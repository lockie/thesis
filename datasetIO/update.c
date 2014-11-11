
#include <stdlib.h>

#include "internal.h"


int dataset_update_sample(void** _dataset, int id,
		float* descriptor, size_t size, char** errMsg)
{
	int r;
	dataset_t* dataset = *_dataset;
	sqlite3_stmt* stmt;

	if((r = sqlite3_prepare_v2(dataset->db,
			"update objects set descriptor = ? where id = ?",
			-1, &stmt, 0)) != SQLITE_OK)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}

	if((r = sqlite3_bind_blob(stmt, 1, descriptor, size * sizeof(float),
			SQLITE_TRANSIENT)) != SQLITE_OK)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}
	if((r = sqlite3_bind_int (stmt, 2, id)) != SQLITE_OK)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}


	if((r = sqlite3_step(stmt)) != SQLITE_DONE)
	{
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}

	if((r = sqlite3_finalize(stmt)) != SQLITE_OK)
	{
		/* TODO : some sort of macro ._. */
		/* TODO : store prepared statement in dataset struct to optimize */
		*errMsg = (char*)sqlite3_errmsg(dataset->db);
		return r;
	}

	return 0;
}

