
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


#include "type.h"
#include "H264encLibApi.h"
#include "capture.h"
#include "enc_type.h"

#include <errno.h>
#include <fcntl.h>

int isFirstframe=1;

typedef struct cache_data        //定义缓存数据结构体
{
	int size;					//尺寸
	int write_offset;			//写偏移
	int part_num;				//标号
	int can_save_data;		
	char *data;
	pthread_mutex_t mut_save_bs;		//互斥锁
}cache_data;

int mVideoWidth = 640;
int mVideoHeight = 480;
int display_time = 30; //ms

bufMrgQ_t	gBufMrgQ;
VENC_DEVICE *g_pCedarV = NULL;


int g_cur_id = -1;
unsigned long long lastTime = 0 ; 


pthread_t thread_camera_id = 0;
pthread_t thread_enc_id = 0;
pthread_t thread_save_bs_id = 0;

pthread_mutex_t mut_cam_buf;
pthread_mutex_t mut_ve;
 
cache_data *save_bit_stream = NULL;

FILE * pEncFile = NULL;

char saveFile[128] = "h264_test.h264";

static __s32 WaitFinishCB(__s32 uParam1, void *pMsg)
{
	return cedarv_wait_ve_ready();
}

static __s32 GetFrmBufCB(__s32 uParam1,  void *pFrmBufInfo)
{
	int write_id;
	int read_id;
	int buf_unused;
	VEnc_FrmBuf_Info encBuf;

	//printf("calling GetFrmBufCB\n"); 

	memset((void*)&encBuf, 0, sizeof(VEnc_FrmBuf_Info));

	write_id 	= gBufMrgQ.write_id;
	read_id 	= gBufMrgQ.read_id;
	buf_unused	= gBufMrgQ.buf_unused;

	if(buf_unused == ENC_FIFO_LEVEL)
	{
		//printf("GetFrmBufCB: no valid fifo\n");
		return -1;
	}

	pthread_mutex_lock(&mut_cam_buf);
	
	encBuf.addrY = gBufMrgQ.omx_bufhead[read_id].buf_info.addrY;
	encBuf.addrCb = gBufMrgQ.omx_bufhead[read_id].buf_info.addrCb;
	encBuf.pts_valid = 1;
	encBuf.pts = (long long)gBufMrgQ.omx_bufhead[read_id].buf_info.pts;

	encBuf.color_fmt = PIXEL_YUV420;
	encBuf.color_space = BT601;

	g_cur_id = gBufMrgQ.omx_bufhead[read_id].id;

	printf("g_cur_id, GetFrmBufCB: %d\n", g_cur_id);
	gBufMrgQ.buf_unused++;
	gBufMrgQ.read_id++;
	gBufMrgQ.read_id %= ENC_FIFO_LEVEL;
	pthread_mutex_unlock(&mut_cam_buf);

	memcpy(pFrmBufInfo, (void*)&encBuf, sizeof(VEnc_FrmBuf_Info));
	
	return 0;
}

cache_data *save_bitstream_int(int size)
{
	cache_data *save_bit_stream = (cache_data *)malloc(sizeof(cache_data));
	memset(save_bit_stream, 0, sizeof(cache_data));
	save_bit_stream->size = size;
	save_bit_stream->part_num = 0;
	save_bit_stream->can_save_data = 0;
	save_bit_stream->write_offset = 0;
	save_bit_stream->data = (char *)malloc(save_bit_stream->size);
	
	pthread_mutex_init(&save_bit_stream->mut_save_bs,NULL);
	if(save_bit_stream->data == NULL)
	{
		printf("malloc fail\n");
		return NULL;
	}
	return save_bit_stream;
}

int save_bitstream_exit(cache_data *save_bit_stream)
{
	if(save_bit_stream)
	{
		if(save_bit_stream->data)
		{
			free(save_bit_stream->data);
			save_bit_stream->data = NULL;
		}

		pthread_mutex_destroy(&save_bit_stream->mut_save_bs);
		free(save_bit_stream);
		save_bit_stream = NULL;
	
	}
	
	return 0;
}

