/*
 * 2014-5-20 LinZh107
 * 1.换新的解码库后播放能力提升导致了回放的速度异常，现已修改，在性能满足的情况下能按帧率播放
 */

package com.topvs.client;

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
import android.os.Handler;
import android.os.Message;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TableLayout;
import android.widget.TextView;

public class RecordPlayActivity extends Activity {
	final static String TAG = "RecordPlayActivity";
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
	// private boolean LOW_RESOLUTION_MSG = false;
	private static boolean isFullScreen = false;
	private boolean isExit = false;

	private int userid = -1;
	private int openchannel = 0;
	private int protocoltype = 1;
	private int channel_type = -1; // 从主码流，0主码流， 1从码流
	// 发布参数
	private int stream_type = 1; // 0:复合流 1:视频流， 2:音频
	private int stream_state = 0; // 0:音视频 1:视频， 2:对讲

	// ************ 2014-5-13 LinZh107 增加视频缩放比例,以区别横、竖屏时的视频长宽比
	public Bitmap mBitmap = null; // 录像时需要，所以必须为public
	public int VideoParams[] = { 720, 576 }; // 视频分辨率
	public int frame_rate = 25; // getVideoFrame return frame_rate

	public AudioTrack track = null;
	static byte PCM_pack[]; // PCM 音频包
	public int AudioParams[] = { 0, 0 }; // 音频帧速和帧长
	public short AudioArray[] = null;

	// 2013-12-9 yms
	private int scaleWidth;
	private int scaleHeight;
	private int deviceWidth;
	private int deviceHeight;

	private float scaleXY;
	private float Hscale = 1.5f;
	private float Vscale = 1.25f; // 默认的长宽比例

