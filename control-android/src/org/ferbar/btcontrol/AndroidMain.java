/*
 *  This file is part of btcontrol
 *
 *  Copyright (C) Christian Ferbar
 *
 *  btcontrol is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  btcontrol is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with btcontrol.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/*
 * Android Main Class
 */

package org.ferbar.btcontrol;
import java.io.IOException;
import java.net.InetAddress;
import java.util.Hashtable;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
// import android.graphics.Bitmap;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.StrictMode;
import android.util.Log;
import android.view.View;
// import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;
import protocol.MessageLayouts;

import javax.jmdns.JmDNS;
import javax.jmdns.NetworkTopologyDiscovery;
import javax.jmdns.ServiceEvent;
// import javax.jmdns.ServiceInfo;
import javax.jmdns.ServiceListener;

import org.ferbar.btcontrol.R;


/**
 * Main Activity:
 * mDNS scanner + connect dialog
 * 
 * @author chris
 *
 */
public class AndroidMain extends Activity {
	public static final String TAG = "btcontrol";
	public static final String PREFS_NAME = "btcontrol";
	
	static BTcommThread btcomm = null;
	static ConnectThread connectThread;
	static String btcommMessage = null;
	
	static int foregroundActivities=0;
	static String sserver; // server name
	
	android.net.wifi.WifiManager wifiManager;
	android.net.wifi.WifiManager.MulticastLock lock=null;
	private String bonjourType = "_btcontrol._tcp.local.";
	// TODO: das auf liste umbaun ArrayAdapter<AvailBonjourServiceItem> listAdapter=null;
	
	// private String bonjourType = "_workstation._tcp.local.";
	// private String bonjourType = "_ssh._tcp.local.";
	private JmDNS jmdns = null;
	private ServiceListener listener = null;
	// private ServiceInfo serviceInfo;
	
	// Need handler for callbacks to the UI thread
    final static Handler mHandler = new Handler();

	BroadcastReceiver myWifiReceiver = new BroadcastReceiver() {
		@Override
		// Hint: das wird beim App Start gleich aufgerufen, auch wenn sich nix ändert!
		public void onReceive(Context arg0, Intent arg1) {
			// TODO Auto-generated method stub
			NetworkInfo networkInfo = (NetworkInfo) arg1.getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);
			if(networkInfo.getType() == ConnectivityManager.TYPE_WIFI){
				// DisplayWifiState();
				AndroidMain.this.notifyUser(networkInfo.toString());
			}
			AndroidMain.this.setIPInterfaces();
		}
	};
    
    /*
    class AvailBonjourServiceItem {
    	public AvailBonjourServiceItem(String name, String ip) {
    		// super(name, img, speed, funcBits);
    		this.name=name;
    	}
    	String name;
    	String ip;
    }*/
	

    
    /**
     * mit dem wird netzwerk io im ui thread erlaubt. ist gefixt jetzt, brauch ma nimma
    public void disableNetworkOnMainThreadException() {
	    if (android.os.Build.VERSION.SDK_INT > 9) {
	        StrictMode.ThreadPolicy policy = 
	            new StrictMode.ThreadPolicy.Builder().permitAll().build();
	        StrictMode.setThreadPolicy(policy);
	    }
    }
    */
    
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
    	// this.disableNetworkOnMainThreadException();
    	
        UncaughtException.setGlobalUncaughtExceptionHandler(this);
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
		MessageLayouts messageLayouts = new MessageLayouts();
		try {
			messageLayouts.load();
			// MessageLayouts.dump();
		} catch (Exception e) {
			Toast.makeText(this, "error loading protocol", Toast.LENGTH_LONG).show();
			e.printStackTrace();
			this.finish();
			return;
		}
		SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
		EditText server = (EditText) findViewById(R.id.editText1);
		String text = settings.getString("server", "");
		server.setText(text);

		this.wifiManager = (android.net.wifi.WifiManager) getSystemService(android.content.Context.WIFI_SERVICE);
		// this.wifiManager.getWifiState()

