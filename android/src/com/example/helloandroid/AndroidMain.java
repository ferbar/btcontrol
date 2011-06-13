package com.example.helloandroid;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.EditText;
import btcontroll.BTcommThread;
import btcontroll.AndroidStream;
import protocol.MessageLayouts;

import com.example.helloandroid.ControllAction;

public class AndroidMain extends Activity {
	
	static BTcommThread btcomm; 
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
		MessageLayouts messageLayouts = new MessageLayouts();
		try {
			messageLayouts.load();
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
/*
        TextView tv = new TextView(this);
        tv.setText("Hello, Android");
        setContentView(tv);
*/
    }
    
	class ConnectThread extends Thread {
		Context c;
		AndroidStream androidStream;
		ConnectThread(Context c, AndroidStream androidStream) {
			this.c=c;
			this.androidStream = androidStream;
		}
		public void run() {
			try {
				System.out.println("connecting:"+this.androidStream.server+":"+this.androidStream.port);

				Object connectedNotifyObject = new Object();
				btcomm = new BTcommThread(this.androidStream, connectedNotifyObject);
						//Thread t = new Thread(btcomm); t.start(); -> da is isAlive auf einmal nicht gesetzt
//				getDisplay().setCurrent(get_controllCanvas(btcomm));
				Intent i = new Intent(this.c, ControllAction.class);
				//startActivityForResult(i, ACTIVITY_CREATE);
				startActivity(i);
				btcomm.start();

				// update check:
				synchronized(connectedNotifyObject) {
					connectedNotifyObject.wait();
				}
				if(!btcomm.connError()) {
					// debugForm.setTitle("connected");
					
// ############				} else if (btcomm.doupdate) {
// ############					exitMIDlet();
				}
			} catch(Exception e) {
// ############				System.out.println("commandAction Exception:"+e.toString());
			}
		}
	}
    
    public void onButtonConnect(View view) {
    	if(btcomm != null)
    		btcomm.close();
    	EditText server = (EditText) findViewById(R.id.editText1);
		String sserver = server.getText().toString();
		System.out.println("server:"+sserver);
		ConnectThread connectThread = new ConnectThread(this, new AndroidStream(sserver,3030));
		connectThread.start();
    }
    
    public void onButtonDisconnect(View view) {
    	if(btcomm != null)
    		btcomm.close();
    }
}