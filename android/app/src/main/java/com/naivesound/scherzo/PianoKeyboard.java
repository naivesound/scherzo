package com.naivesound.scherzo;

import android.content.Context;
import android.graphics.Point;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.SparseArray;
import android.util.SparseIntArray;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import static trikita.anvil.DSL.*;

import trikita.anvil.Anvil;
import trikita.anvil.RenderableView;

public class PianoKeyboard extends RenderableView {

    private static final String TAG = "PianoKeyboard";

    static boolean[] BLACK_KEYS = new boolean[]{
            false, true, false, true, false, false, true, false, true, false, true, false
    };

    private int mWidth;
    private int mHeight;

    private final Map<View, Set<Integer>> mTouchedViews = new HashMap<>();
    private final SparseIntArray mPrevX = new SparseIntArray();
    private final SparseIntArray mPrevY = new SparseIntArray();

    private int mKeyCount = 12;
    private int mFirstKey = 48;

    public PianoKeyboard(Context c) {
        super(c);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        mWidth = MeasureSpec.getSize(widthMeasureSpec);
        mHeight = MeasureSpec.getSize(heightMeasureSpec);
        this.setMeasuredDimension(mWidth, mHeight);
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        Anvil.render();
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        return true;
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        int action = ev.getActionMasked();
        switch (action) {
            case MotionEvent.ACTION_POINTER_DOWN:
            case MotionEvent.ACTION_DOWN:
                int pointerId = ev.getPointerId(ev.getActionIndex());
                mPrevX.put(pointerId, (int) ev.getX(ev.getActionIndex()));
                mPrevY.put(pointerId, (int) ev.getY(ev.getActionIndex()));
                dispatchTouchDown(findViewByPointer(ev, ev.getActionIndex()), ev);
                break;
            case MotionEvent.ACTION_OUTSIDE:
            case MotionEvent.ACTION_CANCEL:
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_UP:
                for (Iterator<View> it = mTouchedViews.keySet().iterator(); it.hasNext(); ) {
                    View v = it.next();
                    if (dispatchTouchUp(v, ev)) {
                        it.remove();
                    }
                }
                break;
            case MotionEvent.ACTION_MOVE:
                for (int i = 0; i < ev.getPointerCount(); i++) {
                    int id = ev.getPointerId(i);
                    int x = (int) ev.getX(i);
                    int y = (int) ev.getY(i);
                    int minDistance = (int) (24 * (getResources().getDisplayMetrics().densityDpi / DisplayMetrics.DENSITY_DEFAULT));
                    if (Math.abs(mPrevX.get(id, x) - x) > minDistance || Math.abs(mPrevY.get(id, y) - y) > minDistance) {
                        mPrevX.put(id, x);
                        mPrevY.put(id, y);
                    } else {
                        continue;
                    }

                    View newView = findViewByPointer(ev, i);
                    if (newView != null) {
                        for (Iterator<View> it = mTouchedViews.keySet().iterator(); it.hasNext(); ) {
                            View oldView = it.next();
                            if (oldView != newView) {
                                if (dispatchTouchUp(oldView, ev)) {
                                    it.remove();
                                }
                            }
                        }
                        dispatchTouchDown(newView, ev);
                    }
                }
                break;
        }
        return true;
    }

    private void dispatchTouchDown(View v, MotionEvent ev) {
        if (v != null) {
            if (mTouchedViews.get(v) == null) {
                MotionEvent e = MotionEvent.obtain(ev.getDownTime(), ev.getEventTime(),
                        MotionEvent.ACTION_DOWN,
                        ev.getX(ev.getActionIndex()), ev.getY(ev.getActionIndex()), 0);
                mTouchedViews.put(v, new HashSet<>());
                v.dispatchTouchEvent(e);
            }
            mTouchedViews.get(v).add(ev.getPointerId(ev.getActionIndex()));
        }
    }

    private boolean dispatchTouchUp(View v, MotionEvent ev) {
        int id = ev.getPointerId(ev.getActionIndex());
        if (v != null && mTouchedViews.get(v) != null && mTouchedViews.get(v).contains(id)) {
            mTouchedViews.get(v).remove(id);
            if (mTouchedViews.get(v).isEmpty()) {
                MotionEvent e = MotionEvent.obtain(ev.getDownTime(), ev.getEventTime(),
                        MotionEvent.ACTION_UP,
                        ev.getX(ev.getActionIndex()), ev.getY(ev.getActionIndex()), 0);
                v.dispatchTouchEvent(e);
                return true;
            }
        }
        return false;
    }

    private View findViewByPointer(MotionEvent ev, int index) {
        int x = (int) (ev.getX(index) + getX());
        int y = (int) (ev.getY(index) + getY());
        return findViewByPos(x, y);
    }

    private View findViewByPos(int x, int y) {
        for (int i = getChildCount() - 1; i >= 0; i--) {
            View v = getChildAt(i);
            int[] pos = new int[2];
            v.getLocationOnScreen(pos);
            Rect rect = new Rect(pos[0], pos[1], pos[0] + v.getWidth(), pos[1] + v.getHeight());
            if (rect.contains(x, y)) {
                return v;
            }
        }
        return null;
    }

    @Override
    public void view() {
        keys(false);
        keys(true);
    }

    private void keys(boolean black) {
        int numWhiteKeys = 0;
        for (int i = 0; i < mKeyCount; i++) {
            if (!BLACK_KEYS[(mFirstKey + i) % 12]) {
                numWhiteKeys++;
            }
        }
        int keyWidth = mWidth / numWhiteKeys;
        int leftOffset = 0;

        for (int i = 0; i < mKeyCount; i++) {
            final int index = (mFirstKey + i) % 12;
            final int offset = leftOffset;
            final int key = mFirstKey + i;
            if (BLACK_KEYS[index] && black) {
                v(MidiKeyView.class, () -> {
                    size(keyWidth-2*dip(3), mHeight/2);
                    x(offset - keyWidth / 2);
                    tag(key);
                    margin(dip(3));
                    backgroundColor(0xff333333);
                });
            }
            if (!BLACK_KEYS[index] && !black) {
                v(MidiKeyView.class, () -> {
                    size(keyWidth - 2 * dip(3), FILL);
                    tag(key);
                    x(offset);
                    margin(dip(3));
                    backgroundColor(0xffe1e1e1);
                });
            }
            if (!BLACK_KEYS[index]) {
                leftOffset = leftOffset + keyWidth;
            }
        }
    }
}
