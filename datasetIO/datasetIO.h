
#ifndef _DATASET_IO_H_
#define _DATASET_IO_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef int (*read_sample_callback)(const IplImage*, int frame, int x, int y);

int dataset_create(void** dataset, const char* path, char** errorMessage);
int dataset_open(void** dataset, const char* path, char** errorMessage);
void dataset_close(void** dataset);
int dataset_create_sample(void** dataset, int frame, IplImage* image,
		const CvRect* bounds, char** errMsg);
int dataset_read_samples(void** dataset, const char* predicate,
		read_sample_callback callback, char** errMsg);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _DATASET_IO_H_ */

