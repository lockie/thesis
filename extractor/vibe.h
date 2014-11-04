
#ifndef _VIBE_H_
#define _VIBE_H_

#include "cv.h"

extern void* init_vibe(const IplImage* first_frame);
extern void do_vibe(void** vibe_handle, const IplImage* frame);
extern CvRect* postprocess_vibe(void* vibe_handle);
extern void finalize_vibe(void** vibe_handle);

#endif  /* !_VIBE_H_ */

