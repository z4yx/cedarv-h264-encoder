#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>             
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>       
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <time.h>
//#include <jpeglib.h>

#include "type.h"
#include "drv_display.h"
#include "capture.h"

#define LOG_NDEBUG 0

#include "enc_type.h"

#define DEV_NAME	"/dev/video0"		

#define CAMERA_DRIV_UPDATE

typedef struct buffer 
{
	void * start;
	size_t length;
}buffer;

static int fd=0;
struct buffer 		*buffers	= NULL;
static unsigned int	n_buffers	= 0;

#define CLEAR(x) memset (&(x), 0, sizeof (x))

int InitCapture()
{
	struct v4l2_capability cap; //构建camera属性结构体
	struct v4l2_format fmt;//构建像素格式结构体
	unsigned int i;
	//modify @2013-5-17,change to BLOCK
	//打开摄像头设备
	//fd = open (DEV_NAME, O_RDWR /* required */ | O_NONBLOCK, 0);
	fd = open (DEV_NAME, O_RDWR );
	if (fd == 0)
	{
		printf("open %s failed\n", DEV_NAME);
		return -1;
	}

	
#ifdef CAMERA_DRIV_UPDATE

	struct v4l2_input inp;
	inp.index = 0;
	if (-1 == ioctl (fd, VIDIOC_S_INPUT, &inp))//设置视频输入，一个视频设备可以有多个输入
	{
		LOGE("VIDIOC_S_INPUT error!\n");
		return -1;
	}
	
#endif	
	//获取摄像头属性信息
	int ff;
	ff=ioctl (fd, VIDIOC_QUERYCAP, &cap);
	if(ff<0)
		printf("failture VIDIOC_QUERTCAP\n");
 
	CLEAR (fmt);//清零操作
	//设置视频的制式和帧格式，制式包括PAL，NTSC，帧的格式个包括宽度和高度等
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = mVideoWidth; //3 
	fmt.fmt.pix.height      = mVideoHeight;  
	//modify @20130517
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;	//V4L2_PIX_FMT_YUV422P;	//
	//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
	ff=ioctl (fd, VIDIOC_S_FMT, &fmt); 	

	if(ff<0)

		printf("failture VIDIOC_S_FMT\n");

	//申请缓冲
	struct v4l2_requestbuffers req;
	CLEAR (req);
	req.count              = 4;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory         = V4L2_MEMORY_MMAP;

	ioctl (fd, VIDIOC_REQBUFS, &req); 

	buffers = calloc (req.count, sizeof(struct buffer));

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) 
	{
	   struct v4l2_buffer buf;   
	   CLEAR (buf);
	   buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	   buf.memory      = V4L2_MEMORY_MMAP;
	   buf.index       = n_buffers;

	   if (-1 == ioctl (fd, VIDIOC_QUERYBUF, &buf)) 
			printf ("VIDIOC_QUERYBUF error\n");
		//内存映射
	   buffers[n_buffers].length = buf.length;
	   buffers[n_buffers].start  = mmap (NULL /* start anywhere */,    
								         buf.length,
								         PROT_READ | PROT_WRITE /* required */,
								         MAP_SHARED /* recommended */,
								         fd, buf.m.offset);

	   if (MAP_FAILED == buffers[n_buffers].start)
			printf ("mmap failed\n");
	}


	//将缓冲入列
	for (i = 0; i < n_buffers; ++i) 
	{
	   struct v4l2_buffer buf;
	   CLEAR (buf);

	   buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	   buf.memory      = V4L2_MEMORY_MMAP;
	   buf.index       = i;

	   if (-1 == ioctl (fd, VIDIOC_QBUF, &buf))
		printf ("VIDIOC_QBUF failed\n");
	}

	return 0;
}

void DeInitCapture()
{
	int i;
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl (fd, VIDIOC_STREAMOFF, &type)) 	
		printf ("VIDIOC_STREAMOFF failed\n");	
	else		
		printf ("VIDIOC_STREAMOFF ok\n");
	
	for (i = 0; i < n_buffers; ++i) {
		if (-1 == munmap (buffers[i].start, buffers[i].length)) {
			printf ("munmap error\n");
		}
	}

	if (fd != 0)
	{
		close (fd);
		fd = 0;
	}

	printf("V4L2 close****************************\n");
}

int StartStreaming()
{
    int ret = -1; 
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 

  
   	 ret = ioctl (fd, VIDIOC_STREAMON, &type);
	
   	 if (ret < 0) 
	{ 
        printf("StartStreaming: Unable to start capture: %s\n", strerror(errno)); 
        return ret; 
    	} 

	printf("V4L2Camera::v4l2StartStreaming OK\n");
   	 return 0; 
}

void ReleaseFrame(int buf_id)
{	
	struct v4l2_buffer v4l2_buf;
	int ret;
	static int index = -1;

	memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
	v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_buf.memory = V4L2_MEMORY_MMAP;
	v4l2_buf.index = buf_id;		// buffer index

	if (index == v4l2_buf.index)
	{
		printf("v4l2 should not release the same buffer twice continuous: index : %d\n", index);
		// return ;
	}
	index = v4l2_buf.index;
	
	ret = ioctl(fd, VIDIOC_QBUF, &v4l2_buf);
	if (ret < 0) {
		printf("VIDIOC_QBUF failed, id: %d\n", v4l2_buf.index);
		return ;
	}
	
}

int GetPreviewFrame(V4L2BUF_t *pBuf)	// DQ buffer for preview or encoder
{
	int ret = -1; 
	struct v4l2_buffer buf;

	memset(&buf, 0, sizeof(struct v4l2_buffer));
    	buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
    	buf.memory = V4L2_MEMORY_MMAP; 
 
    /* DQ */ 
	//从camera队列中取出数据
    ret = ioctl(fd, VIDIOC_DQBUF, &buf); 
    if (ret < 0)                                                               
	{ 
        printf("GetPreviewFrame: VIDIOC_DQBUF Failed\n"); 
        return __LINE__; 
    }

	//将取出的数据，赋值给pBuf?
	pBuf->addrPhyY	= buf.m.offset;
	pBuf->index 	= buf.index;
	pBuf->timeStamp = (int64_t)((int64_t)buf.timestamp.tv_usec + (((int64_t)buf.timestamp.tv_sec) * 1000000));

	//printf("VIDIOC_DQBUF id: %d\n", buf.index);

	return 0;
}



