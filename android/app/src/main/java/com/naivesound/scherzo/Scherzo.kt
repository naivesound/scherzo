package com.naivesound.scherzo

class Scherzo(sampleRate: Int, framesPerBuffer: Int, maxPoly: Int, cb: (Int)->Unit) {

    private var mRef: Long = 0

    init { mRef = create(sampleRate, framesPerBuffer, maxPoly, cb) }

    fun dispose() {
        destroy(mRef)
        mRef = 0
    }

    fun finalize() { dispose() }
    fun noteOn(chan: Int, note: Int, velocity: Int) { midi(mRef, 0x90, note, velocity) }
    fun noteOff(chan: Int, note: Int) { midi(mRef, 0x80, note, 0) }
    fun tap() { midi(mRef, 0xb0, 0x50, 1) }
    fun loop() { midi(mRef, 0xb0, 0x51, 1) }
    fun cancelLoop() { midi(mRef, 0xb0, 0x52, 1) }
    fun midi(msg: Int, a: Int, b: Int) { midi(mRef, msg and 0xff, a and 0xff, b and 0xff) }

    private external fun create(sampleRate: Int, framesPerBuffer: Int, maxPoly: Int, cb: (Int)->Unit): Long
    private external fun destroy(ref: Long)
    private external fun midi(ref: Long, msg: Int, a: Int, b: Int)

    companion object {
        init {
            System.loadLibrary("scherzo-jni")
        }
    }
}
