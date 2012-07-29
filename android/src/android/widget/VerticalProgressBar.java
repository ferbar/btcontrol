package android.widget;

import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.widget.ProgressBar;

public class VerticalProgressBar extends ProgressBar {

	// private OnProgressBarChangeListener myListener;

	public VerticalProgressBar(Context context) {
		super(context);
	}

	public VerticalProgressBar(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
	}

	public VerticalProgressBar(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		super.onSizeChanged(h, w, oldh, oldw);
	}

	@Override
	protected synchronized void onMeasure(int widthMeasureSpec,
			int heightMeasureSpec) {
		super.onMeasure(heightMeasureSpec, widthMeasureSpec);
		setMeasuredDimension(getMeasuredHeight(), getMeasuredWidth());
	}

	/*
	@Override
	public void setOnProgressBarChangeListener(OnSeekBarChangeListener mListener) {
		this.myListener = mListener;
	}
*/
	@Override
	protected void onDraw(Canvas c) {
		c.rotate(-90);
		c.translate(-getHeight(), 0);

		super.onDraw(c);
	}
	
	@Override
	public void setProgress(int p) {
		super.setProgress(p);
	}
	
	/*
	@Override
	public boolean onTouchEvent(MotionEvent event) {
		if (!isEnabled()) {
			return false;
		}

		switch (event.getAction()) {
		case MotionEvent.ACTION_DOWN:
			if (myListener != null)
				myListener.onStartTrackingTouch(this);
			break;
		case MotionEvent.ACTION_MOVE:
			setProgress(getMax()
					- (int) (getMax() * event.getY() / getHeight()));
			onSizeChanged(getWidth(), getHeight(), 0, 0);
			myListener.onProgressChanged(this, getMax()
					- (int) (getMax() * event.getY() / getHeight()), true);
			break;
		case MotionEvent.ACTION_UP:
			myListener.onStopTrackingTouch(this);
			break;

		case MotionEvent.ACTION_CANCEL:
			break;
		}
		return true;
	}
	*/
}