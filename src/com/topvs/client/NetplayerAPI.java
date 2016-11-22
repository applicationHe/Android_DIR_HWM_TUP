package com.topvs.client;

import android.graphics.Bitmap;

public class NetplayerAPI
{
	static
	{
		System.loadLibrary("netplayerlib");
	}

	// 云台控制命令类型
	public static final int PTZ_UPSTART = 0x01;
	public static final int PTZ_UPSTOP = 0x02;
	public static final int PTZ_DOWNSTART = 0x03;
	public static final int PTZ_DOWNSTOP = 0x04;
	public static final int PTZ_LEFTSTART = 0x05;
	public static final int PTZ_LEFTSTOP = 0x06;
	public static final int PTZ_RIGHTSTART = 0x07;
	public static final int PTZ_RIGHTSTOP = 0x08;

	public static final int PTZ_ZOOMADDSTART = 0x14;
	public static final int PTZ_ZOOMADDSTOP = 0x15;
	public static final int PTZ_ZOOMSUBSTART = 0x16;
	public static final int PTZ_ZOOMSUBSTOP = 0x17;
	public static final int PTZ_FOCUSADDSTART = 0x18;
	public static final int PTZ_FOCUSADDSTOP = 0x19;
	public static final int PTZ_FOCUSSUBSTART = 0x1a;
	public static final int PTZ_FOCUSUBSTOP = 0x1b;

	
	/* 接口函数说明： 初始化登录信息
	 * 参数：
	 * String addr 	 		//IP地址
	 * String name			//登录用户名
	 * String pwd			//登录密码
	 * int port				//端口
	 * String cuid 			//设备ID（P2P协议时用）
	 * int protocol			//协议类型（1/2/3 对应TCP/UDT/P2P）
	 * 
	 * 返回值：成功返回：userid， 失败返回小于等于的值。userid用于后续参数项。
	 * */
	public native static int initServerInfo(String ip_addr, int port, String user_name, String userpasword, String cuID, int proto);

	
	/* 接口函数说明： 登录设备
	 * 参数：
	 * int user_id 	 		//登录信息
	 * 
	 * 返回值：成功返回：0。 
	 * */
	public native static int userLogin(int user_id);
	
	/* 接口函数说明：获取通道（即摄像机）个数
	 * 参数：
	 * int user_id 	 		//登录信息
	 * 
	 * 返回值：成功返回：>0。 
	 * */
	public native static int getChannelNum(int user_id);
	
	
	/* 接口函数说明：主从码流支持情况，如果支持双码流，则请求从通道(码流)
	 * 参数：
	 * int user_id 	 		//登录信息
	 * 
	 * 返回值：成功返回：0:主码流 , 1:支持从码流。 
	 * */
	public native static int isSurportSlave(int user_id);
	
	/*
	 * 接口函数说明:获取录像列表
	 */
	public native static int getRecordInfo(int user_id,int channel,int startarr[],int endarr[],String[] name);
	
	/*
	 * 接口函数说明:开始播放录像
	 */
	public native static int startRecordPlayer(int user_id,int index);
	
	/*
	 * 接口函数说明:停止播放录像
	 */
	public native static int stopRecordPlayer(int play_id);
	
	/*
	 * 接口函数说明:下载录像
	 */
	public native static int downLoadRecord(int user_id,int index,String path);
	/*
	 * 接口函数说明:删除远程录像
	 */
	public native static int deleteServerRecord(int user_id,int index);
	
	/*
	 * 接口函数说明:获取下载进度和下载总大小
	 */
	public native static int getDownloadProgress(String[] progress);
	
	/*
	 * 接口函数说明:取消录像下载
	 */
	public native static int cancleDownload(int user_id);
	
	/* 接口函数说明：打开通道
	 * 参数：
	 * int user_id 	 		//登录信息
	 * int channel			//通道号，0 —— (getChannelNum-1)
	 * int protocol_type	// 1:TCP (其中UDT、P2P并入其中)
	 * int channel_type		// 0:主码流, 1:从码流
	 * 
	 * 返回值：成功返回：0。 
	 * */	
	public native static int openChannelStream(int user_id, int channel, int protocol_type, int channel_type);

	
	/* 接口函数说明：开始数据传输
	 * 参数：
	 * int user_id 	 		//登录信息
	 * int channel			//通道号，0 —— (getChannelNum-1)	--同上
	 * int stream_type		// 0:复合流。  1:视频流。 2:音频
	 * int channel_type		// 0:主码流, 1:从码流				--同上
	 * 
	 * 返回值：成功返回：0。 
	 * */	
	public native static int startChannelStream(int user_id, int channel, int stream_type, int channel_type);


