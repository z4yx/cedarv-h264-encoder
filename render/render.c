/*******************************************************************************
--                                                                            --
--                    CedarX Multimedia Framework                             --
--                                                                            --
--          the Multimedia Framework for Linux/Android System                 --
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                         Softwinner Products.                               --
--                                                                            --
--                   (C) COPYRIGHT 2011 SOFTWINNER PRODUCTS                   --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
*******************************************************************************/

#include "render.h"

VIDEO_RENDER_CONTEXT_TYPE * hnd_video_render = NULL;

unsigned long args[4];
unsigned long arg[4];

__disp_video_fb_t 	tmpFrmBufAddr;
__disp_layer_info_t android_layer;
int image_width,image_height;
int num;
int a=0;

/*配置默认参数*/

int compress_yuyv_to_rgb(unsigned char *framebuffer,unsigned char *tempbuffer) 
{

		unsigned char *yuyv = framebuffer;//vd->framebuffer为开辟的640*480*2字节的内存空间
		int z = 0 , i = 0, j = 0;

		for(i = 0 ; i < 479 ; i++) //宽度的像素点数
		{
				for (j = 0; j < 639; j++) //长度的像素点数
				{
						int r, g, b;
						int y, u, v;
						short rgb;

					/*yuv的像素提取*/
						if (!z)
								y = yuyv[0] << 8;
						else
								y = yuyv[2] << 8;
						u = yuyv[1] - 128;
						v = yuyv[3] - 128;

						//rgb格式转换
						r = (y + (359 * v)) >> 8;
						g = (y - (88 * u) - (183 * v)) >> 8;
						b = (y + (454 * u)) >> 8;

						r = (r > 255) ? 255 : ((r < 0) ? 0 : r); // RGB的范围为0~255，0为最弱，255为最亮
						g = (g > 255) ? 255 : ((g < 0) ? 0 : g);
						b = (b > 255) ? 255 : ((b < 0) ? 0 : b);

						r >>= 3; //RGB565格式中red占5位，green占6位，blue占5位，故需进行移位
						g >>= 2;
						b >>= 3;

						rgb = (r<<11)|(g<<5)|(b);//生成一个2个字节长度的像素点

						*tempbuffer++ = (char)rgb;   //每次将一个像素点赋给buffer
						*tempbuffer++ = (char)(rgb >> 8);//取高八位的赋值

						if (z++) 
						{
								z = 0;
								yuyv += 4;
						}
				}
				yuyv += (640-480)*2;//换行，即裁剪，将超出屏幕的部分裁减掉不要
		}	
}	

#if 0
int decodeYUV420SP(__u8 *rgbBuf, __u8 *yuv420sp, int width, int height) 
{    	
	int frameSize = width * height;		
	if (rgbBuf == NULL)			
	printf("buffer 'rgbBuf' is null\n");		
	if (rgbBuf.length < frameSize * 3)			
	printf("buffer 'rgbBuf' size "+ rgbBuf.length + " < minimum " + frameSize * 3);
	if (yuv420sp == null)			
	throw new NullPointerException("buffer 'yuv420sp' is null");		
	if (yuv420sp.length < frameSize * 3 / 2)			
	throw new IllegalArgumentException("buffer 'yuv420sp' size " + yuv420sp.length+ " < minimum " + frameSize * 3 / 2);
 	int i = 0, y = 0;    	
	int uvp = 0, u = 0, v = 0;    	
	int y1192 = 0, r = 0, g = 0, b = 0;    	    	
	for (int j = 0, yp = 0; j < height; j++) 
	{    		
		uvp = frameSize + (j >> 1) * width;    		
		u = 0;    		
		v = 0;    		
		for (i = 0; i < width; i++, yp++) 
			{    			
			y = (0xff & ((int) yuv420sp[yp])) - 16;    			
			if (y < 0) y = 0;    			
			if ((i & 1) == 0) 
				{    				
				v = (0xff & yuv420sp[uvp++]) - 128;    				
				u = (0xff & yuv420sp[uvp++]) - 128;    			
				}    			    			
			y1192 = 1192 * y;    			
			r = (y1192 + 1634 * v);    			
			g = (y1192 - 833 * v - 400 * u);    			
			b = (y1192 + 2066 * u);    			    			
			if (r < 0) 
				r = 0; 
			else if 
				(r > 262143) r = 262143;    			
			if (g < 0) 
				g = 0; 
			else if (g > 262143) 
				g = 262143;    			
			if (b < 0) 
				b = 0; 
			else if (b > 262143) 
				b = 262143;    			    			
			rgbBuf[yp * 3] = (byte)(r >> 10);    			
			rgbBuf[yp * 3 + 1] = (byte)(g >> 10);    			
			rgbBuf[yp * 3 + 2] = (byte)(b >> 10);
    		}    	
		}
	}