int update_bitstream_to_cache(cache_data *save_bit_stream, char *output_data, int data_size)
{
	int left_size;
	int offset;
	int last_write_offset;

	printf("%s: output_data=0x%08x, data_size=%d\n", __func__, output_data, data_size);	
	
	pthread_mutex_lock(&save_bit_stream->mut_save_bs);

	//save_bit_stream->size默认值设置为2M
	if(save_bit_stream->size < data_size)//如果传过来的尺寸大过2M，解锁(释放资源)，退出函数
	{
		printf("%s error\n", __func__);	
		pthread_mutex_unlock(&save_bit_stream->mut_save_bs);
		return -1;
	}

	last_write_offset = save_bit_stream->write_offset;//写的偏移，首次默认为0
	if((save_bit_stream->write_offset + data_size) >= save_bit_stream->size)
	{	
		
		printf("%s cond 1\n", __func__);	
		left_size = save_bit_stream->write_offset + data_size - save_bit_stream->size;
		offset = data_size - left_size;

		//每个参数的含义
		//第一个参数:data的指针+偏移量
		//第二个参数:输出数据地址
		//第三个参数:数据块的大小
		memcpy(save_bit_stream->data + save_bit_stream->write_offset, output_data, data_size - left_size);

		if(left_size > 0)
		{
			memcpy(save_bit_stream->data, output_data + offset, left_size);
		}
		
		save_bit_stream->write_offset = left_size;
	}
	else
	{	
		//内存数据复制
		memcpy(save_bit_stream->data + save_bit_stream->write_offset, output_data, data_size);
		save_bit_stream->write_offset = save_bit_stream->write_offset + data_size;//更新wrie_offset的值
	}

    if(last_write_offset > save_bit_stream->write_offset)
	{	
		printf("write to part0\n");	
		save_bit_stream->part_num = 0;
		save_bit_stream->can_save_data = 1;
	}

	else
	{	
		
		//第一次更新数据，此时write_offset的值已经更新
		if(save_bit_stream->write_offset >= save_bit_stream->size/2 && last_write_offset < save_bit_stream->size/2)
		{	
			
			save_bit_stream->part_num = 1;//部分标号
			save_bit_stream->can_save_data = 1;//是否可以存储1:可以
		}
		printf("write to part1\n");	
	}
	printf("%s: last=%d,offset=%d\n", __func__, last_write_offset, save_bit_stream->write_offset);

	pthread_mutex_unlock(&save_bit_stream->mut_save_bs);
	
	return 0;
}

int get_bitstream_for_save(cache_data *save_bit_stream, char * tem_data, int *datasize)
{
	int tmp_size;
	
	pthread_mutex_lock(&save_bit_stream->mut_save_bs);
	//printf("get_bitstream_for_save save_bit_stream->can_save_data=%d\n",save_bit_stream->can_save_data);
	if(save_bit_stream->can_save_data == 1)
	{
		tmp_size = save_bit_stream->size/2;
		*datasize = tmp_size;
		printf("%s: size=%d\n", __func__, tmp_size);		
		if(save_bit_stream->part_num ==1)
		{
			memcpy(tem_data, save_bit_stream->data, tmp_size);
		}
		else
		{
			memcpy(tem_data, save_bit_stream->data + tmp_size, save_bit_stream->size - tmp_size);
		}

		save_bit_stream->can_save_data = 0;
	}
	else
	{
		//printf("%s: size=zero\n", __func__);		
		*datasize = 0;
	}
	
	pthread_mutex_unlock(&save_bit_stream->mut_save_bs);
	return 0;
}