	// *******2014-4-25 LinZh107 修复横屏下打开问题
	private Configuration config = null;
	private Resources rs;
	private int user_id;
	private int selectedIndexs;
	private int play_id;
	
			
	

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);
		setContentView(R.layout.recordplayview);
		rs = getResources();
		config = getResources().getConfiguration();
		Intent intent = getIntent();
		user_id = intent.getIntExtra("user_id", -1);
		selectedIndexs = intent.getIntExtra("index", 0);
		System.out.println("user_id:" + user_id + " index:" + selectedIndexs);
			new Thread() {

				@Override
				public void run() {
					play_id = NetplayerAPI.startRecordPlayer(user_id,
							selectedIndexs);
					System.out.println("录像播放结果:" + play_id);
//					handler.sendEmptyMessage(1);
				}
			}.start();
			
			initSurfaceView();
	}

	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		System.out.println("调用了销毁方法");
		if (Vthread != null) {
			// Log.i(TAG,"surfaceDestroyed called");
			boolean retry = true;
			isExit = true;
			Vthread.interrupt();
			while (retry) {
				try {
					if (Athread != null) {
						Athread.interrupt();
						Athread.join();
					}
					Vthread.join();
					retry = false;
				} catch (InterruptedException e5) {
					e5.printStackTrace();

				}
			}
		}
		NetplayerAPI.stopRecordPlayer(play_id);
		try {
			Thread.sleep(1000);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	private void initSurfaceView() {
		// TODO Auto-generated method stub
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
		sfh.addCallback(new MyCallBack());//
	}
	
	@Override
	public void onConfigurationChanged(Configuration _newConfig) {
		// TODO Auto-generated method stub
		super.onConfigurationChanged(_newConfig);
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

			LinearLayout TopPanelLine = (LinearLayout) findViewById(R.id.TopPanelLine);
			TopPanelLine.setVisibility(View.GONE);
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
			LinearLayout TopPanelLine = (LinearLayout) findViewById(R.id.TopPanelLine);
			TopPanelLine.setVisibility(View.VISIBLE);
			
		}
	}
	
	class MyCallBack implements SurfaceHolder.Callback {
		@Override
		public void surfaceChanged(SurfaceHolder holder, int format, int width,
				int height) {
			// TODO Auto-generated method stub
		}

		@Override
		public void surfaceCreated(SurfaceHolder holder) {
			// TODO Auto-generated method stub
			Vthread = new Thread(new Runnable() {
				public void run() {
					StartDecode_Video();
				}
			});
			Vthread.start();

			Athread = new Thread(new Runnable() {
				public void run() {
					StartDecode_Audio();
				}
			});
			Athread.start();

			ARthread = new Thread(new Runnable() {
				public void run() {
					StartRecord_Aduio();
				}
			});
			ARthread.start();
		}

		@Override
		public void surfaceDestroyed(SurfaceHolder holder) {
			if (Vthread != null) {
				// Log.i(TAG,"surfaceDestroyed called");
				boolean retry = true;
				isExit = true;
				Vthread.interrupt();
				while (retry) {
					try {
						if (Athread != null) {
							Athread.interrupt();
							Athread.join();
						}
						Vthread.join();
						retry = false;
					} catch (InterruptedException e5) {
						e5.printStackTrace();

					}
				}
			}
		}
	}

	private void StartDecode_Video() {
		Log.i(TAG, "Enter the video_decode thread.");
//		System.out.println("22222");
		try {

			Canvas canvas = null;
			int frame_rate = 0; // getVideoFrame return frame_rate
			int nResidue_Time = 0;
			// 2014-4-25 修复横屏状态下打开问题
			if (config.orientation != 1)
				setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
			
//			System.out.println("1111");
			while (!isExit) {
				// /旧方案，已完全屏蔽
				// 2014-5-10 LinZh107 更换新的显示方案 修改以支持视频格式自适应
				// System.out.println("1111");
				long startTime = System.currentTimeMillis();
                //System.out.println("循环");
				if (VideoParams[0] < scaleWidth) // 107.视频分辨率小于屏幕，底层SWS_SCALE不进行bitmap尺寸变换
				{
					mBitmap = Bitmap.createBitmap(VideoParams[0],
							VideoParams[1], Bitmap.Config.RGB_565);
					frame_rate = NetplayerAPI.getVideoFrame(VideoParams,
							mBitmap); // 底层创建的位图没有进行缩放
					System.out.println("frame_rate:" + frame_rate);
				} else// 107.视频分辨率 >= 屏幕，底层 SWS_SCALE 进行bitmap缩小变换
				{
					VideoParams[0] = scaleWidth;
					VideoParams[1] = scaleHeight;
					mBitmap = Bitmap.createBitmap(VideoParams[0],
							VideoParams[1], Bitmap.Config.RGB_565);
					frame_rate = NetplayerAPI.getVideoFrame(VideoParams,
							mBitmap); // 底层创建完整位图(已经进行缩放)
//					System.out.println("frame_rate:" + frame_rate);
				}

				if (frame_rate > 0) {
					Rect rect = new Rect(0, 0, scaleWidth, scaleHeight);
					canvas = sfh.lockCanvas(rect);
					canvas.drawBitmap(mBitmap, null, rect, null); // 107.java层进行bitmap尺寸变换
					sfh.unlockCanvasAndPost(canvas);// 解锁画布，提交画好的图像

					nResidue_Time = (int) (1000.0 / 200 - (System
							.currentTimeMillis() - startTime));
					// Log.i(TAG, "*** Decode nResidue_Time = "+String.valueOf(
					// nResidue_Time));
					if (nResidue_Time > 0) {
						Thread.sleep(nResidue_Time);
						nResidue_Time = 0;
					}
				} else {
					Thread.sleep(5);
				}
			}
		} catch (Exception e2) {
			e2.printStackTrace();
			System.out.println("抛异常了");
			Log.e(TAG, "StartDecode_Video " + e2.toString());
		}
	}

	private void StartDecode_Audio() {
		Log.i(TAG, "Enter the audio decode thread.");
		try {
			int ret = 0;
			int sampleRateInHz = 8000;
			final int frame_cnt = 10;

			while (!isExit) {
				ret = NetplayerAPI.GetAudioParam(AudioParams);
				if (ret != 0)
					Thread.sleep(20);
				else
					break;
			}

			int bufferSize = frame_cnt * AudioParams[0] * AudioParams[1];

			track = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRateInHz,
					AudioFormat.CHANNEL_OUT_MONO,
					AudioFormat.ENCODING_PCM_16BIT, bufferSize,
					AudioTrack.MODE_STREAM);

			if (track.getState() == AudioTrack.STATE_INITIALIZED) {
				track.play();
				while (!isExit) {
					AudioArray = NetplayerAPI.GetAudioFrame(frame_cnt);
					if (AudioArray != null && AudioArray.length == bufferSize) {
						// Log.i(TAG,"AudioArray.length=" + AudioArray.length);
						track.write(AudioArray, 0, AudioArray.length);
					} else {
						Thread.sleep(20);
					}
				}
				track.stop();
			}
			track.release();
			track = null;

		} catch (Exception e1) {
			e1.printStackTrace();
			Log.e(TAG, "StartDecode_Audio " + e1.toString());
		}
	}

	private void StartRecord_Aduio() {
		Log.i(TAG, "Enter the audio record thread.");
		int recBufSize = 0;
		AudioRecord audioRecord = null;

		/********** LinZh107 以下在创建通用于所有机型的录音和播放代码时可用 ***********/
		for (int sampleRateInHz : new int[] { 8000, 11025, 16000, 22050, 32000,
				47250, 44100, 48000 }) {
			for (short channelConfig : new short[] {
					AudioFormat.CHANNEL_IN_MONO, AudioFormat.CHANNEL_IN_STEREO }) {
				for (short audioFormat : new short[] {
						AudioFormat.ENCODING_PCM_16BIT,
						AudioFormat.ENCODING_PCM_8BIT }) { // Try to initialize
					try { // 用于录音
						recBufSize = AudioRecord.getMinBufferSize(
								sampleRateInHz, channelConfig, audioFormat);

						if (recBufSize <= 0)
							continue;
						Log.i(TAG, "录音: sampleRate=" + sampleRateInHz
								+ " audioFormat=" + audioFormat
								+ " channelConfig=" + channelConfig);
						Log.i(TAG, "Record PCMData Lenght = " + recBufSize);

						/********** LinZh107 以上在创建通用于所有机型的录音和播放代码时可用 ***********/

						audioRecord = new AudioRecord(
								MediaRecorder.AudioSource.MIC, sampleRateInHz,
								channelConfig, audioFormat, recBufSize * 10);

						if (audioRecord.getState() == AudioRecord.STATE_INITIALIZED) {
							// 开始录音
							audioRecord.startRecording();

							if (AudioRecord.RECORDSTATE_RECORDING == audioRecord
									.getRecordingState())
								Log.d(TAG, "RECORDSTATE_RECORDING is true");

							// 发送缓冲区
							int sendlen = 640;
							short[] sendBuffer = new short[sendlen];

							int length = 0;
							while (!isExit) {
								// 从mic读取音频数据
								if (m_talkFlag) {
									length = audioRecord.read(sendBuffer, 0,
											sendlen);
									NetplayerAPI.sendVoiceData(userid,
											sendBuffer, sendlen);
								} else
									Thread.sleep(50);
							}
							// end while(!isExit)
							audioRecord.stop();
						}
						// end if INITIALIZED

						audioRecord.release();
						audioRecord = null;
						if (m_talkFlag) {
							int ret = NetplayerAPI.stopDualTalk(userid);
							Log.i(TAG, "stopDualTalk return " + ret);
						}
						return;
					} catch (Exception e4) {
						// Do nothing
						e4.printStackTrace();
						Log.e(TAG, "StartRecord_Aduio " + e4.toString());
					}
				}
			}
		}
	}
}
