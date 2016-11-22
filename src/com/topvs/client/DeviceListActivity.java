package com.topvs.client;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Timer;
import java.util.TimerTask;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.Point;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Switch;
import android.widget.TabHost;
import android.widget.TextView;
import android.widget.TabHost.OnTabChangeListener;
import android.widget.TableLayout;
import android.widget.TableRow;

@SuppressLint("NewApi") 
public class DeviceListActivity extends Activity
{
	final static String TAG = "DeviceListActivity";
	//public MyActivityManager mam = null;
	
	final int PROTOCOL_TCP = 0x01;//以下两种协议并入TCP
	final int PROTOCOL_UDP = 0x04;
	final int PROTOCOL_MCAST = 0x05;
	private final int ALARM_TYPE = 25;
	
	final static int REQUEST_CONFIG = 1;
	final static int REQUEST_ENCODE = 2;
	final static int REQUEST_PLAY = 3;
	
	private byte[] configinfos = new byte[4];
	private int[] encodeinfos = new int[9];

	private Button btn_open;
	private Button btn_logout;
	private RadioGroup m_layout_channel;
	
	/**
	 * timer for getting the ck_AlarmInfo from native
	 * add Linzh107 2015-8-05
	 * */	
	private final int DELAY = 500; // 0.5s
	private final int PERIOD = 2000; // 1s
	private Timer mAlarmTimer = null;
	private TimerTask mTimerTask;
	private Handler mHandler = null;
	public static dev_ck_data_t dev_info;
	public static dev_alarmtime alarm_time;
	
	public static int MAX_ALARMINFO_COUNT = 32; // 设备套数
	public static int iAlarmIDArray[] = new int[MAX_ALARMINFO_COUNT];	//告警类ID, Every ID have 6 type
	public static String AlarmDateStr[] = new String[MAX_ALARMINFO_COUNT * 2];	// 告警发生时间
	public static String AlarmTypeStr[][] = new String[MAX_ALARMINFO_COUNT][6];	//告警类型名
	private int AlarmTimeArray[] = new int[MAX_ALARMINFO_COUNT];
	private int iAlarmIDIndex = 0;						// 告警信息设备下标
	protected int iAlarmTypeIndex = 0;					// 告警信息下标，即type

	private final SimpleDateFormat mSDFormat = new SimpleDateFormat("MM/dd");
	private final SimpleDateFormat mSDTime = new SimpleDateFormat("HH:mm:ss");

	private TableLayout mTableLayout; 					// 告警输出表格
	private int MAX_TABLEROW = 0; 						// 实际行数
	
	
	//网络开关部分
	private Thread mPreThread;
	public dev_io_control_t m_netswitch;
	private Switch swin_group[], swout_group[];
	private TableLayout table_switch_in1;
	private TableLayout table_switch_in2;
	private TableLayout table_switch_out1;
	private TableLayout table_switch_out2;
	//private TableLayout table_edit_time1;
	//private TableLayout table_edit_time2;
	
	private TabHost myTabHost;
	private int tabIndex;
	
	private int userid;
	private int channel_type;	

	private RadioGroup videoConfigGroup;	
	private Resources rs;
	public Point point = new Point();
	