int save_left_bitstream(cache_data *save_bit_stream, char * tem_data, int *datasize)
{
	int offset;
	pthread_mutex_lock(&save_bit_stream->mut_save_bs);

	printf("save_left_bitstream can_save_data=%d,part=%d\n",save_bit_stream->can_save_data, save_bit_stream->part_num);
	if(save_bit_stream->can_save_data == 0)
	{
		if(save_bit_stream->part_num == 1)
		{
			offset = save_bit_stream->size/2;

			if(save_bit_stream->write_offset > offset)
			{
				memcpy(tem_data, save_bit_stream->data + offset, save_bit_stream->write_offset - offset);
			}

			*datasize = save_bit_stream->write_offset - offset;
		}
		else
		{
			if(save_bit_stream->write_offset > 0)
			{
				memcpy(tem_data, save_bit_stream->data, save_bit_stream->write_offset);
			}

			*datasize = save_bit_stream->write_offset;
		}
	}

	else
	{
		printf("save left bitstream error\n");
	}

	
	pthread_mutex_unlock(&save_bit_stream->mut_save_bs);
	return 0;
}

VENC_DEVICE * CedarvEncInit(__u32 width, __u32 height, __u32 avg_bit_rate, __s32 (*GetFrmBufCB)(__s32 uParam1,  void *pFrmBufInfo))
{
	int ret = -1;

	VENC_DEVICE *pCedarV = NULL;
	
	pCedarV = H264EncInit(&ret);
	
	if (ret < 0)
	{
		printf("H264EncInit failed\n");
	}

	__video_encode_format_t enc_fmt;
	enc_fmt.src_width = width;
	enc_fmt.src_height = height;
	enc_fmt.width = width;
	enc_fmt.height = height;
	enc_fmt.frame_rate = 1;
	enc_fmt.color_format = PIXEL_YUV420;
	enc_fmt.color_space = BT601;
	enc_fmt.qp_max = 40;
	enc_fmt.qp_min = 20;
	enc_fmt.avg_bit_rate = avg_bit_rate;
	enc_fmt.maxKeyInterval = 8;
	
    //enc_fmt.profileIdc = 77; /* main profile */

	enc_fmt.profileIdc = 66; /* baseline profile */
	enc_fmt.levelIdc = 31;

	pCedarV->IoCtrl(pCedarV, VENC_SET_ENC_INFO_CMD, (__u32) &enc_fmt);

	 
	ret = pCedarV->open(pCedarV);
	if (ret < 0)
	{
		printf("open H264Enc failed\n");
	}
	
	pCedarV->GetFrmBufCB = GetFrmBufCB;
	pCedarV->WaitFinishCB = WaitFinishCB;

	printf("H264encoder  init OK!\n");
	return pCedarV;
}

void CedarvEncExit(VENC_DEVICE *pCedarV)
{
	if (pCedarV)
	{
		pCedarV->close(pCedarV);
		H264EncExit(pCedarV);
		pCedarV = NULL;
	}
}


void *thread_camera()
{
	int ret = -1;
	V4L2BUF_t Buf;
	unsigned long long curTime;

	printf("%s start!\n", __func__);
	while(1)
	{	
		int		write_id;
		int		read_id;
		int		buf_unused;
	    curTime = gettimeofday_curr();
		
		if ((curTime - lastTime) > 1000*1000*display_time) 
		{
			printf("Exit camera thread\n");		
			pthread_exit(NULL);
		}

		buf_unused	= gBufMrgQ.buf_unused;

	//	printf("buf_unused: %d\n", buf_unused);
		
		if(buf_unused == 0)
		{
			usleep(10*1000);
			continue;
		}
		// get one frame
		//获取一帧图像，并存入Buf中
		ret = GetPreviewFrame(&Buf);

		printf("GetPreviewFrame: %d\n", ret);
		if (ret != 0)
		{
			usleep(2*1000);
			printf("GetPreviewFrame failed\n");

		}

		pthread_mutex_lock(&mut_cam_buf);

		write_id 	= gBufMrgQ.write_id;
		read_id 	= gBufMrgQ.read_id;
		buf_unused	= gBufMrgQ.buf_unused;
		printf("%s buf_unused=%d\n", __func__, buf_unused);
		if(buf_unused != 0)
		{
			
			gBufMrgQ.omx_bufhead[write_id].buf_info.pts = Buf.timeStamp;//时间戳赋值
			gBufMrgQ.omx_bufhead[write_id].id = Buf.index;//标号 赋值

			gBufMrgQ.omx_bufhead[write_id].buf_info.addrY = (__u8 *) Buf.addrPhyY;//???? 不明白的地方
			gBufMrgQ.omx_bufhead[write_id].buf_info.addrCb = (__u8 *) Buf.addrPhyY + mVideoWidth* mVideoHeight;		

			gBufMrgQ.buf_unused--;//每操作一次自减
			gBufMrgQ.write_id++;
			gBufMrgQ.write_id %= ENC_FIFO_LEVEL;
		}
		else
		{
			
			printf("IN OMX_ErrorUnderflow\n");
		}
		pthread_mutex_unlock(&mut_cam_buf);
		
	}

	return (void *)0;  
}


