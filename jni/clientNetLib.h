#ifndef __CLIENTNETLIB_H__
#define __CLIENTNETLIB_H__

#include 		<stdio.h>
#include		<string.h>
#include 		<stdlib.h>
#include 		<assert.h>
#include        <sys/types.h>   		// basic system data types#include        <sys/socket.h>  		// basic socket definitions#include        <sys/time.h>    		// timeval{} for select()#include        <netinet/in.h>  		// sockaddr_in{} and other Internet defns#include		<netinet/tcp.h>
#include        <arpa/inet.h>   		// inet(3) functions#include        <netdb.h>
#include        <sys/ioctl.h>
#include        <net/if.h>
#include        <pthread.h>
#include        <errno.h>
#include        <signal.h>
#include        <fcntl.h>
#include		<semaphore.h>
#include		<sys/stat.h>
#include        <unistd.h>

#include <udtc/jni/udtc/udtcLib.h>
#include <udtc/jni/udtc/proto.h>

#define	MSG_VERSION_LEN				24
#define	MSG_ADDR_LEN				16
#define	MSG_ALARM_INFO_LEN			48
#define MSG_ALARM_INFO_GROUP		5
#define MAX_TRANSPARENT_CMD_NUM		1024
#define	MSG_RECORD_FILENAME_LEN		48
#define	MSG_DISK_NAME_LEN			12
#define	MSG_MAX_PART_NUM			4
#define	MSG_MAX_HARDDISK_NUM		8
#define	MSG_MAX_LOG_DESC_LEN		48
#define	MSG_USER_NAME_LEN			24
#define	MSG_PASSWD_LEN				24
#define MAX_PTX_CMD_BUM				128
#define SEARCH_PORT					13451
#define PC_SEARCH_PORT				13452
#define	MSG_HOSTNAME_LEN			24
#define	MSG_MAC_ADDR_SIZE			24
#define	MSG_IP_SIZE					16
#define	MAX_DEV_NUM					1024*5
//add by fengzx 2011-07-05
#define FLAG_PORT_OFFSET		0x0609
#define PORT_OFFSET_TCP			1
#define PORT_OFFSET_TALK			3
#define PORT_OFFSET_UDP			4
//end add

//modify  LinZh107  2014-7-25
#define	VIDEO_FRAME	1
#define	AUDIO_FRAME	3

#define	PROTOCOL_TCP		0x01
#define	PROTOCOL_UDT		0x02
#define	PROTOCOL_P2P		0x03
#define	PROTOCOL_UDP		0x04
#define	PROTOCOL_MCAST		0x05

#define	ACK_ERROR_OTHER		0xff	//其他错误#define	ACK_SUCCESS			0x00	//成功#define	ACK_ERROR_NAME		0x01	//用户名错误#define	ACK_ERROR_PWD		0x02	//密码错误#define	ACK_ERROR_RIGHT		0x03	//权限不够#define	ACK_ERROR_CHANNEL	0x04	//错误的通道号#define	ACK_ERROR_PTZ_BUSY	0x05	//云台忙,暂时无法操作#define NET_EXCEPTION		0x06    //网络异常#define TRANSPARENT_SUCCESS 0x00	//透明传输成功#define TRANSPARENT_EXCEPTION 0xF0  //透明传输异常
#define	MSG_LOGIN			0x0001	//登陆#define	MSG_LOGOUT			0x0002	//注消#define	MSG_OPEN_CHANNEL	0x0003	//打开通道#define	MSG_CLOSE_CHANNEL	0x0004	//关闭通道#define	MSG_START_SEND_DATA	0x0005	//开始发送数据#define	MSG_STOP_SEND_DATA	0x0006	//停止发送数据#define	MSG_PTZ_CONTROL		0x0007	//云台控制#define	MSG_SESSION_ID		0x0009	//#define	MSG_MATRIX_PROTOCOL	0x000a
#define	MSG_SET_PARAM		0x0401	//设置参数#define	MSG_GET_PARAM		0x0402	//获取参数#define	MSG_RESET_VIDEO_PARAM	0x0403	//设置视频参数默认值
#define	MSG_GET_RECORD_LIST	0x0801	//查询录像#define	MSG_SET_TIME		0x0802	//设置时间#define	MSG_GET_TIME		0x0803	//获取时间#define	MSG_VERSION_INFO	0x0804	//版本信息#define	MSG_HARDDISK_INFO	0x0805	//硬盘信息#define	MSG_SYSTEM_STATUS	0x0806	//系统状态#define	MSG_GET_LOG			0x0807	//查询日志#define	MSG_DELETE_LOG		0x0808	//删除日志#define	MSG_ALARM_INFO		0x0809	//告警信息#define	MSG_ALARM_CONTROL	0x080a	//告警上传控制#define	MSG_UPGRADE_STATUS	0x080b	//升级状态#define	MSG_HEARTBEAT		0x080c	//心跳包#define	MSG_HEARTBEAT_PORT	0x080d	//心跳包端口消息#define MSG_GET_USERINFO	0x080e
#define MSG_ADD_USER		0x080f
#define MSG_DELETE_USER		0x0810
#define MSG_MODIFY_USER		0x0811

#define MSG_FORCE_KEYFRAME	0x0830	//强迫I帧
#define MSG_TRANSPARENT_CMD	0x0814  //用于透明传输#define MSG_GET_AP_INFO		0x0815 //搜索无线路由器信息//#define MSG_JOIN_AP			0x0816 //加入无线路由器
#define MSG_START_TRANSPARENT		0x0816 //开始透明传输#define MSG_STOP_TRANSPARENT		0x0817 //停止透明传输#define MSG_SEND_TRANSPARENT		0x0818 //发送透明传输数据#define MSG_REV_TRANSPARENT			0x0819 //接收透明传输数据#define	MSG_BROADCAST		0x0f01	//广播探测消息
//透明传输控制子类型

//////////////////////////////////////////////////////////////////////////
#define MSG_REMOTE_START_HAND_RECORD	0x0840  //开始手动录像#define MSG_REMOTE_STOP_HAND_RECORD		0x0841	//停止手动录像#define MSG_DELETE_RECORD_FILE			0x0842  //删除录像文件#define	MSG_HAND_RESTART				0x0843	//手动重启#define	MSG_DEFAULT_SETTING				0x0844	//默认参数设置#define MSG_PARTITION					0x0845	//分区#define MSG_FORMAT						0x0846	//格式化#define MSG_FORMAT_STATUS				0x0847  //格式化状态#define MSG_FDISK_STATUS				0x0848  //#define MSG_START_ALARM					0x0849  //告警布防#define MSG_STOP_ALARM					0x0850	//告警撤防#define	MSG_BROADCAST_DEVICE			0x0851	//广播搜索设备#define	MSG_BROADCAST_IP				0x0852	//广播修改IP#define MSG_DEFAULTOUTPUT_SETING		0x0853	//出厂设置#define MSG_VERSION_SETING				0x0854  //版本设置#define MSG_INPUTFILE_CONFIG			0x0855  //导入文件#define MSG_OUTPUTFILE_CONFIG			0x0856  //导出文件
#define MSG_CCD_DEFAULT					0x0857
#define MSG_ITU_DEFAULT					0x0858