	@Override
	public void onCreate(Bundle icicle)
	{
		super.onCreate(icicle);

		Bundle extras = getIntent().getExtras();			
		int num = extras.getInt("ChannelNum");
		userid = extras.getInt("USERID");
		channel_type = extras.getInt("CHANNEL_TYPE");
		
		rs = getResources();
		setContentView(R.layout.dev_tabhost);
		WindowManager wm = this.getWindowManager();
		wm.getDefaultDisplay().getSize(point);
		
		myTabHost = (TabHost) findViewById(R.id.tablehost);
		myTabHost.setup();
		tabIndex = 1;
		
		LayoutInflater inflater = LayoutInflater.from(this); 
		//把配置文件转换为显示TabHost内容的FrameLayout中的层级  
        inflater.inflate(R.layout.tab_channel, myTabHost.getTabContentView());  
        inflater.inflate(R.layout.tab_mesurement, myTabHost.getTabContentView()); 
        inflater.inflate(R.layout.tab_switch_out, myTabHost.getTabContentView()); 
        inflater.inflate(R.layout.tab_switch_in, myTabHost.getTabContentView()); 
        
		myTabHost.addTab(myTabHost.newTabSpec("one").setIndicator(rs.getString(R.string.camera))
				.setContent(R.id.layout_channel)); 
		myTabHost.addTab(myTabHost.newTabSpec("two").setIndicator(rs.getString(R.string.measurement))
				.setContent(R.id.widget_layout_info));
		myTabHost.addTab(myTabHost.newTabSpec("three").setIndicator(rs.getString(R.string.switch_in))
				.setContent(R.id.layout_switch_out));
		myTabHost.addTab(myTabHost.newTabSpec("four").setIndicator(rs.getString(R.string.switch_out))
				.setContent(R.id.layout_switch_in));
		
		myTabHost.setOnTabChangedListener(tabChangeListener);
		
		m_layout_channel = (RadioGroup) findViewById(R.id.radiogroup_channel);	
		for (int i = 0; i < num; i++)
		{
			RadioButton btn = new RadioButton(this);
			btn.setId(i);
			btn.setText(rs.getString(R.string.chn_act_channelNum) +" "+ (i + 1));
			btn.setTextColor(Color.BLACK);

			m_layout_channel.addView(btn);
		}
		m_layout_channel.check(0);//checked the first one by default
				
		CreateInfoListView(0);

		btn_open = (Button) findViewById(R.id.button_open);
		btn_open.setOnClickListener(listener);
		btn_logout = (Button) findViewById(R.id.button_logout);
		btn_logout.setOnClickListener(listener);
		
		// 得到当前线程的Looper实例，由于当前线程是UI线程也可以通过Looper.getMainLooper()得
		Looper looper = Looper.myLooper();
		// 此处甚至可以不需要设置Looper，因为 Handler默认就使用当前线程的Loope
		mHandler = new MessageHandler(looper);
		dev_info = new dev_ck_data_t();		
		alarm_time = new dev_alarmtime();

		/************* 音频解码线程 ***************/
		mPreThread = new Thread(new Runnable() { 
			public void run(){
				threadRunGetIOState();
			} 
		});
		mPreThread.start();
	}
	
	// 2014-5-13 LinZh107 add noted info
	class MessageHandler extends Handler
	{
		public MessageHandler(Looper looper)
		{
			super(looper);
		}
		@Override
		public void handleMessage(Message msg)
		{
			// TODO Auto-generated method stub
			switch (msg.what)
			{
			case ALARM_TYPE:
				iAlarmIDIndex = 0;
				// 搜索是否有对应的 设备ID iAlarmIDIndex
				if (iAlarmIDArray != null) {
					while (iAlarmIDIndex < MAX_ALARMINFO_COUNT) {
						if (iAlarmIDArray[iAlarmIDIndex] == 0) {
							//if this ID didn't appear yet, add to the array
							iAlarmIDArray[iAlarmIDIndex] = dev_info.dev_id;
						}
						
						if (iAlarmIDArray[iAlarmIDIndex] == dev_info.dev_id)
							break;
						else 
							iAlarmIDIndex++;
					}
					if (iAlarmIDIndex < MAX_ALARMINFO_COUNT 
						&& AlarmTimeArray[iAlarmIDIndex] != alarm_time.second) 
					{	
						// 更新列表
						AlarmTimeArray[iAlarmIDIndex] = alarm_time.second;
						UpdateInfoListView(iAlarmIDIndex);
					}
				}
				break;
			}
		}
	}
		