void *thread_enc()
{
	int ret;
	unsigned long long curTime;
	__vbv_data_ctrl_info_t data_info;
	int motionflag = 0;
	int bFirstFrame = 1; //need do something more in first frame
	
	printf("%s start!\n", __func__);

	while(1)
	{

		curTime = gettimeofday_curr();
	
		if ((curTime - lastTime) > 1000*1000*display_time) 
		{	
			printf("Exit encode thread\n");	
			pthread_exit(NULL);
		}

		/* the value from 0 to 9 can be used to set the level of the sensitivity of motion detection
		it is recommended to use 0 , which represents the hightest level sensitivity*/
		
		g_pCedarV->IoCtrl(g_pCedarV, VENC_LIB_CMD_SET_MD_PARA , 0);


		pthread_mutex_lock(&mut_ve);

		// printf("%s: before encode!\n", __func__);
		/* in this function , the callback function of GetFrmBufCB will be used to get one frame */
		ret = g_pCedarV->encode(g_pCedarV);
		// printf("%s: after encode!\n", __func__);
		
		pthread_mutex_unlock(&mut_ve);

		//printf("encode result: %d\n", ret);

		
		if(ret == 0)
		{
			/* get the motion detection result ,if the result is 1, it means that motion object have been detected*/
			g_pCedarV->IoCtrl(g_pCedarV, VENC_LIB_CMD_GET_MD_DETECT, (__u32) &motionflag);
			//printf("motion detection,result: %d\n", motionflag);
		}

		
		if (ret != 0)
		{
			/* camera frame buffer is empty */
			usleep(10*1000);
			continue;
		}
	

		/* release the camera frame buffer */
		if(ret == 0)
		{
			pthread_mutex_lock(&mut_cam_buf);

			printf("call ReleaseFrame, g_cur_id=%d\n", g_cur_id);
			ReleaseFrame(g_cur_id);	
			
			pthread_mutex_unlock(&mut_cam_buf);

		}

		
		if(ret == 0)
		{
			memset(&data_info, 0 , sizeof(__vbv_data_ctrl_info_t));
			ret = g_pCedarV->GetBitStreamInfo(g_pCedarV, &data_info);
		
			
			if(ret == 0)
			{					
				if(1 == bFirstFrame)//判断是否为第一帧
				{
					bFirstFrame = 0;
					//将解码以后的数据更新到缓存
					//参数说明:
					//第一个参数:保存比特流结构体,在save_bitstream_init中初始化
					//第二个参数:获取到一帧数据的地址
					//第三个参数:获取到一帧数据的长度
					update_bitstream_to_cache(save_bit_stream, data_info.privateData, data_info.privateDataLen);
				}
							
				/* save bitstream to cache buffer */
				if (data_info.uSize0 != 0)
				{
					update_bitstream_to_cache(save_bit_stream, data_info.pData0, data_info.uSize0);
				}
				
				
				if (data_info.uSize1 != 0)
				{
					update_bitstream_to_cache(save_bit_stream, data_info.pData1, data_info.uSize1);

				}
								
				/* encode release bitstream */
				/*释放bitstream*/
				g_pCedarV->ReleaseBitStreamInfo(g_pCedarV, data_info.idx);
				
			}
		}
	}
	

}





