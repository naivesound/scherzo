package com.naivesound.scherzo;

public class Scherzo {
    static { System.loadLibrary("scherzo-lib"); }

    private long mRef;

    public Scherzo(int sampleRate) { mRef = create(sampleRate); }

    public void dispose() {
        destroy(mRef);
        mRef = 0;
    }
    public void finalize() { dispose(); }
    public void noteOn(int chan, int note, int velocity) {
        midi(mRef, 0x90, note, velocity);
    }
    public void noteOff(int chan, int note) { midi(mRef, 0x80, note, 0); }
    public void tap() { midi(mRef, 0xb0, 0x50, 1); }
    public void loop() { midi(mRef, 0xb0, 0x51, 1); }
    public void cancelLoop() { midi(mRef, 0xb0, 0x52, 1); }

    public void midi(byte msg, byte a, byte b) {
        midi(mRef, msg & 0xff, a & 0xff, b & 0xff);
    }
    private native long create(int sampleRate);
    private native void destroy(long ref);
    private native void midi(long ref, int msg, int a, int b);

    // TODO: get note, get CC
}