#endif
int open_layer(int a)
{
	int ret =-1;
	unsigned long  args[4];
	args[0] = 0;
	args[1] = a;
	args[2] = 0;
	args[3] = 0;
	ret=ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_OPEN, args);
	if(ret!=0)
		{
		perror("open layer");
		}
	return 0;

}



int close_layer(int a)
{
	

	unsigned long  args[4];
	args[0] = 0;
	args[1] = a;
	args[2] = 0;
	args[3] = 0;
	ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_CLOSE, args);
	return 0;
}

int release_layer(int a)
{
		int ret=-1;
		args[0] = 0;
		args[1] = a;
		args[2] = 0;
		args[3] = 0;
		ret=ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_RELEASE, args);
		if(ret!=0)
			{
			perror("release failed");
			}
		
		return 0;
}

int set_layer_para(int arg,__disp_layer_info_t * tmp)
{
	int ret=0;

	
	args[0] = 0;
	args[1] = arg;
	args[2] = (unsigned long) (tmp);
	args[3] = 0;
	ret=ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_SET_PARA, args);//设置层属性
	if(ret!=0)
		{
		perror("error");
		printf("set_layer_para%d,failed\n",arg);
		exit(-1);
		}
	printf("success set para\n");

	return 0;

}



int request_layer()
{
	args[0] = 0;
	args[1] = DISP_LAYER_WORK_MODE_SCALER;
//	args[1] = DISP_LAYER_WORK_MODE_GAMMA;
	args[2] = 0;
	args[3] = 0;
	return( ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_REQUEST,args));
	
	return 0;
}