#define MSG_START_BACK					0x0859
#define MSG_STOP_BACK					0x0860

//add 2009.07.15
#define MSG_3G_SIGNAL					0x0861
//////////////////////////////////////////////////////////////////////////

//参数控制子类型
#define	PARAM_AUTO_POWEROFF        0x0001
#define	PARAM_AUTO_POWERON         0x0002
#define	PARAM_USER_INFO            0x0003
#define	PARAM_HAND_RECORD          0x0004
#define	PARAM_NETWORK_CONFIG       0x0005
#define	PARAM_MISC_CONFIG          0x0006
#define	PARAM_PROBER_ALRAM         0x0007
#define	PARAM_RECORD_PARAM         0x0008
#define	PARAM_TERM_CONFIG          0x0009
#define	PARAM_TIMER_RECORD         0x000A
#define	PARAM_VIDEO_LOSE           0x000B
#define	PARAM_VIDEO_MOVE           0x000C //MOTION#define	PARAM_VIDEO_CONFIG         0x000D
#define	PARAM_VIDEO_ENCODE         0x000E
#define	PARAM_ALARM_PARAM          0x000F

#define PARAM_COM_CONFIG		   0x0010
#define PARAM_OSD_SET			   0x0011
#define PARAM_OVERLAY_INFO		   0x0012
#define PARAM_MOTION_ZOOM		   0x0013
#define PARAM_PPPOE				   0x0014
#define	PARAM_RECORD_INFO	       0x0015
#define PARAM_WIRELESS_CONFIG	   0x0017
#define PARAM_MAIL_CONFIG		   0X0018
#define PARAM_FTP_CONFIG		   0X0019
#define PARAM_NA208_INFO		   0x001A
#define PARAM_MATRIX_INFO		   0x001B
#define PARAM_TIMER_CAPTURE		   0x001C
#define PARMA_IMMEDIATE_CAPTURE	   0x001E
#define PARMA_PTZ_CURISE		   0x001F //预置位巡航 20090413 add yangll
//华为
#define PARAM_CENTER_SERVER_CONFIG		   0x0020
#define PARAM_LENS_INFO_CONFIG			   0x0021

#define PARAM_SERIAL_INFO		   0x0022
#define PARAM_TRANSPARENCE_INFO	   0x0023
#define PARAM_SLAVE_ENCODE		   0x0024
#define PARAM_DEVICE_INFO		   0x0025
#define PARAM_AUDIO_INFO		   0x0026
#define PARAM_MEMBER_INFO		   0x0027
#define PARAM_CAMERA_INFO		   0x0028
#define PARAM_NAS_INFO			   0x0029
#define PARAM_ITU_INFO			   0x002A
#define PARAM_TIMER_COLOR2GREY     0x002B
//定时重启
#define PARAM_TIMER_RESTART		   0x002C //add by wy 2009.07.08
#define PARAM_USB_TYPE			   0x002F //----------- add by wy 2009.4.22
//CDMA
#define PARAM_CDMA_TIME			   0x1079 //2009.03.11#define PARAM_CDMA_ALARM_ENABLE    0x1080 //报警触发拨号#define PARAM_CDMA_NONEGO_ENABLE   0x1081 //外部触发拨号#define PARAM_CDMA_ALARM_INFO      0x1082 //报警配置信息#define PARAM_CDMA_STREAM_ENABLE   0x1083 //CDMA自动下线
//解码器参数
#define PARAM_NETSTAR_CONFIG			0x0201
#define PARAM_DECODE_NUM				0x0202
#define PARAM_DECODE_STAT				0x0203
#define PARAM_DEFAULT_ENCODER			0x0204
#define PARAM_DECODE_OSD				0x0205
#define PARAM_DECODER_BUF_TIME			0x0206
#define PARAM_DECODER_BUF_FRAME_COUNT	0x0208

#define PARAM_3511_SYSINFO		   0x0090

//3511配置
#define PARAM_H3C_INFO					   0x0071

//h3c
#define PARAM_H3C_PLATFORM_CONFIG		   0x0030
#define PARAM_H3C_NETCOM_CONFIG			   0x0031

//六盘水煤税监控系统项目 //add 2009-02-09
#define PARAM_LPS_CONFIG_INFO               0x0032

//keda
#define PARAM_KEDA_PLATFORM_CONFIG		   0x0040
//BELL
#define PARAM_BELL_PLATFORM_CONFIG		   0x0050
//ZTE
#define PARAM_ZTE_PLATFORM_CONFIG			0x0052
#define PARAM_ZTE_RECOVERDEF_CONFIG			0x0053
#define PARAM_UDP_NAT_CLINTINFO				0x0055

//互信互通
#define	PARAM_HXHT_PLATFORM_CONFIG			0X0062	//add by lch 2007-06-15#define	PARAM_HXHT_ALARM_SCHEME				0X0063
#define PARAM_HXHT_NAT						0x0064
//烽火
#define PARAM_FENGH_NAT						0x0065
//公信
#define PARAM_GONGX_NAT						0x0066
//海斗
#define PARAM_HAIDO_PARAM                   0x0067
#define PARAM_HAIDO_NAS						0x0070
//浪涛
#define PARAM_LANGTAO_PARAM                 0x0068
//UT
#define PARAM_UT_PARAM						0x0069

//神州
#define PARAM_SZ_INFO					    0x0072

//中兴立维
#define PARAM_ZXLW_PARAM                    0x0073 //2009-03-04
//亿讯
#define PARAM_YIXUN_PARAM                   0x0074

//校时
#define PARAM_CHECK_TIME					0x0075 //--- add by wy 2009.05.06
//ZTE Stream
#define PARAM_ZTE_STREAM					0x0076 //---- add by 2009.05.07
//ZTE Online
#define PARAM_ZTE_ONLINE					0x0077 //---- add by 2009.05.07
//键盘消息参数
#define PARAM_KEY_WEB_PWD					0x0080
#define PARAM_KEY_USER_PWD					0x0081
#define PARAM_KEY_SERVER_INFO				0x0082
#define PARAM_KEY_ENC_INFO					0x0083
#define PARAM_KEY_ENC_CHANNEL_INFO			0x0084
#define PARAM_KEY_DEC_INFO					0x0085
#define PARAM_KEY_TOUR_INFO					0x0086
#define PARAM_KEY_TOUR_STEP_INFO			0x0087
#define PARAM_KEY_GROUP_INFO				0x0088
#define PARAM_KEY_GROUP_STEP_INFO			0x0089

//云台控制子类型

// 协议类型
#define PTZ_SANTACHI		0x01
#define PTZ_PELCO_D			0x02
#define PTZ_PELCO_P			0x03
#define PTZ_B01				0x04
#define PTZ_SANTACHI_IN		0x05

//矩阵类型
#define	MATRIX_LoginConnect          0x001

