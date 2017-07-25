package com.naivesound.scherzo

import android.content.Context
import android.graphics.Rect
import android.util.DisplayMetrics
import android.util.SparseIntArray
import android.view.MotionEvent
import android.view.View

import java.util.HashSet

import trikita.anvil.DSL.*

import trikita.anvil.Anvil
import trikita.anvil.RenderableView

class PianoKeyboard(c: Context) : RenderableView(c) {

    private var mWidth: Int = 0
    private var mHeight: Int = 0

    private val mTouchedViews: MutableMap<View, MutableSet<Int>> = HashMap()
    private val mPrevX = SparseIntArray()
    private val mPrevY = SparseIntArray()

    private val mKeyCount = 12
    private val mFirstKey = 48

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        mWidth = View.MeasureSpec.getSize(widthMeasureSpec)
        mHeight = View.MeasureSpec.getSize(heightMeasureSpec)
        this.setMeasuredDimension(mWidth, mHeight)
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)
        Anvil.render()
    }

    override fun onInterceptTouchEvent(ev: MotionEvent): Boolean {
        return true
    }

    override fun onTouchEvent(ev: MotionEvent): Boolean {
        val action = ev.actionMasked
        when (action) {
            MotionEvent.ACTION_POINTER_DOWN, MotionEvent.ACTION_DOWN -> {
                val pointerId = ev.getPointerId(ev.actionIndex)
                mPrevX.put(pointerId, ev.getX(ev.actionIndex).toInt())
                mPrevY.put(pointerId, ev.getY(ev.actionIndex).toInt())
                dispatchTouchDown(findViewByPointer(ev, ev.actionIndex), ev)
            }
            MotionEvent.ACTION_OUTSIDE, MotionEvent.ACTION_CANCEL, MotionEvent.ACTION_POINTER_UP, MotionEvent.ACTION_UP -> run {
                val it = mTouchedViews.keys.iterator()
                while (it.hasNext()) {
                    val v = it.next()
                    if (dispatchTouchUp(v, ev)) {
                        it.remove()
                    }
                }
            }
            MotionEvent.ACTION_MOVE -> for (i in 0..ev.pointerCount - 1) {
                val id = ev.getPointerId(i)
                val x = ev.getX(i).toInt()
                val y = ev.getY(i).toInt()
                val minDistance = (24 * (resources.displayMetrics.densityDpi / DisplayMetrics.DENSITY_DEFAULT)).toInt()
                if (Math.abs(mPrevX.get(id, x) - x) > minDistance || Math.abs(mPrevY.get(id, y) - y) > minDistance) {
                    mPrevX.put(id, x)
                    mPrevY.put(id, y)
                } else {
                    continue
                }

                val newView = findViewByPointer(ev, i)
                if (newView != null) {
                    val it = mTouchedViews.keys.iterator()
                    while (it.hasNext()) {
                        val oldView = it.next()
                        if (oldView !== newView) {
                            if (dispatchTouchUp(oldView, ev)) {
                                it.remove()
                            }
                        }
                    }
                    dispatchTouchDown(newView, ev)
                }
            }
        }
        return true
    }

    private fun dispatchTouchDown(v: View?, ev: MotionEvent) {
        if (v != null) {
            if (mTouchedViews[v] == null) {
                val e = MotionEvent.obtain(ev.downTime, ev.eventTime,
                        MotionEvent.ACTION_DOWN,
                        ev.getX(ev.actionIndex), ev.getY(ev.actionIndex), 0)
                mTouchedViews.put(v, HashSet<Int>())
                v.dispatchTouchEvent(e)
            }
            mTouchedViews[v]!!.add(ev.getPointerId(ev.actionIndex))
        }
    }

    private fun dispatchTouchUp(v: View?, ev: MotionEvent): Boolean {
        val id = ev.getPointerId(ev.actionIndex)
        if (v != null && mTouchedViews[v] != null && mTouchedViews[v]!!.contains(id)) {
            mTouchedViews[v]!!.remove(id)
            if (mTouchedViews[v]!!.isEmpty()) {
                val e = MotionEvent.obtain(ev.downTime, ev.eventTime,
                        MotionEvent.ACTION_UP,
                        ev.getX(ev.actionIndex), ev.getY(ev.actionIndex), 0)
                v.dispatchTouchEvent(e)
                return true
            }
        }
        return false
    }

    private fun findViewByPointer(ev: MotionEvent, index: Int): View? {
        val x = (ev.getX(index) + x).toInt()
        val y = (ev.getY(index) + y).toInt()
        return findViewByPos(x, y)
    }

    private fun findViewByPos(x: Int, y: Int): View? {
        for (i in childCount - 1 downTo 0) {
            val v = getChildAt(i)
            val pos = IntArray(2)
            v.getLocationOnScreen(pos)
            val rect = Rect(pos[0], pos[1], pos[0] + v.width, pos[1] + v.height)
            if (rect.contains(x, y)) {
                return v
            }
        }
        return null
    }

    override fun view() {
        keys(false)
        keys(true)
    }

    private fun keys(black: Boolean) {
        var numWhiteKeys = 0
        for (i in 0..mKeyCount - 1) {
            if (!BLACK_KEYS[(mFirstKey + i) % 12]) {
                numWhiteKeys++
            }
        }
        val keyWidth = mWidth / numWhiteKeys
        var leftOffset = 0

        for (i in 0..mKeyCount - 1) {
            val index = (mFirstKey + i) % 12
            val offset = leftOffset
            val key = mFirstKey + i
            if (BLACK_KEYS[index] && black) {
                v(MidiKeyView::class.java, {
                    size(keyWidth - 2 * dip(3), mHeight / 2)
                    x((offset - keyWidth / 2).toFloat())
                    tag(key)
                    margin(dip(3))
                    backgroundColor(0xff333333.toInt())
                })
            }
            if (!BLACK_KEYS[index] && !black) {
                v(MidiKeyView::class.java, {
                    size(keyWidth - 2 * dip(3), FILL)
                    tag(key)
                    x(offset.toFloat())
                    margin(dip(3))
                    backgroundColor(0xffe1e1e1.toInt())
                })
            }
            if (!BLACK_KEYS[index]) {
                leftOffset = leftOffset + keyWidth
            }
        }
    }

    companion object {

        private val TAG = "PianoKeyboard"

        internal var BLACK_KEYS = booleanArrayOf(false, true, false, true, false, false, true, false, true, false, true, false)
    }
}
