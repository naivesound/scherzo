package com.naivesound.scherzo;

import android.content.Context;
import android.graphics.Rect;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

public class MidiKeyView extends View {

    private final static String TAG = "MidiKeyView";

    private boolean mTouchDown;
    private boolean mOutOfBounds;
    private Rect mRect;

    public MidiKeyView(Context context) {
        super(context);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        Log.d(TAG, "onTouchEvent(): event="+event.getAction()+" tag="+getTag());
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            mRect = new Rect(getLeft(), getTop(), getRight(), getBottom());
            Log.d(TAG, "DOWN note="+getTag());
            App.getService().noteOn(0, (Integer) getTag(), 127);
            mTouchDown = true;
        }
        if (event.getAction() == MotionEvent.ACTION_MOVE) {
            if (mOutOfBounds) {
                return true;
            }
            if (mRect != null && !mRect.contains(getLeft() + (int) event.getX(), getTop() + (int) event.getY())) {
                Log.d(TAG, "OUTSIDE note="+getTag());
                App.getService().noteOff(0, (Integer) getTag());
                mTouchDown = false;
                mOutOfBounds = true;
                return true;
            }
            if (!mTouchDown) {
                Log.d(TAG, "FIRST MOVE note="+getTag());
                App.getService().noteOn(0, (Integer) getTag(), 127);
                mTouchDown = true;
            }
        }
        if (event.getAction() == MotionEvent.ACTION_UP ||
                event.getAction() == MotionEvent.ACTION_CANCEL) {
            if (mOutOfBounds) {
                return true;
            }
            Log.d(TAG, "UP or CANCEL note="+getTag());
            App.getService().noteOff(0, (Integer) getTag());
            mTouchDown = false;
        }
        return true;
    }
}
