/***************************************************************************
 *
 * Copyright	：2007 santachi corp.
 * File name	：dev_type.h
 * Description	：define all device type for santachi
 * Created		: liuqs<liuqs_santachi@126.com> --- 2007-01-22
 * Modified 	: add some struct ---2007-02-02:zhangkk
 * Modified 	: add some device type by liuqs on 2007-06-04
 * Modified	; change struct frame_head_t by dingjq on 2007-08-21
 *
 ***************************************************************************/

#ifndef __DEV_TYPE_H__
#define __DEV_TYPE_H__
#include <stdint.h>

/*----------------------------device type-----------------------------*/
/* santachi dvr device type define */
#define DEV_TYPE_HD420		0x54500000	/* z1510 :mpeg1/2 4 channel CIF */
#define DEV_TYPE_HD820		0x54500001	/* z1510 :mpeg1/2 8 channel CIF */
#define DEV_TYPE_HD8004		0x54500002	/* vweb2010 :mpeg4 4 channel D1 */
#define DEV_TYPE_HD8008		0x54500003	/* vweb2010 :mpeg4 8 channel D1 */
#define DEV_TYPE_HD8016		0x54500004	/* at204x :mpeg4 16 channel CIF */

/* santachi dvs device type define */
#define DEV_TYPE_NT200A		0x54500100	/* at204x: mpeg4 1 channel encoder */
#define DEV_TYPE_NT200B		0x54500101	/* at204x: mpeg4 1 channel decoder */
#define DEV_TYPE_NT200C		0x54500102	/* at204x: mpeg4 1 channel codec */
#define DEV_TYPE_NT200D		0x54500103	/* at204x: mpeg4 1 channel encoder with hard disk */
#define DEV_TYPE_NT204A		0x54500104 	/* at204x: mpeg4 4 channel encoder */
#define DEV_TYPE_NT204D		0x54500105	/* at204x: mpeg4 4 channel encoder with hard disk */

#define DEV_TYPE_NT200HA	0x54500200	/* hi3510: h.264 1 channel encoder */
#define DEV_TYPE_NT200HB	0x54500201	/* hi3510: h.264 1 channel decoder */
#define DEV_TYPE_NT200HC	0x54500202	/* hi3510: h.264 1 channel codec */
#define DEV_TYPE_NT200HD	0x54500203	/* hi3510: h.264 1 channel encoder with hard disk */
#define DEV_TYPE_NT204HA	0x54500204	/* hi3510: h.264 4 channel encoder */
#define DEV_TYPE_NT204HD	0x54500205	/* hi3510: h.264 4 channel encoder with hard disk */
#define DEV_TYPE_NT202HD	0x54500206	/* hi3510: h.264 2 channel encoder with hard disk */
#define DEV_TYPE_NT204HD_1	0x54500208	/* hi3510: h.264 4 channel encoder with hard disk */
#define DEV_TYPE_NT202HD_1	0x54500209	/* hi3510: h.264 2 channel encoder with hard disk */

/* santahci ip camera device type define */
#define DEV_TYPE_NT6050		0x54500300	/* at204x :mpeg4 1 channel D1 */
#define DEV_TYPE_NT8189		0x54500301	/* at204x :mpeg4 1 channel D1 */
#define DEV_TYPE_NT4094		0x54500302	/* at204x :mpeg4 1 channel D1 --no this device */
#define DEV_TYPE_NT6050H	0x54500303	/* hi3510 :h.264 1 channel D1 */
#define DEV_TYPE_NT8189H	0x54500304	/* hi3510 :h.264 1 channel D1 */
#define DEV_TYPE_NT4094H	0x54500305	/* hi3510 :h.264 1 channel D1 */
#define DEV_TYPE_NT1102H	0x54500306      /* hi3510 :h.264 1 channel D1 */
#define DEV_TYPE_NT9072H	0x54500307      /* hi3510 :h.264 1 channel D1 */
#define DEV_TYPE_NT1102H_D	0x54500310      /* hi3510 :h.264 1 channel D1 */


/* santachi nvr device type define */
#define DEV_TYPE_NVR		0x54500400	/* powerpc 8245 nvr */
#define DEV_TYPE_NT8502H    0x54500500
#define DEV_TYPE_NT3511H    0x54500600
#define DEV_TYPE_HI3515     0x54500700  // Hi3515
#define DEV_TYPE_DECODER_HI3511      0x54500601 /* ST-DEC */
#define DEV_TYPE_KEYBOARD_HI3511     0x54500602 /* ST-KEYBOARD */
#define DEV_TYPE_CM1810_HI3511       0x54500603 /* ST-CM1810 */
/* other device type define */

/*------------------------frame struct------------------------------*/

