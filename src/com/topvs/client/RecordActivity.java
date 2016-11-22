package com.topvs.client;

import java.io.File;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;

import com.topvs.client.view.CBProgressBar;

import android.app.Activity;
import android.app.DatePickerDialog;
import android.app.DatePickerDialog.OnDateSetListener;
import android.app.TimePickerDialog;
import android.app.TimePickerDialog.OnTimeSetListener;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.StatFs;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.DatePicker;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.TimePicker;
import android.widget.Toast;

public class RecordActivity extends Activity {

	protected static final int SELECT = 1;
	private Context mContext;
	private int startArr[] = { 0, 0, 0, 0, 0, 0 };
	private int endArr[] = { 0, 0, 0, 0, 0, 0 };
	private boolean selectStart = false;
	private boolean selectEnd = false;
	private boolean isfirststart = true;
	private boolean isfirstend = true;
	private TextView tv_record_start;
	private TextView tv_record_end;
	private CBProgressBar cbp_download;
	private LinearLayout ll_record_download;
	private int user_id;
	public String[] RecordName = new String[2000];
	private int count;
	private int openchannel;
	private int selectedIndexs;
	private int i = 0;
	private int TIME = 1000;
	private String[] progress = new String[2];
	private boolean downfirst = true;
	Handler handler = new Handler() {
		

		public void handleMessage(Message msg) {
			if (msg.what == 1) {
				NetplayerAPI.getDownloadProgress(progress);
				System.out.println("已下载:" + progress[0] + "总共" + progress[1]);
				int all = Integer.parseInt(progress[1])/1024;
				if(downfirst)
				{
					cbp_download.setMax(all);
					downfirst = false;
				}
				int current = Integer.parseInt(progress[0])/1024;
				System.out.println("current:"+current);
				cbp_download.setProgress(current);
				if (all/(all-current) == 99) {
					task.cancel();
					ll_record_download.setVisibility(View.INVISIBLE);
					NetplayerAPI.cancleDownload(user_id);
					Toast.makeText(mContext, "下载完成", Toast.LENGTH_SHORT).show();
				}

			}
			super.handleMessage(msg);
		};
	};
	Timer timer = new Timer();
	TimerTask task = new TimerTask() {

		@Override
		public void run() {
			// 需要做的事:发送消息
			Message message = new Message();
			message.what = 1;
			handler.sendMessage(message);
		}
	};
	

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		setContentView(R.layout.activity_record);
		mContext = this;

