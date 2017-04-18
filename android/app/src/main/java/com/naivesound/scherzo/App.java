package com.naivesound.scherzo;

import android.app.Application;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;

public class App extends Application {

    private static App sInstance;

    private ScherzoService service;

    @Override
    public void onCreate() {
        super.onCreate();
        sInstance = this;

        Intent intent = new Intent(this, ScherzoService.class);
        startService(intent);
        bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE);
    }

    public static ScherzoService getService() {
        return sInstance.service;
    }

    public static void stopService() {
        if (sInstance.service != null) {
            sInstance.service.destroy();
            sInstance.unbindService(sInstance.serviceConnection);
            sInstance.service = null;
        }
        Intent intent = new Intent(sInstance, ScherzoService.class);
        sInstance.stopService(intent);

    }

    private ServiceConnection serviceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            ScherzoService.ScherzoBinder binder = (ScherzoService.ScherzoBinder)service;
            sInstance.service = binder.getService();
            sInstance.service.handleMIDI();
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            sInstance.service = null;
        }
    };
}