// 1   Login connect
// 2   Login with  operater-password
// 3   login-in Auto
// 4   login-out
// 5   Monitor select
// 6   Camera select
// 7   qu wei select
// 8   Tour  select
// 9   Group select
// 10   Back Seq
// 11   Foward Seq
//12   DEC -1
//13  INC +1
//14  PAUSH
//15  Stop seq
//16  Hold ON
//17   Hold OFF
//18   Display  ALL
//19   DISPLAY TIME
//20   Display TITLE
//21   DISPLAY STATUS
//22   AlarmAreaStatus  防区状态的显示
//23   DetectorStatus   探测器状态的显示
//24  JUMP
//25   MSsetupCommand   功能具体内容由矩阵决定
//26  Display Status
//27  Request Monitor Status
//28  Request Monitor Message
//SetupCommand 设置命令序号
#define	MATRIX_SetupEnter             0x02b  //0  SetupEnter#define	MATRIX_SetupDATA1             0x02c  //1  SetupDATA1#define	MATRIX_SetupDATA2             0x02d  //2  SetupDATA2#define	MATRIX_SetupDATA3             0x02e  //3  SetupDATA3#define	MATRIX_SetupDATA4             0x02f  //4  SetupDATA4#define	MATRIX_SetupDATA5             0x030  //5  SetupDATA5#define	MATRIX_SetupDATA6             0x031  //6  SetupDATA6#define	MATRIX_SetupDATA7             0x032  //7  SetupDATA7#define	MATRIX_SetupDATA8             0x033  //8  SetupDATA8#define	MATRIX_SetupDATA9             0x034  //9  SetupDATA9#define	MATRIX_SetupDATA0             0x035  //10  SetupDATA0#define	MATRIX_SetupUP                0x036  //11  SetupUP#define	MATRIX_SetupDOWN              0x037  //12  SetupDOWN#define	MATRIX_SetupLEFT              0x038  //13  SetupLEFT#define	MATRIX_SetupRIGHT             0x039  //14  SetupRIGHT#define	MATRIX_SetupINC               0x03a  //15  SetupINC#define	MATRIX_SetupDEC               0x03b  //16  SetupDEC#define	MATRIX_SetupESC               0x03c  //17  SetupESC#define	MATRIX_SetupSET               0x03d  //18  SetupSET#define	MATRIX_SetupNEXT              0x03e  //19  SetupNEXT#define	MATRIX_SetupPRE               0x03f  //20  SetupPRE#define	MATRIX_SetupPOPUP             0x040  //21  SetupPOP-UP#define	MATRIX_SetupPRINT             0x041  //22  SetupPRINT#define	MATRIX_SetupCLEAR             0x042  //23  SetupCLEAR#define	MATRIX_SetupSAVE              0x043  //24  SetupSAVE#define	MATRIX_SetupREAD              0x044  //25  SetupREAD#define	MATRIX_SetupDELE              0x045  //26  SetupDELE
//AskAndAnswer询问和问答序号
#define	MATRIX_AskDeveiceID        0x046  //0 AskDeveiceID  询问设备代码#define	MATRIX_AnswerDeveiceID        0x047 //1 AnswerDeveiceID  回答设备代码#define	MATRIX_AskSoftwareVer         0x048 //2 AskSoftwareVer  询问软件版本信息#define	MATRIX_AnswerSoftwareVer      0x049 //3 AnswerSoftwareVer  回答软件版本信息#define	MATRIX_AskBoardStatus         0x04a //4 AskBoardStatus  询问插板信息#define	MATRIX_AnswerBoardStatus      0x04b //5 AnswerBoardStatus  回答插板信息
//AlarmControl 报警控制序号
#define	MATRIX_DefendSetup            0x04c //0 Defend setup  设防#define	MATRIX_DefendSolve            0x04d //1  Defend solve撤防#define	MATRIX_BellOFF   	          0x04e //2  Bell OFF  警铃关#define	MATRIX_BellON                 0x04f //3  Bell ON警铃开#define	MATRIX_BRACHRODE              0x050 //4  BRACH RODE     旁路#define	MATRIX_CLEARRODE              0x051 //5  CLEAR RODE     清旁路#define	MATRIX_RecordON               0x052 //6  Record ON       录像#define	MATRIX_RecordOFF              0x053 //7  Record OFF      停录#define	MATRIX_AlarmInc               0x054 //8  Alarm inc       报警巡视+1#define	MATRIX_ALarmDec               0x055 //9  ALarm dec       报警巡视-1#define	MATRIX_ResetOneRecord         0x056 //10  reset one record 报警复位单1当前#define	MATRIX_ResetAllRecord         0x057 //11  Reset all Record  报警复位全部#define	MATRIX_RecordDisp             0x058 //12  Record disp       报警记录显示#define	MATRIX_WarnModeBYHand         0x059 //13 Warn Mode BY hand 报警模式-人工#define	MATRIX_WarnModeTime           0x05a //14 Warn Mode Time    报警模式-定时#define	MATRIX_WaringRecordExit       0x05b //15 WARING RECORED EXIT   报警记录显示退出//解码器控制序号：
//云台摇杆控制
#define	MATRIX_PANUP				  0x05c //0 PAN UP (Speed 1-8)#define	MATRIX_PANDOWN                0x05d //1  PAN DOWN(Speed 1-8)#define	MATRIX_PANRIGHT               0x05e //2  PAN RIGHT(Speed 1-8)#define	MATRIX_PANLEFT                0x05f  //3  PAN LEFT(Speed 1-8)#define	MATRIX_PANRIGHTUP             0x060 //4  PAN RIGHT UP(Speed 1-8)#define	MATRIX_PANLEFTUP              0x061 //5  PAN LEFT UP(Speed 1-8)#define	MATRIX_PANRIGHTDOWN           0x062 //6  PAN RIGHT DOWN(Speed 1-8)#define	MATRIX_PANLEFTDOWN            0x063 //7  PAN LEFT DOWN(Speed 1-8)#define	MATRIX_PANSTOP                0x064 //8  PAN STOP
//光圈调节
#define	MATRIX_IRISOPEN               0x065 //9  IRIS OPEN#define	MATRIX_IRISCLOSE              0x066 //10  IRIS CLOSE#define	MATRIX_FOCUSSTOP              0x067 //11  IRIS STOP#define	MATRIX_FOCUSNEAR              0x068 //12  FOCUS NEAR#define	MATRIX_FOCUSFAR               0x069 //13  FOCUS FAR#define	MATRIX_FOCUSTITLE             0x06a //14  FOCUS TITLE#define	MATRIX_FOCUSWIDE              0x06b //15  FOCUS WIDE#define	MATRIX_AUTOFOCUSENABLE        0x06c //16  AUTO focus enable   自动对焦点允许#define	MATRIX_AUTOFOCUSDISABLE       0x06d //17  AUTO focus disable  自动对焦点禁止#define	MATRIX_AUTOIRISENABLE         0x06e //18  auto iris  enable  自动光圈允许#define	MATRIX_AUTOIRISDISABLE        0x06f //19  auto iris  disable 自动光圈禁止#define	MATRIX_STOP                   0x070 //20   STOP (--NEAR WIDE FAR TELE) 镜头运动停
//云台自动控制
#define	MATRIX_AUTOHPAN               0x071 //21  AUTO H PAN     云台自动水平运动#define	MATRIX_AUTOPANSTOP            0x072 //22  AUTO PAN STOP  云台自动水平运动停止#define	MATRIX_RANDMHPAN              0x073 //23  RANDM H PAN    云台随机水平运动#define	MATRIX__RANDMPANSTOP          0x074 //24  RANDM PAN STOP  云台随机水平运动停止//
#define MATRIX_AUX1ON                 0x075 //25   AUX1 ON      辅助开关开#define	MATRIX_AUX1OFF                0x076 //26  AUX1 OFF              辅助开关关#define	MATRIX_RainBrushOpen          0x077 //27  Rain Brush Open 雨刷开关开#define	MATRIX_RainBrushClose         0x078 //28  Rain Brush Close 雨刷开关关#define	MATRIX_DeforstOpen            0x079 //29  Deforst Open    除霜开关开#define	MATRIX_DeforstClose           0x07a //30  Deforst Close   除霜开关关#define	MATRIX_HeartOn                0x07b //31  Heart On        加热开#define	MATRIX_HeartClose             0x07c //32  Heart Close     加热关#define	MATRIX_PowerOn                0x07d //33 Power On        电源开关开#define	MATRIX_PowerClose             0x07e //34  Power Close     电源开关关
//摄象机
#define	MATRIX_PCAMERPositioncall     0x07f //35   CAMER Position call 1-999    摄像机位置调用#define	MATRIX_PresetCall     	      0x080	//36   Preset  call     预置位置调用#define	MATRIX_PresetSet     	      0x081	//37   Preset  Set      预置位置设置#define	MATRIX_PresetReset    	      0x082	//38  Preset Reset     预置位置复位#define	MATRIX_FunctionSelect 	      0x083	//39   Function select  功能选择#define	MATRIX_TURNOVER       	      0x084	//40  TURN OVER        转向
#define	MATRIX_CAMENTERSETUPMENU      0x085 //41  CAM ENTER SETUP MENU  摄像机设置菜单进入#define	MATRIX_CURUP                  0x086 //42  CUR UP           摄像机设置-上#define	MATRIX_CURDN                  0x087 //43  CUR DN           摄像机设置-下#define	MATRIX_CURLF                  0x088 //44  CUR LF           摄像机设置-左#define	MATRIX_CURRT                  0x089 //45  CUR RT           摄像机设置-右#define	MATRIX_CONFIREM               0x08a //46  CONFIREM         摄像机设置-确定#define	MATRIX_ESC                    0x08b //47  ESC              摄像机设置-取消#define	MATRIX_INC                    0x08c //48  INC +1               摄像机设置-加一#define	MATRIX_DEC                    0x08d //49  DEC -1               摄像机设置-减一
//智能
#define	MATRIX_SMARTBegin             0x08e //50  SMART-Begin       智能-开始#define	MATRIX_SMARTEnd               0x08f //51  SMART_End#define	MATRIX_SMARTRepeat            0x090 //52  SMART_Repeat#define	MATRIX_SMART_Run              0x091 //53  SMART_Run
//********************************************************************************************
// 命令类型
#define PTZ_UPSTART			0x01
#define PTZ_UPSTOP			0x02
#define PTZ_DOWNSTART		0x03
#define PTZ_DOWNSTOP		0x04
#define PTZ_LEFTSTART		0x05
#define PTZ_LEFTSTOP		0x06
#define PTZ_RIGHTSTART		0x07
#define PTZ_RIGHTSTOP		0x08
#define PTZ_LEFTUPSTART		0x09
#define PTZ_LEFTUPSTOP		0x0a
#define PTZ_RIGHTUPSTART	0x0b
#define PTZ_RIGHTUPSTOP		0x0c
#define PTZ_LEFTDOWNSTART	0x0d
#define PTZ_LEFTDOWNSTOP	0x0e
#define PTZ_RIGHTDOWNSTART	0x0f
#define PTZ_RITHTDOWNSTOP	0x10
#define PTZ_PREVPOINTSET	0x11
#define PTZ_PREVPOINTCALL	0x12
#define PTZ_PREVPOINTDEL	0x13
#define PTZ_ZOOMADDSTART	0x14
#define PTZ_ZOOMADDSTOP		0x15
#define PTZ_ZOOMSUBSTART	0x16
#define PTZ_ZOOMSUBSTOP		0x17
#define PTZ_FOCUSADDSTART	0x18
#define PTZ_FOCUSADDSTOP	0x19
#define PTZ_FOCUSSUBSTART	0x1a
#define PTZ_FOCUSUBSTOP		0x1b
#define PTZ_IRISADDSTART	0x1c
#define PTZ_IRISADDSTOP		0x1d
#define PTZ_IRISSUBSTART	0x1e
#define PTZ_IRISSUBSTOP		0x1f
#define PTZ_GOTOZEROPAN		0x20
#define PTZ_FLIP180			0x21
#define PTZ_SETPATTERNSTART	0x22
#define PTZ_SETPATTERNSTOP	0x23
#define PTZ_RUNPATTERN		0x24
#define PTZ_SETAUXILIARY	0x25
#define PTZ_CLEARAUXILIARY	0x26
#define PTZ_AUTOSCANSTART         0x27	//自动线扫-(开始)#define PTZ_AUTOSCANSTOP          0x28	//自动线扫-(停止)#define PTZ_RANDOMSCANSTART       0x29	//随机线扫-(开始)#define PTZ_RANDOMSCANSTOP        0x2a	//随机线扫-(停止)#define PTZ_CURISESTART		      0x2b	//巡航开始#define PTZ_CURISESTOP            0x2c	//巡航停止#define PTZ_TYPE_REPOSITION       0x2d	//巡航归位#define PTZ_POINCENTER			  0x30
#define PTZ_VIEWCENTER			  0x31
//#define PTZ_TYPE_TRANSPARENCE			0x0100  //透明传输(设备端不作协议转换，直接传送网络命令) //add by liuqs

