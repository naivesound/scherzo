package com.naivesound.scherzo

import android.annotation.TargetApi
import android.app.Service
import android.content.Context
import android.content.Intent
import android.media.AudioManager
import android.media.midi.MidiDeviceInfo
import android.media.midi.MidiManager
import android.media.midi.MidiReceiver
import android.os.Binder
import android.os.Build
import android.os.Handler
import android.os.IBinder
import android.util.Log

class ScherzoService : Service() {

    private var mScherzo: Scherzo? = null
    private val mBinder = ScherzoBinder()
    private val mHandler = Handler()

    override fun onStartCommand(intent: Intent, flags: Int, startId: Int): Int {
        val am = getSystemService(Context.AUDIO_SERVICE) as AudioManager
        val sampleRate = Integer.parseInt(am.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE))
        val framesPerBuffer = Integer.parseInt(am.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER))
        mScherzo = Scherzo(sampleRate, framesPerBuffer, 32) { events ->
            println("events: " + events);
        }
        return super.onStartCommand(intent, flags, startId)
    }

    @TargetApi(Build.VERSION_CODES.M)
    fun handleMIDI() {
        val midiManager = applicationContext.getSystemService(Context.MIDI_SERVICE) as MidiManager
        for (device in midiManager.devices) {
            openMIDIDevice(midiManager, device)
        }
        midiManager.registerDeviceCallback(object : MidiManager.DeviceCallback() {
            override fun onDeviceAdded(device: MidiDeviceInfo) {
                super.onDeviceAdded(device)
                openMIDIDevice(midiManager, device)
            }

            //            @Override
            //            public void onDeviceRemoved(MidiDeviceInfo device) {
            //                super.onDeviceRemoved(device);
            //            }

            //            @Override
            //            public void onDeviceStatusChanged(MidiDeviceStatus
            //            status) {
            //                super.onDeviceStatusChanged(status);
            //            }
        }, mHandler)
    }

    @TargetApi(Build.VERSION_CODES.M)
    private fun openMIDIDevice(midiManager: MidiManager,
                               device: MidiDeviceInfo) {
        midiManager.openDevice(
                device, { device ->
            (0..device.info.outputPortCount - 1)
                    .asSequence()
                    .map { device.openOutputPort(it) }
                    .forEach {
                        it.connect(object : MidiReceiver() {
                            override fun onSend(msg: ByteArray, offset: Int,
                                                count: Int, timestamp: Long) {
                                var offset = offset
                                var count = count
                                while (count >= 3) {
                                    Log.d("MIDI", "" + msg[offset] + " " +
                                            msg[offset + 1] + " " +
                                            msg[offset + 2])
                                    mScherzo!!.midi(msg[offset] as Int, msg[offset + 1] as Int,
                                            msg[offset + 2] as Int)
                                    offset += 3
                                    count -= 3
                                }
                            }
                        })
                    }
        }, mHandler)
    }

    override fun onBind(intent: Intent): IBinder? {
        return mBinder
    }

    fun noteOn(chan: Int, note: Int, volume: Int) {
        mScherzo!!.noteOn(chan, note, volume)
    }

    fun noteOff(chan: Int, note: Int) {
        mScherzo!!.noteOff(chan, note)
    }

    fun tap() {
        mScherzo!!.tap()
    }

    fun destroy() {
        mScherzo!!.dispose()
    }

    fun loop() {
        mScherzo!!.loop()
    }

    fun cancelLoop() {
        mScherzo!!.cancelLoop()
    }

    inner class ScherzoBinder : Binder() {
        internal val service: ScherzoService
            get() = this@ScherzoService
    }
}