static int config_de_parameter(int para,unsigned int width, unsigned int height, __disp_pixel_fmt_t format) 
	{
	__disp_layer_info_t tmpLayerAttr;
	__disp_layer_info_t tmpLayerAttr2;
	
	int ret;

	image_width = width;
	image_height = height;

	args[0] = 0;
	args[1] = hnd_video_render->de_layer_hdl[0];
	args[2] = (unsigned long)(&tmpLayerAttr);
	ret=ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_GET_PARA,args);
	if(ret!=0)
		{
		perror("get para");
		}

	arg[0]=0;
	arg[1]=hnd_video_render->de_layer_hdl[1];
	arg[2]=(unsigned long)(&tmpLayerAttr2);
	ret=ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_GET_PARA,arg);
	if(ret!=0)
		{
		perror("get para");
		}
	printf("get para success\n");


	//set color space设置颜色空间?
	//判断是否为HD输出?
	if (image_height < 720) {
		tmpLayerAttr.fb.cs_mode = DISP_BT601;
		tmpLayerAttr2.fb.cs_mode=DISP_BT601;
	} else {
		tmpLayerAttr.fb.cs_mode = DISP_BT709;
		tmpLayerAttr2.fb.cs_mode=DISP_BT601;
	}
	//tmpFrmBuf.fmt.type = FB_TYPE_YUV;
	//printf("format== DISP_FORMAT_ARGB8888 %d\n", format == DISP_FORMAT_ARGB8888 ? 1 : 0);
	if(format == DISP_FORMAT_ARGB8888){		//如果为rgb格式
		tmpLayerAttr.fb.mode 	= DISP_MOD_NON_MB_PLANAR;
		tmpLayerAttr.fb.format 	= format; 
		tmpLayerAttr.fb.br_swap = 0;
		tmpLayerAttr.fb.cs_mode	= DISP_YCC;
		tmpLayerAttr.fb.seq 	= DISP_SEQ_P3210;
	
	}
	else
	{									//如果为yuv420格式
		tmpLayerAttr.fb.mode 	= DISP_MOD_MB_UV_COMBINED;
		tmpLayerAttr.fb.format 	= format; //DISP_FORMAT_YUV420;
		tmpLayerAttr.fb.br_swap = 0;
		tmpLayerAttr.fb.seq 	= DISP_SEQ_UVUV;

	}


	tmpLayerAttr.fb.addr[0] = tmpFrmBufAddr.addr[0];
	tmpLayerAttr.fb.addr[1] = tmpFrmBufAddr.addr[1];

	tmpLayerAttr.fb.size.width 	= image_width;
	tmpLayerAttr.fb.size.height 	= image_height;


	tmpLayerAttr.mode = DISP_LAYER_WORK_MODE_SCALER;
	//tmpLayerAttr.ck_mode = 0xff;
	//tmpLayerAttr.ck_eanble = 0;
	//alpha通道使能，并且设置为全透明(0xff)
	tmpLayerAttr.alpha_en = 1;
	tmpLayerAttr.alpha_val = 0xff;
#ifdef CONFIG_DFBCEDAR
	tmpLayerAttr.pipe = 1;
#else
	tmpLayerAttr.pipe = 0;
#endif
	//tmpLayerAttr.prio = 0xff;
	//screen window information
	tmpLayerAttr.scn_win.x = 0;
	tmpLayerAttr.scn_win.y = 0;
	tmpLayerAttr.scn_win.width  = hnd_video_render->video_window.width;
	tmpLayerAttr.scn_win.height = hnd_video_render->video_window.height;
	tmpLayerAttr.prio           = 0xff;
	//printf("tmpLayerAttr.scn_win.width %d,tmpLayerAttr.scn_win. height %d\n",tmpLayerAttr.scn_win.width ,tmpLayerAttr.scn_win.height );
	
	//frame buffer pst and size information

	tmpLayerAttr.src_win.x = 0;//tmpVFrmInf->dst_rect.uStartX;
	tmpLayerAttr.src_win.y = 0;//tmpVFrmInf->dst_rect.uStartY;
	tmpLayerAttr.src_win.width = image_width;//tmpVFrmInf->dst_rect.uWidth;
	tmpLayerAttr.src_win.height = image_height;//tmpVFrmInf->dst_rect.uHeight;
	hnd_video_render->src_frm_rect.x = tmpLayerAttr.src_win.x;
	hnd_video_render->src_frm_rect.y = tmpLayerAttr.src_win.y;
	hnd_video_render->src_frm_rect.width = tmpLayerAttr.src_win.width;
	hnd_video_render->src_frm_rect.height = tmpLayerAttr.src_win.height;

	
	//tmpLayerAttr.fb.b_trd_src		= 0;
	//tmpLayerAttr.b_trd_out			= 0;
	//tmpLayerAttr.fb.trd_mode 		=  (__disp_3d_src_mode_t)3;
	//tmpLayerAttr.out_trd_mode		= DISP_3D_OUT_MODE_FP;
	//tmpLayerAttr.b_from_screen 		= 0;
	//set channel
	//tmpLayerAttr.channel = DISP_LAYER_OUTPUT_CHN_DE_CH1;
	//FIOCTRL(hnd_video_render->de_fd, DISP_CMD_LAYER_SET_PARA, hnd_video_render->de_layer_hdl, &tmpLayerAttr);
	printf("set video layer param\n");

