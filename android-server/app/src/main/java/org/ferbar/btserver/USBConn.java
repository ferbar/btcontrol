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
import java.util.concurrent.TimeoutException;

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
    UsbDeviceConnection connection=null;


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

    public void initUSBConnection() {
        this.connection = usbManager.openDevice(device);
        this.serialPort = UsbSerialDevice.createUsbSerialDevice(device, connection);
        if(this.serialPort != null) {
            if(this.serialPort.syncOpen()) { //Set Serial Connection Parameters.

                // serialPort.setBaudRate(9600);
                this.serialPort.setBaudRate(115200);
                this.serialPort.setDataBits(UsbSerialInterface.DATA_BITS_8);
                this.serialPort.setStopBits(UsbSerialInterface.STOP_BITS_1);
                this.serialPort.setParity(UsbSerialInterface.PARITY_NONE);
                this.serialPort.setFlowControl(UsbSerialInterface.FLOW_CONTROL_OFF);
                this.serialPort.read(mCallback);


                for (USBCallbacks c : USBConn.this.callbacks) {
                    try {
                        c.onConnect();
                    } catch (Exception e) {
                        e.printStackTrace();
                        MainActivity.debuglog(e.toString());
                    }
                }
            } else {
                Log.d(TAG, "PORT NOT OPEN");
            }
        } else {
            Log.d(TAG, "PORT IS NULL");
        }
    }


    private final BroadcastReceiver broadcastReceiver = new BroadcastReceiver() { //Broadcast Receiver to automatically start and stop the Serial connection.
        @Override
        public void onReceive(Context context, Intent intent) {
            try {
                if (intent.getAction().equals(ACTION_USB_PERMISSION)) {
                    // da kommts her wenn man vom android die permission bekommen hat aufs device zu schreiben
                    boolean granted = intent.getExtras().getBoolean(UsbManager.EXTRA_PERMISSION_GRANTED);
                    if (granted) {
                        // da kommts her wenn man sagt usb permission is ok
                        initUSBConnection();

                    } else {
                        MainActivity.debuglog("USB PERM NOT GRANTED");
                        // Toast.makeText(MainActivity.this, "USB Permission not granted!", Toast.LENGTH_LONG).show();
                        Log.d(TAG, "PERM NOT GRANTED");
                    }
                } else if (intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_ATTACHED)) {
                    // wenn der arduino angesteckt wurde
                    for (USBCallbacks c : USBConn.this.callbacks) {
                        c.onAttached();
                    }
                } else if (intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_DETACHED)) {
                    for (USBCallbacks c : USBConn.this.callbacks) {
                        c.onDetached();
                    }

                }
            } catch(Exception e) {
                MainActivity.debuglog(e.toString());
                e.printStackTrace();
            }
        };
    };

    public USBConn(Context context) {
        this.context=context;
        usbManager = (UsbManager) context.getSystemService(context.USB_SERVICE);

    }

    void registerCallbacks(USBCallbacks callbacks) {
        if(this.callbacks.size() == 0) {
            IntentFilter filter = new IntentFilter();
            filter.addAction(ACTION_USB_PERMISSION);
            filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
            filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
            context.registerReceiver(broadcastReceiver, filter);
        }
        this.callbacks.add(callbacks);
    }

    void unregisterCallbacks(USBCallbacks callbacks) {
        this.callbacks.remove(callbacks);
        if(this.callbacks.size() == 0) {
            this.context.unregisterReceiver(broadcastReceiver);
        }
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
                    if(!this.usbManager.hasPermission(device)) {
                        PendingIntent pi = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), 0);
                        this.usbManager.requestPermission(device, pi);
                    } else {
                        initUSBConnection();
                    }
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

    public boolean isConnected() {
        return this.connection != null;
    }

    public void close() {
        this.serialPort.syncClose();
        this.serialPort=null;
        this.connection.close();
        this.connection=null;
    }

    public void write(String string) {
        this.serialPort.syncWrite(string.getBytes(), 1000);
    }

    byte[] readBuffer=new byte[1024];
    int readBufferLen=0;
    int readBufferPos=0;

    char readCharBuffered() throws TimeoutException {
        if(this.readBufferPos>=this.readBufferLen) {
            this.readBufferLen=this.serialPort.syncRead(this.readBuffer, 100);
            if(this.readBufferLen <= 0) {
                throw new TimeoutException("timeout ("+this.readBufferLen+")");
            }
            this.readBufferPos=0;
        }
        return (char) this.readBuffer[this.readBufferPos++];
    }

    public String readLineBuffered(int timeout) throws TimeoutException {
        String ret="";
        long beginTime = System.currentTimeMillis();
        long stopTime = beginTime + timeout;
        while(true) {
            if(System.currentTimeMillis() > stopTime) {
                throw new TimeoutException("error reading full line (timeout, data="+ret+")");
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

    /**
     * read data until timeout
     * @param timeout
     */
    public String readFullBufferedTimeout(int timeout) {
        String ret="";
        long beginTime = System.currentTimeMillis();
        long stopTime = beginTime + timeout;
        while(true) {
            if(this.readBufferPos>=this.readBufferLen) {
                this.readBufferLen=this.serialPort.syncRead(this.readBuffer, timeout);
                if(this.readBufferLen <= 0) {
                    Log.i(TAG, "readFullBufferedTimeout ret="+this.readBufferLen);
                    return ret;
                }
                this.readBufferPos=0;
            }
            ret+=new String( this.readBuffer, this.readBufferPos, this.readBufferLen);
            this.readBufferPos=this.readBufferLen;
        }

    }

    /*
    public String sendCommand(String cmd) throws TimeoutException {
        this.serialPort.syncWrite(cmd.getBytes(),1000);
        byte data[]=new byte[1024];
        /*
        int len=serialPort.syncRead(data, 100);
        data = Arrays.copyOf(data, len);
        String ret = new String(data, "UTF-8");
        * /
        String ret=this.readLineBuffered(200);
        for(USBCallbacks c : USBConn.this.callbacks) {
            c.onReceivedData(ret);
        }
        return ret;
    }
    */
}
