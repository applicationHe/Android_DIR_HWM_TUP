package com.topvs.client;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.InputType;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.EditText;
import android.widget.RadioGroup;
//import android.widget.Toast;

public class VideoEncode extends Activity {
	
	public static final int SAVE_ID = Menu.FIRST;
	public static final int CANCEL_ID = Menu.FIRST + 1;
	
	private int PAL=0;
	private int NTSC = 1;
	private RadioGroup standardType;
	private EditText etFrameRate;
	private EditText etIframe;
	private EditText etLevel;
	private RadioGroup grEncodeType;
	
	 @Override
	    public void onCreate(Bundle savedInstanceState) {		 
	        super.onCreate(savedInstanceState);
	        setContentView(R.layout.videoencode);	              
	        Bundle extras = getIntent().getExtras();			
	        int video_format = extras.getInt("video_format");
	        //int resolution = extras.getInt("resolution");
	        int encode_type = extras.getInt("encode_type");
	        int level = extras.getInt("level");
	        int frame_rate = extras.getInt("frame_rate");
	        int Iframe_interval = extras.getInt("Iframe_interval");
	        
	        standardType= (RadioGroup)findViewById(R.id.standardType);
	        if(video_format==PAL)
	        	standardType.check(R.id.standardPal);
	        else if(video_format==NTSC)
	        	standardType.check(R.id.standardNTSC);
	        etFrameRate =  (EditText)findViewById(R.id.frame_rate);
	        etFrameRate.setText(Integer.toString(frame_rate));	
	        etFrameRate.setInputType(InputType.TYPE_CLASS_NUMBER);  
	        etLevel =  (EditText)findViewById(R.id.level);
	        etLevel.setText(Integer.toString(level));
	        etLevel.setInputType(InputType.TYPE_CLASS_NUMBER);  
	        etIframe =  (EditText)findViewById(R.id.Iframe_interval);
	        etIframe.setText(Integer.toString(Iframe_interval));
	        etIframe.setInputType(InputType.TYPE_CLASS_NUMBER);  
	        
	        //0:复合流, 1:视频流,  2:音频流
	        grEncodeType= (RadioGroup)findViewById(R.id.encode_type);
	        if(encode_type==0)
	        	grEncodeType.check(R.id.encode_typeComplex);
	        else if(encode_type==1)
	        	grEncodeType.check(R.id.encode_typeVideo);
	        else if(encode_type==2)
	        	grEncodeType.check(R.id.encode_typeAudio);
	 }
	 
	 @Override 
		public boolean onCreateOptionsMenu(Menu menu) {
			super.onCreateOptionsMenu(menu);
			menu.add(0,SAVE_ID,0,  "设置");
			menu.add(1,CANCEL_ID,1,"返回");
			return true;
			}
		
		@Override
		public boolean onOptionsItemSelected(MenuItem item)
		{ 
			switch (item.getItemId())
			{
			case SAVE_ID:
				Bundle bundle =	new Bundle();
				 int TypeId= standardType.getCheckedRadioButtonId();
			     if(TypeId==R.id.standardPal)
			    	 bundle.putString("video_format",Integer.toString(PAL));
			     else if(TypeId==R.id.standardNTSC)
			    	 bundle.putString("video_format",Integer.toString(NTSC));
			     
			     TypeId= grEncodeType.getCheckedRadioButtonId();
			     if(TypeId==R.id.encode_typeComplex)
			    	 bundle.putString("encode_type",Integer.toString(0));
			     else if(TypeId==R.id.encode_typeVideo)
			    	 bundle.putString("encode_type",Integer.toString(1));
			     else if(TypeId==R.id.encode_typeAudio)
			    	 bundle.putString("encode_type",Integer.toString(2));
			     
				bundle.putString("frame_rate",etFrameRate.getText().toString());
				bundle.putString("level",etLevel.getText().toString());
				bundle.putString("Iframe_interval",etIframe.getText().toString());
				
				Intent	mIntent = new Intent();
				mIntent.putExtras(bundle);
				setResult(RESULT_OK, mIntent);
				finish();
				return true;
			case CANCEL_ID:
				Intent	mIntent2 = new Intent();
				setResult(RESULT_CANCELED, mIntent2);
				finish();
				Log.v("TAG","cancel item");
				return true;
			}
			return super.onOptionsItemSelected(item);
		}
}
