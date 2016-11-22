package com.topvs.client;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager.LayoutParams;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Toast;

public class SelectPicPopupWindow extends Activity implements OnClickListener{

	private Button btn_download, btn_deletelocal,btn_deleteservice,btn_cancel;  
    private LinearLayout layout;
	 
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.select_bottom);
		getWindow().setLayout(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT);
		 btn_cancel = (Button) findViewById(R.id.btn_cancel);
		 btn_download = (Button) findViewById(R.id.btn_download);
		 btn_deletelocal = (Button)findViewById(R.id.btn_deletelocal);
		 btn_deleteservice = (Button)findViewById(R.id.btn_deleteservice);
       layout=(LinearLayout)findViewById(R.id.pop_layout);  
       
     //添加选择窗口范围监听可以优先获取触点，即不再执行onTouchEvent()函数，点击其他地方时执行onTouchEvent()函数销毁Activity  
     layout.setOnClickListener(new OnClickListener() {  
           
         public void onClick(View v) {  
             // TODO Auto-generated method stub  
             
         }

		 
     });  
     //添加按钮监听  
     btn_cancel.setOnClickListener(this);  
     btn_deletelocal.setOnClickListener(this);
     btn_download.setOnClickListener(this);
     btn_deleteservice.setOnClickListener(this);
	}  
//    //实现onTouchEvent触屏函数但点击屏幕时销毁本Activity  
//    @Override  
//    public boolean onTouchEvent(MotionEvent event){  
//        finish();  
//        return true;  
//    }  
  
    public void onClick(View v) {  
    	 Intent intent = new Intent();
        int  result = 0;
        switch (v.getId()) {  
        case R.id.btn_download: 
        	result=1;
            break; 
        case R.id.btn_deletelocal:  
        	result=2;
            break; 
        case R.id.btn_deleteservice:  
        	result=3;
            break; 
        case R.id.btn_cancel:
        	result=0;
            break;  
        default:  
            break;  
        } 
        intent.putExtra("result", result);
        setResult(1, intent);
        finish();  
    }

	

}