/*设置layer0层属性*/

	if(para==0)
		
	ret=set_layer_para(hnd_video_render->de_layer_hdl[0],&tmpLayerAttr);


//设置layer1层属性
#if 1
	if(para==1)
		{
		memcpy(&tmpLayerAttr2,&tmpLayerAttr,sizeof(tmpLayerAttr));		
		tmpLayerAttr2.mode=DISP_LAYER_WORK_MODE_SCALER;	
		tmpLayerAttr2.scn_win.x = 645;
		tmpLayerAttr2.scn_win.y = 0;	
		 ret=set_layer_para(hnd_video_render->de_layer_hdl[1],&tmpLayerAttr2);	


		}
#endif		

	args[0]				= 0;
	args[1]                 		= hnd_video_render->de_layer_hdl[0];
	args[2]                 		= 0;
	args[3]                 		= 0;
	ret                     			= ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_TOP,args);
	if(ret != 0)
	{	
		printf("Open video display layer failed!\n");
		perror("layer0:");
		exit(-1);
		return NULL;
	}	

#if 1

	arg[0]				= 0;
	arg[1]                 		= hnd_video_render->de_layer_hdl[1];
	arg[2]                 		= 0;
	arg[3]                 		= 0;
	ret                     			= ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_TOP,arg);
	if(ret != 0)
	{		
		printf("Open video display layer failed!\n");
		perror("layer1:");
		exit(-1);
		return NULL;
	}	


#endif	
	return 0;
}

int printf_layer(__disp_layer_info_t * android_layer)
{

		/***************************/
		printf("android mode=%d\n",android_layer->mode);
		printf("B_from_screen=%d\n",android_layer->b_from_screen);
		printf("pipe=%d\n",android_layer->pipe);
		printf("prio=%d\n",android_layer->prio);
		printf("alpha_en=%d\n",android_layer->alpha_en);
		printf("alpha_val=%d\n",android_layer->alpha_val);
		printf("ck_enable=%d\n",android_layer->ck_enable);

		printf("src_win.x=%d\n",android_layer->src_win.x);
		printf("src_win.y=%d\n",android_layer->src_win.y);
		printf("android_layer.src_win.width=%d\n",android_layer->src_win.width);
		printf("android_layer.src_win.height=%d\n",android_layer->src_win.height);

		printf("android_layer.scn_win.x=%d\n",android_layer->scn_win.x);
		printf("android_layer.scn_win.y=%d\n",android_layer->scn_win.y);
		printf("android_layer.scn_win.width=%d\n",android_layer->scn_win.width);
		printf("android_layer.scn_win.height=%d\n",android_layer->scn_win.height);

		printf("fb.size.width=%d\n",android_layer->fb.size.width);
		printf("fb.size.height=%d\n",android_layer->fb.size.height);
		printf("android_layer.fb.format=%d\n",android_layer->fb.format);
		printf("android_layer.fb.seq=%d\n",android_layer->fb.seq);
		printf("android_layer.fb.mode=%d\n",android_layer->fb.mode);
		printf("android_layer.fb.br_swap=%d\n",android_layer->fb.br_swap);
		printf("androif .fb.cs_mode=%d\n",android_layer->fb.cs_mode);
		printf("b_trd_src=%d\n",android_layer->fb.b_trd_src);

		/***************************/

	return 0;

}

