#ifndef __CLIENTNETLIB_HH_H__
#define __CLIENTNETLIB_HH_H__

#include 		<stdio.h>
#include		<string.h>
#include 		<stdlib.h>
#include 		<assert.h>
#include        <sys/types.h>   		// basic system data types
#include        <sys/socket.h>  		// basic socket definitions
#include        <sys/time.h>    		// timeval{} for select()
#include        <netinet/in.h>  		// sockaddr_in{} and other Internet defns
#include		<netinet/tcp.h>
#include        <arpa/inet.h>   		// inet(3) functions
#include        <netdb.h>
#include        <sys/ioctl.h>
#include        <net/if.h>
#include        <pthread.h>
#include        <unistd.h>
#include        <errno.h>
#include        <signal.h>
#include        <fcntl.h>
#include		<semaphore.h>
#include		<sys/stat.h>
#include		<sys/select.h>


#ifdef  __cplusplus
extern "C" {
#endif

#include	"dev_type.h"
#include	"clientNetLib.h"
#include	"msg_def.h"
#include	"p2p_cross.h"

#define TRUE				1
#define FALSE				0
typedef int					BOOL;
#define	COMM_ADDRSIZE		20
#define	COMM_ETHNAME		"eth0"
#define	OVER_TIME			10*1000 //second

//#define pthread_cancel(x)

#define	MSG_UPDATE_STATUS	0x080b	//升级状态


#define INDEX_MSG_LOGIN						1
#define INDEX_MSG_LOGOUT					2
#define INDEX_MSG_OPEN_CHANNEL				3
#define INDEX_MSG_CLOSE_CHANNEL				4
#define INDEX_MSG_GET_PARAM					5	//得到参数
#define INDEX_MSG_SET_PARAM					6	//设置参数
#define INDEX_MSG_GET_RECORDLIST			7	//录像查询
#define INDEX_MSG_PLAY_RECORD				8	//录像点播
#define INDEX_MSG_DOWNLOAD_RECORD			9	//录像下载
#define INDEX_MSG_HAND_RECORD				10	//手动录像
#define INDEX_MSG_START_RECORD				11	//远程录像
#define INDEX_MSG_PTZ_CONTROL				12	//云台控制
#define INDEX_MSG_UPGRADESYSTEM				13	//系统升级
#define	INDEX_MSG_GET_TIME_INFO				14	//得到时间
#define	INDEX_MSG_SET_TIME_INFO				15	//设置时间
#define	INDEX_MSG_VERSION_INFO				16	//版本信息
#define	INDEX_MSG_HARDDISK_INFO				17	//硬盘信息
#define	INDEX_MSG_SYSTEM_STATUS				18	//系统状态
#define	INDEX_MSG_DEFAULT_SETTING			19	//出厂设置
#define	INDEX_MSG_ALARM_INFO				20	//告警信息
#define	INDEX_MSG_GET_LOG_INFO				21	//得到日志
#define	INDEX_MSG_DELETE_LOG_INFO			22	//得到日志
#define	INDEX_MSG_START_SEND_DATA			23	//开始发送数据
#define	INDEX_MSG_STOP_SEND_DATA			24	//停止发送数据
#define INDEX_MSG_GET_USERINFO				25
#define INDEX_MSG_ADD_USER					26
#define INDEX_MSG_DELETE_USER				27
#define INDEX_MSG_CHANGE_USER				28
#define INDEX_MSG_ALARM_CONTROL				29
#define INDEX_MSG_GET_RECORD_LIST			30
#define INDEX_MSG_UPGRADE_STATUS			31
#define INDEX_MSG_START_TALK				32	//语音对讲
#define INDEX_MSG_STOP_TALK					33
#define INDEX_MSG_HAND_RESTART				34	//手动重启
#define INDEX_MSG_HEARTBEAT					35
#define INDEX_MSG_REMOTE_START_HAND_RECORD	36
#define INDEX_MSG_REMOTE_STOP_HAND_RECORD	37
#define INDEX_MSG_DELETE_RECORD_FILE		38
#define INDEX_MSG_PARAM_AUTO_POWEROFF		40
#define INDEX_MSG_PARAM_AUTO_POWERON		41
#define INDEX_MSG_PARAM_USER_INFO			42
#define INDEX_MSG_PARAM_HAND_RECORD			43
#define INDEX_MSG_PARAM_NETWORK_CONFIG		44
#define INDEX_MSG_PARAM_MISC_CONFIG			45
#define INDEX_MSG_PARAM_PROBER_ALRAM		46
#define INDEX_MSG_PARAM_RECORD_PARAM		47
#define INDEX_MSG_PARAM_TERM_CONFIG			48
#define INDEX_MSG_PARAM_TIMER_RECORD		49
#define INDEX_MSG_PARAM_VIDEO_LOSE			50
#define INDEX_MSG_PARAM_VIDEO_MOVE			51
#define INDEX_MSG_PARAM_VIDEO_CONFIG		52
#define	INDEX_MSG_PARAM_ALARM_PARAM			53
#define INDEX_MSG_PARAM_VIDEO_ENCODE		54
#define INDEX_MSG_PARAM_COM_CONFIG			55
#define INDEX_MSG_PARAM_OSDSET				56
#define INDEX_MSG_PARAM_OVERLAY				57
#define INDEX_MSG_PARAM_MOTION_ZOOM			58
#define INDEX_MSG_PARAM_PPPOE				59
#define INDEX_MSG_PARAM_RECORD_INFO			60
#define INDEX_MSG_PARAM_RECOVERDEF_CONFIG	61
#define INDEX_MSG_PARAM_NA208_INFO			62
#define INDEX_MSG_PARAM_WIRELESS_CONFIG		63   //配置无线网卡IP xujm add
#define INDEX_MSG_PARAM_MAIL_CONFIG		    64
#define INDEX_MSG_PARAM_FTP_CONFIG			65
#define INDEX_MSG_PARAM_SERIAL_INFO			66
#define INDEX_MSG_PARAM_TRANSPARENCE_INFO	67
#define INDEX_MSG_PARAM_SLAVE_INFO			68
#define INDEX_MSG_PARAM_DEVICE_INFO			69
#define	INDEX_TCP_PREVIEW_RECORD			70	//TCP录像预览
#define	INDEX_TCP_DOWNLOAD_RECORD			71	//录像下载
#define	INDEX_TCP_RE_DOWNLOAD_RECORD		72	//录像断点续传
#define	INDEX_TCP_PLAY_RECORD				73	//录像点播
#define	INDEX_TCP_UPGRADE_SYSTEM			74	//系统升级
#define	INDEX_TCP_AUTH_RECORD				75	//命令返回信息
#define INDEX_MSG_TRANSPARENT				76  //用于透明传输
#define INDEX_MSG_SESSION_ID				77
#define INDEX_MSG_PARTITION					78 //分区
#define INDEX_MSG_FORMAT					79 //格式化
#define INDEX_MSG_FORMAT_STATUS				80 //格式化状态
#define INDEX_MSG_FDISK_STATUS				81 //分区状态
#define INDEX_MSG_START_ALARM				82 //告警布防
#define INDEX_MSG_STOP_ALARM				83 //告警撤防
#define INDEX_MSG_WIRDLESS_SEARCH			84
#define INDEX_MSG_START_TRANSPARENT			85 //开始透明传输
#define INDEX_MSG_STOP_TRANSPARENT			86 //停止透明传输
#define INDEX_MSG_SEND_TRANSPARENT			87 //发送透明传输数据
#define INDEX_MSG_SETVERSION				88
#define INDEX_MSG_RESET_VIDIO				89
#define INDEX_INPUTFILE_CONFIG				90
#define INDEX_OUTPUTFILE_CONFIG				91
//键盘消息参数
#define INDEX_MSG_PARAM_KEY_WEB_PWD			120
#define INDEX_MSG_PARAM_KEY_USER_PWD		121
#define INDEX_MSG_PARAM_KEY_SERVER_INFO		122
#define INDEX_MSG_PARAM_KEY_ENC_INFO		123
#define INDEX_MSG_PARAM_KEY_ENC_CHANNEL_INFO 124
#define INDEX_MSG_PARAM_KEY_DEC_INFO		125
#define INDEX_MSG_PARAM_KEY_TOUR_INFO		126
#define INDEX_MSG_PARAM_KEY_TOUR_STEP_INFO	127
#define INDEX_MSG_PARAM_KEY_GROUP_INFO		128
#define INDEX_MSG_PARAM_KEY_GROUP_STEP_INFO	129
#define	INDEX_MSG_PARAM_UDP_NAT_CLINTINFO	133
#define INDEX_MSG_PARAM_LPS_CONFIG_INFO		134

#define INDEX_MSG_PARAM_HANDIO_CONTROL		144
#define INDEX_MSG_PARAM_TIMEIO_CONTROL		145

#define INDEX_MSG_PARAM_AUDIO_INFO			170
#define INDEX_MSG_PARAM_MEMBER_INFO			171
#define INDEX_MSG_PARAM_CAMERA_INFO			172
#define INDEX_MSG_PARAM_NAS_INFO			173
#define INDEX_MSG_PARAM_ITU_INFO			174
#define INDEX_MSG_PARAM_COLOR_INFO			175
#define INDEX_MSG_PARAM_CCD_DEFAULT			176
#define INDEX_MSG_PARAM_ITU_DEFAULT			177
#define INDEX_MSG_START_BACK				178
#define INDEX_MSG_STOP_BACK					179
#define INDEX_DECORD_PARAM_NETSTAR			180
#define INDEX_DECORD_PARAM_NUM				181
#define INDEX_DECORD_PARAM_STAR				182
#define INDEX_3511_PARAM_SYSINFO			183
#define INDEX_MSG_PAPAM_MATRIX				184
#define INDEX_MSG_PAPAM_MATRIX_INFO			185
#define INDEX_MSG_DEFAULT_ENCODER			186
#define INDEX_MSG_TIMER_CAPTURE				187
#define INDEX_MSG_IMMEDIATE_CAPTURE			188
#define INDEX_MSG_PARAM_DECODE_OSD			189
//CDMA
#define	INDEX_MSG_CDMA_TIME					190  //2009.03.11
#define INDEX_MSG_CDMA_ALARM_ENABLE			191  //报警触发拨号
#define INDEX_MSG_CDMA_NONEGO_ENABLE		192  //外部触发拨号
#define INDEX_MSG_CDMA_ALARM_INFO			193  //报警配置信息
#define INDEX_MSG_CDMA_STREAM_ENABLE		194  //自动下线
#define INDEX_PARMA_PTZ_CURISE				195  //预置位巡航
#define INDEX_PARAM_USB_TYPE				196  //USB add by wy 2009.04.22
#define INDEX_MSG_PARAM_DECODER_BUF_TIME			206
#define INDEX_MSG_PARAM_DECODER_BUF_FRAME_COUNT		208
#define INDEX_MSG_PARAM_TIMER_RESTART				209
#define	INDEX_MSG_3G_SIGNAL							210


///////////////////////////////////////////////////////////////////////
//华为
#define INDEX_MSG_PARAM_CENTER_SERVER_CONFIG			100
#define INDEX_MSG_PARAM_LENS_INFO_CONFIG				101
//h3c
#define INDEX_MSG_PARAM_H3C_PLATFORM_CONFIG				102
#define INDEX_H3C_INFO									103
//keda
#define INDEX_MSG_PARAM_KEDA_PLATFORM_CONFIG			104
//网通
#define INDEX_MSG_PARAM_H3C_NETCOM_CONFIG				105
//BELL
#define INDEX_MSG_PARAM_BELL_PLATFORM_CONFIG			106
//ZTE
#define INDEX_MSG_PARAM_ZTE_PLATFORM_CONFIG				107
//互信互通
#define INDEX_MSG_PARAM_HXHT_PLATFORM_CONFIG			108
#define INDEX_MSG_PARAM_HXHT_ALARM_SCHEME				109
#define INDEX_MSG_PARAM_HXHT_NAT						110
//烽火
#define INDEX_MSG_PARAM_FENGH_NAT						111
//公信
#define INDEX_MSG_PARAM_GONGX_NAT						112
//海斗
#define INDEX_MSG_PARAM_HAIDOU_NAT						113
#define INDEX_MSG_PARAM_HAIDOU_NAS						130
//浪涛
#define INDEX_MSG_PARAM_LANGTAO_NAT						114
//UT
#define  INDEX_MSG_PARAM_UT_NAT							115
//神州
#define  INDEX_MSG_PARAM_SZ_PARAM						116

//中兴立维
#define INDEX_MSG_PARAM_ZXLW_PARAM						117 //2009-03-04

#define INDEX_MSG_PARAM_YIXUN_PARAM                     118  //2009-03-30

//校时
#define INDEX_MSG_PARAM_CHECK_TIME						119 //2009.05.06

//ZTE stream
#define INDEX_MSG_PARAM_ZTE_STREAM						131 //2009.05.07

//ZTE Online
#define INDEX_MSG_PARAM_ZTE_ONLINE						132 //---- add by 2009.05.07

//////////////////////////////////////////////////////////////////////////

typedef struct __callback_t
{
	int channel;
	CHANNEL_STREAM_CALLBACK channel_callback;
	void *context;
}callback_t;

typedef struct __channel_info_t
{
	int	port_no;
	int	preview_flag;
	pthread_t preview_thread_id;

	int	protocol_type;
	int	sock; 		//TcpReceive Socket
	struct	sockaddr_in	client_sockaddr;

	callback_t callback;
	int channel_no;

	BOOL record_flag;
	FILE *record_file;
	char record_name[256];
	BOOL bIflag;

	int msg_sock;	//massage socket

	int bit_rate;
	BOOL bBitRate;
	int frontsecond;
	int tmp_bit_rate;

	unsigned long user_id;

	int recv_flag ;

	/*
	 * add channel_info ==> by LinZh107
	 * int stream_type		断线重连的维护
	 * int channel_type
	 * int protocol_type 	这个原本就已经有了，这里不再更新
	 */
	int stream_type;
	int channel_type;

	//add by LinZh107 2014-12-22
	//modify at 2015-2-13 , for pjproject instead of libnice.
	p2p_thorough_param_t *p2pParam;

}channel_info_t;

typedef	struct	__tcp_sock_info_t
{
	int	sock_fd;
	struct	__tcp_sock_info_t	*next;
}tcp_sock_info_t;

typedef struct __msg_head_t
{
	unsigned	short	msg_type;		//消息类型
	unsigned	short	msg_subtype;	//消息子类型
	unsigned	int		msg_size;		//消息长度
	unsigned	char	ack_flag;		//确认标志
	unsigned	char	reserved[3];
}msg_head_t;

#define	MSG_DATA_LEN		(1024*100)//(1*1024)
typedef struct __net_msg_t
{
	msg_head_t	msg_head;		//消息头
	char	msg_data[MSG_DATA_LEN];		//消息数据
}net_msg_t;

typedef struct __dual_talk_t
{
	int talk_sock;
	pthread_t TalkThread_id;
	int	   iDualRevFalg;
	TALK_STREAM_CALLBACK talk_callback;
	void *context;

}dual_talk_t;

typedef struct __transparent_start_t
{
	int				transparent_id;
	serial_info_t	searial_info;
}transparent_start_t;
typedef struct _bidi_transparent_t
{
	transparent_start_t transparent_start;

	BIDI_TRANSPARENT_CALLBACK bidi_ransparent_callback;
	void *context;

}bidi_transparent_t;

#define NETLIB_MAX_CHANNEL_NUM		16
#define NETLIB_MAX_MSG_NUM			250 //设定信号量,当时最大到200.
typedef struct __server_connect_t
{
	int device_type;
	int user_right;
	int session_id;
	int	msg_flag;
	int	msg_sock;
	struct	sockaddr_in server_sockaddr;

	int talk_sock;
	int remote_play_sock;
	//////////////////////////////////////////////////////////////////////////
	//文件下载
	int downloadfile_tcp_sock;
	unsigned long iDownloadFileSize;
	FILE *downloadfile;
	//////////////////////////////////////////////////////////////////////////

	dual_talk_t dual_talk;
	channel_info_t	channel_info[NETLIB_MAX_CHANNEL_NUM];
	tcp_sock_info_t	*tcp_sock_info;
	pthread_mutex_t tcpmutex_id;
	struct	__server_connect_t	*next;

	char ip_addr[48];
	int  port;

	pthread_t msg_thread_id;
	pthread_t heartbeat_thread_id;
	pthread_t reconnect_thread_id;

	sem_t sem_id[NETLIB_MAX_MSG_NUM];
	BOOL interval_valid[NETLIB_MAX_MSG_NUM];
	net_msg_t *p_net_msg[NETLIB_MAX_MSG_NUM];

	SERVER_MSG_CALLBACK	 msg_callback;
	void *msg_callback_context;

	ALARM_INFO_CALLBACK	 alarm_info_callback;
	void *alarm_info_callback_context;

	PLAYRECORD_DECODE_CALLBACK playrecord_decode_callback;
	void *playrecord_decode_callback_context;

	PLAYRECORD_STATUS_CALLBACK playrecord_status_callback;
	void *playrecord_status_callback_context;

	bidi_transparent_t bidi_transparent; //add by zhangkk for com
	UPDATE_STATUS_CALLBACK update_status_callback;
	void *update_status_callback_context;
	BOOL bPortOffset; //add by fengzx 2011-07-05;改成单端口

	//add by LinZh107 2014-10-10
	int channel;					//用以断线重连的维护
	BOOL bReopenFlag;
	BOOL bReloginFlag;

	//add by LinZh107 2014-12-22
	//modify at 2015-2-13 , for pjproject instead of libnice.
	p2p_thorough_param_t *p2pParam;

}server_connect_t;

typedef struct __tp_server_info_t
{
	server_connect_t *server_connect;
	char	ip_addr[32];
	int		port;
	char	user_name[32];
	char	user_pwd[48];

	int		iLogin;			//0 未登录 1 登录
	int		iMsgStatus;	    //0 未连接 1 连接
	int		ichannelStatus[NETLIB_MAX_CHANNEL_NUM];//0 未连接 1 连接
	int		iAuto_Login;
	int		iAuto_StartStream[NETLIB_MAX_CHANNEL_NUM];

	char	pu_id[32];		//2014-7-25 	LinZh107   add for p2p
	int 	protocol;
	pthread_t p2p_thread_id;

}tp_server_info_t;

typedef struct __pack_head_t
{
	unsigned long	frame_no;  	//视频帧序号DWORD
	unsigned long	frame_size;	//视频帧长度
	unsigned long 	pack_size; 	//小数据包长度
	unsigned char	pack_count;	//小数据包数量
	unsigned char 	pack_no;   	//视频帧中拆分的小数据包序号
	unsigned char	frame_type;
	unsigned char	reserved[1];
}pack_head_t;

#define	MAX_NET_PACK_SIZE	1400
typedef struct __net_pack_t
{
	pack_head_t	pack_head;
	unsigned	char	pack_data[MAX_NET_PACK_SIZE];
}net_pack_t;

typedef struct __search_device_param_t
{
	char	*pData;
	int		sockfd;
	int		device_num;
}search_device_param_t;

#define HEART_BEART_DEAD_TIME 10
typedef struct __remote_play_t
{
	int	run_flag;
	int zte_play_flag; //modify by zhangkk
	int frame_num;
	int remote_play_sock;
	unsigned long user_id;
	msg_play_record_t msg_play_record;
	pthread_t hRomtePlayThread_id;
	PLAYRECORD_DECODE_CALLBACK playrecord_decode_callback;
	void *playrecord_decode_callback_context;

	PLAYRECORD_STATUS_CALLBACK playrecord_status_callback;
	void *playrecord_status_callback_context;
}remote_play_t;

typedef struct __keepalive_t
{
	char ip[48];
	short port;
	short reserverd;
	char user_name[24];
	char user_password[32];
	char puid[24];
}keepalive_t;

typedef struct __deviece_info_t
{
	keepalive_t keepalive;
	time_t refresh_time;
	int heart_beat_num;
	struct __deviece_info_t *next;
}device_info_t;

typedef struct __dev_online_t
{
	char ip[48];
	short port;
	char user_name[24];
	char user_password[32];
}dev_online_t;


int init_my_log(char *filename);
void my_log(const char *format, ...);


#ifdef  __cplusplus
}
#endif
#endif //__CLIENTNETLIB_HH_H__

