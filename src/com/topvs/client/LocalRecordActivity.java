package com.topvs.client;

import java.io.File;

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;
import android.widget.MediaController;
import android.widget.VideoView;

public class LocalRecordActivity extends Activity {
	
	private VideoView video1;  
    MediaController  mediaco; 
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
//		setContentView(R.layout.lo)
		  setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);  
	         
	        /* 设置屏幕常亮 *//* flag：标记 ； */  
	        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);  
		setContentView(R.layout.localrecord);
		video1=(VideoView)findViewById(R.id.vv_localrecord);  
        mediaco=new MediaController(this); 
        String sDir = getApplicationContext().getFilesDir().getAbsolutePath();
		sDir += "/tuopu/record/";
		sDir += "112233";
		File destDir = new File(sDir);
		if (!destDir.exists()) {
			destDir.mkdirs();
			Log.i("down", sDir);
		}

		final String path = sDir + "/1.mp4";
        File file=new File(path);  
        if(file.exists()){  
        	System.out.println("找到文件");
            //VideoView与MediaController进行关联  
        	 video1.setVideoURI(Uri.parse("/mnt/sdcard/1.mp4"));  
            video1.setMediaController(mediaco);  
            mediaco.setMediaPlayer(video1); 
            video1.start();
            //让VideiView获取焦点  
            video1.requestFocus();  
            
        }  
	}
	
}