int render_init() 
{
	int ret;

	/*构建hnd_video_render结构体，申请空间，清零*/
	
	if (hnd_video_render != (VIDEO_RENDER_CONTEXT_TYPE *) 0) {
		printf("Cedar:vply: video play back has been opended already!\n");
		return -1;
	}

	hnd_video_render = (VIDEO_RENDER_CONTEXT_TYPE *) malloc(sizeof(VIDEO_RENDER_CONTEXT_TYPE ));
	if (hnd_video_render == (VIDEO_RENDER_CONTEXT_TYPE *) 0) {
		printf("Cedar:vply: malloc hnd_video_render error!\n");
		return -1;
	}
	memset(hnd_video_render, 0, sizeof(VIDEO_RENDER_CONTEXT_TYPE ));

	hnd_video_render->first_frame_flag = 1;//表明为第一帧
	hnd_video_render->de_fd = open("/dev/disp", O_RDWR);
	
	if (hnd_video_render->de_fd < 0) {
		printf("Open display driver failed!\n");
		return -1;
	}



/*申请显示层0*/
	args[0] = 0;
	args[1] = DISP_LAYER_WORK_MODE_SCALER;
	args[2] = 0;
	args[3] = 0;
	hnd_video_render->de_layer_hdl[0] = ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_REQUEST,args);
	if (hnd_video_render->de_layer_hdl[0] == 0) {
		printf("Open display layer0 failed! de_fd:%d \n", hnd_video_render->de_fd);
		perror("open displayer:");
		return -1;
	}
	printf("request layer 0 success =%d\n",hnd_video_render->de_layer_hdl[0]);
	


/*申请显示层1*/
	arg[0] = 0;
	arg[1] = DISP_LAYER_WORK_MODE_SCALER;
	arg[2] = 0;
	arg[3] = 0;
	hnd_video_render->de_layer_hdl[1] = ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_REQUEST,arg);
	if (hnd_video_render->de_layer_hdl[1] == 0) {
		printf("Open display layer1 failed! de_fd:%d \n", hnd_video_render->de_fd);
		perror("open displayer:");
		return -1;
	}
	printf("request layer 1 success =%d\n",hnd_video_render->de_layer_hdl[1]);


	//set video window information to default value, full screen
	hnd_video_render->video_window.x = 0;
	hnd_video_render->video_window.y = 0;
	args[0] = 0;
	args[1] = hnd_video_render->de_layer_hdl[0];
	args[2] = 0;
	args[3] = 0;
	//modify @20130524
	hnd_video_render->video_window.width = 635;
	hnd_video_render->video_window.height =355;
	//endmodify
	printf("de---w:%d,h:%d\n", hnd_video_render->video_window.width, hnd_video_render->video_window.height);


	return 0;
}

void render_exit(void) {
	if (hnd_video_render == NULL) {
		printf("video playback has been closed already!\n");
		return;
	}
	int			ret;

	//close displayer driver context
	if (hnd_video_render->de_fd) {
		args[0] = 0;
		args[1] = hnd_video_render->de_layer_hdl[0];
		args[2] = 0;
		args[3] = 0;
		ioctl(hnd_video_render->de_fd, DISP_CMD_VIDEO_STOP, args);

		args[0] = 0;
		args[1] = hnd_video_render->de_layer_hdl[0];
		args[2] = 0;
		args[3] = 0;
		ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_RELEASE, args);

		args[0]	= 0;
		ret = ioctl(hnd_video_render->de_fd, DISP_CMD_GET_OUTPUT_TYPE, args);

		if(ret == DISP_OUTPUT_TYPE_HDMI)
		{
			args[0] 					= 0;
			args[1] 					= 0;
			args[2] 					= 0;
			args[3] 					= 0;
			ioctl(hnd_video_render->de_fd,DISP_CMD_HDMI_OFF,(unsigned long)args);

			args[0] 					= 0;
			args[1] 					= hnd_video_render->hdmi_mode;
			args[2] 					= 0;
			args[3] 					= 0;
			ioctl(hnd_video_render->de_fd, DISP_CMD_HDMI_SET_MODE, args);

			args[0] 					= 0;
			args[1] 					= 0;
			args[2] 					= 0;
			args[3] 					= 0;
			ioctl(hnd_video_render->de_fd,DISP_CMD_HDMI_ON,(unsigned long)args);

		}
		close(hnd_video_render->de_fd);
		hnd_video_render->de_fd = 0;
	}

	if (hnd_video_render) {
		free(hnd_video_render);
		hnd_video_render = NULL;
	}
}

