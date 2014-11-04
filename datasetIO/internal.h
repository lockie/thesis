
#ifndef _INTERNAL_H_
#define _INTERNAL_H_

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
	int (*read_callback)(const IplImage*, int frame, int x, int y);
} dataset_t;

extern int _dataset_reopen_archive(dataset_t* dataset, char** errMsg);

#endif  /* _INTERNAL_H_ */

