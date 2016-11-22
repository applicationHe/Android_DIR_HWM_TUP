/***************************************************************************
 *
 * Copyright	：2007 santachi corp.
 * File name	：dev_type.h
 * Description	：define all device type for santachi 
 * Created		: liuqs<liuqs_santachi@126.com> --- 2007-01-22
 * Modified 	: add some struct ---2007-02-02:zhangkk
 * Modified		: modified some struct define by liuqs --- 2007-04-10
 *
 ***************************************************************************/

#ifndef __MSG_DEF_H__
#define __MSG_DEF_H__


#define	ACK_ERROR_OTHER		0xff	//其他错误
#define	ACK_SUCCESS			0x00	//成功
#define	ACK_ERROR_NAME		0x01	//用户名错误
#define	ACK_ERROR_PWD		0x02	//密码错误
#define	ACK_ERROR_RIGHT		0x03	//权限不够
#define	ACK_ERROR_CHANNEL	0x04	//错误的通道号
#define	ACK_ERROR_PTZ_BUSY	0x05	//云台忙,暂时无法操作

#define NET_EXCEPTION		0x06    //网络异常

#define TRANSPARENT_SUCCESS 0x00	//透明传输成功
#define TRANSPARENT_EXCEPTION 0xF0  //透明传输异常

#define	MSG_LOGIN			0x0001	//登陆
#define	MSG_LOGOUT			0x0002	//注消
#define	MSG_OPEN_CHANNEL	0x0003	//打开通道
#define	MSG_CLOSE_CHANNEL	0x0004	//关闭通道
#define	MSG_START_SEND_DATA	0x0005	//开始发送数据
#define	MSG_STOP_SEND_DATA	0x0006	//停止发送数据
#define	MSG_PTZ_CONTROL		0x0007	//云台控制
#define	MSG_SESSION_ID		0x0009	//
#define	MSG_MATRIX_PROTOCOL	0x000a	

#define	MSG_SET_PARAM		0x0401	//设置参数
#define	MSG_GET_PARAM		0x0402	//获取参数
#define	MSG_RESET_VIDEO_PARAM	0x0403	//设置视频参数默认值

#define	MSG_GET_RECORD_LIST	0x0801	//查询录像
#define	MSG_SET_TIME		0x0802	//设置时间
#define	MSG_GET_TIME		0x0803	//获取时间
#define	MSG_VERSION_INFO	0x0804	//版本信息
#define	MSG_HARDDISK_INFO	0x0805	//硬盘信息
#define	MSG_SYSTEM_STATUS	0x0806	//系统状态
#define	MSG_GET_LOG			0x0807	//查询日志
#define	MSG_DELETE_LOG		0x0808	//删除日志
#define	MSG_ALARM_INFO		0x0809	//告警信息
#define	MSG_ALARM_CONTROL	0x080a	//告警上传控制
#define	MSG_UPGRADE_STATUS	0x080b	//升级状态
#define	MSG_HEARTBEAT		0x080c	//心跳包
#define	MSG_HEARTBEAT_PORT	0x080d	//心跳包端口消息
#define MSG_GET_USERINFO	0x080e
#define MSG_ADD_USER		0x080f
#define MSG_DELETE_USER		0x0810
#define MSG_MODIFY_USER		0x0811

#define MSG_FORCE_KEYFRAME	0x0830	//强迫I帧

#define MSG_TRANSPARENT_CMD	0x0814  //用于透明传输
#define MSG_GET_AP_INFO		0x0815 //搜索无线路由器信息
//#define MSG_JOIN_AP			0x0816 //加入无线路由器
#define MSG_START_TRANSPARENT		0x0816 //开始透明传输
#define MSG_STOP_TRANSPARENT		0x0817 //停止透明传输
#define MSG_SEND_TRANSPARENT		0x0818 //发送透明传输数据
#define MSG_REV_TRANSPARENT			0x0819 //接收透明传输数据
#define	MSG_BROADCAST		0x0f01	//广播探测消息

//透明传输控制子类型