int render_render(void *frame_info, int frame_id)
{
	 
	cedarv_picture_t 			*display_info = (cedarv_picture_t *) frame_info; 
		 
	 __disp_layer_info_t         	layer_info;
	 __disp_pixel_fmt_t			pixel_format;
	int ret;
	 
	

	memset(&tmpFrmBufAddr, 0, sizeof(__disp_video_fb_t ));

	tmpFrmBufAddr.interlace 		= display_info->is_progressive? 0: 1;
	tmpFrmBufAddr.top_field_first 	= display_info->top_field_first;
	tmpFrmBufAddr.frame_rate 		= display_info->frame_rate;
	//tmpFrmBufAddr.first_frame = 0;//first_frame_flg;
	//first_frame_flg = 0;



	//像素地址赋值
	tmpFrmBufAddr.addr[0] = display_info->y;
	tmpFrmBufAddr.addr[1] = display_info->u;	

	tmpFrmBufAddr.id = frame_id;


	if (hnd_video_render->first_frame_flag == 1) 
	{
		__disp_layer_info_t        		 layer_info;
		__disp_layer_info_t			 layer_info2;
		


		pixel_format = display_info->pixel_format==CEDARV_PIXEL_FORMAT_AW_YUV422 ? DISP_FORMAT_YUV422 : DISP_FORMAT_YUV420;		
		if(display_info->display_width && display_info->display_height)
			//config_de_parameter(display_info->display_width, display_info->display_height, pixel_format);
			{
			config_de_parameter(0,640, 480, pixel_format);
			config_de_parameter(1,640, 480, pixel_format);
			}
		else
			//config_de_parameter(display_info->width, display_info->height,pixel_format);
			{
			config_de_parameter(0,640, 480, pixel_format);
			config_de_parameter(1,640, 480, pixel_format);
			}
		//set_display_mode(display_info, &layer_info);

		
	    args[0] = 1;
	    args[1] = hnd_video_render->de_layer_hdl[0];
	    args[2] = (unsigned long) (&layer_info);
	    args[3] = 0;
	    ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_GET_PARA, args);

		
    	
		args[0] 				= 0;
		args[1] 				= hnd_video_render->de_layer_hdl[0];
		args[2] 				= 0;
		args[3] 				= 0;
		ret = ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_OPEN, args);
		printf("layer0 open hdl:%d,ret:%d w:%d h:%d\n", hnd_video_render->de_layer_hdl[0], ret,display_info->width,display_info->height);
		if (ret != 0){
			//open display layer failed, need send play end command, and exit
			printf("Open video display layer failed!\n");
			return -1;
		}


		args[0] 				= 0;
		args[1] 				= hnd_video_render->de_layer_hdl[1];
		args[2] 				= 0;
		args[3] 				= 0;
		ret = ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_OPEN, args);
		printf("layer0 open hdl:%d,ret:%d w:%d h:%d\n", hnd_video_render->de_layer_hdl[1], ret,display_info->width,display_info->height);
		if (ret != 0){
			//open display layer failed, need send play end command, and exit
			printf("Open video display layer failed!\n");
			return -1;
		}


		args[0] 				= 0;
		args[1] 				= hnd_video_render->de_layer_hdl[2];
		args[2] 				= 0;
		args[3] 				= 0;
		ret = ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_OPEN, args);
		printf("layer0 open hdl:%d,ret:%d w:%d h:%d\n", hnd_video_render->de_layer_hdl[2], ret,display_info->width,display_info->height);
		if (ret != 0){
			//open display layer failed, need send play end command, and exit
			printf("Open video display layer failed!\n");
			return -1;
		}

