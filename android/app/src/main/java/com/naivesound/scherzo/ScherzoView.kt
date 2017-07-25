package com.naivesound.scherzo

import android.content.Context
import android.graphics.Typeface
import android.util.DisplayMetrics
import android.util.Log
import android.view.MotionEvent
import android.view.WindowManager
import android.widget.LinearLayout

import trikita.anvil.RenderableView
import trikita.anvil.DSL.*

class ScherzoView(context: Context) : RenderableView(context) {

    private val TAG = "ScherzoView"

    private val mWidth: Int
    private val mHeight: Int

    init {
        val wm = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
        val display = wm.defaultDisplay
        val metrics = DisplayMetrics()
        display.getMetrics(metrics)
        mWidth = metrics.widthPixels
        mHeight = metrics.heightPixels
    }

    override fun view() {
        linearLayout {
            size(FILL, FILL)
            orientation(LinearLayout.VERTICAL)

            buttons()
            knobs()
            v(PianoKeyboard::class.java, {
                size(FILL, 0)
                weight(2f)
            })
        }
    }

    private fun buttons() {
        linearLayout {
            size(FILL, 0)
            weight(1f)
            padding(dip(15))
            gravity(CENTER_VERTICAL)

            button {
                size(0, FILL)
                weight(1f)
                margin(dip(10), 0)
                text("TAP")
                onTouch { _, e ->
                    if (e.actionMasked == MotionEvent.ACTION_DOWN) {
                        Log.d(TAG, "TAP click")
                        App.getService()?.tap()
                    }
                    false
                }
            }

            button {
                size(0, FILL)
                weight(1f)
                margin(dip(10), 0)
                text("LOOP")
                onTouch { _, e ->
                    if (e.actionMasked == MotionEvent.ACTION_DOWN) {
                        Log.d(TAG, "LOOP click")
                        App.getService()?.loop()
                    }
                    false
                }
            }

            button {
                size(0, FILL)
                weight(1f)
                margin(dip(10), 0)
                text("CANCEL")
                onTouch { _, e ->
                    if (e.actionMasked == MotionEvent.ACTION_DOWN) {
                        Log.d(TAG, "CANCEL click")
                        App.getService()?.cancelLoop()
                    }
                    false
                }

            }
        }
    }

    private fun knobs() {
        linearLayout {
            size(FILL, 0)
            weight(1f)
            gravity(CENTER_VERTICAL)

            linearLayout {
                size(0, FILL)
                weight(1f)
                padding(dip(10), dip(5))
                orientation(LinearLayout.VERTICAL)

                textView {
                    size(WRAP, WRAP)
                    text("Volume")
                    textSize(sip(18).toFloat())
                    typeface(Typeface.DEFAULT_BOLD)
                }

                seekBar {
                    size(FILL, WRAP)
                    padding(dip(15), dip(10))
                    onSeekBarChange({ _, progress, _ -> Log.d(TAG, "VOLUME: progress=" + progress) })
                }
            }

            linearLayout {
                size(0, FILL)
                weight(1f)
                padding(dip(10), dip(5))
                orientation(LinearLayout.VERTICAL)

                textView {
                    size(WRAP, WRAP)
                    text("Metronome")
                    textSize(sip(18).toFloat())
                    typeface(Typeface.DEFAULT_BOLD)
                }

                seekBar {
                    size(FILL, WRAP)
                    padding(dip(15), dip(10))
                    onSeekBarChange({ _, progress, _ -> Log.d(TAG, "METRONOME: progress=" + progress) })
                }
            }

            linearLayout {
                size(0, FILL)
                weight(1f)
                padding(dip(10), dip(5))
                orientation(LinearLayout.VERTICAL)

                textView {
                    size(WRAP, WRAP)
                    text("Slider #3")
                    textSize(sip(18).toFloat())
                    typeface(Typeface.DEFAULT_BOLD)
                }

                seekBar {
                    size(FILL, WRAP)
                    padding(dip(15), dip(10))
                    onSeekBarChange({ _, progress, _ -> Log.d(TAG, "SLIDER3: progress=" + progress) })
                }
            }

            linearLayout {
                size(0, FILL)
                weight(1f)
                padding(dip(10), dip(5))
                orientation(LinearLayout.VERTICAL)

                textView {
                    size(WRAP, WRAP)
                    text("Slider #4")
                    textSize(sip(18).toFloat())
                    typeface(Typeface.DEFAULT_BOLD)
                }

                seekBar {
                    size(FILL, WRAP)
                    padding(dip(15), dip(10))
                    onSeekBarChange({ _, progress, _ -> Log.d(TAG, "SLIDER4: progress=" + progress) })
                }
            }
        }
    }
}