	private void CreateTimerTask()
	{
		/*
	 	* 获取监测信息 2014-4-1定制 by：LinZh107
	 	*/
		if (mTimerTask == null) {
			mTimerTask = new TimerTask() {	// TimerTask 是个抽象类,实现的是Runable类
				@Override
				public void run()
				{
					// 定义一个消息传过去
					Log.i(TAG, "this is the time thread.");
					int ret = NetplayerAPI.GetdevInfo(0/*channel*/, dev_info, alarm_time);
					mHandler.sendEmptyMessage(ret);
				}
			};
			
			if (mAlarmTimer == null) {
				mAlarmTimer = new Timer();
				mAlarmTimer.schedule(mTimerTask, DELAY, PERIOD);
			}
		}
	}
	
	private void CancelTimerTask()
	{
		if (mAlarmTimer != null) {
			mAlarmTimer.cancel();
			mAlarmTimer = null;
		}
		if (mTimerTask != null) {
			mTimerTask.cancel();
			mTimerTask = null;
		}
	}
	
	private void CreateInfoListView(int count)
	{
		mTableLayout = (TableLayout) findViewById(R.id.widget_layout_info);
		mTableLayout.setStretchAllColumns(true);
		for (int row = count; row < MAX_TABLEROW; row++) {
			TableRow tablerow = new TableRow(this);
			if (row % 2 == 0)
				tablerow.setBackgroundColor(Color.rgb(240, 250, 250));
			for (int list = 0; list < 4; list++) {
				TextView tv = new TextView(this);
				tv.setTextSize(15);
				if (list == 0) {
					if (row < 9)
						tv.setText(" 0" + (row + 1));
					else tv.setText(" " + (row + 1));
				}
				else tv.setText("value");
				tablerow.addView(tv);
			}
			mTableLayout.addView(tablerow, new TableLayout.LayoutParams(
					ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT));
		}
	}
	
	private void CreateNetSwitchLayout()
    {
	    // TODO Auto-generated method stub
		m_netswitch = new dev_io_control_t();
		NetplayerAPI.getAllIOState(userid, m_netswitch);

		swin_group = new Switch[m_netswitch.input_num];		
		swout_group = new Switch[m_netswitch.output_num];
		
		int height = point.y;
    	if(height > 900)
    		height /= 16;
    	else
    		height /= 12;
		TableRow.LayoutParams swRowLayoutParam = new TableRow.LayoutParams(
				TableRow.LayoutParams.WRAP_CONTENT, height);
		
		int i;
		int list = 2;
		int row = m_netswitch.input_num/list;		
		table_switch_in1 = (TableLayout) findViewById(R.id.table_switch_in1);
		table_switch_in2 = (TableLayout) findViewById(R.id.table_switch_in2);
		for(i = 0; i < row; i++)
		{
			TableRow tablerow1 = new TableRow(this);
			Switch sw1 = new Switch(this);
			sw1.setId(i);
			sw1.setText(String.valueOf(i + 1));
			sw1.setTextColor(Color.BLACK);
			sw1.setTextAlignment(View.TEXT_ALIGNMENT_TEXT_END);
			sw1.setLayoutParams(swRowLayoutParam);
			sw1.setEnabled(false);
			tablerow1.addView(sw1);
			swin_group[i] = sw1;
			if(m_netswitch.input[i] == 0x01)
				sw1.setChecked(true);
			else
				sw1.setChecked(false);
			table_switch_in1.addView(tablerow1);
			
			TableRow tablerow2 = new TableRow(this);
			Switch sw2 = new Switch(this);
			sw2.setId(i + row);
			sw2.setText(String.valueOf(i + row + 1));
			sw2.setTextColor(Color.BLACK);
			sw2.setTextAlignment(View.TEXT_ALIGNMENT_TEXT_END);
			sw2.setLayoutParams(swRowLayoutParam);
			sw2.setEnabled(false);
			tablerow2.addView(sw2);
			swin_group[i + row] = sw2;
			if(m_netswitch.input[i + row] == 0x01)
				sw2.setChecked(true);
			else
				sw2.setChecked(false);
			table_switch_in2.addView(tablerow2);
		}	
		
		row = m_netswitch.output_num/list;
		table_switch_out1 = (TableLayout) findViewById(R.id.table_switch_out1);
		table_switch_out2 = (TableLayout) findViewById(R.id.table_switch_out2);
		for(i = 0; i < row; i++)
		{
			TableRow tablerow1 = new TableRow(this);
			Switch sw1 = new Switch(this);
			sw1.setId(i);
			sw1.setText(String.valueOf(i + 1));
			sw1.setTextColor(Color.BLACK);
			sw1.setTextAlignment(View.TEXT_ALIGNMENT_TEXT_END);
			sw1.setLayoutParams(swRowLayoutParam);
			sw1.setOnClickListener(swlistener);
			sw1.setOnCheckedChangeListener(swChangeListener);
			tablerow1.addView(sw1);
			swout_group[i] = sw1;
			if(m_netswitch.output[i] == 0x01)
				sw1.setChecked(true);
			else
				sw1.setChecked(false);			
			table_switch_out1.addView(tablerow1);
			
			TableRow tablerow2 = new TableRow(this);
			Switch sw2 = new Switch(this);
			sw2.setId(i + row);
			sw2.setText(String.valueOf(i + row + 1));
			sw2.setTextColor(Color.BLACK);
			sw2.setTextAlignment(View.TEXT_ALIGNMENT_TEXT_END);
			sw2.setLayoutParams(swRowLayoutParam);
			sw2.setOnClickListener(swlistener);
			sw2.setOnCheckedChangeListener(swChangeListener);
			tablerow2.addView(sw2);
			swout_group[i + row] = sw2;
			if(m_netswitch.output[i + row] == 0x01)
				sw2.setChecked(true);
			else
				sw2.setChecked(false);
			table_switch_out2.addView(tablerow2);
		}			
    }

