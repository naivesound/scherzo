package com.naivesound.scherzo;

import android.annotation.TargetApi;
import android.app.Service;
import android.content.Intent;
import android.media.midi.MidiDevice;
import android.media.midi.MidiDeviceInfo;
import android.media.midi.MidiDeviceStatus;
import android.media.midi.MidiManager;
import android.media.midi.MidiOutputPort;
import android.media.midi.MidiReceiver;
import android.os.Binder;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;

public class ScherzoService extends Service {

    private final Scherzo mScherzo;
    private final IBinder mBinder = new ScherzoBinder();
    private final Handler mHandler = new Handler();

    public ScherzoService() {
        //        mScherzo = new Scherzo(44100);
        mScherzo = new Scherzo(48000);
    }

    @TargetApi(Build.VERSION_CODES.M)
    public void handleMIDI() {
        final MidiManager midiManager =
            (MidiManager)getApplicationContext().getSystemService(MIDI_SERVICE);
        for (MidiDeviceInfo device : midiManager.getDevices()) {
            openMIDIDevice(midiManager, device);
        }
        midiManager.registerDeviceCallback(new MidiManager.DeviceCallback() {
            @Override
            public void onDeviceAdded(MidiDeviceInfo device) {
                super.onDeviceAdded(device);
                openMIDIDevice(midiManager, device);
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
        }, mHandler);
    }

    @TargetApi(Build.VERSION_CODES.M)
    private void openMIDIDevice(MidiManager midiManager,
                                MidiDeviceInfo device) {
        midiManager.openDevice(
            device, new MidiManager.OnDeviceOpenedListener() {
                public void onDeviceOpened(MidiDevice device) {
                    for (int i = 0; i < device.getInfo().getOutputPortCount();
                         i++) {
                        MidiOutputPort outputPort = device.openOutputPort(i);
                        outputPort.connect(new MidiReceiver() {
                            public void onSend(byte[] msg, int offset,
                                               int count, long timestamp) {
                                while (count >= 3) {
                                    Log.d("MIDI", "" + msg[offset] + " " +
                                                      msg[offset + 1] + " " +
                                                      msg[offset + 2]);
                                    mScherzo.midi(msg[offset], msg[offset + 1],
                                                  msg[offset + 2]);
                                    offset += 3;
                                    count -= 3;
                                }
                            }
                        });
                    }
                }
            }, mHandler);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    public void noteOn(int chan, int note, int volume) {
        mScherzo.noteOn(chan, note, volume);
    }

    public void noteOff(int chan, int note) { mScherzo.noteOff(chan, note); }

    public void tap() { mScherzo.tap(); }

    public void destroy() { mScherzo.dispose(); }

    public void loop() { mScherzo.loop(); }

    public void cancelLoop() { mScherzo.cancelLoop(); }

    public class ScherzoBinder extends Binder {
        ScherzoService getService() { return ScherzoService.this; }
    }
}
