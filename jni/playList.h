#ifndef __PLAY_LIST_H__
#define __PLAY_LIST_H__

#ifdef  __cplusplus
extern "C"
{
#endif
#include "dev_type.h"

#define MAX_FRAME_LIST_NUM      100

#define VIDEO_FORMAT_D1         0x01
#define VIDEO_FORMAT_HD1        0x02
#define VIDEO_FORMAT_CIF        0x03
#define VIDEO_FORMAT_QCIF       0x04
#define	VIDEO_FRAME				1
#define	AUDIO_FRAME				3

typedef struct __Hi3510_frame_t
{
	unsigned long device_type;
	unsigned long frame_size;
	unsigned long frame_no;
	unsigned char video_format;   //视频时表分辨率,音频时为AMR码率模式(bit7~2)和AMR打包格式(bit1~0)
	unsigned char frame_type;     //video frame: i, P and audio frame
	unsigned char frame_rate;     //当为视频帧时表帧率 当为音频帧时表采样率(bit7~2)和采样位宽(bit1~0)
	unsigned char video_standard; //当为视频帧时表N/P (就是说NTSC 或 PAL制式), 为音频帧时表编码方式
	unsigned long sec;
	unsigned long usec;
	unsigned long long pts;

	unsigned char *data;

	struct __Hi3510_frame_t *next;
}Hi3510_frame_t;

typedef struct __frame_list_t
{
	Hi3510_frame_t *pframe;
}frame_list_t;

#define MAX_NAME_LENGH			256
#define PLAY_TYPE_LOCAL_FILE	1
#define PLAY_TYPE_NET_STREAM	2
#define PLAY_TYPE_PLAY_BACK		3

int st_initFrameList();
int st_destroyFrameList();
Hi3510_frame_t *st_getFrameListHead(int flags);
void st_removeFrameListHead(int flags); //LinZh107 曾加语音链表
int st_inputFrameListTail(char *inputFrame,int size);
void st_setFrameListBroadcast();
int getFrameNum(int flags);//modify by Linzh107
#ifdef  __cplusplus
}
#endif
#endif//__PLAY_LIST_H__

