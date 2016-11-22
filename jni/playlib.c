/*
 **	author zhangrs
 **	created 2010.06.22
 **	for android platform
 **/

#include "playlib.h"
#include <ffmpeg/include/libavcodec/avcodec.h>
#include <ffmpeg/include/libavformat/avformat.h>
#include <ffmpeg/include/libswscale/swscale.h>

#include <android/log.h>
#include <android/bitmap.h>

#define null  0
#define LOG_TAG  "playlib.c"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define MAX_HD_FRAME_RATE  15
int abandon_frame = 1;

AVCodec *pVCodec = null;			// Codec
AVCodecContext *pVCodecCtx = null;	// Codec Context
AVFrame *pAVFrame = null;		  	// Frame
AVFrame *pFrameRGB = null;
struct SwsContext *img_convert_ctx = NULL;

//AVCodec *pACodec = null;
//AVCodecContext *pACodecCtx = null;
//AVCodec *pVCodec2 = null;				// Codec
//AVCodecContext *pVCodecCtx2 = null;	// Codec Context

int initPlayer()
{
	av_register_all();
	avcodec_register_all();

	pVCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!pVCodec)
	{
		LOGE("not found the pVCodec!");
		return -1;
	}
	pVCodecCtx = avcodec_alloc_context3(pVCodec);
	if (!pVCodecCtx)
	{
		LOGE("not found the pVCodecCtx!");
		return -2;
	}
	if (avcodec_open2(pVCodecCtx, pVCodec, NULL) < 0)
	{
		LOGE("Can't open the pVCodec!");
		return -3;
	}

	/*
	 * Libav for sure supports G.711. The associated codec ID are	AV_CODEC_ID_PCM_MULAW and AV_CODEC_ID_PCM_ALAW.
	 *
	 */ 

//	pACodec = avcodec_find_decoder(AV_CODEC_ID_PCM_ALAW/*AV_CODEC_ID_PCM_MULAW*//*AV_CODEC_ID_ADPCM_G726*//*AV_CODEC_ID_MP2*/);
//	if (!pACodec)
//	{
//		LOGE("not found the pACodec!");
//		return -4;
//	}
//	pACodecCtx = avcodec_alloc_context3(pACodec);
//	if (!pACodecCtx)
//	{
//		LOGE("not found the pACodecCtx!");
//		return -5;
//	}
//	int ret = avcodec_open2(pACodecCtx, pACodec, NULL);
//	if (ret < 0)
//	{
//		LOGE("Can't open the pACoder!");
//		return ret;
//	}

	pAVFrame = avcodec_alloc_frame();
	if (!pAVFrame)
		return -7;

	pFrameRGB = avcodec_alloc_frame();
	if (!pFrameRGB)
		return -8;

	return 0;
}

//更新ffmpeg库的测试
int getAvcodecVersion(void)
{
	return (int) avcodec_version();
}

//	Data： 2014-4-25  by:LinZh107  增加多 格式支持
//	Data：2014-5-7 更新解码库 和 更换解码方案
int decodeH264video(char *frame_data, int frame_size, int frame_rate, int *pParam, char *pOutBuf)
{
	AVPacket avpkt = {0};
	avpkt.size = frame_size;
	avpkt.data = frame_data;
	int got_picture = 0;
	//LOGI("begin decodeH264video ");
	avcodec_decode_video2(pVCodecCtx, pAVFrame, &got_picture, &avpkt);
	//LOGI("end decodeH264video ");
	if (got_picture)
	{
		//高清时计算要丢弃的帧数  LinZh107
		if(pVCodecCtx->width >= 1280)
		{
			if( frame_rate > MAX_HD_FRAME_RATE )
			{
				if(abandon_frame > MAX_HD_FRAME_RATE /(frame_rate - MAX_HD_FRAME_RATE))
				{
					LOGE("abandon this frame!");
					abandon_frame = 1;
					return -1;
				}
			}
		}
		abandon_frame++;

		avpicture_fill( (AVPicture *)pFrameRGB, pOutBuf, PIX_FMT_RGB565, pVCodecCtx->width, pVCodecCtx->height);
//		avpicture_fill((AVPicture *)pFrameRGB, pOutBuf, PIX_FMT_RGB565, *pWidth, *pHeight);

		/*
		 * *************** 以下为旧的方案，会引起内存泄露 *****************
		 * */
		img_convert_ctx = sws_getCachedContext(img_convert_ctx, pVCodecCtx->width, pVCodecCtx->height,
				pVCodecCtx->pix_fmt, pParam[0], pParam[1], PIX_FMT_RGB565, SWS_FAST_BILINEAR, NULL, NULL, NULL);
//		img_convert_ctx = sws_getContext(pVCodecCtx->width, pVCodecCtx->height,
//				pVCodecCtx->pix_fmt, pParam[0], pParam[1], PIX_FMT_RGB565, SWS_FAST_BILINEAR, NULL, NULL, NULL);

		if (img_convert_ctx == NULL)
		{
			LOGE("could not initialize conversion context\n");
			return -2;
		}
		//将YUV转化为RGB
		sws_scale(img_convert_ctx, (const uint8_t* const *) pAVFrame->data, pAVFrame->linesize,
					0, pVCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

		pParam[0] = pVCodecCtx->width ;
		pParam[1] = pVCodecCtx->height ;

		return 0;
	}
	else
		LOGE("avcodec_decode_video2 failed!\n");
		return -3;
}

//不使用播放库时要release_player
void releasePlayer()
{
	if (pVCodecCtx)
	{
		avcodec_close(pVCodecCtx);
		av_free(pVCodecCtx);
		pVCodecCtx = null;
	}
//	if (pACodecCtx)
//	{
//		avcodec_close(pACodecCtx);
//		av_free(pACodecCtx);
//		pACodecCtx = null;
//	}
	if (pAVFrame)
	{
		av_free(pAVFrame);
		pAVFrame = null;
	}
	if (pFrameRGB)
	{
		av_free(pFrameRGB);
		pFrameRGB = null;
	}
}