	private void threadRunGetIOState()
	{
		//Initialize the layout
		CreateNetSwitchLayout();
	}
	
	private void UpdateInfoListView(int nindex)
	{
		iAlarmTypeIndex = nindex;
		int curRow = iAlarmTypeIndex * 6;
		if (curRow + 6 > MAX_TABLEROW) {
			int SumTable = MAX_TABLEROW;
			MAX_TABLEROW = curRow + 6;
			CreateInfoListView(SumTable);
		}
		if (AlarmTypeStr != null) {
			AlarmTypeStr[iAlarmTypeIndex][0] = "空气温度:" + dev_info.ck_val[0] + " ℃";
			AlarmTypeStr[iAlarmTypeIndex][1] = "空气湿度:" + dev_info.ck_val[1] + " %";
			AlarmTypeStr[iAlarmTypeIndex][2] = "土壤温度:" + dev_info.ck_val[2] + " ℃";
			AlarmTypeStr[iAlarmTypeIndex][3] = "土壤湿度:" + dev_info.ck_val[3] + " %";
			AlarmTypeStr[iAlarmTypeIndex][4] = "光照强度:" + dev_info.ck_val[4] + " lux";
			AlarmTypeStr[iAlarmTypeIndex][5] = "CO2浓度:" + dev_info.ck_val[5] + " ppm";
			
			AlarmDateStr[iAlarmTypeIndex * 2] = String.valueOf(alarm_time.year)
										+ "-" + String.valueOf(alarm_time.month)
										+ "-" + String.valueOf(alarm_time.day);
			AlarmDateStr[iAlarmTypeIndex * 2 + 1] = String.valueOf(alarm_time.hour)
											+ ":" + String.valueOf(alarm_time.minute)
											+ ":" + String.valueOf(alarm_time.second);
			TableRow tmpRow = null;
			TextView tv = null;
			// 更新告警列值
			for (int row = curRow; row < curRow + 6; row++) {
				tmpRow = (TableRow) mTableLayout.getChildAt(row + 1);
				tv = (TextView) tmpRow.getChildAt(3);// 更新告警列值
				tv.setText(AlarmTypeStr[iAlarmTypeIndex][row % 6]);

				tv = (TextView) tmpRow.getChildAt(1);// 更新时间列值
				tv.setText("...");
				tv = (TextView) tmpRow.getChildAt(2);// 更新设备ID列值
				tv.setText(String.format("%1$#10x", iAlarmIDArray[iAlarmTypeIndex]));
			}
			tmpRow = (TableRow) mTableLayout.getChildAt(curRow + 1);
			tv = (TextView) tmpRow.getChildAt(1);// 更新时间列 年月日值
			tv.setText(AlarmDateStr[iAlarmTypeIndex * 2]);

			tmpRow = (TableRow) mTableLayout.getChildAt(curRow + 2);
			tv = (TextView) tmpRow.getChildAt(1);// 更新时间列 分秒值
			tv.setText(AlarmDateStr[iAlarmTypeIndex * 2 + 1]);
		}
	}

