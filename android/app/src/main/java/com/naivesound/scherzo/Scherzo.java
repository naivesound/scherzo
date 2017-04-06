package com.naivesound.scherzo;

public class Scherzo {
    static {
        System.loadLibrary("scherzo-lib");
    }

    private long mRef;

    public Scherzo(int sampleRate) {
        mRef = create(sampleRate);
    }

    public void dispose() {
        destroy(mRef);
        mRef = 0;
    }
    public void finalize() { dispose(); }
    public void noteOn(int chan, int note, int velocity) {
        noteOn(mRef, chan, note, velocity);
    }
    public void noteOff(int chan, int note) { noteOff(mRef, chan, note); }
    public void tapBPM() { tapBPM(mRef); }
    public void looperCommand(boolean primary) { looperCommand(mRef, primary); }

    public void midi(byte msg, byte a, byte b) { midi(mRef, msg&0xff, a&0xff, b&0xff); }
    private native long create(int sampleRate);
    private native void destroy(long ref);
    private native void midi(long ref, int msg, int a, int b);
    private native void noteOn(long ref, int chan, int note, int velocity);
    private native void noteOff(long ref, int chan, int note);
    private native void tapBPM(long ref);
    private native void looperCommand(long mRef, boolean primary);
}
