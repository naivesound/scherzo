package com.naivesound.scherzo

import android.annotation.TargetApi
import android.app.Application
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.Build
import android.os.IBinder

class App : Application() {

    private var service: ScherzoService? = null

    override fun onCreate() {
        super.onCreate()
        sInstance = this

        val intent = Intent(this, ScherzoService::class.java)
        startService(intent)
        bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE)
    }

    private val serviceConnection = object : ServiceConnection {
        @TargetApi(Build.VERSION_CODES.M)
        override fun onServiceConnected(className: ComponentName, service: IBinder) {
            val binder = service as ScherzoService.ScherzoBinder
            sInstance!!.service = binder.service
            sInstance!!.service!!.handleMIDI()
        }

        override fun onServiceDisconnected(className: ComponentName) {
            sInstance!!.service = null
        }
    }

    companion object {

        private var sInstance: App? = null

        fun getService(): ScherzoService? {
            return sInstance!!.service
        }

        fun stopService() {
            if (sInstance!!.service != null) {
                sInstance!!.service!!.destroy()
                sInstance!!.unbindService(sInstance!!.serviceConnection)
                sInstance!!.service = null
            }
            val intent = Intent(sInstance, ScherzoService::class.java)
            sInstance!!.stopService(intent)
        }
    }
}
