package org.ferbar.btserver;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.io.UnsupportedEncodingException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeoutException;

import ferbar.org.btserver.R;



public class MainActivity extends AppCompatActivity {
    final String TAG="MainActivity";

    Button startButton, sendButton, clearButton, stopButton;
    static TextView textView=null;
    static Activity context=null;
    EditText editText=null;

    static BTcommServer commServer=null;
    static USBArduino usbArduino=null;
    private GuiUSBCallbacks guiUSBCallbacks;

    class GuiUSBCallbacks implements USBConn.USBCallbacks{

        @Override
        public void onReceivedData(String data) {
            String d = ">" + data + "\n";
            debuglog(d);
        }

        @Override
        public void onConnect() {
            setUiEnabled(true);
            debuglog("connected");
        }

        @Override
        public void onAttached() {
            MainActivity.this.setUiEnabled(true);
            debuglog("attached");
        }

        @Override
        public void onDetached() {
            MainActivity.this.setUiEnabled(true);
            debuglog("detached");
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.context=this;
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        /*
        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });
        */

        if(this.usbArduino == null)
            this.usbArduino=new USBArduino(new USBConn(this));

        this.usbArduino.usbConn.registerCallbacks(this.guiUSBCallbacks=new MainActivity.GuiUSBCallbacks());

        startButton = (Button) findViewById(R.id.buttonStart);
        sendButton = (Button) findViewById(R.id.buttonSend);
        clearButton = (Button) findViewById(R.id.buttonClear);
        stopButton = (Button) findViewById(R.id.buttonStop);
        editText = (EditText) findViewById(R.id.editText);
        textView = (TextView) findViewById(R.id.textView);
        textView.setMovementMethod(new ScrollingMovementMethod());

        setUiEnabled(this.usbArduino.usbConn.isConnected());

        if(this.commServer==null) {
            try {
                this.commServer=new BTcommServer(this);
                this.commServer.start();
            } catch (Exception e) {
                Toast.makeText(this, "error creating message listening server", Toast.LENGTH_LONG).show();
                debuglog("error creating BTcommServer");
                e.printStackTrace();
            }
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        MainActivity.usbArduino.usbConn.unregisterCallbacks(this.guiUSBCallbacks);

    }

    public void setUiEnabled(boolean bool) {
        startButton.setEnabled(!bool);
        sendButton.setEnabled(bool);
        stopButton.setEnabled(bool);
        textView.setEnabled(bool);

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    public void onClickStart(View view) {
        debuglog("\nSerial connection starting\n");
        MainActivity.usbArduino.usbConn.connect(this);
    }

    public void onClickSend(View view) {
        String string = editText.getText().toString();
        debuglog("tx: " + string + "\n");
        try {
            String s=MainActivity.usbArduino.sendCommand(string, true);
            debuglog("rx " + s + "\n");
        } catch (TimeoutException e) {
            debuglog("cmd error:" + e.toString() + "\n");
        }
    }

    public void onClickStop(View view) {
        setUiEnabled(false);
        debuglog("\nSerial connection closing\n");
        MainActivity.usbArduino.usbConn.close();
    }

    public void onClickClear(View view) {
        textView.setText(" ");
    }

    public static void debuglog(CharSequence text) {
        final TextView ftv = MainActivity.textView;
        final CharSequence ftext = text;

        MainActivity.context.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                ftv.append(ftext);
            }
        });
    }

}