//////////////////////////////////////////////////////////////////////////
#define MSG_REMOTE_START_HAND_RECORD	0x0840  //开始手动录像
#define MSG_REMOTE_STOP_HAND_RECORD		0x0841	//停止手动录像
#define MSG_DELETE_RECORD_FILE			0x0842  //删除录像文件
#define	MSG_HAND_RESTART				0x0843	//手动重启
#define	MSG_DEFAULT_SETTING				0x0844	//默认参数设置
#define MSG_PARTITION					0x0845	//分区
#define MSG_FORMAT						0x0846	//格式化
#define MSG_FORMAT_STATUS				0x0847  //格式化状态
#define MSG_FDISK_STATUS				0x0848  //
#define MSG_START_ALARM					0x0849  //告警布防	
#define MSG_STOP_ALARM					0x0850	//告警撤防
#define	MSG_BROADCAST_DEVICE			0x0851	//广播搜索设备
#define	MSG_BROADCAST_IP				0x0852	//广播修改IP
#define MSG_DEFAULTOUTPUT_SETING		0x0853	//出厂设置	
#define MSG_VERSION_SETING				0x0854  //版本设置 
#define MSG_INPUTFILE_CONFIG			0x0855  //导入文件
#define MSG_OUTPUTFILE_CONFIG			0x0856  //导出文件

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
#define	PARAM_VIDEO_MOVE           0x000C //MOTION
#define	PARAM_VIDEO_CONFIG         0x000D
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
#define PARAM_CDMA_TIME			   0x1079 //2009.03.11
#define PARAM_CDMA_ALARM_ENABLE    0x1080 //报警触发拨号
#define PARAM_CDMA_NONEGO_ENABLE   0x1081 //外部触发拨号
#define PARAM_CDMA_ALARM_INFO      0x1082 //报警配置信息
#define PARAM_CDMA_STREAM_ENABLE   0x1083 //CDMA自动下线


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

#define PARAM_HANDIO_CONTROL               0x0038//手动IO控制 20091223
#define PARAM_TIMEIO_CONTROL		       0x0039//定时IO控制 20100114

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
#define	PARAM_HXHT_PLATFORM_CONFIG			0X0062	//add by lch 2007-06-15
#define	PARAM_HXHT_ALARM_SCHEME				0X0063
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
#define	MATRIX_LoginConnect          0x001 //1   Login connect
#define	MATRIX_LoginWithPassword     0x002 //2   Login with  operater-password
#define	MATRIX_LoginInAuto           0x003 //3   login-in Auto
#define	MATRIX_LoginOut              0x004 //4   login-out
#define	MATRIX_MonitorSelect         0x005 //5   Monitor select
#define	MATRIX_CameraSelect          0x006 //6   Camera select
#define	MATRIX_QuWeiSelect           0x007 //7   qu wei select
#define	MATRIX_TourSelect            0x008 //8   Tour  select
#define	MATRIX_GroupSelect           0x009 //9   Group select
#define	MATRIX_BackSeq               0x00a //10   Back Seq
#define	MATRIX_FowardSeq             0x00b //11   Foward Seq
#define	MATRIX_SEQ_DEC                   0x00c //12   DEC -1 
#define	MATRIX_SEQ_INC                   0x00d //13  INC +1 
#define	MATRIX_PAUSH                 0x00e //14  PAUSH 
#define	MATRIX_StopSeq               0x00f //15  Stop seq
#define	MATRIX_HoldON                0x010 //16  Hold ON
#define	MATRIX_HoldOFF    	         0x011 //17   Hold OFF
#define	MATRIX_DisplayALL   	     0x012 //18   Display  ALL
#define	MATRIX_DisplayTIME   	     0x013 //19   DISPLAY TIME
#define	MATRIX_DisplayTITLE          0x014 //20   Display TITLE
#define	MATRIX_DisplaySTATUS         0x015 //21   DISPLAY STATUS
#define	MATRIX_AlarmAreaStatus       0x016 //22   AlarmAreaStatus  防区状态的显示
#define	MATRIX_DetectorStatus        0x017 //23   DetectorStatus   探测器状态的显示
#define	MATRIX_KeyJump                0x018 //24  JUMP
#define	MATRIX_MSsetupCommand  	      0x019 //25   MSsetupCommand   功能具体内容由矩阵决定
#define	MATRIX_DisplayStatus   	      0x01a //26  Display Status
#define	MATRIX_RequestMonitorStatus   0x01b //27  Request Monitor Status
#define	MATRIX_RequestMonitorMessage  0x01c //28  Request Monitor Message
#define	MATRIX_RequestCAMStatus       0x01d //29  Request CAM Status
#define	MATRIX_RequestCAMMessage      0x01e //30  Request CAM Message
#define	MATRIX_RequestAlamMessage     0x01f //31  Request Alam Message
#define	MATRIX_RequestControlsMessage 0x020 //32  Request Controls Message
#define	MATRIX_AlarmStatusMenu        0x021 //33  AlarmStatusMenu 报警状态菜单进入
#define	MATRIX_AlarmRecordMenu        0x022 //34  AlarmRecordMenu 报警记录菜单进入
#define	MATRIX_SystemStatusMenu       0x023 //35  SystemStatusMenu 系统状态菜单进入
#define	MATRIX_MenuExit        	      0x024  //36  MenuExit 菜单退出
#define	MATRIX_SubnetSelect    	      0x025  //37 SubnetSelect 子网选择
#define	MATRIX_SubnetExit     	      0x026  //38 SubnetExit 子网退出
#define	MATRIX_SystemTime             0x027  //39 transfer system time
#define	MATRIX_UserKey                0x028  //40 transfer user key   用户口令
#define	MATRIX_SystemSetup            0x029  //41 system setup   系统设置
#define	MATRIX_SubnetMonSelect        0x02a  //42  Subnet Mon select
                                                                                
