package com.topvs.client;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;

public class ClientUI extends Activity
{
	private static final long SPLASH_DELAY_MILLIS = 800;
	//public MyActivityManager mam = null;
	
	@Override
	public void onCreate(Bundle icicle)
	{
		super.onCreate(icicle);
		//mam = MyActivityManager.getInstance();
		//mam.pushOneActivity(ClientUI.this);
		
		setContentView(R.layout.main);
		
		new Handler().postDelayed(new Runnable()
		{
			@Override
            public void run()
            {
	            // TODO Auto-generated method stub
	            Intent intent = new Intent(ClientUI.this, IPListActivity.class); 
	            ClientUI.this.startActivity(intent);
	            //mam.popOneActivity(mam.getLastActivity());
                ClientUI.this.finish();
            }			
		}, SPLASH_DELAY_MILLIS);
	}
	
}
