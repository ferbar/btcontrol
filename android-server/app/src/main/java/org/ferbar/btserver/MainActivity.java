package org.ferbar.btserver;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
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

import ferbar.org.btserver.R;



public class MainActivity extends AppCompatActivity {
    final String TAG="MainActivity";

    Button startButton, sendButton, clearButton, stopButton;
    TextView textView;
    EditText editText;

    BTcommServer commServer=null;
    USBConn usbConn=null;
    static USBArduino usbArduino=null;

    class GuiUSBCallbacks implements USBConn.USBCallbacks{

        @Override
        public void onReceivedData(String data) {
            data.concat("/n");
            tvAppend(textView, data);
        }

        @Override
        public void onConnect() {
            setUiEnabled(true);
            tvAppend(textView, "Serial Connection Opened!\n");
        }

        @Override
        public void onAttached() {
            onClickStart(startButton);
        }

        @Override
        public void onDetached() {
            onClickStop(stopButton);
        }
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });

        startButton = (Button) findViewById(R.id.buttonStart);
        sendButton = (Button) findViewById(R.id.buttonSend);
        clearButton = (Button) findViewById(R.id.buttonClear);
        stopButton = (Button) findViewById(R.id.buttonStop);
        editText = (EditText) findViewById(R.id.editText);
        textView = (TextView) findViewById(R.id.textView);
        setUiEnabled(false);


        this.usbConn=new USBConn(this, new GuiUSBCallbacks());
        this.usbArduino=new USBArduino(this.usbConn);

        try {
            this.commServer=new BTcommServer(this);
            this.commServer.start();
        } catch (Exception e) {
            Toast.makeText(this,"error creating message listening server",Toast.LENGTH_LONG).show();
            e.printStackTrace();
        }

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
        this.usbConn.connect(this);

    }

    public void onClickSend(View view) {
        String string = editText.getText().toString();
        this.usbConn.write(string);
        tvAppend(textView, "\nData Sent : " + string + "\n");

    }

    public void onClickStop(View view) {
        setUiEnabled(false);
        this.usbConn.close();
        tvAppend(textView, "\nSerial Connection Closed! \n");

    }

    public void onClickClear(View view) {
        textView.setText(" ");
    }

    private void tvAppend(TextView tv, CharSequence text) {
        final TextView ftv = tv;
        final CharSequence ftext = text;

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                ftv.append(ftext);
            }
        });
    }

}
