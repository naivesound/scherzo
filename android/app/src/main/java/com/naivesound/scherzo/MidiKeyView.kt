package com.naivesound.scherzo

import android.content.Context
import android.view.MotionEvent
import android.view.View

class MidiKeyView(context: Context) : View(context) {

    override fun onTouchEvent(event: MotionEvent): Boolean {
        val note = tag as Int
        if (event.action == MotionEvent.ACTION_DOWN) {
            App.getService()?.noteOn(0, note, 127)
        }
        if (event.action == MotionEvent.ACTION_UP || event.action == MotionEvent.ACTION_CANCEL) {
            App.getService()?.noteOff(0, note)
        }
        return true
    }
}