//常量定义
#define MAX_TITLE_LEN				8

typedef struct _msg_time_t
{
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
} msg_time_t;

typedef struct _msg_alarm_info_t
{
	unsigned int alarm_ack;
	msg_time_t tm;
	unsigned int alarm_id;
	unsigned int alarm_channel;
	unsigned int alarm_type;
	char ip[MSG_ADDR_LEN];
	char alarm_info[MSG_ALARM_INFO_LEN];
} msg_alarm_info_t;


#define MSG_MAX_CK_CH_NUM 8
typedef struct __ck_dev_data_t //add by LinZh107
{
	unsigned int dev_id;
	char ck_type[MSG_MAX_CK_CH_NUM];
	float ck_val[MSG_MAX_CK_CH_NUM];
} ck_dev_data_t;


typedef struct __msg_transparent_rev_t
{
	int cmd_size;
	char cmd_buf[MAX_TRANSPARENT_CMD_NUM];
} msg_transparent_rev_t;

typedef struct _msg_ptz_control_t
{
	int channel;						//通道号
	int param;						//预置点号或速度或辅助开关号
	char user_name[12];
	char user_pwd[12];
//  int cmd_size;						//用于透明传输 add by liuqs on 2007-4-11
//	char cmd_buf[MAX_PTX_CMD_BUM];		//用于透明传输 add by liuqs on 2007-4-11
} msg_ptz_control_t;

typedef struct _msg_matrix_login_t
{
	char user_name[12];
	char user_pwd[12];
} msg_matrix_login_t;