//SetupCommand 设置命令序号                                                                                                   
#define	MATRIX_SetupEnter             0x02b  //0  SetupEnter  
#define	MATRIX_SetupDATA1             0x02c  //1  SetupDATA1  
#define	MATRIX_SetupDATA2             0x02d  //2  SetupDATA2  
#define	MATRIX_SetupDATA3             0x02e  //3  SetupDATA3  
#define	MATRIX_SetupDATA4             0x02f  //4  SetupDATA4  
#define	MATRIX_SetupDATA5             0x030  //5  SetupDATA5  
#define	MATRIX_SetupDATA6             0x031  //6  SetupDATA6  
#define	MATRIX_SetupDATA7             0x032  //7  SetupDATA7  
#define	MATRIX_SetupDATA8             0x033  //8  SetupDATA8  
#define	MATRIX_SetupDATA9             0x034  //9  SetupDATA9  
#define	MATRIX_SetupDATA0             0x035  //10  SetupDATA0 
#define	MATRIX_SetupUP                0x036  //11  SetupUP    
#define	MATRIX_SetupDOWN              0x037  //12  SetupDOWN  
#define	MATRIX_SetupLEFT              0x038  //13  SetupLEFT  
#define	MATRIX_SetupRIGHT             0x039  //14  SetupRIGHT 
#define	MATRIX_SetupINC               0x03a  //15  SetupINC     
#define	MATRIX_SetupDEC               0x03b  //16  SetupDEC     
#define	MATRIX_SetupESC               0x03c  //17  SetupESC     
#define	MATRIX_SetupSET               0x03d  //18  SetupSET     
#define	MATRIX_SetupNEXT              0x03e  //19  SetupNEXT   
#define	MATRIX_SetupPRE               0x03f  //20  SetupPRE     
#define	MATRIX_SetupPOPUP             0x040  //21  SetupPOP-UP
#define	MATRIX_SetupPRINT             0x041  //22  SetupPRINT 
#define	MATRIX_SetupCLEAR             0x042  //23  SetupCLEAR 
#define	MATRIX_SetupSAVE              0x043  //24  SetupSAVE   
#define	MATRIX_SetupREAD              0x044  //25  SetupREAD   
#define	MATRIX_SetupDELE              0x045  //26  SetupDELE   