// FIXME: E/ActivityThread(29021): Activity com.example.helloandroid.AndroidMain has leaked IntentReceiver com.example.helloandroid.AndroidMain$3@40555948 that was originally registered here. Are you missing a call to unregisterReceiver()?
// FIXME:		E/ActivityThread(29021): android.app.IntentReceiverLeaked: Activity com.example.helloandroid.AndroidMain has leaked IntentReceiver com.example.helloandroid.AndroidMain$3@40555948 that was originally registered here. Are you missing a call to unregisterReceiver()?


		// FIXME: da hats was !!!!!
    	this.registerReceiver(myWifiReceiver,
		         new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION));
    	
    }
    
    /**
     * wird beim app start 2* aufgerufen - vom wifi broadcast receiver und vom onResume
     */
    public synchronized void setIPInterfaces() {
    	InetAddress[] addresses = NetworkTopologyDiscovery.Factory.getInstance().getInetAddresses();
    	RadioGroup rg = (RadioGroup) this.findViewById(R.id.radioGroupIPs);
    	rg.removeAllViews();
    	rg.clearCheck();
    	final SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
    	String bonjourIPAddress=settings.getString("bonjourIPAddress", "");

    	int n=0;
    	for(InetAddress a : addresses) {
    		n++;
    		RadioButton b = new RadioButton(this);
    		String ip=a.getHostAddress();
    		b.setText(ip);
    		b.setId(n);
    		/*
    		b.setOnCheckedChangeListener(new OnCheckedChangeListener (){
				@Override
				public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
					if(isChecked) {
						SharedPreferences.Editor editor = settings.edit();
						editor.putString("bonjourIPAddress", buttonView.getText().toString());
						editor.commit();
						AndroidMain.this.initBonjour();
					}
				}
			});
			*/
    		rg.addView(b);
    		// If the view is checked before adding it to the parent, it will be impossible to uncheck it.
    		b.setChecked(ip.equals(bonjourIPAddress));
    	}
    	// System.out.println("****************** checked radio button id: "+rg.getCheckedRadioButtonId());
    	
        rg.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
             @Override
             public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
                 RadioButton rb = (RadioButton) radioGroup.findViewById(checkedId);
                 if(rb != null) {
					SharedPreferences.Editor editor = settings.edit();
					editor.putString("bonjourIPAddress", rb.getText().toString());
					editor.commit();
					AndroidMain.this.initBonjour();
                 }
             }
         });
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
        
        // unregister wifi broadcast receiver
        try {
        	this.unregisterReceiver(this.myWifiReceiver);
        } catch(IllegalArgumentException e) { // kA wie das geht dass das hin und wieder nicht registriert ist ....
        	Log.d(TAG, "unregisterReceiver failed: "+e.getMessage());
        }
    }
    
    Hashtable<String, String> mDNSHosts=new Hashtable<String,String>();
    
    @Override
    public void onResume() {
    	super.onResume();
    	
    	this.setIPInterfaces();
    	
    	this.lock = this.wifiManager.createMulticastLock("btcontrol.jmDNS.lock");
        this.lock.setReferenceCounted(true);
        this.lock.acquire();
       
        initBonjour();
    }
    
    /**
     * wird von onResume und radio button changed aufgerufen (=beim app start 2*)
     */
    public void initBonjour() {
    	// ned schön aber beim app start wird das von den callback dingsen aufgerufen.
    	if(this.lock == null) return;
    	
    	Thread initBonjourThread = new Thread() {
    	    @Override
    	    public void run() {
		        try {
		        	final SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
		        	String bonjourIPAddress=settings.getString("bonjourIPAddress", null);
		        	if(bonjourIPAddress != null) {
		        		System.setProperty("net.mdns.interface", bonjourIPAddress);
		        	}
					AndroidMain.this.jmdns = JmDNS.create();  // Achtung !!! im strict mode macht das im UI thread eine Network Exception!!!
					synchronized(AndroidMain.this.jmdns) {
						Log.i(TAG, "initBonjour.run()");
						AndroidMain.this.jmdns.addServiceListener(bonjourType, listener = new ServiceListener() {
				            public void serviceResolved(final ServiceEvent ev) {
				                notifyUser("Service resolved: "
				                         + ev.getInfo().getQualifiedName()
				                         + " port:" + ev.getInfo().getPort());
				                final EditText eHost=(EditText) findViewById(R.id.editText1);
				                final LinearLayout list=(LinearLayout) findViewById(R.id.linearLayoutBonjourServer);
				                runOnUiThread(new Runnable() {
				                	public void run() {
				                		String ip=ev.getInfo().getHostAddresses()[0];
				                		String hostname=ev.getInfo().getServer();
				                		if(mDNSHosts.containsKey(hostname)) { // hoffentlich macht der da ein compare.to ...
				                			return;
				                		}
				                		mDNSHosts.put(hostname, ip);
				                		RelativeLayout horizontal=new RelativeLayout(list.getContext());
				                		horizontal.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
				                		//horizontal.setBackgroundResource(R.color.white);
				                		
				                		RelativeLayout.LayoutParams lpView = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
				                		lpView.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
				    	                TextView tvIP=new TextView(list.getContext());
				    	                tvIP.setText(ip);
				    	                //tvIP.setGravity(Gravity.CENTER);
				    	                tvIP.setLayoutParams(lpView);
				                		horizontal.addView(tvIP);
				                		
				                		TextView tvTxt=new TextView(list.getContext());
				                		tvTxt.setText(ev.getInfo().getName());
				                		//tvTxt.setBackgroundResource(R.color.yellow);
				                		//tvTxt.setGravity(Gravity.CENTER);
				                		//lpView.gravity=Gravity.RIGHT;
				                		lpView = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
				                		lpView.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
				                		tvTxt.setLayoutParams(lpView);
				                		horizontal.addView(tvTxt);
				                		list.addView(horizontal);
				                		
				                		final Button bHost=new Button(list.getContext());
				                		bHost.setText(hostname);
				                		bHost.setOnClickListener(new View.OnClickListener() {
				                			public void onClick(View v) {
				                				eHost.setText(mDNSHosts.get(bHost.getText()));
				                			}
				                		});
				                		list.addView(bHost);
				                	}
				                });
				            }
				            public void serviceRemoved(ServiceEvent ev) {
				                notifyUser("Service removed: " + ev.getInfo().getQualifiedName()+ " port:" + ev.getInfo().getPort());
				            }
				            public void serviceAdded(ServiceEvent event) {
				                // Required to force serviceResolved to be called again
				                // (after the first search)
				            	AndroidMain.this.jmdns.requestServiceInfo(event.getType(), event.getName(), 1);
				            }
				        });
						AndroidMain.this.runOnUiThread(new Runnable() {
				    		public void run() {
				    			TextView t=(TextView) AndroidMain.this.findViewById(R.id.textViewInfo);
				    			try {
				    				// FIXME: da war mal eine NullPointer Exception. ka wie das geht ( AndroidMain.this.jmdns )
				    				Log.i(TAG, "AndroidMain.this.jmdns.getInetAddress():"+AndroidMain.this.jmdns.getInetAddress());
									t.setText(AndroidMain.this.getText(R.string.main_found_serives) + "(" + AndroidMain.this.jmdns.getInetAddress().getHostAddress() + ")");
								} catch (Exception e) {
									Log.e(TAG,"initBonjour.run", e);
									t.setText("Exception:" + e.getMessage());
								}
				    		}
						});
					} // synchronized
				} catch (IOException e) {
					e.printStackTrace();
					AndroidMain.this.notifyUser("exception:" + e.getMessage());
				}
    	    }
    	};
    	initBonjourThread.start();
    }
    
    @Override
    public void onPause() {
    	super.onPause();
    	
    	if(this.lock != null) {
    		this.lock.release();
    		this.lock=null;
    	}
    	
    	if(this.jmdns != null) {
    		synchronized(this.jmdns) {
	    		this.jmdns.removeServiceListener(bonjourType, listener);
	    		try {
					this.jmdns.close();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
	    		this.jmdns=null;
	    	}
    	}
    }
    
	static class ConnectThread extends Thread {
		ProgressDialog loadProgressDialog;
		// AndroidStream androidStream;
		ConnectThread(ProgressDialog loadProgressDialog) {
			this.loadProgressDialog=loadProgressDialog;
			// this.androidStream = androidStream;
		}
		public void run() {
			try {

						//Thread t = new Thread(btcomm); t.start(); -> da is isAlive auf einmal nicht gesetzt
//				getDisplay().setCurrent(get_controlCanvas(btcomm));
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
					Intent i = new Intent(this.loadProgressDialog.getContext(), ControlAction.class);
					this.loadProgressDialog.getContext().startActivity(i);
					synchronized(this.loadProgressDialog) {
						this.loadProgressDialog.dismiss();
						this.loadProgressDialog=null;
					}
					// debugForm.setTitle("connected");
				} else {
					AndroidMain.btcommMessage="btcomm connError (state: "+BTcommThread.statusText[AndroidMain.btcomm.connState]+")";
					AndroidMain.stopConnection();
				}
				// TextView tv=(TextView)c.findViewById(R.id.textViewStatus);
				
			} catch(Exception e) {
// ############				System.out.println("commandAction Exception:"+e.toString());
				AndroidMain.btcommMessage=e.getMessage();
				AndroidMain.stopConnection();
			}
			if(AndroidMain.notifyStatusChange != null) {
				AndroidMain.notifyStatusChange.interrupt();
				AndroidMain.notifyStatusChange=null;
			}
		}
	}
	
	/**
	 * wenn BTcomm connected ist oder so dann repaint
	 */
	static NotifyStatusChange notifyStatusChange = null;
	static class NotifyStatusChange extends Thread {
		Runnable repaint=null;
		NotifyStatusChange(Runnable repaint) {
			this.repaint=repaint;
		}
		public void run() {
			// Debuglog.debugln("starting NotifyStatusChange");
			while(true && AndroidMain.btcomm != null) {
				try {
					synchronized(AndroidMain.btcomm.statusChange) {
						// Debuglog.debugln("starting NotifyStatusChange locked");
						AndroidMain.btcomm.statusChange.wait();
					}
					mHandler.post(this.repaint);
				} catch (InterruptedException ex) {
					break;
				}
			}
			synchronized(this) { // nachricht dass wir beendet sind
				this.notifyAll();
			}
		}
	}

	public static ProgressDialog createConnectingProgressDialog(Context c) {
		// warte dialog auf:
		final ProgressDialog loadProgressDialog = new ProgressDialog(c);
	    loadProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
	    loadProgressDialog.setMessage("Connecting...");
	    loadProgressDialog.setCancelable(false);
	    loadProgressDialog.setProgress(0);
	    loadProgressDialog.setMax(10);
	    loadProgressDialog.setButton("Abbrechen", new DialogInterface.OnClickListener() {
	    	public void onClick(DialogInterface dialog, int which) {
	    		AndroidMain.stopConnection();
	    		synchronized(loadProgressDialog) {
	    			loadProgressDialog.dismiss();
	    			// loadProgressDialog=null;
	    		}
	    	}
	
	    });
	    loadProgressDialog.show();
	    return loadProgressDialog;
	}
    
	public void onButtonConnect(View view) {
		if(AndroidMain.btcomm != null) {
			synchronized(AndroidMain.btcomm) {
				AndroidMain.btcomm.close(true);
				AndroidMain.btcomm=null;
			}
		}
		EditText server = (EditText) findViewById(R.id.editText1);
		AndroidMain.sserver = server.getText().toString();
		Log.d(TAG, "server:"+AndroidMain.sserver);
		//startActivityForResult(i, ACTIVITY_CREATE);
		
		final ProgressDialog loadProgressDialog=AndroidMain.createConnectingProgressDialog(this);
        
    	// Create runnable for posting
        Runnable mUpdateProgressDialog = new Runnable() {
            public void run() {
                AndroidMain.repaint(loadProgressDialog, (TextView)AndroidMain.this.findViewById(R.id.textViewStatus));
            }
        };
		AndroidMain.restartConnection(loadProgressDialog, mUpdateProgressDialog);
	}
	
    public void onButtonDisconnect(View view) {
    	AndroidMain.stopConnection();
    }
    
    public static void stopConnection() {
    	if(AndroidMain.btcomm != null) {
    		synchronized(AndroidMain.btcomm) {
    			AndroidMain.btcomm.close(true);
    			AndroidMain.btcomm.interrupt();
    	    	AndroidMain.btcomm=null;
    		}
    	}
    	
    	if(AndroidMain.notifyStatusChange != null) { // ... wenn schon gestoppt isses null
    		AndroidMain.notifyStatusChange.interrupt(); // thread killen damit das wait(); nicht für immer und ehwig auf einem alten btcomm object lauscht
    		AndroidMain.notifyStatusChange=null;
    	}
    }
    

    public static void restartConnection(ProgressDialog loadProgressDialog, Runnable repaint) {
		if(AndroidMain.btcomm == null) {
			AndroidMain.btcomm = new BTcommThread(new AndroidStream(AndroidMain.sserver,3030));
			Debuglog.debugln("reconnecting to server");
			AndroidMain.notifyStatusChange=new NotifyStatusChange(repaint);
			AndroidMain.notifyStatusChange.start();
			AndroidMain.connectThread = new ConnectThread(loadProgressDialog);
			AndroidMain.connectThread.start();
		} else {
			Debuglog.debugln("restart conn - still connected");
		}
	}
    
    public static void repaint(ProgressDialog loadProgressDialog, TextView tvInfo) {
    	String text=BTcommThread.statusText[BTcommThread.STATE_DISCONNECTED];
    	if(AndroidMain.btcomm != null) {
    		text=BTcommThread.statusText[btcomm.connState];
    		if(AndroidMain.btcomm.stateMessage != null && AndroidMain.btcomm.stateMessage.length() > 0)
    			text+=" ("+AndroidMain.btcomm.stateMessage+")";
    	}
    	if(AndroidMain.btcommMessage != null) {
    		text+=" ("+AndroidMain.btcommMessage+")";
    	}
		if(tvInfo != null) {
			tvInfo.setText(text);
		}
		if(loadProgressDialog != null) {
			synchronized(loadProgressDialog) {
				if(AndroidMain.btcomm != null) {
					loadProgressDialog.setProgress(btcomm.connState);
					loadProgressDialog.setMessage(text);
				} else {
					loadProgressDialog.setProgress(0);
					loadProgressDialog.setMessage("no connection");
				}
			}
		}
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
    		Log.d(AndroidMain.TAG, "****************** no activity in foreground ********");
    		AndroidMain.stopConnection();
    	}
    }
    public static Runnable checkBackgroundTask = new Runnable() {
    	// @Override
    	public void run() {
    		checkIfBackgrounded();
    	}
    };
    
    public void notifyUser(final String text) {
    	Log.i(TAG,"NotifyUser:" + text);
    	this.runOnUiThread(new Runnable() {
    		public void run() {
    	    	makeToast(text);
    		}
    	});
    }
    public void makeToast(String text) {
    	Toast.makeText(this, text, Toast.LENGTH_SHORT).show();
    }
}
