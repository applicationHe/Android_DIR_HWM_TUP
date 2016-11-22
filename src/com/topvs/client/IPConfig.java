package com.topvs.client;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.widget.TableRow;
import android.widget.Toast;

public class IPConfig extends Activity
{
	final static String TAG = "IPConfig";
	//public MyActivityManager mam = null;
	
	final int PROTOCOL_TCP = 0x01;
//	final int PROTOCOL_UDT = 0x02;
	final int PROTOCOL_P2P = 0x03;
	final int PROTOCOL_UDP = 0x04;
	final int PROTOCOL_MCAST = 0x05;
	
	TableRow tcp_ip;
	TableRow tcp_port;
	TableRow p2p_id;
	
//	EditText nameEdit;
	EditText ipEdit;
	EditText portEdit;
	EditText usrEdit;
	EditText pwdEdit;
	EditText p2pPuID;
	RadioGroup protocolGroup;
	RadioButton tcpChecked;
	RadioButton udtChecked;
	RadioButton p2pChecked;
	
//	String nameValue;
	String userValue;
	String pwdValue;
	String addrValue;
	String portValue;
	String puidValue;	
	String strTmp;
	
	int protocol;
	int isModify;
	int iID;

	// String str = null;

	@Override
	public void onCreate(Bundle icicle)
	{
		super.onCreate(icicle);
		//mam = MyActivityManager.getInstance();
		//mam.pushOneActivity(IPConfig.this);
		
		setContentView(R.layout.ipconfig);
		// 2014-04-01
		tcp_ip = (TableRow) findViewById(R.id.layout_tcp_ip);
		tcp_port = (TableRow) findViewById(R.id.layout_tcp_port);
		p2p_id = (TableRow) findViewById(R.id.layout_p2pid);
		
//		nameEdit = (EditText) findViewById(R.id.configname);
		ipEdit = (EditText) findViewById(R.id.configip);
		portEdit = (EditText) findViewById(R.id.configport);
		usrEdit = (EditText) findViewById(R.id.configusr);
		pwdEdit = (EditText) findViewById(R.id.configpwd);
		p2pPuID = (EditText) findViewById(R.id.configpu_id);
		
		tcp_ip.setVisibility(View.GONE);
		tcp_port.setVisibility(View.GONE);
		p2pPuID.requestFocus();
		
		protocolGroup = (RadioGroup) findViewById(R.id.Radio_Tcp_P2p);
		tcpChecked = (RadioButton) findViewById(R.id.Radio_Tcp);
		p2pChecked = (RadioButton) findViewById(R.id.Radio_P2p);
		
		p2pChecked.setChecked(true);
		protocol = PROTOCOL_P2P;
		
		Bundle extras = getIntent().getExtras();
		isModify = extras.getInt("IsModify");
		if (isModify == 1)
		{
			// 2014-04-01
			/* 测试不可打印字符和String之间的转换 和 String类对其的支持，如String.length() 是否能正常获取长度等。 */
			// char arr[] = new char[8];
			// arr[0] = 0x55; //同步头
			// arr[1] = 0xAA; //同上
			// arr[2] = 0x05; //长度= 7 + 2 * N
			// arr[3] = 0x07; //命令
			// arr[4] = 0xA0; //采集终端ID 高8bit
			// arr[5] = 0x01; //采集终端ID 低8bit
			// arr[6] = 0x00; //附件A(只有当0xSS = 0时数据才OK)
			// arr[7] = 0xFE; //传感器状态: bit7:空气温度; bit6:空气湿度; bit5: 土壤温度;
			// bit4:土壤湿度;bit3:光照度; bit2: 二氧化碳浓度:bit1: 氨气浓度;bit0:保留 1:错误; 0:NG
			// str = new String(arr);
			// int length = str.length();
			// nameEdit.setText(str);
			/* 结果是String.length 完全可用 */


			iID = extras.getInt("ID");
			//nameValue = extras.getString("NAME");
			userValue = extras.getString("USER");
			pwdValue = extras.getString("PWD");
			addrValue = extras.getString("IP");
			portValue = String.valueOf(extras.getInt("PORT"));
			
			puidValue = extras.getString("PUID");
			protocol =  extras.getInt("PROTOCOL");
			
			switch(protocol)
			{
				case PROTOCOL_TCP:
				{
					tcpChecked.setChecked(true);
					tcp_ip.setVisibility(View.VISIBLE);
					tcp_port.setVisibility(View.VISIBLE);
					p2p_id.setVisibility(View.GONE);
					ipEdit.setText(addrValue);
					portEdit.setText(portValue);
					ipEdit.requestFocus();
					break;					
				}					
				case PROTOCOL_P2P:
				{
					p2pChecked.setChecked(true);
					tcp_ip.setVisibility(View.GONE);
					tcp_port.setVisibility(View.GONE);
					p2p_id.setVisibility(View.VISIBLE);			
					p2pPuID.setText(puidValue); //add.
					p2pPuID.requestFocus();
					break;
				}
				default:
					break;
			}
			
			//nameEdit.setText(nameValue);
			usrEdit.setText(userValue);
			pwdEdit.setText(pwdValue);			
		}

		Button btn_save = (Button) findViewById(R.id.button_save);
		btn_save.setOnClickListener(listener);
		Button btn_exit = (Button) findViewById(R.id.button_exit);
		btn_exit.setOnClickListener(listener);
		
		protocolGroup.setOnCheckedChangeListener(new OnCheckedChangeListener()
		{
			@Override
			public void onCheckedChanged(RadioGroup group, int checkedId)
			{
				// TODO Auto-generated method stub
				if(R.id.Radio_Tcp == checkedId)
				{
					protocol = PROTOCOL_TCP;
					tcpChecked.setChecked(true);
					tcp_ip.setVisibility(View.VISIBLE);
					tcp_port.setVisibility(View.VISIBLE);
					p2p_id.setVisibility(View.GONE);
					ipEdit.requestFocus();
					portEdit.setText("5000");
				}
				else if(R.id.Radio_P2p == checkedId)
				{
					protocol = PROTOCOL_P2P;
					p2pChecked.setChecked(true);
					tcp_ip.setVisibility(View.GONE);
					tcp_port.setVisibility(View.GONE);
					p2p_id.setVisibility(View.VISIBLE);
					p2pPuID.requestFocus();
				}
			}
		});
	}//end Create.