//分区 格式化
typedef struct _msg_hard_fdisk_t
{
	unsigned int disk_id;
	unsigned int part_num;
} msg_hard_fdisk_t;

typedef struct _msg_hard_fdisk_status_t
{
	unsigned int disk_id;
	unsigned int fdisk_status;
} msg_hard_fdisk_status_t;

typedef struct _msg_part_format_t
{
	unsigned int disk_id;
	unsigned int part_id;
	unsigned int part_type; // 0 录像分区; 1 备份分区
} msg_part_format_t;

typedef struct _msg_hard_format_status_t
{
	unsigned int disk_id;
	unsigned int part_id;
	unsigned int format_status;
} msg_hard_format_status_t;

#define	MOVE_JPEG_FLAG		0x00000081	//移动报警图片#define	LOSE_JPEG_FLAG		0x00000082	//丢失报警图片#define	PROBER_JPEG_FLAG	0x00000083	//探头报警图片#define	ALL_JPEG_FLAG		0x00000084	//所有报警图片#define	HAND_RECORD_FLAG	0x00000001	//手动录像#define	MOVE_RECORD_FLAG	0x00000002	//移动录像#define	LOST_RECORD_FLAG	0x00000004	//丢失录像#define	INPUT_RECORD_FLAG	0x00000008	//探头录像#define	TIMER_RECORD_FLAG	0x00000010	//定时录像#define	ALL_RECORD_FLAG		0x0000001f	//所有录像
typedef struct _msg_search_file_t
{
	int channel;		//通道号
	int record_type;//录像类型
	int flag;//分区标志,0:录像分区,1:备份分区
	msg_time_t s_tm;//开始时间
	msg_time_t e_tm;//结束时间
}msg_search_file_t;

typedef struct _msg_file_info_t
{
	char filename[MSG_RECORD_FILENAME_LEN];
	unsigned long filesize;					//文件大小
//	msg_time_t	s_tm;							//文件开始时间
//	msg_time_t	e_tm;							//文件结束时间
} msg_file_info_t;

typedef struct
{
	msg_time_t start_time; /*段开始时间*/
	msg_time_t end_time; /*段结束时间*/
	unsigned int data_start_offset; /*数据起始位置*/
	unsigned int data_end_offset; /*数据结束位置*/
	unsigned int index_start_offset; /*帧索引起始位置*/
	unsigned int index_end_offset; /*帧索引结束位置*/
	unsigned int record_type; /*段录像类型*/
	char file_name[MSG_RECORD_FILENAME_LEN];/*文件名*/
} msg_fragment_t;

typedef struct __del_file_info
{
	int ch;
	int year;
	int month;
	int day;

	int start_hour;
	int start_minute;
	int start_second;

	int end_hour;
	int end_minute;
	int end_second;

}del_file_info_t;


typedef struct _msg_delete_file_t
{
	char filename[MSG_RECORD_FILENAME_LEN];
} msg_delete_file_t;

typedef struct _msg_version_info_t
{
	char version_number[MSG_VERSION_LEN];	//版本号
	char device_type[MSG_VERSION_LEN];		//产品型号
	char version_date[MSG_VERSION_LEN];		//版本日期
	unsigned int server_type;
	unsigned char is_support_d1 :1;			//D1 0: 不支持, 1:支持
	unsigned char is_support_wifi :1;			//无线 0: 不支持, 1:支持
	unsigned char reserve_1 :6;
	char sub_type;	//子设备类型, 0: 无; 1: NT9030; 2:NT6032; 3:NT6042; 4:NT6052 5:NT4062 6:NT9062/NT9072
	char unusd[2];
	char device_serial_no[32];			//产品序列号
	char leave_factory_date[MSG_VERSION_LEN];	//出厂日期
	char kernel_version[MSG_VERSION_LEN];	//内核版本
} msg_version_info_t;

#pragma pack(4)
typedef struct _msg_part_info_t
{
	unsigned char part_no;                //分区号
	unsigned char part_type;              //分区类型,0:录像分区,1:备份分区
	unsigned char cur_status;             //当前状态,0:未挂载,1:未用,2:正在使用
	unsigned char reserved;               //保留位
	unsigned char reserved1[4];
	uint64_t total_size;             //分区总容量
	uint64_t free_size;              //分区剩余容量
} msg_part_info_t;
typedef struct _msg_hardisk_info_t
{
	unsigned char disk_no;                //硬盘号
	unsigned char part_num;               //分区数
	unsigned char filesystem;             //文件系统,0:未知,1:EXT3,2:FAT32
	unsigned char part_status;            //分区状态,0:未分区,1:已分区
	char diskname[MSG_DISK_NAME_LEN];    //硬盘名
	uint64_t harddisk_size;          //硬盘总容量
	msg_part_info_t partinfo[MSG_MAX_PART_NUM];
} msg_harddisk_info_t;
typedef struct _msg_harddisk_data_t
{
	int disk_num;                       //硬盘数
	int reserved;
	msg_harddisk_info_t hdinfo[MSG_MAX_HARDDISK_NUM];
} msg_harddisk_data_t;
typedef struct _msg_system_status_t
{
	int cpu_percent;		//CPU利用率
	int run_hour;			//运行小时数
	int run_minute;			//运行分钟数
	int mem_percent;
	int flash_size;
	msg_harddisk_data_t disk_info;  //disk info
	int unused[64];
} msg_system_status_t;

#pragma pack()

typedef struct _msg_search_log_t
{
	int log_type;		//日志类型
	msg_time_t s_tm;
	msg_time_t e_tm;
} msg_search_log_t;

