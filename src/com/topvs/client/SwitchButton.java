/**
 * 
 */
package com.topvs.client;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;

/**
 * @author LinZh107
 *	2015-6-4
 */
public class SwitchButton extends View implements OnClickListener, OnTouchListener
{
	private Bitmap mSwitchBottom, mSwitchThumb, mSwitchFrame, mSwitchMask;
	private Rect mSrcRect = null;	//the view's SRC file Rect
	private Rect mDestRect = null;
	private boolean mSwitchOn = true;	//Switch is on by default.
	private float mCurrentX = 0;	//current touched x position
	private float mLastX = 0;	//last touched x position
	private int mMoveLength;	//Max move length of the touched move even
		
	public SwitchButton(Context context)
	{
		super(context);
		// TODO Auto-generated constructor stub
	}

	public SwitchButton(Context context, AttributeSet attrs)
	{
		super(context, attrs);
		// TODO Auto-generated constructor stub
	}

	public SwitchButton(Context context, AttributeSet attrs, int defStyleAttr)
	{
		super(context, attrs, defStyleAttr);
		// TODO Auto-generated constructor stub
	}

	/* (non-Javadoc)
	 * @see android.view.View.OnTouchListener#onTouch(android.view.View, android.view.MotionEvent)
	 */
	@Override
	public boolean onTouch(View v, MotionEvent event)
	{
		// TODO Auto-generated method stub
		return false;
	}

	/* (non-Javadoc)
	 * @see android.view.View.OnClickListener#onClick(android.view.View)
	 */
	@Override
	public void onClick(View v)
	{
		// TODO Auto-generated method stub

	}

}
