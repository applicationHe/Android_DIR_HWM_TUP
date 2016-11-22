/**by LinZh107
 * 
 * 2014-4-01 增加alarm_info显示
 * 2014-4-15 修改底层，按顺序从alarm_info对象组获取alarm_info对象
 * 2014-4-18 从底层获取对应组dev_info，区别于4组全部获取
 * 2014-4-18 给 alarm_info 加上组号标识 channel(直连)
 * 2014-4-24 解决问题：如果当前位于选择通道界面时，如果终端处于横屏状态，则画面比例异常
 * 2014-4-25 增加多 格式支持
 * 2014-5-10 增加全格式支持，更新解码库，更换解码方案和视频帧创建方式及显示方案
 * 2014-5-10 修改以支持视频格式自适应
 * 2014-5-15 修复高清时帧数过大导致的卡顿现象
 * 2014-5-22 更换alarm_info的结构，即dev_data_t
 * 2014-5-29 增加音频解码，目前噪声还是很大~
 * 2014-7-22 更换 TCP-->UDT
 * 2014-7-23 修复打开通道失败会crash 的bug
 * 2014-8-02 增加P2P。
 * 2014-8-14 解决音频输出噪声问题
 * 2014-9-09 增加对讲功能（目前已经可以本地播放录音，下一步将进行网络传输）
 * 2014-9-16 修复更换解码库后某个ffmpeg函数存在内存泄露导致app崩溃的问题；
 * 2014-9-17 优化Android直连版容易断线的问题(下一步将增加断线从连的问题);
 * 2014-9-17 ***
 **/

package com.topvs.client;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.hardware.SensorManager;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.os.Environment;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TableLayout;
import android.widget.TextView;
import android.widget.Toast;
//2014-5-11 LinZh107 更改视频帧创建及显示方案

public class PlayActivity extends Activity
{
	final static String TAG = "PlayActicity";
	//public MyActivityManager mam = null;
	
	private SurfaceView sfv;
	private SurfaceHolder sfh;
	private Button btnRecord;
	private Button btnRecord_f;
	private Button audiobtn = null;
	
	private TextView TextCameraName = null;
	private String strName = "";
	private SensorManager sensorManager;
	
	private Thread Vthread = null;
	private Thread Athread = null;
	private Thread ARthread = null;
	
	private boolean isConnected = false;
	private boolean m_talkFlag = false;
	//private boolean LOW_RESOLUTION_MSG = false;
	private static boolean isFullScreen = false;
	private boolean isExit = false;
	
	private int userid = -1;
	private int openchannel = 0;
	private int protocoltype = 1;
	private int channel_type = -1; 	// 从主码流，0主码流， 1从码流
	// 发布参数
	private int stream_type = 1;	// 0:复合流 1:视频流， 2:音频
	private int stream_state = 0;	// 0:音视频  1:视频， 2:对讲
	
	
	//************ 2014-5-13 LinZh107 增加视频缩放比例,以区别横、竖屏时的视频长宽比
	public Bitmap mBitmap = null;			// 录像时需要，所以必须为public
	public int VideoParams[] = {720, 576};	// 视频分辨率	
	public int frame_rate = 25; 			// getVideoFrame return frame_rate
	
	public AudioTrack track = null;
	static byte PCM_pack[];					// PCM 音频包
	public int AudioParams[] = {0, 0};		// 音频帧速和帧长
	public short AudioArray[] = null;
	
	// 2013-12-9 yms
	private int scaleWidth;
	private int scaleHeight;
	private int deviceWidth;
	private int deviceHeight;

	private float scaleXY;
	private float Hscale = 1.5f;
	private float Vscale = 1.25f; // 默认的长宽比例
	
	//*******2014-4-25 LinZh107 修复横屏下打开问题
	private Configuration config = null;
	private Resources rs;

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		setContentView(R.layout.playview);
		rs = getResources();
		config = getResources().getConfiguration();
		