void *thread_save_bs()
{
	unsigned long long curTime;
	int data_size = 0;
	//一Mb大小空间
	char *tem_data = malloc(1*1024*1024);

	printf("%s start!\n", __func__);
	
	while(1)
	{	
		unsigned long ret;
		
		curTime = gettimeofday_curr();
		//printf("%s: curTime=%llu, lastTime=%llu\n", __func__, curTime, lastTime);	
		if ((curTime - lastTime) > 1000*1000*display_time) 
	
		{	
			//从save_bit_stream中获取到数据存放到tem_data中
			get_bitstream_for_save(save_bit_stream, tem_data, &data_size);
			
			
			
			if(data_size > 0)
			{
				fwrite(tem_data, data_size, 1, pEncFile);	//将数据写入h264.dat中			
				
				printf("success write %d!!\n",data_size);
			}
			
			//不明白的函数
			save_left_bitstream(save_bit_stream, tem_data, &data_size);

			if(data_size > 0)
			{
				fwrite(tem_data, data_size, 1, pEncFile);	
			
				printf("success write left %d!!\n",data_size);
			}

			if(tem_data)
			{
				free(tem_data);
				tem_data = NULL;
			}
	
			printf("Exit bs thread========================\n");
			pthread_exit(NULL);					
		}

		
		get_bitstream_for_save(save_bit_stream, tem_data, &data_size);

		if(data_size > 0)
		{
			fwrite(tem_data, data_size, 1, pEncFile);	
			
			printf("write size: %d\n",data_size);
		}

		else
		{
			usleep(10 * 1000);
		}
		
	}
}

int main()
{


	
	int ret = -1;
	
	/* init video engine */
	ret = VE_hardware_Init(0);
	
	if (ret < 0)
	{
		printf("cedarx_hardware_init failed\n");
	}
	printf("cedarx_hardware_init success!\n");

	InitCapture();

	g_pCedarV = CedarvEncInit(mVideoWidth,mVideoHeight,1024*1024, GetFrmBufCB);

	/* set VE 320M */
	VE_set_frequence(320);

#if 1
	pEncFile = fopen(saveFile, "wb");
	if (pEncFile == NULL)
	{
		printf("open %s failed\n", saveFile);
		goto EXIT;
	}


	save_bit_stream = save_bitstream_int(2*1024*1024);//尺寸大小为2M
	if (save_bit_stream == NULL)
	{
		printf("save_bitstream_int failed\n");
		goto EXIT;
	}

#endif

	/* start camera */
	StartStreaming();//camer 开始捕捉图像

	pthread_mutex_init(&mut_cam_buf,NULL);
	pthread_mutex_init(&mut_ve,NULL);
	

	lastTime = gettimeofday_curr();

	/* create camera thread*/
	if(pthread_create(&thread_camera_id, NULL, thread_camera, NULL) != 0)
	{
		printf("Create thread_camera fail !\n");
	}

	/* create encode thread*/
	if(pthread_create(&thread_enc_id, NULL, thread_enc, NULL) != 0)
	{
		printf("Create thread_enc fail !\n");
	}

	/* create save bitstream thread*/               
  	 if(pthread_create(&thread_save_bs_id, NULL, thread_save_bs, NULL) != 0)
	{
		printf("Create thread_save_bs fail !\n");
	}

EXIT:

	if(thread_camera_id !=0) 
	{                 
        pthread_join(thread_camera_id,NULL);
    }

	if(thread_enc_id !=0) 
	{                 
        pthread_join(thread_enc_id,NULL);
    }

	if(thread_save_bs_id !=0) 
	{                 
        pthread_join(thread_save_bs_id,NULL);
    }

	pthread_mutex_destroy(&mut_cam_buf);
	pthread_mutex_destroy(&mut_ve);

	save_bitstream_exit(save_bit_stream);

	DeInitCapture();

	CedarvEncExit(g_pCedarV);

	if (pEncFile)
	{
		fclose(pEncFile);
		pEncFile = NULL;
	}

	VE_hardware_Exit(0);
		
	return 0;
}