		Bundle extras = getIntent().getExtras();
		user_id = extras.getInt("USER_ID");
		openchannel = extras.getInt("CHANNAL");
		int stream_type = extras.getInt("STREAM_TYPE");
		int channel_type = extras.getInt("CHANNEL_TYPE");
		// NetplayerAPI.stopChannelStream(user_id, openchannel, stream_type,
		// channel_type);
		// NetplayerAPI.closeChannelStream(user_id, openchannel, channel_type);
		Button btn_record_start = (Button) findViewById(R.id.btn_record_start);
		Button btn_record_end = (Button) findViewById(R.id.btn_record_end);
		Button btn_record_search = (Button) findViewById(R.id.btn_record_search);
		tv_record_start = (TextView) findViewById(R.id.tv_record_start);
		tv_record_end = (TextView) findViewById(R.id.tv_record_end);
		cbp_download = (CBProgressBar) findViewById(R.id.cbp_download);
		ll_record_download = (LinearLayout) findViewById(R.id.ll_record_download);
		// findViewById(R.id.lv)
		final ListView lv_record_info = (ListView) findViewById(R.id.lv_record_info);
		lv_record_info.setOnItemClickListener(new OnItemClickListener() {

			@Override
			public void onItemClick(AdapterView<?> parent, View view,
					int position, long id) {
				// TODO Auto-generated method stub
				Intent intent = new Intent(mContext, RecordPlayActivity.class);
				intent.putExtra("index", position);
				intent.putExtra("user_id", user_id);
				startActivity(intent);

			}
		});
		lv_record_info
				.setOnItemLongClickListener(new OnItemLongClickListener() {

					@Override
					public boolean onItemLongClick(AdapterView<?> parent,
							View view, int position, long id) {
						selectedIndexs = position;
						Intent intent = new Intent(mContext,
								SelectPicPopupWindow.class);
						startActivityForResult(intent, SELECT);
						return true;
					}
				});
		btn_record_start.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				isfirststart = true;
				Calendar c = Calendar.getInstance();
				new DatePickerDialog(mContext, new OnDateSetListener() {
					private boolean first = true;

					@Override
					public void onDateSet(DatePicker view, int year,
							int monthOfYear, int dayOfMonth) {
						startArr[0] = year;
						startArr[1] = monthOfYear + 1;
						startArr[2] = dayOfMonth;
						tv_record_start.setText("" + year + "-"
								+ (monthOfYear + 1) + "-" + dayOfMonth);
						if (year > 0) {
							if (first) {
								first = false;
								showTimePick();
							}

						}
					}

					private void showTimePick() {
						Calendar c = Calendar.getInstance();
						new TimePickerDialog(mContext, new OnTimeSetListener() {
							@Override
							public void onTimeSet(TimePicker view,
									int hourOfDay, int minute) {
								if (isfirststart) {
									;
									isfirststart = false;
									selectStart = true;
									first = true;
									startArr[3] = hourOfDay;
									startArr[4] = minute;
									tv_record_start.setText(tv_record_start
											.getText().toString()
											+ " "
											+ hourOfDay + ":" + minute);
								}

							}
						}, c.get(Calendar.HOUR_OF_DAY), c.get(Calendar.MINUTE),
								true).show();

					}
				}, c.get(Calendar.YEAR), c.get(Calendar.MONTH), c
						.get(Calendar.DAY_OF_MONTH)).show();
			}
		});
		btn_record_end.setOnClickListener(new OnClickListener() {

//			private boolean first = true;

			@Override
			public void onClick(View v) {
				isfirstend = true;
				Calendar c = Calendar.getInstance();
				new DatePickerDialog(mContext, new OnDateSetListener() {
					private boolean first = true;
					@Override
					public void onDateSet(DatePicker view, int year,
							int monthOfYear, int dayOfMonth) {
						endArr[0] = year;
						endArr[1] = monthOfYear + 1;
						endArr[2] = dayOfMonth;
						tv_record_end.setText("" + year + "-"
								+ (monthOfYear + 1) + "-" + dayOfMonth);
						if (year > 0) {
							if (first) {
								first = false;
								System.out.println("弹两次?");
								showTimePick();
							}
						}

					}

					private void showTimePick() {
						Calendar c = Calendar.getInstance();
						new TimePickerDialog(mContext, new OnTimeSetListener() {
							@Override
							public void onTimeSet(TimePicker view,
									int hourOfDay, int minute) {
								if (isfirstend) {
									;
									isfirstend = false;
									selectEnd = true;
									first = true;
									endArr[3] = hourOfDay;
									endArr[4] = minute;
									tv_record_end.setText(tv_record_end
											.getText().toString()
											+ " "
											+ hourOfDay + ":" + minute);
								}

							}
						}, c.get(Calendar.HOUR_OF_DAY), c.get(Calendar.MINUTE),
								true).show();
					}
				}, c.get(Calendar.YEAR), c.get(Calendar.MONTH), c
						.get(Calendar.DAY_OF_MONTH)).show();
			}
		});
		btn_record_search.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				if (!selectStart) {
					Toast.makeText(mContext, "请选择开始日期", Toast.LENGTH_SHORT)
							.show();
					return;
				}
				if (!selectEnd) {
					Toast.makeText(mContext, "请选择结束日期", Toast.LENGTH_SHORT)
							.show();
					return;
				}
				System.out.println("开始时间戳:"
						+ getTime(tv_record_start.getText().toString()));
				System.out.println("结束时间戳:"
						+ getTime(tv_record_end.getText().toString()));
				if (getTime(tv_record_end.getText().toString()) > getTime(tv_record_start
						.getText().toString())) {
					RecordName = new String[2000];
					int ret = NetplayerAPI.getRecordInfo(user_id, openchannel,
							startArr, endArr, RecordName);
					if (ret > 0) {
						getCount();
						lv_record_info.setAdapter(new recordAdapter());
					}
				} else {
					Toast.makeText(mContext, "结束日期必须大于开始日期", Toast.LENGTH_SHORT)
							.show();
					return;
				}

			}

			private void getCount() {
				count = 0;
				int i;
				for (i = 0; i < RecordName.length; i++) {
					String name = RecordName[i];
					if (name == null || name.length() < 1) {
						count = i;
						break;
					}
				}

			}
		});
	}

	class recordAdapter extends BaseAdapter {

		@Override
		public int getCount() {
			// TODO Auto-generated method stub
			return count;
		}

		@Override
		public Object getItem(int position) {
			// TODO Auto-generated method stub
			return RecordName[position];
		}

		@Override
		public long getItemId(int position) {
			// TODO Auto-generated method stub
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			View v;
			if (convertView == null) {
				v = View.inflate(mContext, R.layout.list_record, null);
			} else {
				v = convertView;
			}
			TextView tv_list_record = (TextView) v
					.findViewById(R.id.tv_list_record);
			tv_list_record.setText(RecordName[position]);
			return v;
		}

	}

	public static long getTime(String user_time) {
		long l = 0;
		SimpleDateFormat sdf = new SimpleDateFormat("yyyy-M-d H:m");
		Date d;
		try {
			d = sdf.parse(user_time);
			l = d.getTime();
		} catch (ParseException e) {
			// TODO Auto-generated catch block e.printStackTrace();
		}
		return l;
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == SELECT && resultCode == 1) {
			Bundle extras = data.getExtras();
			int result = extras.getInt("result");
			switch (result) {
			case 1:// 下载
				readSystem();
				readSDCard();
				startRecordVideo();
				timer.schedule(task, 3000, 1000);
				ll_record_download.setVisibility(View.VISIBLE);
				break;
			case 2:// 删除本地录像
				deleteLocalRecord();
				break;
			case 3:// 删除远程录像
				int ret = NetplayerAPI.deleteServerRecord(user_id,
						selectedIndexs);
				String msg = null;
				if (ret == 0) {
					msg = "删除成功";
				} else {
					msg = "删除失败";
				}
				Toast.makeText(mContext, msg, Toast.LENGTH_SHORT).show();
				break;
			case 4:

				break;
			default:
				break;
			}
		}
	}

	private void readSystem() {
		File root = Environment.getRootDirectory();
		StatFs sf = new StatFs(root.getPath());
		long blockSize = sf.getBlockSize();
		long blockCount = sf.getBlockCount();
		long availCount = sf.getAvailableBlocks();
		Log.d("", "block大小:" + blockSize + ",block数目:" + blockCount + ",总大小:"
				+ blockSize * blockCount / (1024 * 1024) + "MB");
		Log.d("", "可用的block数目：:" + availCount + ",可用大小:" + availCount
				* blockSize / (1024 * 1024) + "MB");
	}

	private void readSDCard() {
		String state = Environment.getExternalStorageState();
		if (Environment.MEDIA_MOUNTED.equals(state)) {
			File sdcardDir = Environment.getExternalStorageDirectory();
			StatFs sf = new StatFs(sdcardDir.getPath());
			long blockSize = sf.getBlockSize();
			long blockCount = sf.getBlockCount();
			long availCount = sf.getAvailableBlocks();
			Log.d("", "block大小:" + blockSize + ",block数目:" + blockCount
					+ ",总大小:" + blockSize * blockCount / (1024 * 1024) + "MB");
			Log.d("", "可用的block数目：:" + availCount + ",剩余空间:" + availCount
					* blockSize / (1024 * 1024) + "MB");
		}
	}

	private int startRecordVideo() {
		String sDir = getApplicationContext().getFilesDir().getAbsolutePath();
		sDir += "/tuopu/record/";
		sDir += "112233";
		File destDir = new File(sDir);
		if (!destDir.exists()) {
			destDir.mkdirs();
			Log.i("down", sDir);
		}
		String name = RecordName[selectedIndexs];
		final String path = sDir + "/" + name + ".avi";
		new Thread() {

			public void run() {
				NetplayerAPI.downLoadRecord(user_id, selectedIndexs, path);
			};
		}.start();
		return 0;
	}

	private String deleteLocalRecord() {
		String sDir = getApplicationContext().getFilesDir().getAbsolutePath();
		sDir += "/tuopu/record/";
		sDir += "112233";
		String name = RecordName[selectedIndexs];
		final String path = sDir + "/" + name + ".avi";
		File file = new File(path);
		if (file.exists()) {
			file.delete();
			Toast.makeText(mContext, "删除本地录像成功", Toast.LENGTH_SHORT).show();
		} else {
			Toast.makeText(mContext, "该录像不存在", Toast.LENGTH_SHORT).show();
		}
		file.exists();
		return null;
	}
	
	/**
	 * 取消下载
	 * @param v
	 */
	public void cancleDownload(View v)
	{
		task.cancel();
		ll_record_download.setVisibility(View.INVISIBLE);
		NetplayerAPI.cancleDownload(user_id);
		deleteLocalRecord();
		Toast.makeText(mContext, "下载取消", Toast.LENGTH_SHORT).show();
	}
}