	//2014-8-22 LinZh107
	/* 接口函数说明：切换音视频开关
	 * 参数：
	 * int user_id 	 		//登录信息
	 * int channel			//通道号，0 —— (getChannelNum-1)	--同上
	 * int stream_type		// 0:复合流。  1:视频流。 2:音频
	 * int channel_type		// 0:主码流, 1:从码流				--同上
	 * 
	 * 返回值：成功返回：0。 
	 * */	
	public native static int changeStreamType(int user_id, int channel, int stream_type, int channel_type);
	
	
//	//以下为旧方案
//	public native static byte[] getOneFrame(int[] ary);
		
	
	/* 接口函数说明：获取视频帧
	 * 参数：
	 * int VideoParams[2] 	//想要创建的视频分辨率[长、宽]
	 * 		VideoParams[0] = VideoWidth
	 		VideoParams[1] = VideoHight
	 * Bitmap bitmap		//java层根据 VideoParams 创建的位图缓冲区
	 * 
	 * 返回值：成功返回：frame_rate。 否则返回<0.
	 * */	
	public native static int getVideoFrame(int[] VideoParams, Bitmap bitmap); //2014-5-12 LinZh107 更换解bmp方案	


	/* 接口函数说明：获取视频链表缓冲有的帧数
	 * 参数：
	 * 
	 * 返回值：成功返回：当前链表节点数（视频帧数）。 
	 * */	
	public native static int getFrameNum();

	
	
	//2014-5-28 LinZh107 增加音频播放
	/* 接口函数说明：获取音频参数
	 * 参数：
	 * int AudioParam[] 	//想要创建的视频分辨率[组成音频帧的包数，音频帧长]
	 	   AudioParam[0] = packNum
	 	   AudioParam[1] = frameSize
	 * 返回值：成功返回：0。 
	 * */
	public native static int GetAudioParam(int[] AudioParam);
	
	
	/* 接口函数说明：获取音频
	 * 参数：
	 * int cnt			 	//AudioTrack 每次 write 的音频帧数
	 * 
	 * 返回值：成功返回：0。 
	 * */
	public native static short[] GetAudioFrame(int cnt);
	
	
	/*
	 * 2014-8-22 LinZh107
	 * 目的： 存储音频帧，用于分析音频帧结构
	 * 结果：已确定，废弃该 function */
	//public native static byte[] GetAudioFrame2(int cnt);
	
	
	/* 接口函数说明：获取视频编码信息(更换新的解码方案后未用)
	 * 参数：
	 * int user_id 	 		//登录信息
	 * int channel			//通道号，0 —— (getChannelNum-1)	--同上
	 * byte byteArray[]		//数组含义如下
		   	byteArray[0] = brightness;
			byteArray[1] = contrast;
			byteArray[2] = hue;
			byteArray[3] = saturation; 
	 * 
	 * 返回值：成功返回：0。 
	 * */
	public native static int getVideoConfig(int user_id, int channel, byte[] array);
	
	

	/* 接口函数说明：云台控制
	 * 参数：
	 * int user_id 	 		//登录信息
	 * int type				//参考  [ 云台控制命令类型 ]
	 * int channel			//通道号，0 —— (getChannelNum-1)	--同上
	 * int para				// 云台转速率(预置点号或速度或辅助开关号) 通常为55
	 * 
	 * 返回值：成功返回：0。 
	 * */	
	public native static int ptzControl(int userid, int type, int channel, int para);
	
	
	// 2014-4-25 by:LinZh107 增加多格式支持，所以去掉了对vc[1]的判断
	public native static int getVideoEncode(int user_id, int channel, int stream_type, int[] array);
	
	
	// 2014-3-28
	/* 接口函数说明： 设置登录信息
	 * 参数：
	 * int userid 	 		//登录信息的id值
	 * String addr 	 		//IP地址
	 * String name			//登录用户名
	 * String pwd			//登录密码
	 * int port				//端口
	 * 
	 * 返回值：成功返回：0， 失败返回-1。
	 * */
	public native static int setServerInfo(int userid, String ip_addr, int port, String user_name, String userpasword);