//AskAndAnswer询问和问答序号
#define	MATRIX_AskDeveiceID        0x046  //0 AskDeveiceID  询问设备代码         		     
#define	MATRIX_AnswerDeveiceID        0x047 //1 AnswerDeveiceID  回答设备代码               
#define	MATRIX_AskSoftwareVer         0x048 //2 AskSoftwareVer  询问软件版本信息            
#define	MATRIX_AnswerSoftwareVer      0x049 //3 AnswerSoftwareVer  回答软件版本信息         
#define	MATRIX_AskBoardStatus         0x04a //4 AskBoardStatus  询问插板信息                
#define	MATRIX_AnswerBoardStatus      0x04b //5 AnswerBoardStatus  回答插板信息    

//AlarmControl 报警控制序号		
#define	MATRIX_DefendSetup            0x04c //0 Defend setup  设防              
#define	MATRIX_DefendSolve            0x04d //1  Defend solve撤防              	
#define	MATRIX_BellOFF   	          0x04e //2  Bell OFF  警铃关           	
#define	MATRIX_BellON                 0x04f //3  Bell ON警铃开           		
#define	MATRIX_BRACHRODE              0x050 //4  BRACH RODE     旁路                         
#define	MATRIX_CLEARRODE              0x051 //5  CLEAR RODE     清旁路                       
#define	MATRIX_RecordON               0x052 //6  Record ON       录像                        
#define	MATRIX_RecordOFF              0x053 //7  Record OFF      停录                        
#define	MATRIX_AlarmInc               0x054 //8  Alarm inc       报警巡视+1                  
#define	MATRIX_ALarmDec               0x055 //9  ALarm dec       报警巡视-1                  
#define	MATRIX_ResetOneRecord         0x056 //10  reset one record 报警复位单1当前           
#define	MATRIX_ResetAllRecord         0x057 //11  Reset all Record  报警复位全部             
#define	MATRIX_RecordDisp             0x058 //12  Record disp       报警记录显示             
#define	MATRIX_WarnModeBYHand         0x059 //13 Warn Mode BY hand 报警模式-人工             
#define	MATRIX_WarnModeTime           0x05a //14 Warn Mode Time    报警模式-定时            
#define	MATRIX_WaringRecordExit       0x05b //15 WARING RECORED EXIT   报警记录显示退出  
//解码器控制序号：		
	//云台摇杆控制		       
#define	MATRIX_PANUP				  0x05c //0 PAN UP (Speed 1-8)                             
#define	MATRIX_PANDOWN                0x05d //1  PAN DOWN(Speed 1-8)                                   
#define	MATRIX_PANRIGHT               0x05e //2  PAN RIGHT(Speed 1-8)                                 
#define	MATRIX_PANLEFT                0x05f  //3  PAN LEFT(Speed 1-8)                                   
#define	MATRIX_PANRIGHTUP             0x060 //4  PAN RIGHT UP(Speed 1-8)                            
#define	MATRIX_PANLEFTUP              0x061 //5  PAN LEFT UP(Speed 1-8)                              
#define	MATRIX_PANRIGHTDOWN           0x062 //6  PAN RIGHT DOWN(Speed 1-8)                        
#define	MATRIX_PANLEFTDOWN            0x063 //7  PAN LEFT DOWN(Speed 1-8)                          
#define	MATRIX_PANSTOP                0x064 //8  PAN STOP      
		
//光圈调节                                                                              
#define	MATRIX_IRISOPEN               0x065 //9  IRIS OPEN                                           
#define	MATRIX_IRISCLOSE              0x066 //10  IRIS CLOSE                                        
#define	MATRIX_FOCUSSTOP              0x067 //11  IRIS STOP                                         
#define	MATRIX_FOCUSNEAR              0x068 //12  FOCUS NEAR                                        
#define	MATRIX_FOCUSFAR               0x069 //13  FOCUS FAR                                          
#define	MATRIX_FOCUSTITLE             0x06a //14  FOCUS TITLE                                      
#define	MATRIX_FOCUSWIDE              0x06b //15  FOCUS WIDE                                        
#define	MATRIX_AUTOFOCUSENABLE        0x06c //16  AUTO focus enable   自动对焦点允许          
#define	MATRIX_AUTOFOCUSDISABLE       0x06d //17  AUTO focus disable  自动对焦点禁止         
#define	MATRIX_AUTOIRISENABLE         0x06e //18  auto iris  enable  自动光圈允许            
#define	MATRIX_AUTOIRISDISABLE        0x06f //19  auto iris  disable 自动光圈禁止           
#define	MATRIX_STOP                   0x070 //20   STOP (--NEAR WIDE FAR TELE) 镜头运动停    
		
