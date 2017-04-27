package com.naivesound.scherzo;

import android.content.Context;
import android.graphics.Typeface;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.MotionEvent;
import android.view.WindowManager;
import android.widget.LinearLayout;

import trikita.anvil.RenderableView;
import static trikita.anvil.DSL.*;

public class ScherzoView extends RenderableView {

    private final static String TAG = "ScherzoView";

    private int mWidth;
    private int mHeight;

    public ScherzoView(Context context) {
        super(context);
        WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        Display display = wm.getDefaultDisplay();
        DisplayMetrics metrics = new DisplayMetrics();
        display.getMetrics(metrics);
        mWidth = metrics.widthPixels;
        mHeight = metrics.heightPixels;
    }

    @Override
    public void view() {
        linearLayout(() -> {
            size(FILL, FILL);
            orientation(LinearLayout.VERTICAL);

            buttons();
            knobs();
            v(PianoKeyboard.class, () -> {
                size(FILL, 0);
                weight(2f);
            });
        });
    }

    private void buttons() {
        linearLayout(() -> {
            size(FILL, 0);
            weight(1f);
            padding(dip(15));
            gravity(CENTER_VERTICAL);

            button(() -> {
                size(0, FILL);
                weight(1f);
                margin(dip(10), 0);
                text("TAP");
                onTouch((v, e) -> {
                    if (e.getActionMasked() == MotionEvent.ACTION_DOWN) {
                        Log.d(TAG, "TAP click");
                        App.getService().tap();
                    }
                    return false;
                });
            });

            button(() -> {
                size(0, FILL);
                weight(1f);
                margin(dip(10), 0);
                text("LOOP");
                onTouch((v, e) -> {
                    if (e.getActionMasked() == MotionEvent.ACTION_DOWN) {
                        Log.d(TAG, "LOOP click");
                        App.getService().loop();
                    }
                    return false;
                });
            });

            button(() -> {
                size(0, FILL);
                weight(1f);
                margin(dip(10), 0);
                text("CANCEL");
                onTouch((v, e) -> {
                    if (e.getActionMasked() == MotionEvent.ACTION_DOWN) {
                        Log.d(TAG, "CANCEL click");
                        App.getService().cancelLoop();
                    }
                    return false;
                });

            });
        });
    }

    private void knobs() {
        linearLayout(() -> {
            size(FILL, 0);
            weight(1f);
            gravity(CENTER_VERTICAL);

            linearLayout(() -> {
                size(0, FILL);
                weight(1f);
                padding(dip(10), dip(5));
                orientation(LinearLayout.VERTICAL);

                textView(() -> {
                    size(WRAP, WRAP);
                    text("Volume");
                    textSize(sip(18));
                    typeface(Typeface.DEFAULT_BOLD);
                });

                seekBar(() -> {
                    size(FILL, WRAP);
                    padding(dip(15), dip(10));
                    onSeekBarChange((sbar, progress, fromUser) -> {
                        Log.d(TAG, "VOLUME: progress="+progress);
                    });
                });
            });

            linearLayout(() -> {
                size(0, FILL);
                weight(1f);
                padding(dip(10), dip(5));
                orientation(LinearLayout.VERTICAL);

                textView(() -> {
                    size(WRAP, WRAP);
                    text("Metronome");
                    textSize(sip(18));
                    typeface(Typeface.DEFAULT_BOLD);
                });

                seekBar(() -> {
                    size(FILL, WRAP);
                    padding(dip(15), dip(10));
                    onSeekBarChange((sbar, progress, fromUser) -> {
                        Log.d(TAG, "METRONOME: progress="+progress);
                    });
                });
            });

            linearLayout(() -> {
                size(0, FILL);
                weight(1f);
                padding(dip(10), dip(5));
                orientation(LinearLayout.VERTICAL);

                textView(() -> {
                    size(WRAP, WRAP);
                    text("Slider #3");
                    textSize(sip(18));
                    typeface(Typeface.DEFAULT_BOLD);
                });

                seekBar(() -> {
                    size(FILL, WRAP);
                    padding(dip(15), dip(10));
                    onSeekBarChange((sbar, progress, fromUser) -> {
                        Log.d(TAG, "SLIDER3: progress="+progress);
                    });
                });
            });

            linearLayout(() -> {
                size(0, FILL);
                weight(1f);
                padding(dip(10), dip(5));
                orientation(LinearLayout.VERTICAL);

                textView(() -> {
                    size(WRAP, WRAP);
                    text("Slider #4");
                    textSize(sip(18));
                    typeface(Typeface.DEFAULT_BOLD);
                });

                seekBar(() -> {
                    size(FILL, WRAP);
                    padding(dip(15), dip(10));
                    onSeekBarChange((sbar, progress, fromUser) -> {
                        Log.d(TAG, "SLIDER4: progress="+progress);
                    });
                });
            });
        });
    }
}
