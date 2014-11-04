
#ifndef _DATASET_IO_H_
#define _DATASET_IO_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int dataset_create(void** dataset, const char* path,
	const char* title, char** errorMessage);
void dataset_close(void** dataset);
int dataset_create_sample(void** dataset, int frame, IplImage* image,
		const CvRect* bounds, char** errMsg);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _DATASET_IO_H_ */