//云台自动控制                                                                             
#define	MATRIX_AUTOHPAN               0x071 //21  AUTO H PAN     云台自动水平运动                   
#define	MATRIX_AUTOPANSTOP            0x072 //22  AUTO PAN STOP  云台自动水平运动停止            
#define	MATRIX_RANDMHPAN              0x073 //23  RANDM H PAN    云台随机水平运动                  
#define	MATRIX__RANDMPANSTOP          0x074 //24  RANDM PAN STOP  云台随机水平运动停止           
	//     
#define MATRIX_AUX1ON                 0x075 //25   AUX1 ON      辅助开关开                             
#define	MATRIX_AUX1OFF                0x076 //26  AUX1 OFF              辅助开关关                    
#define	MATRIX_RainBrushOpen          0x077 //27  Rain Brush Open 雨刷开关开                    
#define	MATRIX_RainBrushClose         0x078 //28  Rain Brush Close 雨刷开关关                  
#define	MATRIX_DeforstOpen            0x079 //29  Deforst Open    除霜开关开                      
#define	MATRIX_DeforstClose           0x07a //30  Deforst Close   除霜开关关                     
#define	MATRIX_HeartOn                0x07b //31  Heart On        加热开                              
#define	MATRIX_HeartClose             0x07c //32  Heart Close     加热关                           
#define	MATRIX_PowerOn                0x07d //33 Power On        电源开关开                           
#define	MATRIX_PowerClose             0x07e //34  Power Close     电源开关关 

//摄象机                                                                                   
#define	MATRIX_PCAMERPositioncall     0x07f //35   CAMER Position call 1-999    摄像机位置调用
#define	MATRIX_PresetCall     	      0x080	//36   Preset  call     预置位置调用                      
#define	MATRIX_PresetSet     	      0x081	//37   Preset  Set      预置位置设置                      
#define	MATRIX_PresetReset    	      0x082	//38  Preset Reset     预置位置复位                     
#define	MATRIX_FunctionSelect 	      0x083	//39   Function select  功能选择                     
#define	MATRIX_TURNOVER       	      0x084	//40  TURN OVER        转向    
		                                             
#define	MATRIX_CAMENTERSETUPMENU      0x085 //41  CAM ENTER SETUP MENU  摄像机设置菜单进入   
#define	MATRIX_CURUP                  0x086 //42  CUR UP           摄像机设置-上                        
#define	MATRIX_CURDN                  0x087 //43  CUR DN           摄像机设置-下                        
#define	MATRIX_CURLF                  0x088 //44  CUR LF           摄像机设置-左                        
#define	MATRIX_CURRT                  0x089 //45  CUR RT           摄像机设置-右                         
#define	MATRIX_CONFIREM               0x08a //46  CONFIREM         摄像机设置-确定                   
#define	MATRIX_ESC                    0x08b //47  ESC              摄像机设置-取消                        
#define	MATRIX_INC                    0x08c //48  INC +1               摄像机设置-加一                    
#define	MATRIX_DEC                    0x08d //49  DEC -1               摄像机设置-减一  
                  
         	//智能                                                                                     
#define	MATRIX_SMARTBegin             0x08e //50  SMART-Begin       智能-开始                        
#define	MATRIX_SMARTEnd               0x08f //51  SMART_End                                            
#define	MATRIX_SMARTRepeat            0x090 //52  SMART_Repeat                                      
#define	MATRIX_SMART_Run              0x091 //53  SMART_Run 

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
#define PTZ_AUTOSCANSTART         0x27	//自动线扫-(开始)
#define PTZ_AUTOSCANSTOP          0x28	//自动线扫-(停止)
#define PTZ_RANDOMSCANSTART       0x29	//随机线扫-(开始)
#define PTZ_RANDOMSCANSTOP        0x2a	//随机线扫-(停止)

