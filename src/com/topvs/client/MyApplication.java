package com.topvs.client;

import java.util.Locale;

import android.app.Application;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.util.DisplayMetrics;

public class MyApplication extends Application {
	@Override
	public void onCreate() {
		super.onCreate();
		Resources res = getResources();
		Configuration config = res.getConfiguration();
		if (!"zh_CN".equals(config.locale.toString())  ){
			config.locale =  Locale.ENGLISH;
		}
		DisplayMetrics dm = res.getDisplayMetrics(); 
		res.updateConfiguration(config, dm); 
	}
}