	//(未实现)
	public native static int setVideoConfig(int user_id, int channel, byte[] array);

	
	//(未实现)
	public native static int setVideoEncode(int user_id, int channel, int[] array);
	
	

	// 7.28号拍照
	//(未实现)
	public native static int downloadPicture(int user_id, int channel, String path);
	
	
	// 以对象的形式向处传递告警信息 Date: 2014-4-1 Author: yms	 --  已修改
	/* 定制型接口 -- 忽略 
	 * 接口函数说明： 获取设备告警信息(测控信息)  -- modify  2014-5-03  LinZh107
	 * 参数：
	 * int channel			//通道号，0 —— (getChannelNum-1)  —— 同[openChannelStream]一致
	 * 
	 * */	
	public native static int GetdevInfo(int channel, dev_ck_data_t obj_Info, dev_alarmtime obj_time);
	

	//2014-10-15  	LinZh107
	/* 接口函数说明：创建对讲线程, 主要处理的是将接收的音频放入列表(接收后还是要用以上GetAudioParam()来解码，不处理发送)
	 * 参数：
	 * int userid 	 		//登录信息的id值
	 * 
	 * 返回值：成功返回：0， 失败返回-1。
	 * */	
	public native static int startDualTalk(int user_id);
	
	//2014-10-15  	LinZh107
	/* 接口函数说明：发送音频到前端
	 * 参数：
	 * int userid 	 		//登录信息的id值
	 * short VoiceArray[]	//原始的音频数据(即PCM数据,不包含任何头)
	 * int bufLen			//sizeof VoiceArray[] (PCM长度) 
	 * 
	 * 返回值：成功返回：发送长度(bufLen)， 失败返回  < 0。
	 * */
	public native static int sendVoiceData(int user_id, short[] VoiceArray, int bufLen);
	
	//2014-10-15  	LinZh107
	/* 接口函数说明：退出对讲线程
	 * 参数：
	 * int userid 	 		//登录信息的id值
	 * 
	 * 返回值：成功返回：0， 失败返回-1。
	 * */
	public native static int stopDualTalk(int user_id);
	
	
	
	/* 接口函数说明：关闭通道
	 * 参数：  —— 同[openChannelStream]一致
	 * int user_id 	 		//登录信息
	 * int channel			//通道号，0 —— (getChannelNum-1)
	 * int channel_type		// 0:主码流, 1:从码流				
	 * 
	 * 返回值：成功返回：0。 
	 * */	
	public native static int closeChannelStream(int user_id, int channel, int channel_type);

	
	/* 接口函数说明：停止数据传输
	 * 参数：  —— 同[startChannelStream]一致
	 * int user_id 	 		//登录信息
	 * int channel			//通道号，0 —— (getChannelNum-1)
	 * int stream_type		// 0:复合流。  1:视频流。 2:音频
	 * int channel_type		// 0:主码流, 1:从码流
	 * 
	 * 返回值：成功返回：0。 
	 * */	
	public native static int stopChannelStream(int user_id, int channel, int stream_type, int channel_type);
	
	
	/* 接口函数说明：登出（退出登录）
	 * 参数：
	 * int userid 	 		//登录信息
	 * 返回值：成功返回：0.
	 * */
	public native static int userLogout(int user_id);
	
	
	/* 接口函数说明：释放初始化信息
	 * 参数：
	 * int userid 	 		//登录信息
	 * 返回值：成功返回：0.
	 * */
	public native static int deinitServerInfo(int userid);

	/* 接口与函数说明：获取网络开关状态
	 * 参数：
	 * int userid			//登录信息
	 * 返回值：成功返回0， 失败返回-1；
	 * */
	public native static int getAllIOState(int userid, Object obj);
	
	/* 接口与函数说明：设置网络开关状态
	 * 参数：
	 * int userid			//登录信息
	 * int param			//开关存储区
	 * 返回值：成功返回0， 失败返回-1；
	 * */
	public native static int setAllIOState(int userid, Object obj);
	
	/* 接口与函数说明：设置网络开关状态
	 * 参数：
	 * int userid			//登录信息
	 * int param			//开关存储区
	 * 返回值：成功返回0， 失败返回-1；
	 * */
	public native static int setIOState(int userid, int swindex, boolean flags, int sec);
}
