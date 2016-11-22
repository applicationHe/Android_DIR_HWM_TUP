/*
**	author zhangrs
**	created 2010.06.22
**	for android platform
**/
#ifndef __PLAYLIB_H__
#define __PLAYLIB_H__
#include <ffmpeg/include/libavformat/avformat.h>


/*void cvt_420p_to_rgb565(int width,	int height,
						const unsigned char *yuv_y,
						const unsigned char *yuv_u,
						const unsigned char *yuv_v,
						unsigned short *dst);*/

int getAvcodecVersion(void);

//使用时先init_player
int initPlayer();

//不使用播放库时要release_player
void releasePlayer();

//解码音频数据
uint8_t * decode_audio(char *frame_data,int frame_size);

//解析h264格式的视频
int decodeH264video(char *frame_data, int frame_size, int frame_rate, int *pParam, char *pOutBuf);

int st_destroyFrameList();

#endif /* __PLAYLIB_H__ */
