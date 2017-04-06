package com.naivesound.scherzo;

import android.Manifest;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;

public class MainActivity extends Activity {

    private static final int PERMISSION_REQEUST_ID = 1000;
    private static final String TAG = "MainActivity";
    private ScherzoService mService;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent intent = new Intent(this, ScherzoService.class);
        startService(intent);
        bindService(intent, mConnection, Context.BIND_AUTO_CREATE);

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);

        Button bpmButton = new Button(this);
        bpmButton.setText("BPM");
        bpmButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService != null) {
                    mService.tapBPM();
                }
            }
        });
        layout.addView(bpmButton);

        Button looperButton = new Button(this);
        looperButton.setText("Looper");
        looperButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService != null) {
                    mService.looperCommand(true);
                }
            }
        });
        layout.addView(looperButton);

        Button looperExtButton = new Button(this);
        looperExtButton.setText("Looper (ext)");
        looperExtButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService != null) {
                    mService.looperCommand(false);
                }
            }
        });
        layout.addView(looperExtButton);

        setContentView(layout);


        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)  != PackageManager.PERMISSION_GRANTED) {
                requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, PERMISSION_REQEUST_ID);
            }
        }
    }

    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case PERMISSION_REQEUST_ID:
                //
                break;
        }
    }

    @Override
    protected void onDestroy() {
        if (isFinishing()) {
            if (mService != null) {
                mService.destroy();
                unbindService(mConnection);
                mService = null;
            }
            Intent intent = new Intent(this, ScherzoService.class);
            stopService(intent);
        }
        super.onDestroy();
    }

    private ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            ScherzoService.ScherzoBinder binder = (ScherzoService.ScherzoBinder) service;
            mService = binder.getService();
            mService.handleMIDI();
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            mService = null;
        }
    };
}
