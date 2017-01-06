package org.ferbar.btserver;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.util.Log;
import android.widget.Toast;

import com.felhr.usbserial.UsbSerialDevice;
import com.felhr.usbserial.UsbSerialInterface;

import java.io.UnsupportedEncodingException;
import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Created by chris on 05.01.17.
 */
public class USBConn {
    public final String ACTION_USB_PERMISSION = "com.hariharan.arduinousb.USB_PERMISSION";
    public final String TAG = "USBConn";
    private final Context context;

    UsbManager usbManager;
    UsbDevice device;
    UsbSerialDevice serialPort;
    UsbDeviceConnection connection;

    interface USBCallbacks {
        void onReceivedData(String data);
        void onConnect() throws Exception;
        void onAttached();
        void onDetached();
    }

    List<USBCallbacks> callbacks = new ArrayList<USBCallbacks>();

    UsbSerialInterface.UsbReadCallback mCallback = new UsbSerialInterface.UsbReadCallback() { //Defining a Callback which triggers whenever data is read.
        @Override
        public void onReceivedData(byte[] arg0) {
            try {
                String data = new String(arg0, "UTF-8");
                for(USBCallbacks c : USBConn.this.callbacks) {
                    c.onReceivedData(data);
                }

            } catch (UnsupportedEncodingException e) {
                e.printStackTrace();
            }


        }
    };


    private final BroadcastReceiver broadcastReceiver = new BroadcastReceiver() { //Broadcast Receiver to automatically start and stop the Serial connection.
        @Override
        public void onReceive(Context context, Intent intent) {
            try {
                if (intent.getAction().equals(ACTION_USB_PERMISSION)) {
                    // da kommts her wenn man auf connect drückt
                    boolean granted = intent.getExtras().getBoolean(UsbManager.EXTRA_PERMISSION_GRANTED);
                    if (granted) {
                        // da kommts her wenn man sagt usb permission is ok
                        connection = usbManager.openDevice(device);
                        serialPort = UsbSerialDevice.createUsbSerialDevice(device, connection);
                        if (serialPort != null) {
                            if (serialPort.syncOpen()) { //Set Serial Connection Parameters.

                                // serialPort.setBaudRate(9600);
                                serialPort.setBaudRate(115200);
                                serialPort.setDataBits(UsbSerialInterface.DATA_BITS_8);
                                serialPort.setStopBits(UsbSerialInterface.STOP_BITS_1);
                                serialPort.setParity(UsbSerialInterface.PARITY_NONE);
                                serialPort.setFlowControl(UsbSerialInterface.FLOW_CONTROL_OFF);
                                serialPort.read(mCallback);


                                for (USBCallbacks c : USBConn.this.callbacks) {
                                    c.onConnect();
                                }
                            } else {
                                Log.d(TAG, "PORT NOT OPEN");
                            }
                        } else {
                            Log.d(TAG, "PORT IS NULL");
                        }
                    } else {
                        // Toast.makeText(MainActivity.this, "USB Permission not granted!", Toast.LENGTH_LONG).show();
                        Log.d(TAG, "PERM NOT GRANTED");
                    }
                } else if (intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_ATTACHED)) {
                    for (USBCallbacks c : USBConn.this.callbacks) {
                        c.onAttached();
                    }
                } else if (intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_DETACHED)) {
                    for (USBCallbacks c : USBConn.this.callbacks) {
                        c.onDetached();
                    }

                }
            } catch(Exception e) {
                e.printStackTrace();
            }
        };
    };

    USBConn(Context context, USBCallbacks callbacks) {
        this.context=context;
        this.callbacks.add(callbacks);
        usbManager = (UsbManager) context.getSystemService(context.USB_SERVICE);

        IntentFilter filter = new IntentFilter();
        filter.addAction(ACTION_USB_PERMISSION);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        context.registerReceiver(broadcastReceiver, filter);
    }

    void registerCallbacks(USBCallbacks callbacks) {
        this.callbacks.add(callbacks);
    }

    void connect(Context context) {
        HashMap<String, UsbDevice> usbDevices = usbManager.getDeviceList();
        if (!usbDevices.isEmpty()) {
            boolean keep = true;
            for (Map.Entry<String, UsbDevice> entry : usbDevices.entrySet()) {
                device = entry.getValue();
                int deviceVID = device.getVendorId();
                Log.d(TAG, "---- connected device: --- "+deviceVID+":"+device.getDeviceId()+" ("+device.getDeviceName()+")");
                // if (deviceVID == 0x2341)//Arduino Vendor ID
                if (deviceVID == 0x0403) // 20161231: Arduino Vendor ID von einem arduino uno
                {
                    PendingIntent pi = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), 0);
                    this.usbManager.requestPermission(device, pi);
                    keep = false;
                } else {
                    connection = null;
                    device = null;
                }

                if (!keep)
                    break;
            }
        }
    }


    public void close() {
        this.serialPort.syncClose();
        this.context.unregisterReceiver(broadcastReceiver);

    }

    public void write(String string) {
        this.serialPort.syncWrite(string.getBytes(), 1000);
    }

    byte[] readBuffer=new byte[1024];
    int readBufferLen=-1;
    int readBufferPos=-1;

    char readCharBuffered() throws Exception {
        if(this.readBufferPos>=this.readBufferLen) {
            this.readBufferLen=this.serialPort.syncRead(this.readBuffer, 100);
            if(this.readBufferLen <= 0) {
                throw new Exception("timeout");
            }
            this.readBufferPos=0;
        }
        return (char) this.readBuffer[this.readBufferPos++];
    }

    public String readLineBuffered(int timeout) throws Exception {
        String ret="";
        long beginTime = System.currentTimeMillis();
        long stopTime = beginTime + timeout;
        while(true) {
            if(System.currentTimeMillis() > stopTime) {
                throw new Exception("error reading full line (timeout, data="+ret+")");
            }
            char c=this.readCharBuffered();
            if(c=='\n') break;
            ret += c;
        }
        if(ret.length() > 0 && ret.charAt(ret.length()-1)=='\r') {
            ret=ret.substring(0,ret.length()-1);
        }
        return ret;
    }

    public String sendCommand(String cmd) throws Exception {
        this.serialPort.syncWrite(cmd.getBytes(),1000);
        byte data[]=new byte[1024];
        /*
        int len=serialPort.syncRead(data, 100);
        data = Arrays.copyOf(data, len);
        String ret = new String(data, "UTF-8");
        */
        String ret=this.readLineBuffered(200);
        for(USBCallbacks c : USBConn.this.callbacks) {
            c.onReceivedData(ret);
        }
        return ret;
    }
}