#define SANTACHI_I_FRAME_TYPE 1 //I Frame
#define SANTACHI_P_FRAME_TYPE 2 //P Frame
#define SANTACHI_A_FRAME_TYPE 3 //Audio Frame
#define SANTACHI_OSD_FRAME_TYPE 4 // Osd Frame
//#if HUAWEI_PLATFORM
#define HUAWEI_A_FRAME_TYPE 5 // HUAWEI AUDIO TYPE
//#endif

#define VIDEO_FORMAT_D1		0x01
#define VIDEO_FORMAT_HD1	0x02
#define VIDEO_FORMAT_CIF	0x03
#define VIDEO_FORMAT_QCIF	0x04

#define VIDEO_PAL_MODE		0x01
#define VIDEO_NTSC_MODE		0x02


//音频参数
#define AUDIO_SAMPLE_WIDTH_8	0
#define AUDIO_SAMPLE_WIDTH_16	1

/* max decode/encode frame size, 60ms at 8k samperate or 10ms at 48k samplerate */
#define HI_VOICE_MAX_FRAME_SIZE       (480+1)    /* dont change it */

typedef enum __AUDIO_SAMPLE_RATE
{
	AUDIO_SAMPLE_R8     = 0,   /* 8K Sample rate     */
	AUDIO_SAMPLE_R11025 = 1,   /* 11.025K Sample rate*/
	AUDIO_SAMPLE_R16    = 2,   /* 16K Sample rate    */
	AUDIO_SAMPLE_R22050 = 3,   /* 22.050K Sample rate*/
	AUDIO_SAMPLE_R24    = 4,   /* 24K Sample rate    */
	AUDIO_SAMPLE_R32    = 5,   /* 32K Sample rate    */
	AUDIO_SAMPLE_R441   = 6,   /* 44.1K Sample rate  */
	AUDIO_SAMPLE_R48    = 7,   /* 48K Sample rate    */
}AUDIO_SAMPLE_RATE;

typedef enum __AUDIO_CODEC_FORMAT
{
	AUDIO_FORMAT_NULL	= 0,   /* 原始码流	     */
	AUDIO_FORMAT_G711A	= 1,   /* G.711 A            */
	AUDIO_FORMAT_G711Mu	= 2,   /* G.711 Mu           */
	AUDIO_FORMAT_ADPCM	= 3,   /* ADPCM              */
	AUDIO_FORMAT_G726_16    = 4,   /* G.726              */
	AUDIO_FORMAT_G726_24    = 5,   /* G.726              */
	AUDIO_FORMAT_G726_32    = 6,   /* G.726              */
	AUDIO_FORMAT_G726_40    = 7,   /* G.726              */
	AUDIO_FORMAT_AMR	= 8,   /* AMR encoder format */
	AUDIO_FORMAT_AMRDTX	= 9,   /* AMR encoder formant and VAD1 enable */
	AUDIO_FORMAT_AAC	= 10,   /* AAC encoder        */
}AUDIO_CODEC_FORMAT;

/* The package type of amr */
typedef enum __AMR_PACKAGE_TYPE
{
	AMR_PACKAGE_MIME = 0,  /* Using for file saving        */
	AMR_PACKAGE_IF_1 = 1,  /* Using for wireless translate */
	AMR_PACKAGE_IF_2 = 2,  /* Using for wireless translate */
}AMR_PACKAGE_TYPE;

/* The bit rate type of amr */
typedef enum __AUDIO_AMR_MODE
{
	AMR_MODE_R475 = 0,     /* AMR bit rate as  4.75k */
	AMR_MODE_R515,         /* AMR bit rate as  5.15k */
	AMR_MODE_R59,          /* AMR bit rate as  5.90k */
	AMR_MODE_R67,          /* AMR bit rate as  6.70k */
	AMR_MODE_R74,          /* AMR bit rate as  7.40k */
	AMR_MODE_R795,         /* AMR bit rate as  7.95k */
	AMR_MODE_R102,         /* AMR bit rate as 10.20k */
	AMR_MODE_R122,         /* AMR bit rate as 12.20k */
}AUDIO_AMR_MODE;

#if 0
typedef struct __frame_head_t
{
        unsigned long	device_type;
        unsigned long	frame_size;
        unsigned long	frame_no;
        unsigned char	video_resolution;//视频时表分辨率,音频时为AMR码率模式(bit7~2)和AMR打包格式(bit1~0)
        unsigned char	frame_type;	//video frame: i, P and audio frame
        unsigned char	frame_rate;  //当为视频帧时表帧率 当为音频帧时表采样率(bit7~2)和采样位宽(bit1~0)
        unsigned char	video_standard;		//当为视频帧时表N/P, 为音频帧时表编码方式
        unsigned long	sec;
        unsigned long	usec;
	uint64_t	pts;
}frame_head_t;