//--------- add by wy 2009.04.22 ----------
#define PTZ_CURISESTART		      0x2b	//巡航开始
#define PTZ_CURISESTOP            0x2c	//巡航停止
#define PTZ_TYPE_REPOSITION       0x2d	//巡航归位
//------------------ add end --------------
#define PTZ_POINCENTER			  0x30
#define PTZ_VIEWCENTER			  0x31
//#define PTZ_TYPE_TRANSPARENCE			0x0100  //透明传输(设备端不作协议转换，直接传送网络命令) //add by liuqs

typedef struct __msg_login_t
{
	char user_name[MSG_USER_NAME_LEN];
	char user_pwd[MSG_PASSWD_LEN];
	char is_support_d1;
	char unuserd;
	short	local_right;
	int	remote_right;
	int	device_type;
	msg_time_t local_time;
}msg_login_t;


//typedef struct __msg_logout_t
//{
//	int session_id;
//}msg_logout_t;

typedef struct __msg_open_channel_t
{
	int	channel_no;
	int	protocol_type;
	struct sockaddr_in client_sockAddr;
	unsigned char channel_type;
	unsigned char unused[3];	
}msg_open_channel_t;

typedef struct __msg_close_channel_t
{
	int	channel_no;
	unsigned char channel_type;
	unsigned char unused[3];	
}msg_close_channel_t;

typedef struct __msg_preview_t
{
	int channel;
	unsigned char channel_type;//0:主码流   1:从码流
	unsigned char unused[3];
}msg_preview_t;

typedef	struct	_msg_matrix_control_t
{
	int 	param_control;					
	char    unusd[32];					
}msg_matrix_control_t;

typedef struct __msg_transparent_cmd_t
{
	int com_no;
	int cmd_size;
	char cmd_buf[MAX_TRANSPARENT_CMD_NUM];
}msg_transparent_cmd_t;

typedef	struct	__msg_record_list_t
{
	msg_file_info_t	msg_record_info;
	struct	__msg_record_list_t *next;
}msg_record_list_t;

typedef	struct	_msg_delete_log_t
{
	int	log_type;		//日志类型
	msg_time_t	s_tm;
	msg_time_t	e_tm;
}msg_delete_log_t;
typedef	struct	__msg_log_list_t
{
	msg_log_info_t	msg_log_info;
	struct	__msg_log_list_t *next;
}msg_log_list_t;

#define ALARM_ALL 0
#define	ALARM_PROBER 1
#define ALARM_VIDEO_LOSE 2
#define ALARM_VIDEO_MOTION 3
#define ALARM_OSD 4
#define ALARM_DATE 5
#define ALARM_VIDEO_LOSE_STOP 6
#define ALARM_VIDEO_MOTION_STOP 7
#define	ALARM_PROBER_STOP 8

typedef struct __msg_alarm_ack_t
{
	msg_time_t tm;
	unsigned int alarm_id;
}msg_alarm_ack_t;
typedef	struct	__msg_user_list_t
{
	msg_user_info_t	msg_user_info;
	struct	__msg_user_list_t *next;
}msg_user_list_t;

typedef struct __msg_force_keyframe_t
{
	int channel;
}msg_force_keyframe_t;

#define	TCP_TYPE_PREVIEW		0x0000		//实时预览
#define	TCP_TYPE_DOWNLOAD_RECORD	0x0001	//录像下载
#define	TCP_TYPE_PLAY_RECORD		0x0002	//录像点播
#define	TCP_TYPE_ASK_DATA		0x0003		//请求数据(点播使用)
#define	TCP_TYPE_UPGRADE_SYSTEM	0x0004		//升级
#define	TCP_TYPE_UPGRADE_RESULT	0x0005		//升级结果
#define	TCP_TYPE_START_DUAL_TALK 0x0006		//开始语音对讲
#define	TCP_TYPE_STOP_DUAL_TALK 0x0007		//结束语音对讲
#define	TCP_TYPE_START_DOWNLOAD_PIC 0x0008	//开始图片下载