	OnClickListener swlistener = new OnClickListener()
	{
		@Override
		public void onClick(View v)
		{
			Log.v(TAG, "your have clicked the "+v.getId()+" switch!");
			swout_group[v.getId()].setChecked(swout_group[v.getId()].isChecked());
			NetplayerAPI.setIOState(userid, v.getId(), swout_group[v.getId()].isChecked(), 0);		
		}
	};
		
	OnTabChangeListener tabChangeListener = new OnTabChangeListener()
	{
		@Override
		public void onTabChanged(String tabId)
		{
			CancelTimerTask();
			switch(tabId)
			{
				case "one":
					btn_open.setText(rs.getString(R.string.open_btn));
					btn_logout.setText(rs.getString(R.string.logout_btn));
					btn_open.setEnabled(true);
					btn_logout.setEnabled(true);
					tabIndex = 1;
					break;
				case "two":
					btn_open.setText(rs.getString(R.string.open_btn));
					btn_logout.setText(rs.getString(R.string.logout_btn));
					btn_open.setEnabled(false);
					btn_logout.setEnabled(true);
					tabIndex = 2;
					CreateTimerTask();
					break;
				case "three":
					btn_open.setText(rs.getString(R.string.refresh_btn));
					btn_logout.setText(rs.getString(R.string.set_btn));
					btn_open.setEnabled(true);
					btn_logout.setEnabled(true);
					tabIndex = 3;
					break;
				case "four":
					btn_open.setText(rs.getString(R.string.refresh_btn));
					btn_logout.setText(rs.getString(R.string.set_btn));
					btn_open.setEnabled(true);
					btn_logout.setEnabled(false);
					tabIndex = 4;
					break;
				default:
					break;
			}
		}
	};
	
	OnClickListener listener = new OnClickListener()
	{
		public void onClick(View v)
		{
			if(tabIndex == 1)
			{
				if (v.getId() == R.id.button_open){
					int[] vc = new int[9];
					int channel = m_layout_channel.getCheckedRadioButtonId();
					NetplayerAPI.getVideoEncode(userid, channel, channel_type, vc);
	
					Intent in = new Intent(DeviceListActivity.this, PlayActivity.class);
					in.putExtra("OpenChannel", channel);
					in.putExtra("USERID", userid);
					in.putExtra("CHANNEL_TYPE", channel_type);
					in.putExtra("PROTOCOL", PROTOCOL_TCP); //PROTOCOL_UDP / PROTOCOL_MCAST
					in.putExtra("VC", vc);
					startActivity(in);
				}
				else if (v.getId() == R.id.button_logout)
					finish();
			}
			else if(tabIndex == 2 && v.getId() == R.id.button_logout)
				finish();
			else if(tabIndex == 3)
			{
				if (v.getId() == R.id.button_open){
					NetplayerAPI.getAllIOState(userid, m_netswitch);
					int i = 0;
					while(i< swout_group.length){
						if(m_netswitch.output[i] == 0x01)
							swout_group[i].setChecked(true);
						else
							swout_group[i].setChecked(false);
						i++;
					}
				}
				else if (v.getId() == R.id.button_logout){
					NetplayerAPI.setAllIOState(userid, m_netswitch);
				}
			}
			else if(tabIndex == 4)
			{
				if (v.getId() == R.id.button_open){
					NetplayerAPI.getAllIOState(userid, m_netswitch);
					int i = 0;
					while(i< swin_group.length){
						if(m_netswitch.input[i] == 0x01)
							swin_group[i].setChecked(true);
						else
							swin_group[i].setChecked(false);
						i++;
					}
				}
			}				
		}
	};

