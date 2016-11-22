package com.topvs.client;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.InputType;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.EditText;

public class VideoConfig extends Activity {
	
	public static final int SAVE_ID = Menu.FIRST;
	public static final int CANCEL_ID = Menu.FIRST + 1;
	
	private EditText etBright;
	private EditText etContrast;
	private EditText etHue;
	private EditText etSaturation;
	
	 @Override
	    public void onCreate(Bundle savedInstanceState) {		 
	        super.onCreate(savedInstanceState);
	        setContentView(R.layout.videoconfig);	              
	        Bundle extras = getIntent().getExtras();
	        int brightness = extras.getInt("brightness");
	        int contrast = extras.getInt("contrast");
	        int hue = extras.getInt("hue");
	        int saturation = extras.getInt("saturation");
	        etBright = (EditText)findViewById(R.id.brightness);
	        etBright.setText(Integer.toString(brightness));
	        etBright.setInputType(InputType.TYPE_CLASS_NUMBER);  
	        etContrast = (EditText)findViewById(R.id.contrast);
	        etContrast.setText(Integer.toString(contrast));
	        etContrast.setInputType(InputType.TYPE_CLASS_NUMBER);  
	        etHue = (EditText)findViewById(R.id.hue);
	        etHue.setText(Integer.toString(hue));
	        etHue.setInputType(InputType.TYPE_CLASS_NUMBER);  
	        etSaturation = (EditText)findViewById(R.id.saturation);
	        etSaturation.setText(Integer.toString(saturation));
	        etSaturation.setInputType(InputType.TYPE_CLASS_NUMBER);  	        
	 }
	 
	 @Override 
		public boolean onCreateOptionsMenu(Menu menu) {
			super.onCreateOptionsMenu(menu);
			menu.add(0,SAVE_ID,0,  "…Ë÷√");
			menu.add(1,CANCEL_ID,1,"∑µªÿ");
			return true;
			}
		
		@Override
		public boolean onOptionsItemSelected(MenuItem item)
		{ 
			switch (item.getItemId())
			{
			case SAVE_ID:
				Bundle bundle =	new Bundle();
				bundle.putString("brightness",etBright.getText().toString());
				bundle.putString("contrast",etContrast.getText().toString());
				bundle.putString("hue",etHue.getText().toString());
				bundle.putString("saturation",etSaturation.getText().toString());
				Intent	mIntent = new Intent();
				mIntent.putExtras(bundle);
				setResult(RESULT_OK, mIntent);
				finish();
				return true;
			case CANCEL_ID:
				finish();
				return true;
			}
			return super.onOptionsItemSelected(item);
		}
}
