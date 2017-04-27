package com.naivesound.scherzo;

import android.content.Context;
import android.graphics.Rect;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

public class MidiKeyView extends View {

    private final static String TAG = "MidiKeyView";

    public MidiKeyView(Context context) {
        super(context);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int note = (Integer) getTag();
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            App.getService().noteOn(0, note, 127);
        }
        if (event.getAction() == MotionEvent.ACTION_UP ||
                event.getAction() == MotionEvent.ACTION_CANCEL) {
            App.getService().noteOff(0, note);
        }
        return true;
    }
}