#define	LOG_ALL                 0	//所有日志#define	LOG_SYSTEM              1	//系统日志#define	LOG_ALARM               2	//报警日志#define	LOG_OPERATE             3	//操作日志#define	LOG_NETWORK             4	//网络日志
typedef struct _msg_log_info_t
{
	int log_type;						//日志类型
	msg_time_t tm;//日志时间
	char log_desc[MSG_MAX_LOG_DESC_LEN];
}msg_log_info_t;
typedef struct _msg_alarm_control_t
{
	int status;		//释放定制告警信息,0:不定制,1:定制
} msg_alarm_control_t;
typedef struct _msg_upgrade_status_t
{
	int percent;
} msg_upgrade_status_t;
typedef struct _msg_user_info_t
{
	char user_name[MSG_USER_NAME_LEN];
	char user_pwd[MSG_PASSWD_LEN];
	int local_right;
	int remote_right;
} msg_user_info_t;
typedef struct _msg_download_pic_t
{
	int channel_no;
	unsigned int pic_len;
} msg_download_pic_t;
typedef struct _msg_download_t
{
	char filename[MSG_RECORD_FILENAME_LEN];
} msg_download_t;
typedef struct _msg_file_down_Status
{
	int iSocket;
//	int iConnectStatus;				//0 连接失败;1 连接成功;
//	int iTransfersStatus;			//0 停止传输;1 开始传输;
	int iFileSize;
	int iTransfersSize;
} msg_file_down_Status;
typedef struct _msg_play_record_t
{
	char file_name[MSG_RECORD_FILENAME_LEN];
} msg_play_record_t;
typedef struct _msg_recordplay_t
{
	int total_frame_num;
	int first_frame_no;
	int total_time;
} msg_recordplay_t;
typedef struct __msg_remote_hand_record_t
{
	int channel;
	unsigned char record_hour_len;
	unsigned char record_minute_len;
	unsigned char record_status;
	unsigned char reserve;

} msg_remote_hand_record_t;
typedef struct __sys_config_t
{
	unsigned int dev_type;
	unsigned int sub_type; //1: ST-NT200H-01J  2: ST-NT200H-04J  3:ST-NT200H-08J   4:ST-NT200H-6032  5:ST-NT200H-6032   6:ST-NT200H-8189H 7:其他
	unsigned char chip_num;
	unsigned char channel_num;
	unsigned char prober_num;
	unsigned char output_num;
	unsigned char serial_num;
	unsigned char ccd_enable :1; /* ccd镜头使用与否 0:不启用CCD 1：启用CCD */
	unsigned char ptz_type :7; /* 0:ptz参数可设 1：ptz参数固定，用于高速球 */
	unsigned char adchip_type; /* 0:no ad chip 1:tvp5150  2:TW2815A  3: TW2815AB 4.其他 */
	unsigned char flat_type; /* 平台 0：无 1：中兴-st 2：中兴-zte*/
	unsigned char resolv_capable_m[16]; /*      设备各通道能力级 主码流
	 *      bit0:QCIF bit1:CIF  bit2:HD1 bit3:D1
	 *      bit4:MD1  bit5:QVGA bit6:VGA bit7:SXGA */
	unsigned char resolv_capable_s[16]; /*      设备各通道能力级 子码流
	 *      bit0:QCIF bit1:CIF  bit2:HD1 bit3:D1
	 *      bit4:MD1  bit5:QVGA bit6:VGA bit7:SXGA */
	unsigned char ds_enable_flag :1; /* 双码流使能 */
	unsigned char D1_type :7; /* D1分辨率类型 0:704*576 1:720*576 */
	unsigned char unused[7]; /* 保留  */
} sys_config_t;

//编解码设置
typedef struct _video_encode_t
{
	int video_format;           //制式选择 0:PAL   1:NTSC
	int resolution;             //分辨率,0:QCIF,1:CIF,2:H-D1,3:D1,5:QVGA,6:VGA,240*192 8,320*240 9
	int bitrate_type;           //位率类型,0:CBR,1:VBR
	int level;					//画质,1级~8级/128kbit~3000kbit
	int frame_rate;				//帧率（5-30）
	int Iframe_interval;		//I侦间隔（默认为50）
	int prefer_frame;
	int Qp;
	int encode_type;
} video_encode_t;

//视频设置
typedef struct _video_config_t
{
	unsigned char dis_title[MAX_TITLE_LEN];
	unsigned char brightness;    	//亮度值,范围:0-255
	unsigned char contrast;		//对比度值,范围:0-255
	unsigned char hue;			//色调值,范围:0-255
	unsigned char saturation;		//饱和度值,范围:0-255
} video_config_t;

typedef struct _msg_broadcast_t
{
	int server_type;				//设备类型
	int port_no;					//监听端口
	int net_type;					//网络类型
	char host_name[MSG_HOSTNAME_LEN];	//设备名
	char mac_addr[MSG_MAC_ADDR_SIZE];	//MAC地址
	char ip_addr[MSG_IP_SIZE];		//IP地址
	char net_mask[MSG_IP_SIZE];		//网络掩码
	char gw_addr[MSG_IP_SIZE];		//网关地址
	char version_no[MSG_VERSION_LEN];		//版本号
	unsigned int port_offset;			//端口偏移
	char first_dns_addr[MSG_IP_SIZE];	//主DNS
	char second_dns_addr[MSG_IP_SIZE];	//备用DNS
	unsigned short dns_manu_flag;		//dns 自动获得标志,0: 自动获得, 1: 手动获得
	unsigned short web_port;			//web 端口配置
	char dhcp_flag;
	char unused[3];
	char device_type[MSG_VERSION_LEN];           //产品型号
	sys_config_t sys_config;                     //配置结构体
} msg_broadcast_t;

typedef struct _msg_broadcast_setnetinfo_t
{
	int src_server_type;				//设备类型
	int src_port_no;					//监听端口
	int src_net_type;					//网络类型
	char src_host_name[MSG_HOSTNAME_LEN];					//设备名
	char src_mac_addr[MSG_MAC_ADDR_SIZE];					//MAC地址
	char src_ip_addr[MSG_IP_SIZE];		//IP地址
	char src_net_mask[MSG_IP_SIZE];		//网络掩码
	char src_gw_addr[MSG_IP_SIZE];		//网关地址
	char src_version_no[MSG_VERSION_LEN];		//版本号
	unsigned int src_port_offset;			//端口偏移

	int dst_server_type;				//设备类型
	int dst_port_no;					//监听端口
	int dst_net_type;					//网络类型
	char dst_host_name[MSG_HOSTNAME_LEN];	//设备名
	char dst_mac_addr[MSG_MAC_ADDR_SIZE];	//MAC地址
	char dst_ip_addr[MSG_IP_SIZE];		//IP地址
	char dst_net_mask[MSG_IP_SIZE];		//网络掩码
	char dst_gw_addr[MSG_IP_SIZE];		//网关地址
	char dst_version_no[MSG_VERSION_LEN];		//版本号
	unsigned int dst_port_offset;			//端口偏移

	char src_first_dns_addr[MSG_IP_SIZE];	//主DNS
	char src_second_dns_addr[MSG_IP_SIZE];	//备用DNS
	unsigned short src_dns_manu_flag;		//dns 自动获得标志,0: 自动获得, 1: 手动获得
	unsigned short src_web_port;

	char dst_first_dns_addr[MSG_IP_SIZE];	//主DNS
	char dst_second_dns_addr[MSG_IP_SIZE];	//备用DNS
	unsigned short dst_dns_manu_flag;		//dns 自动获得标志,0: 自动获得, 1: 手动获得
	unsigned short dst_web_port;
	char dst_dhcp_flag;
	char unused[3];
} msg_broadcast_setnetinfo_t;

typedef struct _msg_broadcast_default_t
{
	int server_type;				//设备类型
	int port_no;					//监听端口
	int net_type;					//网络类型
	char host_name[MSG_HOSTNAME_LEN];					//设备名
	char mac_addr[MSG_MAC_ADDR_SIZE];					//MAC地址
	char ip_addr[MSG_IP_SIZE];		//IP地址
	char net_mask[MSG_IP_SIZE];		//网络掩码
	char gw_addr[MSG_IP_SIZE];		//网关地址
	char version_no[MSG_VERSION_LEN];		//版本号
	unsigned int port_offset;			//端口偏移
	char first_dns_addr[MSG_IP_SIZE];	//主DNS
	char second_dns_addr[MSG_IP_SIZE];	//备用DNS
	unsigned short dns_manu_flag;		//dns 自动获得标志,0: 自动获得, 1: 手动获得
	unsigned short web_port;			//web 端口配置

} msg_broadcast_default_t;