typedef struct __frame_t
{
	frame_head_t frame_head;
	unsigned char *frame_data;
}frame_t;
#else

typedef struct __frame_head_t
{
	unsigned long   device_type;
	unsigned long   frame_size;
	unsigned long   frame_no;
	unsigned char   video_resolution;//视频时表分辨率,音频时为AMR码率模式(bit7~2)和AMR打包格式(bit1~0)
	unsigned char   frame_type;     //video frame: i, P and audio frame
	unsigned char   frame_rate;  	//当为视频帧时表帧率 当为音频帧时表采样率(bit7~2)和采样位宽(bit1~0)
	unsigned char   video_standard; //当为视频帧时表N/P, 为音频帧时表编码方式
	unsigned long   sec;
	unsigned long   usec;
	unsigned long long pts;
}frame_head_t;

typedef struct __frame_t
{
	frame_head_t frame_head;
	unsigned char frame_data[0x80000]; //ClientNetLib.c   DATA_BUF_LEN = 512*1024
}frame_t;
#endif
/*-----------------------file struct------------------------------*/
#define MAX_LOGO_SIZE	32
#define SANTACHI_FLIE_VERSION01 0x00000001 /*first created 07-02-02*/
#define SANTACHI_FLIE_VERSION02 0x00000002

typedef struct __file_head_t
{
	int file_version;
	int device_type;
	int record_type;
	char logo[MAX_LOGO_SIZE];
}file_head_t;

typedef struct __file_t
{
	file_head_t file_head;
	frame_t *p_frame;
}file_t;

#define DEV_FLAT_TYPE_NUM	0x0b

#define DEV_FLAT_TYPE_HW	0x01//华为
#define DEV_FLAT_TYPE_ZTE	0x02//中兴
#define DEV_FLAT_TYPE_GX	0x03//公信
#define DEV_FLAT_TYPE_HXHT	0x04//互信
#define DEV_FLAT_TYPE_H3C	0x05//华三
#define DEV_FLAT_TYPE_LT	0x06//浪涛
#define DEV_FLAT_TYPE_BELL	0x07//贝尔
#define DEV_FLAT_TYPE_KEDA	0x08//科达
#define DEV_FLAT_TYPE_WSX	0x09//网视星
#define DEV_FLAT_TYPE_FH	0x0a//烽火
#define DEV_FLAT_TYPE_SZ	0x0b//神舟

//手动IO控制
#define MAX_IO_INPUT_NUM	16
#define MAX_IO_OUTPUT_NUM	16
typedef struct __io_input_t
{
	char status;//状态  1:闭合 0:断开
	char unused[3];
}io_input_t;

typedef struct __io_output_t //手动控制IO
{
	int time;//输出持续时长  0:永久 单位:秒
	char status;//状态  1:闭合 0:断开
	char unused[3];
}io_output_t;

typedef struct __dev_io_control_t
{
	io_input_t  input[MAX_IO_INPUT_NUM];
	io_output_t output[MAX_IO_OUTPUT_NUM];
	char input_num;//有效输入总数
	char output_num;//有效输出总数
	char unused[2];//保留
}dev_io_control_t;

// my_dev_io_control_t need for java
typedef struct __my_dev_io_control_t
{
	char input[MAX_IO_INPUT_NUM];
	char output[MAX_IO_OUTPUT_NUM];
	int timer[MAX_IO_OUTPUT_NUM];
	char input_num;//有效输入总数
	char output_num;//有效输出总数
}my_dev_io_control_t;

//定时IO输出控制
#define  DEV_MAX_WEEK_DAY           7        	//一星期最多天数
#define  DEV_MAX_TIME_SEG_NUM       4        	//最多时间段数
//录像时间段结构
typedef struct __dev_rec_time_seg_t
{
	unsigned char	start_hour;
	unsigned char	start_minute;
	unsigned char	start_second;
	unsigned char	end_hour;
	unsigned char	end_minute;
	unsigned char	end_second;
	unsigned char	unused[2];
}dev_rec_time_seg_t;

typedef struct __dev_time_seg_t
{
	unsigned char	start_hour;
	unsigned char	start_minute;
	unsigned char	start_second;
	unsigned char	unused;
}dev_time_seg_t;

typedef struct __dev_timer_week_t
{
	dev_rec_time_seg_t	time_seg[DEV_MAX_TIME_SEG_NUM];
	char enable_flag;	//1有效， 0无效
	char unused[3];
}dev_timer_week_t;

typedef struct __dev_timer_output_t
{
	dev_timer_week_t	timer_week[DEV_MAX_WEEK_DAY];
}dev_timer_output_t;

#endif /* __DEV_TYPE_H__ */


