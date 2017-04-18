package com.naivesound.scherzo;

import android.content.Context;
import android.graphics.Typeface;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
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
            keyboard();
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
                onClick(v -> {
                    Log.d(TAG, "TAP click");
                    App.getService().tap();
                });
            });

            button(() -> {
                size(0, FILL);
                weight(1f);
                margin(dip(10), 0);
                text("LOOP");
                onClick(v -> {
                    Log.d(TAG, "LOOP click");
                    App.getService().loop();
                });
            });

            button(() -> {
                size(0, FILL);
                weight(1f);
                margin(dip(10), 0);
                text("CANCEL");
                onClick(v -> {
                    Log.d(TAG, "CANCEL click");
                    App.getService().cancelLoop();
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

    private void keyboard() {
        frameLayout(() -> {
            size(FILL, 0);
            weight(2f);

            linearLayout(() -> {
                size(FILL, FILL);

                for (int i = 0; i < 7; i++) {
                    final int index = i;
                    int tag = 48 + index * 2;
                    v(MidiKeyView.class, () -> {
                        size(0, FILL);
                        tag(index > 2 ? tag-1 : tag);
                        weight(1f);
                        margin(dip(3), dip(3), (index == 6 ? dip(3) : 0), dip(3));
                        backgroundColor(0xffe1e1e1);
                    });
                }
            });

            int keyWidth = mWidth/7;
            frameLayout(() -> {
                size(FILL, mHeight/4);

                for (int i = 0; i < 5; i++) {
                    final int index = i;
                    int marginLeft = dip(3) + keyWidth*i + keyWidth/2;
                    if (i >= 2) {
                        marginLeft += keyWidth;
                    }
                    int finalMarginLeft = marginLeft;
                    int tag = 49 + i * 2;
                    v(MidiKeyView.class, () -> {
                        size(keyWidth-2*dip(3), FILL);
                        tag(index > 1 ? tag+1 : tag);
                        margin(finalMarginLeft, dip(3), dip(3), 0);
                        backgroundColor(0xff333333);
                    });
                }
            });
        });
    }
}
