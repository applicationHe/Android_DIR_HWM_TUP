package com.topvs.client;

import android.app.Activity;
import android.content.Intent;
import android.content.res.Resources;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.AdapterView.OnItemClickListener;

public class IPListActivity extends Activity
{
	final static String TAG = "IPListActivity";
	//public MyActivityManager mam = null;
	
	final int PROTOCOL_TCP = 0x01;
//	final int PROTOCOL_UDT = 0x02;
	final int PROTOCOL_P2P = 0x03;
	final int PROTOCOL_UDP = 0x04;
	final int PROTOCOL_MCAST = 0x05;
	
	final static int LOGIN_ID = Menu.FIRST;
	final static int MODIFY_ID = Menu.FIRST + 2;
	final static int DEL_ID = Menu.FIRST + 3;
	final static int IPCONFIG_REQUEST = 0;
	
	final int ACK_ERROR_OTHER	= 0xff;	//其他错误
	final int ACK_SUCCESS		= 0x00;	//成功
	final int ACK_ERROR_NAME	= 0x01;	//用户名错误
	final int ACK_ERROR_PWD		= 0x02;	//密码错误
	final int ACK_ERROR_RIGHT	= 0x03;	//权限不够
	final int ACK_ERROR_CHANNEL	= 0x04;	//错误的通道号
	final int ACK_ERROR_PTZ_BUSY= 0x05;	//云台忙,暂时无法操作
	final int NET_EXCEPTION		= 0x06; //网络异常
	final int LOGIN_ERROR = -1;
	final int CHANNEL_ERROR = -2;
	final int INITSERVER_ERROR = -3;
	final int DELETE_ITERM = 1;
	final int LOGIN = 2;
	final int LOGOUT = 3;
	
	private Button btn_add;
	private Button btn_exit;
	private ListView list;
	
	private Handler mHandler = null;
	private DatabaseHelper dbHelper;
	private ListMoreAdapter listItemAdapter;
	private int selectedItem = -1;
	private int[] ids;
	private int camera_num;
	
	private String[] puids;
	private String[] users;
	
	public int userid;
	private int retLogin;
	private int channel_type = -1;//从、主码流
//	protected Thread p2p_thread;
	
	public enum iplist_data{ID, IP, USER, PWD, PORT, PROTOCOL, PUID};
	Resources rs;
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		//mam = MyActivityManager.getInstance();
		//mam.pushOneActivity(IPListActivity.this);
		
		setContentView(R.layout.iplist);
		rs = getResources();

		dbHelper = new DatabaseHelper(this);
		// 绑定Layout里面的ListView
		list = (ListView) findViewById(R.id.ipListView);

		//init and display
		initDatas();
		
		listItemAdapter = new ListMoreAdapter(this);
		list.setAdapter(listItemAdapter);

		btn_add = (Button) findViewById(R.id.button_add);
		btn_add.setOnClickListener(listener);
		btn_exit = (Button) findViewById(R.id.button_cancel);
		btn_exit.setOnClickListener(listener);

		mHandler = new Handler()
		{
			public void handleMessage(Message msg)
			{				
				switch (msg.what)
				{
					case INITSERVER_ERROR :
						Toast.makeText(IPListActivity.this, rs.getString(R.string.ipact_msg_initfailed), 
								Toast.LENGTH_SHORT).show();
						break;
					// case LOGIN_ERROR:
					case ACK_ERROR_OTHER :	// 其他错误
						Toast.makeText(IPListActivity.this, rs.getString(R.string.ipact_msg_other_error), 
								Toast.LENGTH_SHORT).show();
						break;
					case ACK_ERROR_NAME :	// 用户名错误
						Toast.makeText(IPListActivity.this, rs.getString(R.string.ipact_msg_error_name), 
								Toast.LENGTH_SHORT).show();
						break;
					case ACK_ERROR_PWD :		// 密码错误
						Toast.makeText(IPListActivity.this, rs.getString(R.string.ipact_msg_error_pwd), 
								Toast.LENGTH_SHORT).show();
						break;
					case ACK_ERROR_RIGHT :	// 权限不够
						Toast.makeText(IPListActivity.this, rs.getString(R.string.ipact_msg_error_right), 
								Toast.LENGTH_SHORT).show();
						break;
					case ACK_ERROR_CHANNEL :	// 错误的通道号
						Toast.makeText(IPListActivity.this, rs.getString(R.string.ipact_msg_error_channel), 
								Toast.LENGTH_SHORT).show();
						break;
					case LOGIN_ERROR :
					case NET_EXCEPTION : 	// 网络异常
						Toast.makeText(IPListActivity.this, rs.getString(R.string.ipact_msg_error_net), 
								Toast.LENGTH_SHORT).show();
						break;
					case CHANNEL_ERROR :
						Toast.makeText(IPListActivity.this, rs.getString(R.string.ipact_msg_getdevfailed),
						        Toast.LENGTH_SHORT).show();
						break;
					default :
						if (list != null) {
							initDatas();
							list.invalidate();
							list.setAdapter(listItemAdapter);
						}
						break;
				}
			}
		};

