
#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#include <limits.h>

#include <sqlite3.h>

#include <archive.h>
#include <archive_entry.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>


typedef struct dataset_t
{
	sqlite3* db;
	struct archive* ar_write;
	struct archive* ar_read;
	char* archive_path;  /* to reopen ar_read */
	struct archive_entry* ent;
	const char* path;
	int lastFrame;
	int frameObjectCounter;
	int (*read_callback)(const IplImage*, int id, int frame, int x, int y, void*);
	void* read_callback_parameter;
} dataset_t;

extern int _dataset_reopen_archive(dataset_t* dataset, int w, char** errMsg);

extern const char* tempFileName;

extern char query[8192];
extern char filename[PATH_MAX];

#endif  /* _INTERNAL_H_ */