typedef struct _msg_wireless_info_t
{
	int current_used;					//是否使用  1:使用   0:未使用
	char essid[64];
	int signalStrength;					//信号强度
	int isEncrypt;						//是否加密  1:加密  0:未加密
	int safety_type;					//1:WEP   3:WPA   2:WPA-PSK
	int safety_option;					//安全选项  1:自动选择   2:开放系统   3:共享密钥
	int key_numb;
	char password[96];					//web 端口配置
} msg_wireless_info_t;

//双向透明传输
typedef struct __serial_info_t
{
	char serial_no;		//串口号,值定为0
	char data_bit;		//数据位,值:5/6/7/8 默认8
	char stop_bit;		//停止位,值:0/1/2 默认1
	char unuserd;
	int verify_rule;    //验证规则,-1:无,0:偶校验,1:奇校验 默认-1
	int baut_rate;      //波特率,协议特定 默认9600
} serial_info_t;

typedef struct __serial_info_config
{
	unsigned char data_bit; //数据位,值:5/6/7/8 默认8
	unsigned char stop_bit; //停止位,值:0/1/2 默认1
	unsigned char reserve[2];
	int verify_rule;    //验证规则,-1:无,0:偶校验,1:奇校验 默认-1
	int baut_rate;      //波特率,协议特定 默认9600
} serial_info_config;

//云台设置
typedef struct __ptz_config_t
{
	unsigned char ptz_addr; //云台地址0 ~ 255
	unsigned char ptz_type; //云台类型索引.PELCO_D-1,PELCO_P-2,松下-3,YAAN-4,santachi-5,santachi_in-6,Matrix750  9
	unsigned char serial_no; //串口号,值默认为1
	unsigned char reserve;
	serial_info_config serial_info;
} ptz_config_t;

typedef struct __msg_transparent_start_t
{
	serial_info_t searial_info;
} msg_transparent_start_t;

typedef struct __msg_transparent_send_t
{
	int cmd_size;
	char cmd_buf[MAX_TRANSPARENT_CMD_NUM];
} msg_transparent_send_t;

//2014-4-15 added by LinZh107
msg_alarm_info_t g_AlarmInfoGroup[MSG_ALARM_INFO_GROUP];

typedef int (*CHANNEL_STREAM_CALLBACK)(void *buf, unsigned long size_buf, void *text);
typedef int (*SERVER_MSG_CALLBACK)(char *pserver_ip, int channel, void *buf, int len, void *context);
typedef int (*ALARM_INFO_CALLBACK)(msg_alarm_info_t msg_alarm_info, void *context);
typedef int (*PLAYRECORD_STATUS_CALLBACK)(void *context);
typedef int (*PLAYRECORD_DECODE_CALLBACK)(void *buf, int len, void *context);
typedef int (*REGISTER_CALLBACK)(int flag, char *ip, short port, char *user_name, char *user_password, char *puid);
typedef int (*UPDATE_STATUS_CALLBACK)(int percent, void *context);
typedef int (*TALK_STREAM_CALLBACK)(char *pserver_ip, void *buf, int len, void *context);
typedef int (*BIDI_TRANSPARENT_CALLBACK)(int current_status, msg_transparent_rev_t msg_transparent_rev, void *context);




#ifdef __cplusplus extern "C" {
#endif

int st_net_initClientNetLib(void);
int st_net_deinitClientNetLib(void);

//服务器信息
long st_net_initServerInfo(char *ip_addr, int port, char *user_name, char *user_pwd, char *cuid, int proto);
int st_net_deinitServerInfo(unsigned long user_id);
int st_net_modifyServerInfo(unsigned long user_id, char *ip_addr, int port, char *user_name, char *user_pwd);

//域名转IP
int st_net_GetIPAddress(char *name, char *ip);

//登陆、注销
// add by LinZh for debug ice
int st_net_set_iceinfo(unsigned long user_id, const char *iceinfo, const int ice_size);
int st_net_userLogin(unsigned long user_id);
int st_net_userLogout(unsigned long user_id, int really_logout);
int st_net_getUserRight(unsigned long user_id);

int st_net_sendMsgToServer(unsigned long user_id, int msg_type, int msg_subtype, void *buf, int len);

//打开通道、关闭通道
int st_net_openChannelStream(long user_id, int channel, int protocol_type, int channel_type);
int st_net_closeChannelStream(unsigned long user_id, int channel, int channel_type);

//开始、停止发送数据
int st_net_startChannelStream(unsigned long user_id, int channel, int stream_type, int channel_type);
int st_net_stopChannelStream(unsigned long user_id, int channel, int stream_type, int channel_type);

//强制I帧
int st_net_forceKeyFrame(unsigned long user_id, int channel);

//语音对讲
int st_net_startDualTalk(unsigned long user_id);
int st_net_stopDualTalk(unsigned long user_id);
int st_net_sendAudioData(unsigned long user_id, char *buf, int len);

//设置参数、获取参数
int st_net_setParam(unsigned long user_id, int type, void *param, int size);
int st_net_getParam(unsigned long user_id, int type, void *param, int size);

//云台控制
int st_net_ptzControl(unsigned long user_id, int type, msg_ptz_control_t info);

//矩阵控制
int st_net_matrixControl(unsigned long user_id, int type, msg_ptz_control_t *info);
int st_net_matrixControl_login(unsigned long user_id, int type, msg_matrix_login_t *info);
int st_net_matrixControl_loginout(unsigned long user_id, int type);

//参数设置
int st_net_defaultSetting(unsigned long user_id);
int st_net_defOutPutSetting(unsigned long user_id);
int st_net_defaultVidioParame(unsigned long user_id, unsigned char channel_no);
//手动重启设备
int st_net_restartDevice(unsigned long user_id);

//分区
int st_net_harddiskPartition(unsigned long user_id, msg_hard_fdisk_t msg_hard_fdisk);
int st_net_fdiskStatus(unsigned long user_id, msg_hard_fdisk_status_t msg_hard_fdisk_status);

//格式化
int st_net_harddiskFormat(unsigned long user_id, msg_part_format_t msg_part_format);
int st_net_harddiskFormatStatus(unsigned long user_id, msg_hard_format_status_t msg_hard_format_status);

//查询录像文件 图片文件
int st_net_queryFileList(unsigned long user_id, msg_search_file_t file_param, msg_file_info_t *p_file_info);

//查询文件(时间)
int st_net_queryFileListByTime(unsigned long user_id, msg_search_file_t file_param, msg_fragment_t *p_file_info);

//删除录像文件 图片文件
int st_net_deleteFile(unsigned long user_id, msg_delete_file_t msg_delete_file);

//设置、获取系统时间
int st_net_setSystemTime(unsigned long user_id, msg_time_t tm);
int st_net_getSystemTime(unsigned long user_id, msg_time_t *tm);

//版本信息
int st_net_getVersionInfo(unsigned long user_id, msg_version_info_t *info);
int st_net_setVersion(unsigned long user_id, msg_version_info_t info);

//硬盘信息
int st_net_getHarddiskInfo(unsigned long user_id, msg_harddisk_data_t *info);

//系统状态
int st_net_getSystemStatus(unsigned long user_id, msg_system_status_t *info);

//查询、删除日志
int st_net_queryLog(unsigned long user_id, msg_search_log_t msg_search_log, msg_log_info_t *p_msg_log_info);
int st_net_deleteLog(unsigned long user_id, msg_search_log_t msg_search_log);

