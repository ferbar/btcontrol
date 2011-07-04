package com.example.helloandroid;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;
import btcontroll.BTcommThread;
import btcontroll.AndroidStream;
import btcontroll.Debuglog;
import protocol.MessageLayouts;

import com.example.helloandroid.ControllAction;

public class AndroidMain extends Activity {
	public static final String PREFS_NAME = "btcontroll";
	
	static BTcommThread btcomm = null;
	static ConnectThread connectThread;
	static String btcommMessage = null;
	
	static int foregroundActivities=0;
	static String sserver; // server name
	
	// Need handler for callbacks to the UI thread
    final Handler mHandler = new Handler();
	// Create runnable for posting
    final Runnable mUpdateResults = new Runnable() {
        public void run() {
            repaint();
        }
    };
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
		MessageLayouts messageLayouts = new MessageLayouts();
		try {
			messageLayouts.load();
			// MessageLayouts.dump();
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
		EditText server = (EditText) findViewById(R.id.editText1);
		String text = settings.getString("server", "");
		server.setText(text);
/*
        TextView tv = new TextView(this);
        tv.setText("Hello, Android");
        setContentView(tv);
*/
    }
    
    @Override
    public void onStop() {
    	super.onStop();
    	
    	// einstellungen speichern
        // We need an Editor object to make preference changes.
        // All objects are from android.context.Context
        SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
        SharedPreferences.Editor editor = settings.edit();
    	EditText server = (EditText) findViewById(R.id.editText1);
        editor.putString("server", server.getText().toString());

        // Commit the edits!
        editor.commit();
    }
    
	class ConnectThread extends Thread {
		Context c;
		// AndroidStream androidStream;
		ConnectThread(Context c) {
			this.c=c;
			// this.androidStream = androidStream;
		}
		public void run() {
			try {

						//Thread t = new Thread(btcomm); t.start(); -> da is isAlive auf einmal nicht gesetzt
//				getDisplay().setCurrent(get_controllCanvas(btcomm));
				AndroidMain.btcomm.start();
				// update check:
				synchronized(AndroidMain.btcomm.connectedNotifyObject) {
					try {
						AndroidMain.btcomm.connectedNotifyObject.lock();
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
				if(!btcomm.connError()) {
					Intent i = new Intent(this.c, ControllAction.class);
					startActivity(i);
					// debugForm.setTitle("connected");
				} else {
					AndroidMain.stopConnection();
				}
				// TextView tv=(TextView)c.findViewById(R.id.textViewStatus);
				
			} catch(Exception e) {
// ############				System.out.println("commandAction Exception:"+e.toString());
				AndroidMain.btcommMessage=e.getMessage();
				AndroidMain.stopConnection();
			}
		}
	}
	
	/**
	 * wenn BTcomm connected ist oder so dann repaint
	 */
	static NotifyStatusChange notifyStatusChange = null;
	class NotifyStatusChange extends Thread {
		public void run() {
			// Debuglog.debugln("starting NotifyStatusChange");
			while(true && AndroidMain.btcomm != null) {
				try {
					synchronized(AndroidMain.btcomm.statusChange) {
						// Debuglog.debugln("starting NotifyStatusChange locked");
						AndroidMain.btcomm.statusChange.wait();
					}
					mHandler.post(mUpdateResults);
				} catch (InterruptedException ex) {
					break;
				}
			}
			synchronized(this) { // nachricht dass wir beendet sind
				this.notifyAll();
			}
		}
	}
    
	public void onButtonConnect(View view) {
		if(AndroidMain.btcomm != null) {
			AndroidMain.btcomm.close(true);
			AndroidMain.btcomm=null;
		}
		EditText server = (EditText) findViewById(R.id.editText1);
		AndroidMain.sserver = server.getText().toString();
		System.out.println("server:"+AndroidMain.sserver);
		//startActivityForResult(i, ACTIVITY_CREATE);
		
		this.restartConnection();
	}
	
    public void onButtonDisconnect(View view) {
    	AndroidMain.stopConnection();
    }
    
    public static void stopConnection() {
    	if(AndroidMain.btcomm != null) {
    		AndroidMain.btcomm.close(true);
    		AndroidMain.btcomm.interrupt();
    	}
    	AndroidMain.btcomm=null;
    	AndroidMain.notifyStatusChange.interrupt(); // thread killen damit das wait(); nicht für immer und ehwig auf einem alten btcomm object lauscht
    	AndroidMain.notifyStatusChange=null;
    }

    public void restartConnection() {
		if(AndroidMain.btcomm == null) {
			AndroidMain.btcomm = new BTcommThread(new AndroidStream(AndroidMain.sserver,3030));
			Debuglog.debugln("reconnecting to server");
			AndroidMain.notifyStatusChange=new NotifyStatusChange();
			AndroidMain.notifyStatusChange.start();
			AndroidMain.connectThread = new ConnectThread(this);
			connectThread.start();
		} else {
			Debuglog.debugln("restart conn - still connected");
		}
	}
    
    public void repaint() {
    	String text=BTcommThread.statusText[BTcommThread.STATE_DISCONNECTED];
    	if(AndroidMain.btcomm != null) {
    		text=BTcommThread.statusText[btcomm.connState];
    	}
    	if(AndroidMain.btcommMessage != null) {
    		text+=" ("+AndroidMain.btcommMessage+")";
    	}
		TextView tv=(TextView)this.findViewById(R.id.textViewStatus);
		tv.setText(text);
    }
    
    /**
     * foregroundActivities zeug:
     * wenn nach 2sekunden keine activity von uns wieder gestartet wurde btcomm killen
     */
    static Handler timingHandler = new Handler(); 
    public static void plusActivity() {
    	AndroidMain.foregroundActivities++;
    }
    public static void minusActivity() {
    	AndroidMain.foregroundActivities--;
    	timingHandler.removeCallbacks(checkBackgroundTask);
    	timingHandler.postDelayed(checkBackgroundTask, 2000);
    }
    public static void checkIfBackgrounded() {
    	if(AndroidMain.foregroundActivities==0) {
    		System.out.println("****************** no activity in foreground ********");
    		AndroidMain.stopConnection();
    	}
    }
    public static Runnable checkBackgroundTask = new Runnable() {
    	// @Override
    	public void run() {
    		checkIfBackgrounded();
    	}
    };
}