		list.setOnItemClickListener(new OnItemClickListener()
		{
			public void onItemClick(AdapterView<?> arg0, View arg1, int arg2, long arg3)
			{
				selectedItem = arg2;
				listItemAdapter.notifyDataSetChanged();
			}
		});
	}
	
	public void initDatas()
	{
		Cursor cursor = dbHelper.loadAll();
		camera_num = cursor.getCount();
		cursor.moveToFirst();
		if (camera_num > 0)
		{
			ids = new int[camera_num];
			puids = new String[camera_num];
			users = new String[camera_num];
		}
		for (int i = 0; i < camera_num; i++)
		{
			switch(cursor.getInt(iplist_data.PROTOCOL.ordinal()))
			{
				case PROTOCOL_TCP:
					puids[i] = cursor.getString(iplist_data.IP.ordinal());					
					break;
				case PROTOCOL_P2P:
					puids[i] = cursor.getString(iplist_data.PUID.ordinal());
					break;				
			}
			users[i] = cursor.getString(iplist_data.USER.ordinal());
			ids[i] = cursor.getInt(iplist_data.ID.ordinal());
			cursor.moveToNext();
		}
	}
	
	OnClickListener listener = new OnClickListener()
	{
		public void onClick(View v)
		{
			if (v.getId() == R.id.button_add)
			{
				Intent in = new Intent(IPListActivity.this, IPConfig.class);
				in.putExtra("IsModify", 0);
				startActivityForResult(in, IPCONFIG_REQUEST);
			}
			else if (v.getId() == R.id.button_cancel)
			{				
				//NetplayerAPI.deinitServerInfo(userid); //abandon ,fix the stack flow
				dbHelper.close();
				finish();
				//mam.popOneActivity(mam.getLastActivity());
			}
			if (v.getId() == LOGIN_ID)
			{
				if (selectedItem < 0)
					return;
				Cursor cursor = dbHelper.queryByID(ids[selectedItem]);
				if (cursor.getCount() <= 0)
					return;
				cursor.moveToFirst();
				String addr = cursor.getString(cursor.getColumnIndex("IP"));
				String user = cursor.getString(cursor.getColumnIndex("USER"));
				String pwd = cursor.getString(cursor.getColumnIndex("PWD"));
				String cuid = cursor.getString(cursor.getColumnIndex("PUID"));
				int port = cursor.getInt(cursor.getColumnIndex("PORT"));
				int protocol = cursor.getInt(cursor.getColumnIndex("PROTOCOL"));
				
				userid = NetplayerAPI.initServerInfo(addr, port, user, pwd, cuid, protocol);
				System.out.println("userid__:"+userid);
				if (userid == -1){					
					mHandler.sendEmptyMessage(INITSERVER_ERROR);
					return;
				}
				
				retLogin = NetplayerAPI.userLogin(userid);
				if (retLogin != 0){
					mHandler.sendEmptyMessage(retLogin);						
					return;
				}
				
				int num = NetplayerAPI.getChannelNum(userid);
				if (num <= 0){
					NetplayerAPI.userLogout(userid);
					mHandler.sendEmptyMessage(CHANNEL_ERROR);	
					return;
				}
				
				channel_type = NetplayerAPI.isSurportSlave(userid);// 如果支持双码流，则请求从通道(码流)。
				Log.i(TAG,"channel_type(0:主码流 , 1:支持从码流)=" + channel_type);
				
				Intent in = new Intent(IPListActivity.this, DeviceListActivity.class);
				in.putExtra("ChannelNum", num);
				in.putExtra("USERID", userid);
				in.putExtra("CHANNEL_TYPE", channel_type);
				startActivity(in);				
			}
			else if (v.getId() == MODIFY_ID)
			{
				if (selectedItem < 0)
					return;
				Intent in = new Intent(IPListActivity.this, IPConfig.class);
				in.putExtra("IsModify", 1);
				Cursor cursor = dbHelper.queryByID(ids[selectedItem]);
				if (cursor.getCount() <= 0)
					return;
				cursor.moveToFirst();
				Log.i(TAG, "queryByID  count" + cursor.getCount());
				in.putExtra("ID", cursor.getInt(cursor.getColumnIndex("ID")));
				// 2014.04.01
				//in.putExtra("NAME", cursor.getString(cursor.getColumnIndex("NAME")));
				in.putExtra("IP", cursor.getString(cursor.getColumnIndex("IP")));
				in.putExtra("PORT", cursor.getInt(cursor.getColumnIndex("PORT")));
				in.putExtra("USER", cursor.getString(cursor.getColumnIndex("USER")));
				in.putExtra("PWD", cursor.getString(cursor.getColumnIndex("PWD")));				
				in.putExtra("PUID", cursor.getString(cursor.getColumnIndex("PUID")));
				
				in.putExtra("PROTOCOL", cursor.getInt(cursor.getColumnIndex("PROTOCOL")));
				startActivityForResult(in, IPCONFIG_REQUEST);
			}
			else if (v.getId() == DEL_ID)
			{
				// menus
				final PopupWindow pop;
				LayoutInflater inflater;
				View layout;
				Button delete_yes;
				Button delete_no;
				Display display = getWindowManager().getDefaultDisplay();

				// add view by View.inflate
				inflater = (LayoutInflater) getSystemService(LAYOUT_INFLATER_SERVICE);
				// menus to display
				layout = inflater.inflate(R.layout.warn, null);

				pop = new PopupWindow(layout, display.getWidth(), 300);// get PopupWindow size
				pop.showAtLocation(v, Gravity.CENTER, 0, 0); // window's position
				pop.showAsDropDown(layout); //pop out menus

				delete_yes = (Button) layout.findViewById(R.id.delete_yes);
				delete_yes.setOnClickListener(new OnClickListener()
				{
					public void onClick(View v)
					{
						dbHelper.delete(ids[selectedItem]);
						selectedItem = -1;	
						mHandler.sendEmptyMessage(DELETE_ITERM);	
						Toast.makeText(IPListActivity.this, rs.getString(R.string.ipact_msg_deletesuc), Toast.LENGTH_LONG).show();
						pop.dismiss();
					}
				});

				delete_no = (Button) layout.findViewById(R.id.delete_no);
				delete_no.setOnClickListener(new OnClickListener()
				{
					public void onClick(View v)
					{
						pop.dismiss();
					}
				});
			}
		}
	};

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		if (requestCode == IPCONFIG_REQUEST)
		{
			if (resultCode == RESULT_OK)
			{
				// 2014-04-01 add the name
//				String strNAME = (String) data.getCharSequenceExtra("NAME");
//				String strNAME = "";
				String strIP = (String) data.getCharSequenceExtra("IP");
				String strPort = (String) data.getCharSequenceExtra("PORT");
				String strUser = (String) data.getCharSequenceExtra("USER");
				String strPwd = (String) data.getCharSequenceExtra("PWD");
				int Protocol = data.getIntExtra("PROTOCOL", 1);
				String strPUid = (String) data.getCharSequenceExtra("PUID");
				if (data.getIntExtra("IsModify", 0) == 1)
				{
					int id = data.getIntExtra("ID", -1);
					dbHelper.update(id, strIP, strUser, strPwd, Integer.parseInt(strPort), Protocol, strPUid);
				}
				else dbHelper.save(strIP, strUser, strPwd, Integer.parseInt(strPort), Protocol, strPUid);
				mHandler.sendMessage(mHandler.obtainMessage());
			}
		}
	}

	public class ListMoreAdapter extends BaseAdapter
	{
		Activity activity;
		LayoutInflater lInflater;

		public ListMoreAdapter(Activity a)
		{
			activity = a;
			lInflater = activity.getLayoutInflater();
		}

		public int getCount()
		{
			return camera_num;
		}

		public Object getItem(int position)
		{
			return null;
		}

		public long getItemId(int position)
		{
			return position;
		}

		public View getView(int position, View convertView, ViewGroup parent)
		{
			LinearLayout layout = new LinearLayout(activity);
			layout.setOrientation(LinearLayout.VERTICAL);
			layout.addView(addTitleView(position));

			if (selectedItem == position)
			{
				layout.addView(addCustomView(position));
			}
			return layout;
		}

		public View addTitleView(int i)
		{
			RelativeLayout layout = new RelativeLayout(activity);
			layout = (RelativeLayout) lInflater.inflate(R.layout.iplistitem, null);
			
			TextView tv = (TextView) layout.findViewById(R.id.ip_ItemTitle);
			tv.setText(puids[i]);
			
			TextView tv2 = (TextView) layout.findViewById(R.id.ip_ItemText);
			tv2.setText(users[i]);
			
			return layout;
		}

		public View addCustomView(int i)
		{
			LinearLayout layout = new LinearLayout(activity);
			layout.setOrientation(LinearLayout.HORIZONTAL);

			Button btnLogin = new Button(activity);
			btnLogin.setText(R.string.login_btn);
			btnLogin.setTextSize(16);
			btnLogin.setId(LOGIN_ID);
			btnLogin.setOnClickListener(listener);
			layout.addView(btnLogin, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
			        LinearLayout.LayoutParams.WRAP_CONTENT));

			Button btnModify = new Button(activity);
			btnModify.setText(R.string.modify_btn);
			btnModify.setTextSize(16);
			btnModify.setId(MODIFY_ID);
			btnModify.setOnClickListener(listener);
			layout.addView(btnModify, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
			        LinearLayout.LayoutParams.WRAP_CONTENT));

			Button btnDel = new Button(activity);
			btnDel.setText(R.string.del_btn);
			btnDel.setId(DEL_ID);
			btnDel.setOnClickListener(listener);
			btnDel.setTextSize(16);

			layout.addView(btnDel, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
			        LinearLayout.LayoutParams.WRAP_CONTENT));
			return layout;
		}
	}

}
