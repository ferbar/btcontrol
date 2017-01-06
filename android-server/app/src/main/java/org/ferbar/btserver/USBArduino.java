package org.ferbar.btserver;

import android.util.Log;

import java.io.UnsupportedEncodingException;

/**
 * Created by chris on 05.01.17.
 */
public class USBArduino {
    USBConn usbConn=null;
    final String TAG="USBArduino";


    class ArduinoUSBCallbacks implements USBConn.USBCallbacks{

        @Override
        public void onReceivedData(String data) {
            // data.concat("/n");
            // tvAppend(textView, data);
        }

        @Override
        public void onConnect() throws Exception {
            /*
            setUiEnabled(true);
            tvAppend(textView, "Serial Connection Opened!\n");
            */
            String ret=sendCommand("V");
            Log.d(TAG, ret);
        }

        @Override
        public void onAttached() {
            // onClickStart(startButton);
        }

        @Override
        public void onDetached() {
            // onClickStop(stopButton);
        }
    }

    public USBArduino(USBConn usbConn) {
        this.usbConn=usbConn;
        this.usbConn.registerCallbacks(new ArduinoUSBCallbacks());
    }

    public String sendCommand(String cmd) throws Exception {
        return this.usbConn.sendCommand(cmd+"\r\n");
    }

}