	OnCheckedChangeListener swChangeListener = new OnCheckedChangeListener()
	{
		@Override
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
        {
	        // TODO Auto-generated method stub
			Log.v(TAG, "your have checked the "+buttonView.getId()+" switch!");			
			if(swout_group[buttonView.getId()].isChecked())
				m_netswitch.output[buttonView.getId()] = (byte)(0x01);
			else
				m_netswitch.output[buttonView.getId()] = (byte)(0x00);
        }
	};
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		if (requestCode == REQUEST_CONFIG)
		{
			if (resultCode == RESULT_OK)
			{
				String brightness = (String) data.getCharSequenceExtra("brightness");
				String contrast = (String) data.getCharSequenceExtra("contrast");
				String hue = (String) data.getCharSequenceExtra("hue");
				String saturation = (String) data.getCharSequenceExtra("saturation");

				int ibrightness = Integer.valueOf(brightness);
				int icontrast = Integer.valueOf(contrast);
				int ihue = Integer.valueOf(hue);
				int isaturation = Integer.valueOf(saturation);
				if (ibrightness > 127)
					ibrightness = -256 + ibrightness;
				if (icontrast > 127)
					icontrast = 256 + icontrast;
				if (ihue > 127)
					ihue = 256 + ihue;
				if (isaturation > 127)
					isaturation = 256 + isaturation;

				int channel = videoConfigGroup.getCheckedRadioButtonId() - 1;
				configinfos[0] = (byte) ibrightness;
				configinfos[1] = (byte) icontrast;
				configinfos[2] = (byte) ihue;
				configinfos[3] = (byte) isaturation;
				NetplayerAPI.setVideoConfig(userid, channel, configinfos);// ,pBuffer);
			}
		}
		else if (requestCode == REQUEST_ENCODE)
		{
			if (resultCode == RESULT_OK)
			{
				String strVideoformat = (String) data.getCharSequenceExtra("video_format");
				String strResolution = (String) data.getCharSequenceExtra("resolution");
				String strBitrate = (String) data.getCharSequenceExtra("bitrate_type");
				String strEncodeType = (String) data.getCharSequenceExtra("encode_type");
				String strLevel = (String) data.getCharSequenceExtra("level");
				String strFrameRate = (String) data.getCharSequenceExtra("frame_rate");
				String strIframe = (String) data.getCharSequenceExtra("Iframe_interval");

				int channel = videoConfigGroup.getCheckedRadioButtonId() - 1;
				encodeinfos[0] = Integer.valueOf(strVideoformat);
				encodeinfos[1] = Integer.valueOf(strResolution);
				encodeinfos[2] = Integer.valueOf(strBitrate);
				encodeinfos[3] = Integer.valueOf(strLevel);
				encodeinfos[4] = Integer.valueOf(strFrameRate);
				encodeinfos[5] = Integer.valueOf(strIframe);
				encodeinfos[8] = Integer.valueOf(strEncodeType);

				NetplayerAPI.setVideoEncode(userid, channel, encodeinfos);// ,pBuffer);
			}
		}
	}

	protected void onDestroy()
	{
		super.onDestroy();
		
		CancelTimerTask();
		
		NetplayerAPI.userLogout(userid);
		NetplayerAPI.deinitServerInfo(userid);
		Log.i(TAG, "channel onDestroy called");
		System.gc();
	}
}
