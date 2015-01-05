
#ifndef _DATASET_IO_H_
#define _DATASET_IO_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define QUERY_ABORT 4
/* SQLITE_ABORT */

typedef int (*read_sample_callback)(const IplImage*, int id, int frame,
		int x, int y, int _class, void* data);

int dataset_create(void** dataset, const char* path, char** errorMessage);
int dataset_open(void** dataset, const char* path, int write, char** errorMessage);
void dataset_close(void** dataset);
int dataset_create_sample(void** dataset, int frame, IplImage* image,
		const CvRect* bounds, char** errMsg);
int dataset_read_samples(void** dataset, const char* predicate,
		read_sample_callback callback, void* param, char** errorMessage);
// TODO : dataset_read_samples_metadata fn (w/out actual image reading)
int dataset_read_sample_descriptor(void** _dataset, int id,
		float** descriptor, size_t* size, char** errMsg);
int dataset_sample_count(void** dataset, int* count, char** errMsg);
int dataset_minimum_size(void** dataset, int* min_width,
		int* min_height, char** errMsg);
int dataset_update_sample_descriptor(void** dataset, int id,
/*  size is in elements, not bytes */
		float* descriptor, size_t size, char** errorMessage);
int dataset_update_sample_class(void** _dataset, int id,
		int _class, char** errMsg);
int dataset_delete_samples(void** dataset, const char* predicate,
		char** errorMessage);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _DATASET_IO_H_ */