		Bundle extras = getIntent().getExtras();		
		openchannel = extras.getInt("OpenChannel");
		userid = extras.getInt("USERID");
		//protocoltype = extras.getInt("PROTOCOL"); // PROTOCOL_UDP / PROTOCOL_MCAST
		channel_type = extras.getInt("CHANNEL_TYPE"); // 0:主码流, 1:从码流
		//channel_type = 0;
		Log.i(TAG, "Open channel_type = " + channel_type);

		int ret = -1;
		Log.i(TAG,"userid = "+userid);
		if (openchannel >= 0)//2015-11-30   sandy   在android5.0后去掉了对userid的值的判断
			ret = NetplayerAPI.openChannelStream(userid, openchannel, protocoltype, channel_type);
		if (ret == 0)
		{
			ret = NetplayerAPI.startChannelStream(userid, openchannel, stream_type, channel_type);
			if (ret == 0)
				isConnected = true;
			else{
				Toast.makeText(this, "startChannelStream error. ", Toast.LENGTH_SHORT).show();
				finish();
				return;
			}
		}
		else{
			Toast.makeText(this, "openChannelStream error. ", Toast.LENGTH_SHORT).show();
			finish();
			return;
		}

		// 将球机控制 和 layout界面的初始化 封装成初始化函数
		initControlButton();
		initSurfaceView();				
	}
	private void initControlButton()
	{
		// TODO Auto-generated method stub

		TextCameraName = (TextView) findViewById(R.id.cameraname);
		TextCameraName.setText(strName);
		Button btn1 = (Button) findViewById(R.id.leftbtn);
		btn1.setOnTouchListener(onTouchListener);
		Button btn2 = (Button) findViewById(R.id.rightbtn);
		btn2.setOnTouchListener(onTouchListener);
		Button btn3 = (Button) findViewById(R.id.upbtn);
		btn3.setOnTouchListener(onTouchListener);
		Button btn4 = (Button) findViewById(R.id.downbtn);
		btn4.setOnTouchListener(onTouchListener);
		Button btn7 = (Button) findViewById(R.id.zoominbtn);
		btn7.setOnTouchListener(onTouchListener);
		Button btn8 = (Button) findViewById(R.id.zoomoutbtn);
		btn8.setOnTouchListener(onTouchListener);
		Button btn_search_record = (Button) findViewById(R.id.btn_search_record);
		btn_search_record.setOnClickListener(onListener);
		
		audiobtn = (Button) findViewById(R.id.audiobtn);
		audiobtn.setOnClickListener(onListener);
		btnRecord = (Button) findViewById(R.id.recordbtn);
		btnRecord.setVisibility(View.INVISIBLE);

		Button btn1_f = (Button) findViewById(R.id.leftbtn_f);
		btn1_f.setOnTouchListener(onTouchListener);
		Button btn2_f = (Button) findViewById(R.id.rightbtn_f);
		btn2_f.setOnTouchListener(onTouchListener);
		Button btn3_f = (Button) findViewById(R.id.upbtn_f);
		btn3_f.setOnTouchListener(onTouchListener);
		Button btn4_f = (Button) findViewById(R.id.downbtn_f);
		btn4_f.setOnTouchListener(onTouchListener);

		Button btn6_f = (Button) findViewById(R.id.takephotobtn_f);
		btn6_f.setOnClickListener(onListener);

		Button btn7_f = (Button) findViewById(R.id.zoominbtn_f);
		// btn7_f.setOnClickListener(listener);
		btn7_f.setOnTouchListener(onTouchListener);
		Button btn8_f = (Button) findViewById(R.id.zoomoutbtn_f);
		btn8_f.setOnTouchListener(onTouchListener);
		// btnRecord_f = (Button) findViewById(R.id.recordbtn_f);
		// btnRecord_f.setOnClickListener(listener);
	}

	private void initSurfaceView()
	{
		// TODO Auto-generated method stub
		// 2013-12-9 yms 获取屏幕密度（方法3）
		DisplayMetrics dm = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(dm);
		deviceWidth = dm.widthPixels;
		deviceHeight = dm.heightPixels;

		sfv = (SurfaceView) this.findViewById(R.id.SurfaceViewID);
		
		LayoutParams lp = sfv.getLayoutParams();
		lp.width = scaleWidth = deviceWidth;
		 // 默認竖屏，按竖屏比例进行变换，在真正获取视频帧后将改为视频的比例
		lp.height = scaleHeight = (int) (deviceWidth * 1.0 / Vscale);
		sfv.setLayoutParams(lp);

		sfh = sfv.getHolder();
		sfh.setKeepScreenOn(true);
		sfh.addCallback(new MyCallBack());// 自动运行surfaceCreated以及surfaceChanged
	}

	private void StartDecode_Video()
	{
		Log.i(TAG, "Enter the video_decode thread.");
		try
		{
			Canvas canvas = null;
			int frame_rate = 0; // getVideoFrame return frame_rate
			int nResidue_Time = 0;
			// 2014-4-25 修复横屏状态下打开问题
			if (config.orientation != 1)
				setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

			while (!isExit)
			{
				// /旧方案，已完全屏蔽
				// 2014-5-10 LinZh107 更换新的显示方案 修改以支持视频格式自适应
				long startTime = System.currentTimeMillis();
				
				if (VideoParams[0] < scaleWidth) // 107.视频分辨率小于屏幕，底层SWS_SCALE不进行bitmap尺寸变换
				{
					mBitmap = Bitmap.createBitmap(VideoParams[0], VideoParams[1], Bitmap.Config.RGB_565);
					frame_rate = NetplayerAPI.getVideoFrame(VideoParams, mBitmap); // 底层创建的位图没有进行缩放
				}
				else// 107.视频分辨率 >= 屏幕，底层 SWS_SCALE 进行bitmap缩小变换
				{
					VideoParams[0] = scaleWidth;
					VideoParams[1] = scaleHeight;
					mBitmap = Bitmap.createBitmap(VideoParams[0], VideoParams[1], Bitmap.Config.RGB_565);
					frame_rate = NetplayerAPI.getVideoFrame(VideoParams, mBitmap); // 底层创建完整位图(已经进行缩放)
					System.out.println("frame_rate__:"+frame_rate);
				}
				
				if (frame_rate > 0)
				{
					Rect rect = new Rect(0, 0, scaleWidth, scaleHeight);					
					canvas = sfh.lockCanvas(rect);
					canvas.drawBitmap(mBitmap, null, rect, null); // 107.java层进行bitmap尺寸变换
					sfh.unlockCanvasAndPost(canvas);// 解锁画布，提交画好的图像
					
					nResidue_Time = (int) (1000.0 / frame_rate - (System.currentTimeMillis() - startTime) - 5);
					//Log.i(TAG, "*** Decode nResidue_Time = "+String.valueOf( nResidue_Time));
					if (nResidue_Time > 0)
					{
						Thread.sleep(nResidue_Time);
						nResidue_Time = 0;
					}
				}
				else
				{
					Thread.sleep(10);
				}
			}
		} catch (Exception e2)
		{
			e2.printStackTrace();
			Log.e(TAG, "StartDecode_Video " + e2.toString());
		}			
	}
	
	private void StartDecode_Audio()
	{
		Log.i(TAG, "Enter the audio decode thread.");
		try
		{
			int ret = 0;
			int sampleRateInHz = 8000;
			final int frame_cnt = 10;

			while (!isExit )
			{
				ret = NetplayerAPI.GetAudioParam(AudioParams);
				if (ret != 0)
					Thread.sleep(20);
				else 
					break;
			}

			int bufferSize = frame_cnt * AudioParams[0] * AudioParams[1];

			track = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRateInHz, AudioFormat.CHANNEL_OUT_MONO,
			        AudioFormat.ENCODING_PCM_16BIT, bufferSize, AudioTrack.MODE_STREAM);

			if (track.getState() == AudioTrack.STATE_INITIALIZED)
			{
				track.play();
				while (!isExit)
				{
					AudioArray = NetplayerAPI.GetAudioFrame(frame_cnt);
					if (AudioArray != null && AudioArray.length == bufferSize)
					{
						//Log.i(TAG,"AudioArray.length=" + AudioArray.length);
						track.write(AudioArray, 0, AudioArray.length);
					}
					else
					{
						Thread.sleep(20);
					}
				}
				track.stop();			
			}
			track.release();
			track = null;

		} catch (Exception e1)
		{
			e1.printStackTrace();
			Log.e(TAG, "StartDecode_Audio " + e1.toString());
		}
	}

	private void StartRecord_Aduio()
	{
		Log.i(TAG, "Enter the audio record thread.");
		int recBufSize = 0;
		AudioRecord audioRecord = null;

		/********** LinZh107 以下在创建通用于所有机型的录音和播放代码时可用 ***********/
		for (int sampleRateInHz : new int[]{8000, 11025, 16000, 22050, 32000, 47250, 44100, 48000})
		{
			for (short channelConfig : new short[]{AudioFormat.CHANNEL_IN_MONO, AudioFormat.CHANNEL_IN_STEREO})
			{
				for (short audioFormat : new short[]{AudioFormat.ENCODING_PCM_16BIT, AudioFormat.ENCODING_PCM_8BIT})
				{	// Try to initialize
					try
					{	// 用于录音
						recBufSize = AudioRecord.getMinBufferSize(sampleRateInHz, channelConfig, audioFormat);
			
						if (recBufSize <= 0)
							continue;
						Log.i(TAG, "录音: sampleRate=" + sampleRateInHz + " audioFormat=" + audioFormat
						        + " channelConfig=" + channelConfig);
						Log.i(TAG, "Record PCMData Lenght = " + recBufSize);

						/********** LinZh107 以上在创建通用于所有机型的录音和播放代码时可用 ***********/

						audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, sampleRateInHz, channelConfig,
						        audioFormat, recBufSize * 10);

						if (audioRecord.getState() == AudioRecord.STATE_INITIALIZED)
						{
							// 开始录音
							audioRecord.startRecording();

							if (AudioRecord.RECORDSTATE_RECORDING == audioRecord.getRecordingState())
								Log.d(TAG, "RECORDSTATE_RECORDING is true");

							// 发送缓冲区
							int sendlen = 640;
							short[] sendBuffer = new short[sendlen];

							int length = 0;
							while (!isExit)
							{
								// 从mic读取音频数据
								if (m_talkFlag)
								{
									length = audioRecord.read(sendBuffer, 0, sendlen);
									NetplayerAPI.sendVoiceData(userid, sendBuffer, sendlen);
								}
								else 
									Thread.sleep(50);	
							}
							// end while(!isExit)
							audioRecord.stop();							
						}
						// end if INITIALIZED

						audioRecord.release();
						audioRecord = null;
						if (m_talkFlag)
						{
							int ret = NetplayerAPI.stopDualTalk(userid);
							Log.i(TAG, "stopDualTalk return " + ret);							
						}
						return ;
					} catch (Exception e4)
					{
						// Do nothing
						e4.printStackTrace();
						Log.e(TAG, "StartRecord_Aduio " + e4.toString());
					}
				}
			}
		}
	}

	//音频降噪处理
	public void calc1(short[] lin, int off, int len)
	{
		int i, j;

		for (i = 0; i < len; i++)
		{
			j = lin[i + off];
			lin[i + off] = (short) (j >> 2);
		}
	}

	//surface view de callback
	class MyCallBack implements SurfaceHolder.Callback
	{
		@Override
		public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
		{
			// TODO Auto-generated method stub
		}

		@Override
		public void surfaceCreated(SurfaceHolder holder)
		{
			// TODO Auto-generated method stub
			Vthread = new Thread(new Runnable()
			{
				public void run()
				{
					StartDecode_Video();
				}
			});
			Vthread.start();
			
			Athread = new Thread(new Runnable()
			{
				public void run()
				{
					StartDecode_Audio();
				}
			});
			Athread.start();			

			ARthread = new Thread(new Runnable()
			{
				public void run()
				{
					StartRecord_Aduio();
				}
			});
			ARthread.start();
		}

		@Override
		public void surfaceDestroyed(SurfaceHolder holder)
		{
			if (Vthread != null)
			{
				// Log.i(TAG,"surfaceDestroyed called");
				boolean retry = true;
				isExit = true;
				Vthread.interrupt();
				while (retry)
				{
					try
					{
						if (Athread != null)
						{
							Athread.interrupt();
							Athread.join();
						}
						Vthread.join();
						retry = false;
					} catch (InterruptedException e5)
					{
						e5.printStackTrace();
						Log.e(TAG, "surfaceDestroyed " + e5.toString());
					}
				}
			}
		}
	}
	
	// 2014-5-19 API14 以上的要自己捕获返回键
	@Override
	public void onBackPressed()
	{
		super.onBackPressed();		
		finish();
		//mam.popOneActivity(mam.getLastActivity());
	}

	// 7.14号用于log打印时显示哪个activity的哪一行的打印信息
	public String getLineNumber(Exception e4)
	{
		StackTraceElement[] trace = e4.getStackTrace();
		if (trace == null || trace.length == 0)
			return null; //
		return trace[0].getFileName() + "    " + String.valueOf(trace[0].getLineNumber());
	}

	// 以触摸事件响应形式控制云台，此方式按下时开始，弹起时停止。 Date: 2013-12-31 Author: yms
	OnTouchListener onTouchListener = new OnTouchListener()
	{
		public boolean onTouch(View v, MotionEvent event)
		{
			if ((v.getId() == R.id.leftbtn) || (v.getId() == R.id.leftbtn_f))
			{
				if (event.getAction() == MotionEvent.ACTION_DOWN)
				{
					NetplayerAPI.ptzControl(userid, NetplayerAPI.PTZ_LEFTSTART, openchannel, 55);
				}
				else if (event.getAction() == MotionEvent.ACTION_UP)
				{
					NetplayerAPI.ptzControl(userid, NetplayerAPI.PTZ_LEFTSTOP, openchannel, 55);
				}
			}
			else if ((v.getId() == R.id.rightbtn) || (v.getId() == R.id.rightbtn_f))
			{
				if (event.getAction() == MotionEvent.ACTION_DOWN)
				{
					NetplayerAPI.ptzControl(userid, NetplayerAPI.PTZ_RIGHTSTART, openchannel, 55);
				}
				else if (event.getAction() == MotionEvent.ACTION_UP)
				{
					NetplayerAPI.ptzControl(userid, NetplayerAPI.PTZ_RIGHTSTOP, openchannel, 55);
				}
			}
			else if ((v.getId() == R.id.upbtn) || (v.getId() == R.id.upbtn_f))
			{
				if (event.getAction() == MotionEvent.ACTION_DOWN)
				{
					NetplayerAPI.ptzControl(userid, NetplayerAPI.PTZ_UPSTART, openchannel, 55);
				}
				else if (event.getAction() == MotionEvent.ACTION_UP)
				{
					NetplayerAPI.ptzControl(userid, NetplayerAPI.PTZ_UPSTOP, openchannel, 55);
				}
			}
			else if ((v.getId() == R.id.downbtn) || (v.getId() == R.id.downbtn_f))
			{
				if (event.getAction() == MotionEvent.ACTION_DOWN)
				{
					NetplayerAPI.ptzControl(userid, NetplayerAPI.PTZ_DOWNSTART, openchannel, 55);
				}
				else if (event.getAction() == MotionEvent.ACTION_UP)
				{
					NetplayerAPI.ptzControl(userid, NetplayerAPI.PTZ_DOWNSTOP, openchannel, 55);
				}
			}
			else if ((v.getId() == R.id.zoominbtn) || (v.getId() == R.id.zoominbtn_f))
			{
				if (event.getAction() == MotionEvent.ACTION_DOWN)
				{
					NetplayerAPI.ptzControl(userid, NetplayerAPI.PTZ_ZOOMADDSTART, openchannel, 55);
				}
				else if (event.getAction() == MotionEvent.ACTION_UP)
				{
					NetplayerAPI.ptzControl(userid, NetplayerAPI.PTZ_ZOOMADDSTOP, openchannel, 55);
				}
			}
			else if ((v.getId() == R.id.zoomoutbtn) || (v.getId() == R.id.zoomoutbtn_f))
			{
				if (event.getAction() == MotionEvent.ACTION_DOWN)
				{
					NetplayerAPI.ptzControl(userid, NetplayerAPI.PTZ_ZOOMSUBSTART, openchannel, 55);
				}
				else if (event.getAction() == MotionEvent.ACTION_UP)
				{
					NetplayerAPI.ptzControl(userid, NetplayerAPI.PTZ_ZOOMSUBSTOP, openchannel, 55);
				}
			}
			return false;
		}
	};

	// (已屏蔽)以单击事件响应形式控制云台，此方式第一次单击开始，再次单击停止。 Date: 2013-12-31 Author: yms
	OnClickListener onListener = new OnClickListener()
	{
		@Override
		public void onClick(View v)
		{
			// TODO Auto-generated method stub
			switch (v.getId())
			{
				case R.id.SurfaceViewID :// 全屏
					isFullScreen = !isFullScreen;
					// scalePlayWnd();
					break;
				case R.id.btn_search_record:
//					if (isConnected) // 2014-7-23 LinZh107 修复打开通道失败会crash 的bug
//					{
//						NetplayerAPI.stopChannelStream(userid, openchannel, stream_type, channel_type);
//						NetplayerAPI.closeChannelStream(userid, openchannel, channel_type);
//					}
//					try {
//				        Thread.sleep(50);
//			        } catch (InterruptedException e) {
//				        // TODO Auto-generated catch block
//				        e.printStackTrace();
//				        Log.e(TAG, ""+e.toString());
//			        }
//					isExit = true;
//					if (isConnected) // 2014-7-23 LinZh107 修复打开通道失败会crash 的bug
//					{
//						NetplayerAPI.stopChannelStream(userid, openchannel, stream_type, channel_type);
//						NetplayerAPI.closeChannelStream(userid, openchannel, channel_type);
//					}
					finish();
					Intent intent = new Intent(PlayActivity.this, RecordActivity.class);
					intent.putExtra("USER_ID",userid);
					intent.putExtra("CHANNAL",openchannel);
					intent.putExtra("STREAM_TYPE", stream_type);
					intent.putExtra("CHANNEL_TYPE", channel_type);
					startActivity(intent);
					break;
				case R.id.audiobtn :
				{
					// initialization stream_state = 0
					if (stream_state == 0)
					{							
						int ret = 0;
						// 关闭对讲
						if (m_talkFlag) // initialization isTalking = 0
						{
							ret = NetplayerAPI.stopDualTalk(userid);
							Log.e(TAG, "stopDualTalk return ret=" + ret);
							m_talkFlag = false;
						}
						if (ret == 0)
						{
							// 打开音频流
							if(stream_type == 1)
							{
								stream_type = 0; // stream_type == 0 打开复合流
								ret = NetplayerAPI.changeStreamType(userid, openchannel, stream_type, channel_type);
							}								
							if (ret == 0)
							{
								Log.i(TAG, "change stream_type to " + stream_type);
								stream_state = 1;
								audiobtn.setBackgroundResource(R.drawable.audio_on);
								if (track != null && track.getPlayState() != AudioTrack.PLAYSTATE_PLAYING)
								{
									track.play();
								}
							}
						}
					}
					//对讲进行中
					else if(stream_state==1) 
					{
						// 关闭音频流
						if(stream_type == 0)
						{
							stream_type = 1;
							Log.i(TAG, "change stream_type to " + stream_type);							
							NetplayerAPI.changeStreamType(userid, openchannel, stream_type, channel_type);
							if (track != null && track.getPlayState() == AudioTrack.PLAYSTATE_PLAYING)
							{								
								track.pause();
							}
						}
						
						// 打开对讲
						int ret = -1;
						if (!m_talkFlag)
						{
							ret = NetplayerAPI.startDualTalk(userid);
							Log.d(TAG, "startDualTalk return ret=" + ret);
						}
						if (0 == ret)
						{
							m_talkFlag = true;
							stream_state = 2;
							audiobtn.setBackgroundResource(R.drawable.dualtalk_on);
							if (track != null && track.getPlayState() != AudioTrack.PLAYSTATE_PLAYING)
							{
								track.play();									
							}
						}
					}
					//对讲结束，静音模式
					else if(stream_state == 2)
					{
						int ret = 0;
						// 关闭对讲
						if (m_talkFlag) // initialization isTalking = 0
						{
							ret = NetplayerAPI.stopDualTalk(userid);
							Log.e(TAG, "stopDualTalk return ret=" + ret);
							m_talkFlag = false;
						}
						if (ret == 0)
						{
							if (track != null && track.getPlayState() == AudioTrack.PLAYSTATE_PLAYING)
							{
								track.pause();
								stream_state = 0;
								audiobtn.setBackgroundResource(R.drawable.audio_btn);
							}
						}
					}// end { if (stream_type == 1)}
					break;
				}
				default :
					break;
			}
		}
	};

	public void onConfigurationChanged(Configuration _newConfig)
	{
		super.onConfigurationChanged(_newConfig);
		// WindowManager.LayoutParams attrs = getWindow().getAttributes();
		if (_newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE)
		{
			//横屏
			LayoutParams lp = sfv.getLayoutParams();
			scaleXY = Hscale;
			scaleWidth = deviceHeight;
			scaleHeight = deviceWidth;
			lp.width = scaleWidth;
			lp.height = scaleHeight;
			sfv.setLayoutParams(lp);

			// attrs.flags |= WindowManager.LayoutParams.FLAG_FULLSCREEN;

			LinearLayout layout = (LinearLayout) findViewById(R.id.ytline);
			layout.setVisibility(View.GONE);

			LinearLayout layout2 = (LinearLayout) findViewById(R.id.TopPanelLine);
			layout2.setVisibility(View.GONE);

			LinearLayout layout3 = (LinearLayout) findViewById(R.id.cameranameline);
			layout3.setVisibility(View.GONE);

			RelativeLayout layout4 = (RelativeLayout) findViewById(R.id.surfaceline);
			layout4.setGravity(Gravity.CENTER);

			RelativeLayout layout5 = (RelativeLayout) findViewById(R.id.fullscreenbar);
			layout5.setVisibility(View.VISIBLE);

			TableLayout layout6 = (TableLayout) findViewById(R.id.widget_layout_info);
			layout6.setVisibility(View.GONE);
		}
		else	// 竖屏
		{
			//LOW_RESOLUTION_MSG = false;
			LayoutParams lp = sfv.getLayoutParams();
			scaleXY = Vscale;
			scaleWidth = deviceWidth;
			scaleHeight = (int) (deviceWidth * 1.0 / scaleXY);
			lp.width = scaleWidth;
			lp.height = scaleHeight;
			sfv.setLayoutParams(lp);

			// attrs.flags &= ~WindowManager.LayoutParams.FLAG_FULLSCREEN;
			LinearLayout layout = (LinearLayout) findViewById(R.id.ytline);
			layout.setVisibility(View.VISIBLE);

			LinearLayout layout2 = (LinearLayout) findViewById(R.id.TopPanelLine);
			layout2.setVisibility(View.VISIBLE);

			LinearLayout layout3 = (LinearLayout) findViewById(R.id.cameranameline);
			layout3.setVisibility(View.VISIBLE);

			RelativeLayout layout5 = (RelativeLayout) findViewById(R.id.fullscreenbar);
			layout5.setVisibility(View.GONE);

			TableLayout layout6 = (TableLayout) findViewById(R.id.widget_layout_info);
			layout6.setVisibility(View.VISIBLE);
		}
		// getWindow().setAttributes(attrs);
	}

	/*
	 * 时间：2014年8月6日 LinZh107 目的：测试放音方案正确性 结果：方案可行 具体：网上找的demo
	 * 包括pcm文件，循环获取pcm块，然后track.write（），生音正常
	 */
	public int readPCMDataFile()
	{
		// final String filePath =
		// Environment.getExternalStorageDirectory().getAbsolutePath() +
		// "/testmusic.pcm";
		final String filePath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/Record.pcm";

		/*
		 * 获得PCM音频数据
		 */
		File file = new File(filePath);
		FileInputStream inStream = null;
		try
		{
			inStream = new FileInputStream(file);
			Log.i(TAG, "Get the PCM file!");
		} catch (FileNotFoundException e6)
		{
			e6.printStackTrace();
			Log.e(TAG, "readPCMDataFile " + e6.toString());
			return -2;
		}

		PCM_pack = null;
		if (inStream != null)
		{
			long size = file.length();

			PCM_pack = new byte[(int) size];
			try
			{
				inStream.read(PCM_pack);
			} catch (IOException e7)
			{
				// TODO Auto-generated catch block
				e7.printStackTrace();
				Log.e(TAG, "readPCMDataFile " + e7.toString());
				return -3;
			}
		}
		try {
            inStream.close();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
		return 0;
	}

	/*
	 * 时间：2014年8月7日 LinZh107 目的：测试解码和放音 整个过程是可行的 结果：权限不够 无法保存文件，无法进下一步
	 * 具体：自解g711a成pcm，然后保存成文件，再独立播放看是否还会有噪音
	 */
	public int writePCMData(short[] sArray)
	{
		Log.i(TAG, "sArray.length=" + sArray.length);
		final String filePath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/Record.pcm";
		File file = new File(filePath);
		if (!file.exists())
		{
			try
			{
				// 在指定的文件夹中创建文件
				file.createNewFile();
			} catch (Exception e8)
			{
				e8.printStackTrace();
				Log.e(TAG, "writePCMData " + e8.toString());
				return -2;
			}
		}

		FileWriter fw = null;
		BufferedWriter bw = null;
		try
		{
			fw = new FileWriter(filePath, true);//
			// 创建FileWriter对象，用来写入字符流
			bw = new BufferedWriter(fw); // 将缓冲对文件的输出

			// String str = "";
			byte buf[] = new byte[2 * sArray.length];

			for (int i = 0; i < sArray.length; i++)
			{
				short sTemp = sArray[i];

				/********* begin **************************************/
				buf[2 * i] = (byte) (sTemp & 0x00ff);
				bw.write(buf[2 * i]);
				sTemp >>= 8;
				buf[2 * i + 1] = (byte) (sTemp & 0x00ff);
				bw.write(buf[2 * i + 1]);

				/********* begin 以下区别于 short-->byte 的文件保存方法 *********/
				// buf = (byte) (sTemp & 0x00ff);
				// str += buf;
				// sTemp >>= 8;
				// buf = (byte) (sTemp & 0x00ff);
				// str += buf;

				/********* end ****************************************/
			}
			Log.i(TAG, "Write audioData length is " + sArray.length);

			// bw.write(str);
			// Log.i(TAG, "Write audioData length is " + str.length());

			bw.flush(); // 刷新该流的缓冲
			bw.close();
			fw.close();
		} catch (IOException e9)
		{
			// TODO Auto-generated catch block
			e9.printStackTrace();
			Log.e(TAG, "writePCMData " + e9.toString());
			try
			{
				bw.close();
				fw.close();
			} catch (IOException e10)
			{
				// TODO Auto-generated catch block
				e10.printStackTrace();
				Log.e(TAG, "writePCMData " + e10.toString());
			}
		}
		return 0;
	}

	protected void onDestroy()
	{
		super.onDestroy();
		isExit = true;
		if (isConnected) // 2014-7-23 LinZh107 修复打开通道失败会crash 的bug
		{
			NetplayerAPI.stopChannelStream(userid, openchannel, stream_type, channel_type);
			NetplayerAPI.closeChannelStream(userid, openchannel, channel_type);
			System.out.println("销毁成功");
		}
		// ImageLayout.invalidate();
		if (mBitmap != null)
		{
			mBitmap.recycle();
			mBitmap = null;
		}
		try {
	        Thread.sleep(50);
        } catch (InterruptedException e) {
	        // TODO Auto-generated catch block
	        e.printStackTrace();
	        Log.e(TAG, ""+e.toString());
        }
		Log.i(TAG, "play onDestroy called");
	}
}