//告警信息控制
int st_net_alarmControl(unsigned long user_id, msg_alarm_control_t param);

//升级状态
int st_net_getUpgradeStatus(unsigned long user_id, msg_upgrade_status_t *info); //upgrade

//用户管理
int st_net_queryUserInfo(unsigned long user_id, msg_user_info_t *p_user);
int st_net_addUser(unsigned long user_id, msg_user_info_t user);
int st_net_deleteUser(unsigned long user_id, msg_user_info_t user);
int st_net_changeUser(unsigned long user_id, msg_user_info_t user); //modify

//远程图片抓拍下载
int st_net_downloadPicture(unsigned long user_id, msg_download_pic_t info, char *pic_name);

//文件(录像、图片)下载MSG_FILENAME_LEN
int st_net_downloadFile(unsigned long user_id, msg_download_t info, char *savefilename,
		msg_file_down_Status* file_down_Status);

//文件下载(时间)
int st_net_downloadFileByTime(unsigned long user_id, msg_fragment_t info, char *savefilename,
		msg_file_down_Status* file_down_Status);

//停止下载
int st_net_stopDownloadFile(unsigned long user_id);

//停止下载(时间)
int st_net_stopDownloadFileByTime(unsigned long user_id);

//录像点播(文件)
unsigned long st_net_startRemotePlay(unsigned long user_id, msg_play_record_t msg_play_record,
		msg_recordplay_t *msg_recordplay);
int st_net_stopRemotePlay(unsigned long remote_play_id);
int st_net_RemotePlayPosition(unsigned long remote_play_id, int current_playpos);

//录像点播(时间)
unsigned long st_net_startRemotePlayByTime(unsigned long user_id, msg_fragment_t msg_play_record,
		msg_recordplay_t *msg_recordplay);
int st_net_startRemotePlayEx(long user_id,msg_fragment_t msg_play_record, msg_recordplay_t *msg_recordplay);
int st_net_stopRemotePlayByTime(unsigned long remote_play_id);
int st_net_RemotePlayPositionByTime(unsigned long remote_play_id, int current_playpos);

//本地录像
int st_net_startRecord(unsigned long user_id, int channel, char *filename); //add by yangll
int st_net_stopRecord(unsigned long user_id, int channel);

//PU手动录像
int st_net_remote_start_hand_record(unsigned long user_id, msg_remote_hand_record_t msg_remote_hand_record);
int st_net_remote_stop_hand_record(unsigned long user_id, msg_remote_hand_record_t msg_remote_hand_record);

//升级系统
int st_net_upgradeSystem(unsigned long user_id, char *file_name);

//获取设备类型
int st_net_getDeviceType(unsigned long user_id); //device

//码流
int st_net_getBitRate(unsigned long user_id, int channel);

//服务器状态
int st_net_getLoginStatus(unsigned long user_id);

//通道状态
int st_net_getChannelStatus(unsigned long user_id, int ichannel);

//心跳消息
int st_net_getHeartbeatStatus(unsigned long user_id);

//广播搜索设备
int st_net_searchDevice(msg_broadcast_t *msg_broadcast, int iMaxCount);

//广播修改IP
int st_net_broadcastSetIp(msg_broadcast_setnetinfo_t msg_broadcast_setnetinfo);

//广播恢复参数设置
int st_net_broadcastDefaultSetting(msg_broadcast_default_t msg_broadcast_default);
//广播恢复出厂设置
int st_net_broadcastFactorySetting(msg_broadcast_default_t msg_broadcast_default);
//无线配置
int st_net_searchWirelessDevice(unsigned long user_id, msg_wireless_info_t *p_wireless_info);

//双向透明传输
int st_net_startBidiTransparentCmd(unsigned long user_id, msg_transparent_start_t msg_transparent_start);
int st_net_stopBidiTransparentCmd(unsigned long user_id);
int st_net_sendBidiTransparentCmd(unsigned long user_id, msg_transparent_send_t msg_transparent_send);

//导入文件
int st_net_inputFileCmd(unsigned long user_id, char *file_name);
//导出文件
int st_net_outputFileCmd(unsigned long user_id, char *file_name);
//////////////////////////////////////////////////////////////////////////

int st_net_registerChannelStreamCallback(unsigned long user_id, int channel, CHANNEL_STREAM_CALLBACK callback,
		void *context);
int st_net_registerServerMsgCallback(unsigned long user_id, SERVER_MSG_CALLBACK callback, void *context);
int st_net_registerServerAlarmInfoCallback(unsigned long user_id, ALARM_INFO_CALLBACK callback, void *context);

int st_net_registerServerPlayrecordStatusCallback(unsigned long remote_play_id, PLAYRECORD_STATUS_CALLBACK callback,
		void *context);
int st_net_registerServerPlayrecordDecodeCallback(unsigned long remote_play_id, PLAYRECORD_DECODE_CALLBACK callback,
		void *context);
int st_net_registerUpdateStatusCallback(unsigned long user_id, UPDATE_STATUS_CALLBACK callback, void *context);
int st_net_registerTalkStreamCallback(unsigned long user_id, TALK_STREAM_CALLBACK callback, void *context);
int st_net_registerTransparentCmdCallback(unsigned long user_id, BIDI_TRANSPARENT_CALLBACK callback, void *context);

//zte
int st_net_registerZTERegisterCallback(REGISTER_CALLBACK callback, void *context);

unsigned long st_net_openRemotePlay(unsigned long user_id, msg_play_record_t msg_play_record);
int st_net_closeRemotePlay(unsigned long play_id);

int st_net_startPlayStream(unsigned long play_id);
int st_net_stopPlayStream(unsigned long play_id);

int st_net_beginplayback(unsigned long user_id);
int st_net_stopplayback(unsigned long user_id);

int st_net_defaultCcdParame(unsigned long play_id);
int st_net_defaultItuParame(unsigned long play_id);

int st_net_RemotePlayMode(unsigned long remote_play_id, int current_frameno, int flag);

int st_net_get3GSignalVlaue(unsigned long user_id);

// 2014-4-1
int tp_net_getAlarmInfo(msg_alarm_info_t *pAlarmInfo, int channel);
int tp_net_get_DevData(int channel, ck_dev_data_t *pDevInfo, msg_time_t *alarmTime);

// 2015-6-1 add By LinZh107
int tp_net_get_io_state(int user_id, void* param);
int tp_net_set_io_state(int user_id, void* param);

int st_net_startrecord(long user_id,unsigned int index);
int st_net_deleteRecord(long user_id,unsigned int index);
int st_net_getprogress(msg_file_down_Status  *down_progress);
int st_net_stopdownrecord(unsigned long user_id);

int data_callback(void *buf, unsigned long size_buf, void *text);
int st_net_startDownloadFileEx(unsigned long user_id, msg_fragment_t info, char *savefilename,
		msg_file_down_Status *file_down_Status);

int Recv(int fd, char *vptr, int n, int flag);
int Send(int fd, char *vptr, int n,int flag);

int startRecv();
int stopRecv();


#ifdef __cplusplus }
#endif

#endif //__CLIENTNETLIB_H__