	OnClickListener listener = new OnClickListener()
	{
		public void onClick(View v)
		{
			if (v.getId() == R.id.button_save)
			{
				Bundle bundle = new Bundle();
				bundle.putInt("IsModify", isModify);

				// 2014-7-29 
//				nameValue = nameEdit.getText().toString();
				userValue = usrEdit.getText().toString();
				pwdValue =  pwdEdit.getText().toString();
				
				bundle.putInt("ID", iID);
//				bundle.putString("NAME", nameValue);
				bundle.putString("USER", userValue);
				bundle.putString("PWD", pwdValue);	
				
				if(protocol == PROTOCOL_P2P)
				{
					puidValue = p2pPuID.getText().toString();
					if(puidValue.length() == 0 || puidValue.equals("null"))
						p2pPuID.setText("null");
					else
					{
						bundle.putInt("PROTOCOL", protocol);
						bundle.putString("PUID", puidValue);
						
						// tcp/udp info don't needed anymore
						addrValue = ipEdit.getText().toString();
						if(addrValue.length() == 0 )
							bundle.putString("IP", "192.168.0.100");
						else
							bundle.putString("IP", addrValue);						
						bundle.putString("PORT", "5000");
						
						Intent mIntent = new Intent();
						mIntent.putExtras(bundle);
						setResult(RESULT_OK, mIntent);
						finish();
						//mam.popOneActivity(mam.getLastActivity());
					}
				}
				else
				{					
					addrValue = ipEdit.getText().toString();
					portValue = portEdit.getText().toString();
					if(addrValue.length() == 0 || addrValue.equals("null"))
						ipEdit.setText("null");
					else if(portValue.length() == 0 || portValue.equals("null"))
						portEdit.setText("null");
					else
					{
						bundle.putString("IP", addrValue);
						bundle.putString("PORT", portValue);
						bundle.putInt("PROTOCOL", protocol);
						
						// p2p info don't needed anymore
						bundle.putString("PUID", "123456");
						
						Intent mIntent = new Intent();
						mIntent.putExtras(bundle);
						setResult(RESULT_OK, mIntent);
						finish();
						//mam.popOneActivity(mam.getLastActivity());
					}
				}				
			}
			else if (v.getId() == R.id.button_exit)
			{
				finish();
				//mam.popOneActivity(mam.getLastActivity());
			}
		}
	};
	

}
