
#ifndef __CAPTURE__H__
#define __CAPTURE__H__

#include <asm/types.h> 
#include <linux/videodev2.h>
#include "type.h"


extern int mVideoWidth;
extern int mVideoHeight;

int InitCapture();
void DeInitCapture();
int StartStreaming();
void ReleaseFrame(int buf_id);
int GetPreviewFrame(V4L2BUF_t *pBuf);


#endif // __CAPTURE__H__