#if 0
		args[0] 				= 0;
		args[1] 				= hnd_video_render->de_layer_hdl[3];
		args[2] 				= 0;
		args[3] 				= 0;
		ret = ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_OPEN, args);
		printf("layer0 open hdl:%d,ret:%d w:%d h:%d\n", hnd_video_render->de_layer_hdl[3], ret,display_info->width,display_info->height);
		if (ret != 0){
			//open display layer failed, need send play end command, and exit
			printf("Open video display layer failed!\n");
			return -1;
		}

#endif

		args[0] =0;
		args[1] = hnd_video_render->de_layer_hdl[0];
		args[2] = 0;
		args[3] = 0;
		ret = ioctl(hnd_video_render->de_fd, DISP_CMD_VIDEO_START, args);//打开视频流服务
		if(ret != 0) {
			printf("Start video layer0 failed!\n");
			return -1;
		}


		args[0] =0;
		args[1] = hnd_video_render->de_layer_hdl[1];
		args[2] = 0;
		args[3] = 0;
		ret = ioctl(hnd_video_render->de_fd, DISP_CMD_VIDEO_START, args);//打开视频流服务
		if(ret != 0) {
			printf("Start video layer1 failed!\n");
			return -1;
		}


		args[0] =0;
		args[1] = hnd_video_render->de_layer_hdl[2];
		args[2] = 0;
		args[3] = 0;
		ret = ioctl(hnd_video_render->de_fd, DISP_CMD_VIDEO_START, args);//打开视频流服务
		if(ret != 0) {
			printf("Start video layer1 failed!\n");
			return -1;
		}

#if 0
		args[0] =0;
		args[1] = hnd_video_render->de_layer_hdl[3];
		args[2] = 0;
		args[3] = 0;
		ret = ioctl(hnd_video_render->de_fd, DISP_CMD_VIDEO_START, args);//打开视频流服务
		if(ret != 0) {
			printf("Start video layer1 failed!\n");
			return -1;
		}
#endif
		/***************************/
		hnd_video_render->layer_open_flag = 1;
		hnd_video_render->first_frame_flag = 0;		
	}
	else
	{
	
		


			args[0] = 0;
			args[1] = hnd_video_render->de_layer_hdl[0];
			args[2] = (unsigned long) (&tmpFrmBufAddr);
			args[3] = 0;
			ret=ioctl(hnd_video_render->de_fd, DISP_CMD_VIDEO_SET_FB, args);
			if(ret!=0)
				{
 				perror("layer 0 set fb failed");
				}



			arg[0] = 0;
			arg[1] = hnd_video_render->de_layer_hdl[1];
			arg[2] = (unsigned long) (&tmpFrmBufAddr);
			arg[3] = 0;
			ret=ioctl(hnd_video_render->de_fd, DISP_CMD_VIDEO_SET_FB, arg);
			if(ret!=0)
				{
				perror("layer 1 set fb failed");
				}

			
			
			arg[0] = 0;
			arg[1] = hnd_video_render->de_layer_hdl[2];
			arg[2] = (unsigned long) (&tmpFrmBufAddr);
			arg[3] = 0;
			ret=ioctl(hnd_video_render->de_fd, DISP_CMD_VIDEO_SET_FB, arg);
			if(ret!=0)
				{
				perror("layer 2 set fb failed");
				}


#if 0
			arg[0] = 0;
			arg[1] = hnd_video_render->de_layer_hdl[3];
			arg[2] = (unsigned long) (&tmpFrmBufAddr);
			arg[3] = 0;
			ret=ioctl(hnd_video_render->de_fd, DISP_CMD_VIDEO_SET_FB, arg);
			if(ret!=0)
				{
				perror("layer 3 set fb failed");
				}

#endif

		/****************************/


		
	}	

	return 0;
}

int render_get_disp_frame_id(void)
{
	return ioctl(hnd_video_render->de_fd, DISP_CMD_VIDEO_GET_FRAME_ID, args);
}

