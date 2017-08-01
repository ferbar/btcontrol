package org.ferbar.btserver;

import android.util.Log;

import java.io.UnsupportedEncodingException;
import java.util.concurrent.TimeoutException;

/**
 * Created by chris on 05.01.17.
 */
public class USBArduino {
    public USBConn usbConn=null;
    final String TAG="USBArduino";


    class ArduinoUSBCallbacks implements USBConn.USBCallbacks{

        @Override
        public void onReceivedData(String data) {
            // data.concat("/n");
            // tvAppend(textView, data);
        }

        @Override
        public void onConnect() throws Exception {
            String ret=sendCommand("V",true);
            MainActivity.debuglog(ret);
            //Log.d(TAG, ret);
        }

        @Override
        public void onAttached() {
            // onClickStart(startButton);
        }

        @Override
        public void onDetached() {
            // onClickStop(stopButton);
            usbConn.close();
        }
    }

    public USBArduino(USBConn usbConn) {
        this.usbConn=usbConn;
        this.usbConn.registerCallbacks(new ArduinoUSBCallbacks());
    }


    public String sendCommand(String cmd) throws TimeoutException {
        return this.sendCommand(cmd, false);
    }

    /**
     *
     * @param cmd command
     * @param multilineReturn nach \r\n lesen abbrechen oder nach timeout
     * @return readLine()
     * @throws TimeoutException
     */
    public String sendCommand(String cmd, boolean multilineReturn) throws TimeoutException {
        MainActivity.debuglog(">" + cmd);
        this.usbConn.write(cmd+"\r\n");
        String ret;
        if(multilineReturn)
            ret=this.usbConn.readFullBufferedTimeout(200);
        else
            ret=this.usbConn.readLineBuffered(200);
        MainActivity.debuglog("<"+ret);
        return ret;
    }

}