#define	SERVER_LISTEN_PORT 9660 //服务端使用
#define UDP_BASE_PORT 9880		//客户端使用
//#define SEARCH_PORT 9001		//服务端使用

typedef	struct	_msg_filesize_t
{
	unsigned	long	filesize;
}msg_filesize_t;
typedef	struct	_msg_ask_data_t
{
	int	datalen;	//请求数据长度或祯个数
}msg_ask_data_t;

////////////////////////////////////////////2007-03-21
typedef struct __msg_send_data_t
{
	int channel;
	int data_type;//音视频 0 ,视频1,音频2
	unsigned char channel_type;
	unsigned char unusd[3];
}msg_send_data_t;

typedef struct __msg_upgrade_info_t
{
	unsigned long filesize;
}msg_upgrade_info_t;

#define MSG_GET_COM_INFO 0x08000018
#define MSG_SET_COM_INFO 0x08000019

typedef struct __msg_com_info_t
{
	unsigned int com_index;
	unsigned int baudrate;
	unsigned int data_bit;
	unsigned int stop_bit;
	unsigned int parity;
}msg_com_info_t;

#define OSD_LOGO_LEN 64
typedef struct __msg_osd_info_t
{
	unsigned int x_pos;
	unsigned int y_pos;
	char logo[OSD_LOGO_LEN];
}msg_osd_info_t;

#define MSG_OSD_SET 0x08000012			//OSD设置
#define MSG_OSD_QUETY 0x08000013		//OSD查询
#define MSG_OSD_ENABLE 0x08000014		//OSD显示状态
#define MSG_OSD_ENABLE_QUERY 0x08000015	//OSD显示状态查询

typedef struct __msg_osd_enable_t
{
	unsigned int logo_enable;
	unsigned int time_enable;
}msg_osd_enable_t;

#define MSG_COM_TRANSMIT 0x08000017
#define COM_DATA_LEN 512
typedef struct _msg_com_transmit_t
{
	unsigned int index;
	unsigned int len;
	char data[COM_DATA_LEN];
}msg_com_transmit_t;

#define MSG_GET_USERLOGIN_STATE 0x08000016
typedef struct __msg_user_login_state_query_t
{
	unsigned int channel_no; //0xffffffff 所有通道
}msg_user_login_state_query_t;

typedef struct __msg_user_login_state_t
{
	unsigned int channel_no;
	char user_name[MSG_USER_NAME_LEN];
	char user_ip[MSG_IP_SIZE];
	unsigned protocol_type;
	msg_time_t tm;
	unsigned int seconds;
	unsigned int net_state;
}msg_user_login_state_t;

typedef struct __msg_user_login_list_t
{
	msg_user_login_state_t msg_user_login_state;
	struct __msg_user_login_list_t *next; 
}msg_user_login_list_t;

typedef struct __msg_heart_t
{
	unsigned int heart_seq;
	unsigned int heart_ack;
}msg_heart_t;

typedef struct __msg_heartbeat_port_t
{
	int port;
}msg_heartbeat_port_t;


typedef struct _msg_session_id_t
{	
	int session_id;
}msg_session_id_t;

typedef struct _msg_remote_play_position_t
{	
	int flag;
	int play_position;
}msg_remote_play_position_t;

typedef struct __param_file_head_t
{
	unsigned	long	dev_type;	//设备类型
	unsigned	short	file_type;	//消息子类型
	unsigned	int		file_size;
	unsigned	char	unres[4];
}param_file_head_t;

typedef struct _msg_matrix_infor_t
{
	int serrial_no;//0:串口1 1:串口2
	int protrol_type; //0:750传输协议
	unsigned char unused[4];
}msg_matrix_infor_t;

//-------------- add by wy 2009.07.15 ----------------
typedef struct __msg_3G_module_info_t
{
	int signal;//信号强度
}msg_3G_module_info_t;
//--------------------- add end ----------------------

#endif /* __MSG_DEF_H__ */

