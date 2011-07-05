/**
 * das dings da ist für user eingabe zuständig
 * TODO: schieber als input verwenden
 * TODO: sencommand in eine func
 * TODO: mehrfachsteuerung
 * 
 */

package com.example.helloandroid;

import java.io.InputStream;
import java.util.Hashtable;
import java.util.Timer;
import java.util.TimerTask;


import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
// import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.ContextMenu;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewParent;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.EditText;
import android.widget.Toast;
// import android.widget.ToggleButton;
import protocol.FBTCtlMessage;
import protocol.MessageLayouts;
import btcontroll.AndroidStream;
import btcontroll.BTcommThread;
import btcontroll.Debuglog;

public class ControllAction extends Activity implements BTcommThread.Callback, OnSeekBarChangeListener {
	private static final int ACTIVITY_SELECT_LOK=0;
	String infoMsg="";
	int currAddr=0;
	
	// damit sich der bildschirmschoner nicht einschaltet:
	PowerManager.WakeLock powerManager_wl;
	String [] funcNames=null;
	boolean [] funcStates=null; // beim repaint wird das geupdatet  

	// zuordnung adresse -> lokbezeichnung,bild
	static class AvailLocosListItem {public String name; public Bitmap img; public int speed; public int funcBits;
		public AvailLocosListItem(String name, Bitmap img, int speed, int funcBits) {
			this.name=name; this.img=img; this.speed=speed; this.funcBits=funcBits;} }
	static public Hashtable<Integer, AvailLocosListItem> availLocos=new Hashtable<Integer, AvailLocosListItem>();
	// private Hashtable imgCache=new Hashtable();
	
	// Need handler for callbacks to the UI thread
    final Handler mHandler = new Handler();

    // Create runnable for posting
    final Runnable mUpdateResults = new Runnable() {
        public void run() {
            repaint();
        }
    };
    MenuItem powerMenuItem=null;
    int powerMenuItemState=0;
    final Runnable mUpdatePowerMenuItemOn = new Runnable() {
    	public void run() {
    		// MenuItem power = (MenuItem) findViewById(R.id.menu_Power);
    		if(powerMenuItem != null)
    			powerMenuItem.setIcon(R.drawable.ic_power_on);
    	}
    };
    final Runnable mUpdatePowerMenuItemOff = new Runnable() {
    	public void run() {
    		// MenuItem power = (MenuItem) findViewById(R.id.menu_Power);
    		if(powerMenuItem != null)
    			powerMenuItem.setIcon(R.drawable.ic_power_off);
    	}
    };
    
    
    SeekBar seekBarDirection;
    int [] viewFunctions={R.id.bF0, R.id.bF1, R.id.bF2, R.id.bF3, R.id.bF4, R.id.bF5, R.id.bF6, R.id.bF7, R.id.bF8, R.id.bF9};

	
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
	    super.onCreate(savedInstanceState);
		PowerManager pm = (PowerManager) getSystemService(ControllAction.POWER_SERVICE);
		powerManager_wl = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK, "My Tag");
		
	    if(savedInstanceState != null) {
	    	this.currAddr=savedInstanceState.getInt("currAddr");
	    }
        setContentView(R.layout.controll);
        
        
        if(this.currAddr == 0) {
        	// lok liste laden
        	Intent i = new Intent(this, ControllListAction.class);
        	startActivityForResult(i, ACTIVITY_SELECT_LOK);
        }
		
		this.seekBarDirection = (SeekBar)findViewById(R.id.seekBarDirection);
        this.seekBarDirection.setOnSeekBarChangeListener(this);
        this.update_btcomm();
        
        // TODO: geht das ned vielleicht gleich übers xml ?
        for(int i=0; i < this.viewFunctions.length; i++) {
        	ImageButton ib=(ImageButton) this.findViewById(this.viewFunctions[i]);
        	registerForContextMenu(ib);
        }
	}
	
	@Override
	public void onResume() {
		super.onResume();
		System.out.println("ControllAction::onResume");
		AndroidMain.plusActivity();
		powerManager_wl.acquire();
	}
	@Override
	public void onPause() {
		super.onPause();
		System.out.println("ControllAction::onPause isFinishing:"+this.isFinishing());
		AndroidMain.minusActivity();
		powerManager_wl.release();
		// TODO: alle threads stoppen

	}
	@Override
	public void onSaveInstanceState(Bundle outState) {
		outState.putInt("currAddr", this.currAddr);
	}
	/*
	@Override
	public void onStop() {
		super.onStop();
		// @todo: btcom zumachen (ist die activity gepaused oder das ganze prog?)
		System.out.println("ControllAction::onStop");
		// AndroidMain.stopConnection();
	}
	@Override
	public void onStart() {
		super.onStart();
		System.out.println("ControllAction::onStart");
	}
	@Override
	public void onDestroy() {
		super.onStart();
		System.out.println("ControllAction::onDestroy");
	} */

	/**
	 * lok ausgewählt
	 */
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
        super.onActivityResult(requestCode, resultCode, intent);
        if(intent != null) {
        	this.currAddr=intent.getIntExtra("currAddr", -1);
        	// funcNames einlesen:
	    	FBTCtlMessage msg = new FBTCtlMessage();
	    	try {
				msg.setType(MessageLayouts.messageTypeID("GETFUNCTIONS"));
		    	msg.get("addr").set(this.currAddr);
				AndroidMain.btcomm.addCmdToQueue(msg,this);
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
	        this.repaint();
        }
    }
    
/* ====================================================================================================================================
	menü zeug
 */
    
    @Override
	public boolean onPrepareOptionsMenu(Menu menu) {
    	FBTCtlMessage msg = new FBTCtlMessage();
    	try {
    		AvailLocosListItem lok=ControllAction.availLocos.get(this.currAddr);
        	if((lok != null) && (lok.speed != 0)) {
    			msg.setType(MessageLayouts.messageTypeID("STOP"));
    	    	msg.get("addr").set(this.currAddr);
    			AndroidMain.btcomm.addCmdToQueue(msg,this);
    		}
    		powerMenuItem = (MenuItem) menu.findItem(R.id.menu_Power);
    		msg = new FBTCtlMessage();
			msg.setType(MessageLayouts.messageTypeID("POWER"));
	    	msg.get("value").set(-1);
			AndroidMain.btcomm.addCmdToQueue(msg,this);
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return true;
    	
    }
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.controll_menu, menu);
        return true;
    }
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle item selection
        switch (item.getItemId()) {
        case R.id.menu_selectLok:
        	Intent i = new Intent(this, ControllListAction.class);
        	startActivityForResult(i, ACTIVITY_SELECT_LOK);
        	powerMenuItem=null;
            return true;
        case R.id.menu_Power:
            //showHelp();
        	FBTCtlMessage msg = new FBTCtlMessage();
			try {
				msg.setType(MessageLayouts.messageTypeID("POWER"));
		    	msg.get("value").set(powerMenuItemState==0 ? 1 : 0);
				AndroidMain.btcomm.addCmdToQueue(msg,this);
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
	    	powerMenuItem=null;
            return true;
        case R.id.menu_functions: {
        	//List items

        	// final CharSequence[] items = {"Milk", "Butter", "Cheese"};

        	//Prepare the list dialog box
        	AlertDialog.Builder builder = new AlertDialog.Builder(this);
        	AvailLocosListItem lok=ControllAction.availLocos.get(this.currAddr);
        	if(lok == null) {
        		Toast.makeText(this,"keine lok", Toast.LENGTH_LONG).show();
        		return true;
        	}
        	//Set its title
        	builder.setTitle("Funktionen von "+lok.name);
        	
        	builder.setNeutralButton("done", new DialogInterface.OnClickListener() {
        		 // Click listener on the neutral button of alert box
                public void onClick(DialogInterface dialog, int arg1) {

                    // The neutral button was clicked
                    // Toast.makeText(getApplicationContext(), "'OK' button clicked", Toast.LENGTH_LONG).show();
                	dialog.dismiss();
                }
            });
// TODO: namen schön machen
        	builder.setMultiChoiceItems(this.funcNames, this.funcStates,  new DialogInterface.OnMultiChoiceClickListener() {
        		// Click listener

        		public void onClick(DialogInterface dialog, int item, boolean on) {

        	        // Toast.makeText(getApplicationContext(), funcNames[item], Toast.LENGTH_SHORT).show();

    	        	FBTCtlMessage msg = new FBTCtlMessage();
    	        	try {
        	        	msg.setType(MessageLayouts.messageTypeID("SETFUNC"));
        	        	msg.get("funcnr").set(item);
        	        	msg.get("value").set(on ? 1 : 0);
        	        	msg.get("addr").set(currAddr);
						AndroidMain.btcomm.addCmdToQueue(msg); // TODO: callback handler vom ControllAction aufrufen
					} catch (Exception e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
        	    }
        	}
        	);

        	AlertDialog alert = builder.create();

        	//display dialog box

        	alert.show(); }
        	return true; 
        case R.id.menu_help: {
        	  // Create the alert box
            AlertDialog.Builder alertbox = new AlertDialog.Builder(this);
            alertbox.setTitle("Hilfe");
            // Set the message to display
            alertbox.setMessage("back taste(kurz) =power off\n" +
            	"Menu = aktueller zug stop (TODO)\n"+
            	"im menü wird aktueller power-status richtig angezeigt"); // TODO

            // Add a neutral button to the alert box and assign a click listener
            alertbox.setNeutralButton("Ok", new DialogInterface.OnClickListener() {

                // Click listener on the neutral button of alert box
                public void onClick(DialogInterface arg0, int arg1) {

                    // The neutral button was clicked
                    // Toast.makeText(getApplicationContext(), "'OK' button clicked", Toast.LENGTH_LONG).show();
                }
            });

             // show the alert box
            alertbox.show(); }
        	return true;
        case R.id.menu_POM:
        	// Context mContext = getApplicationContext();
    		AvailLocosListItem lok=ControllAction.availLocos.get(this.currAddr);
    		if(lok != null) {
            	final Dialog dialog = new Dialog(this);

            	dialog.setContentView(R.layout.pom);
            	dialog.setTitle("Programming on the Main");

            	// nettes bild + lokname setzen
            	TextView tv = (TextView) dialog.findViewById(R.id.textView1);
            	tv.setText(lok.name);
            	ImageView iv = (ImageView) dialog.findViewById(R.id.imageViewLok);
    			iv.setImageBitmap(lok.img);
    			
    			// onChange -> valueBits updaten
    		    ((EditText)dialog.findViewById(R.id.editTextValue)).addTextChangedListener(new TextWatcher() {
    		        public void afterTextChanged(Editable s) {
    		        	View v=(View)(EditText) dialog.findViewById(R.id.editTextValue);
		        		String value=s.toString();
		        		int vi=Integer.parseInt(value);
		        		if(vi > 255) {
		        			v.setBackgroundColor(Color.RED);
		        		} else {
		        			// v.setBackgroundResource(0);
		        			v.setBackgroundColor(Color.WHITE);  // TODO: da den normalen background wieder reintun
	    		        	if(v.hasFocus()) {
	    		        		String valueBits="";
	    		        		for(int i=0; i < 8; i++) {
	    		        			valueBits=(((vi >> i) & 1) == 1 ? 1 : 0 ) + valueBits;
	    		        		}
	    		        		EditText editTextValueBits=(EditText) dialog.findViewById(R.id.editTextValueBits);
	    		        		editTextValueBits.setText(valueBits);
    		        		}
    		        	}
    		        }
    		        public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
    		        public void onTextChanged(CharSequence s, int start, int before, int count) { }
    		    });
    			// onChange -> value updaten
    		    ((EditText)dialog.findViewById(R.id.editTextValueBits)).addTextChangedListener(new TextWatcher() {
    		        public void afterTextChanged(Editable s) {
    		        	View v=(View)(EditText) dialog.findViewById(R.id.editTextValueBits);
    		        	if(v.hasFocus()) {
    		        		String valueBits=s.toString();
    		        		int vbi;
    		        		try{
    		        			vbi=Integer.parseInt(valueBits,1);
    		        		} catch(Exception e) {
    		        			vbi=0;
    		        		}
    		        		String value=""+vbi;
    		        		EditText editTextValueBits=(EditText) dialog.findViewById(R.id.editTextValue);
    		        		editTextValueBits.setText(value);
    		        	}
    		        }
    		        public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
    		        public void onTextChanged(CharSequence s, int start, int before, int count) { }
    		    });
		        ((Button)dialog.findViewById(R.id.buttonGo)).setOnClickListener(new View.OnClickListener() {
		        	public void onClick(View v) {
		        		try {
		        			FBTCtlMessage msg = new FBTCtlMessage();
			    			msg.setType(MessageLayouts.messageTypeID("POWER"));
			    	    	msg.get("value").set(-1);
			    	    	FBTCtlMessage reply = AndroidMain.btcomm.execCmd(msg);
			    			Toast.makeText(v.getContext(), "gesendet", Toast.LENGTH_LONG).show();
			    		} catch (Exception e) {
			    			// TODO Auto-generated catch block
			    			e.printStackTrace();
			    		}

		        	}
		        });
            	dialog.setOwnerActivity(this);
            	dialog.show();
    		} else {
    			Toast.makeText(this,"keine lok", Toast.LENGTH_LONG).show();
    		}
        	return true;
        default:
        	powerMenuItem=null;
            return super.onOptionsItemSelected(item);
        }
    }
    
/* ====================================================================================================================================
	Func Display, wird bei einem long press aufgerufen
 */
    @Override
    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenu.ContextMenuInfo menuInfo){
    	// System.out.println("onCreateContextMenu");
    	// TODO: da was sinnvolles tun ;-)
    	int viewid=view.getId();
    	int func = -1;
		for(int i=0; i < this.viewFunctions.length; i++) {
			if(viewid == this.viewFunctions[i]) {
				func=i;
			}
		}
		if(func != -1) {
			String funcname=this.funcNames[func];
			if(funcname.length() > 0)
				funcname=funcname.substring(1);
		}

    }
/* ====================================================================================================================================
	UI/button
 */
    /// util damit man die absolute pos von einem view rausbekommt
    private int getRelativeLeft(View myView){
        if(myView.getParent()==myView.getRootView())
            return myView.getLeft();
        else {
            return myView.getLeft() + getRelativeLeft( (View) myView.getParent());
        }
    }


    private int getRelativeTop(View myView){
        if(myView.getParent()==myView.getRootView())
            return myView.getTop();
        else
            return myView.getTop() + getRelativeTop((View) myView.getParent());
    }
    
	/**
	 * als callback funk im .xml definiert
	 * @param view
	 */
	public void onClickButton(View view) {
		FBTCtlMessage msg = new FBTCtlMessage();
		try {
			switch(view.getId()) {
			case R.id.buttonBREAK:
				msg.setType(MessageLayouts.messageTypeID("BREAK"));
				break;
			case R.id.buttonACC:
				msg.setType(MessageLayouts.messageTypeID("ACC"));
				break;
			case R.id.seekBarDirection:
				if(Math.abs(availLocos.get(this.currAddr).speed) > 1) {
					System.out.println("error: changing dir only when stopped ("+availLocos.get(this.currAddr).speed+")");
					// TODO: message ausgeben
					return;
				}
				msg.setType(MessageLayouts.messageTypeID("DIR"));
				int val=0;
				SeekBar sb=(SeekBar) view;
				val=(sb.getProgress() == 0) ? -1 : 1;
				int currDir=availLocos.get(this.currAddr).speed < 0 ? -1 : 1;
				if(val == currDir) return; // nur senden wenn sichs geändert hat
				msg.get("dir").set(val);
				break;
			default:
				// check ob Fx taste gedrückt wurde:
				int func=-1;
				int viewid=view.getId();
				for(int i=0; i < this.viewFunctions.length; i++) {
					if(viewid == this.viewFunctions[i]) {
						func=i;
					}
				}
				if(func != -1) {
					String funcname=this.funcNames[func];
					if(funcname.length() > 0)
						funcname=funcname.substring(1);
					// Toast.makeText(getBaseContext(), funcname, Toast.LENGTH_SHORT).show();
					Toast toast = Toast.makeText(this, funcname, Toast.LENGTH_LONG);
					toast.getView().buildDrawingCache(true); // resolveSize(size, measureSpec);
					int top=this.getRelativeTop(view)-view.getHeight(); // view=button - stimmt zwar nicht ganz aber kann ned so daneben sein
					int left=this.getRelativeLeft(view);
					// toast.setGravity(Gravity.TOP|Gravity.LEFT,this.getRelativeLeft(v),this.getRelativeTop(v));
					toast.setGravity(Gravity.TOP|Gravity.LEFT,left,top);
					// toast.setText(funcname);
					// toast.setDuration(Toast.LENGTH_LONG);
					toast.show();

					msg.setType(MessageLayouts.messageTypeID("SETFUNC"));
					msg.get("funcnr").set(func);
					int value;

					value=this.funcStates[func] ? 0 : 1; // togglen
					msg.get("value").set(value);
					break;
				}
				// andere taste -> stop
				msg.setType(MessageLayouts.messageTypeID("STOP"));
				break;
			}
			msg.get("addr").set(this.currAddr);
			AndroidMain.btcomm.addCmdToQueue(msg,this);
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	/**
	 *  zeug für Key wird länger gedrückt
     *  also echte taste, derzeit nur für vol up/vol down
     */
	Timer timer;
	Object timerwait;
	TimerTask task=null;
	public class HoldDownKeyTask extends TimerTask {
		BTcommThread.Callback parent;
		private FBTCtlMessage cmd;	// fürs releaseKey public
		public HoldDownKeyTask(FBTCtlMessage cmd, BTcommThread.Callback parent) {
			this.parent=parent;
			this.cmd=cmd;
		}
		public void run() {
			try {
				if(AndroidMain.btcomm.nextMessage == null) // nur was neues in die queue schieben wenns leer is
					AndroidMain.btcomm.addCmdToQueue(cmd,parent);
			} catch (Exception e) {
				infoMsg="err: HoldDown: Exception "+e.toString();
			}
		}
	}

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if((keyCode == KeyEvent.KEYCODE_VOLUME_DOWN || keyCode == KeyEvent.KEYCODE_VOLUME_UP)) {
        	if(event.getRepeatCount()==0) { // beim ersten durchgang den timer starten
        		int repeatTimeout=250; // 4*/s senden
        		// repeatTimeout=1000; <- zum debuggen
	        	FBTCtlMessage msg = new FBTCtlMessage();
	        	try {
	    			msg.setType(MessageLayouts.messageTypeID(keyCode == KeyEvent.KEYCODE_VOLUME_DOWN ? "BREAK" : "ACC"));
	    	    	msg.get("addr").set(this.currAddr);
	    			AndroidMain.btcomm.addCmdToQueue(msg,this);
	    		} catch (Exception e) {
	    			// TODO Auto-generated catch block
	    			e.printStackTrace();
	    		}
	
	    		// init holdDownKey
				if(task != null) {
					timer.cancel();
				}
				timer = new Timer();
				timerwait=new Object();
				task = new HoldDownKeyTask(msg,this);
				timer.schedule(task, repeatTimeout, repeatTimeout);
	        	System.out.println("key down ("+event.toString()+")");
        	}
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }
    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if((keyCode == KeyEvent.KEYCODE_VOLUME_DOWN || keyCode == KeyEvent.KEYCODE_VOLUME_UP)) {
            // to your stuff here
        	System.out.println("key up ("+event.toString()+")");
        	timer.cancel();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    /**
     * strom abdrehn wenn back gedrück wurde
     */
    @Override
    public void onBackPressed() {
    	FBTCtlMessage msg = new FBTCtlMessage();
    	try {
			msg.setType(MessageLayouts.messageTypeID("POWER"));
	    	msg.get("value").set(0);
			AndroidMain.btcomm.addCmdToQueue(msg,this);
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		super.onBackPressed();
    }
//    @Override
//    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
//        if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) 
//        {
//            // to your stuff here
//            return true;
//        }
//        return super.onKeyLongPress(keyCode, event);
//    }
/* ====================================================================================================================================
	com
 */

    /**
     * wird für die ping - antworten aufgerufen
     */
	public void BTCallback(FBTCtlMessage reply) {
		try {
			if(reply == null) {
				throw new Exception("callback-timeout");
			} else {
				if(reply.isType("STATUS_REPLY")) {
					// debugForm.debug("pingReply rx ");
					int an=reply.get("info").getArraySize();
					// debugForm.debug("asize:"+an+" ");
					for(int i=0; i < an; i++) {
						int addr= reply.get("info").get(i).get("addr").getIntVal();
						
						System.out.println("addr:"+addr+
							" speed: "+reply.get("info").get(i).get("speed").getIntVal()+
							" func: "+reply.get("info").get(i).get("functions").getIntVal());
						AvailLocosListItem item=(AvailLocosListItem)ControllAction.availLocos.get(new Integer(addr));
						if(item != null) {
							item.speed=reply.get("info").get(i).get("speed").getIntVal();
							item.funcBits=reply.get("info").get(i).get("functions").getIntVal();
						} else {
							System.out.println("addr:"+addr+"not in list!!!");
						}
					}
				} else if(reply.isType("GETFUNCTIONS_REPLY")) {
					int n=reply.get("info").getArraySize();
			        this.funcNames=new String[n];
			        this.funcStates=new boolean[n];
					for(int i=0; i < n; i++) {
						this.funcNames[i]=reply.get("info").get(i).get("name").getStringVal();
					}
				} else if(reply.isType("POWER_REPLY")) {
		    		powerMenuItemState=reply.get("value").getIntVal();
					if(powerMenuItemState > 0) {
						mHandler.post(mUpdatePowerMenuItemOn);
					} else {
						mHandler.post(mUpdatePowerMenuItemOff);
					}
					return;
				} else {
					throw new Exception("didn't receive REPLY");
				}
			}
		} catch (Exception e) {
			this.infoMsg="callback err:"+e.getMessage();
		}
		
		mHandler.post(mUpdateResults); // TODO: runOnUiThread checken
	}

	/**
	 * wenn BTcomm connected ist oder so dann repaint
	 */
	NotifyStatusChange notifyStatusChange = null;
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
					return;
				}
			}
		}
	}
	
	public void update_btcomm()  {
		AndroidMain.btcomm.pingCallback=this;
		// this.
		this.infoMsg=null;

		if(this.notifyStatusChange != null) this.notifyStatusChange.interrupt();
		this.notifyStatusChange = new NotifyStatusChange();
		this.notifyStatusChange.start();
	}
	
	
	public void repaint() {
		TextView tv=(TextView)this.findViewById(R.id.textView1);
		tv.setText("currLok:"+this.currAddr);
		AvailLocosListItem item=ControllAction.availLocos.get(this.currAddr);
		if(item != null) {
			SeekBar seekBar = (SeekBar) findViewById(R.id.seekBar1);
			seekBar.setMax(255);
			seekBar.setProgress(Math.abs(item.speed));
			tv.setText(item.name);
			seekBar = (SeekBar) findViewById(R.id.seekBarDirection);
			int currProgress=seekBar.getProgress();
			int currDir=item.speed < 0 ? 0 : 2;
			if(currProgress != currDir) { // nur updaten wenn was anderes angezeigt wird als die lok grad hat
				this.lastDirProgress=-1;
				seekBar.setProgress(item.speed < 0 ? 0 : 2);
			}
			/*
			for(int i=0; i < this.viewFunctions.length; i++) {
				try {
					ToggleButton tb=(ToggleButton)this.findViewById(this.viewFunctions[i]);
					if(tb != null) {
						tb.setChecked(((1 << i) & item.funcBits) != 0);
					}
				} catch(Exception e) {
					
				}
			}
			*/
			ImageView iv=(ImageView)this.findViewById(R.id.imageViewLok);
			iv.setImageBitmap(item.img);
		}
		String title="btcontroll ";
		String info="";
		if(AndroidMain.btcomm != null) {
			info = BTcommThread.statusText[AndroidMain.btcomm.connState];
			title += info; 
			if(AndroidMain.btcomm.connState == BTcommThread.STATE_CONNECTED) {
				title += "("+((AndroidStream) AndroidMain.btcomm.BTStreamConnection).server+")";
			}
		}
		if(this.infoMsg != null) {
			title += " " + this.infoMsg;
		}
		this.setTitle(title);
		tv=(TextView)this.findViewById(R.id.textViewStatus);
		if(AndroidMain.btcomm != null && AndroidMain.btcomm.connState == BTcommThread.STATE_CONNECTED) {
			info=Debuglog.pingstat;
			tv.setBackgroundColor(Color.TRANSPARENT);
		} else {
			tv.setBackgroundColor(Color.RED);
		}
		tv.setText(info);
		
		
		// func buttons neu zeichnen TODO: nur nach änderung machen!!!
		if(this.funcNames != null) {
			int n=this.funcNames.length;
			for(int i=0; i < this.viewFunctions.length; i++) {
				this.funcStates[i]=((item.funcBits >> i) & 1) != 0;
				/*
				try {
					ToggleButton tb=(ToggleButton)this.findViewById(this.viewFunctions[i]);
					if(tb != null) {
						if(i < n) {
							String funcName=this.funcNames[i];
							char c=funcName.length() > 0 ? funcName.charAt(0) : 'x';
							switch(c) {
							case 's': tb.setBackgroundResource(R.drawable.toggle_button_sound);
								break;
							case 'l': tb.setBackgroundResource(R.drawable.toggle_button_light);
								break;
							default:
								tb.setBackgroundResource(android.R.drawable.btn_star);
							}
						} else {
							tb.setBackgroundResource(android.R.drawable.btn_star);
						}
					}
				} catch(Exception e) {
					*/
					ImageButton ib=(ImageButton)this.findViewById(this.viewFunctions[i]);
					if(ib != null) {
						if(i < n) {
							String funcName=this.funcNames[i];
							char c=funcName.length() > 0 ? funcName.charAt(0) : 'x';
							switch(c) {
							case 's':
								Drawable drawableSound=this.getResources().getDrawable(R.drawable.image_button_sound);
								ib.setImageDrawable(drawableSound);
								ib.setImageLevel(this.funcStates[i] ? 1 : 0);
								break;
							case 'l':
								Drawable drawableLight=this.getResources().getDrawable(R.drawable.image_button_light);
								ib.setImageDrawable(drawableLight);
								ib.setImageLevel(this.funcStates[i] ? 1 : 0);
								break;
							case 'p':
								Drawable drawablePantograph=this.getResources().getDrawable(R.drawable.image_button_pantograph);
								ib.setImageDrawable(drawablePantograph);
								ib.setImageLevel(this.funcStates[i] ? 1 : 0);
								break;
							default:
								//tb.setBackgroundResource(android.R.drawable.btn_star);
							}
						} else {
							// tb.setBackgroundResource(android.R.drawable.btn_star);
						}
					}
					
				// }
			}
		}
	}
	
	/**
	 * @param reply antwort auf GETLOCOS - da steht auch aktueller speed + func der loks drinnen
	 * @param selectList liste die filllistthread erzeugt hat (wegen den bildern)
	 * @throws java.lang.Exception
	 */
	public static void setAvailLocos(FBTCtlMessage reply) throws Exception {
		int n=reply.get("info").getArraySize();
		for(int i=0; i < n; i++) {
			Integer addr = new Integer(reply.get("info").get(i).get("addr").getIntVal());
			String name = reply.get("info").get(i).get("name").getStringVal();
			int speed = reply.get("info").get(i).get("speed").getIntVal();
			int func = reply.get("info").get(i).get("functions").getIntVal();
			String imgname=reply.get("info").get(i).get("imgname").getStringVal();
			Bitmap img=ControllAction.getImageCached(imgname);
			ControllAction.availLocos.put(addr,new AvailLocosListItem(name, img, speed,func));
		}
	}

	/**
	 * ladet ein bild vom server, btcomm muss gerade frei sein, blocking
	 * @param imageName
	 * @return bild
	 */
	static Hashtable<String,Bitmap> imgCache=new Hashtable<String,Bitmap>();
	private static Bitmap getImageCached(String imageName) {
		
		FBTCtlMessage msg = new FBTCtlMessage();
		Debuglog.debugln("getImageCached ("+imageName+")");
		if(imageName.length() == 0) {
			return null;
		}
		try {
			Bitmap cached = (Bitmap) imgCache.get(imageName);
			if(cached != null)
				return cached;
			msg.setType(MessageLayouts.messageTypeID("GETIMAGE"));
			msg.get("imgname").set(imageName);
			FBTCtlMessage reply=AndroidMain.btcomm.execCmd(msg);
			InputStream imageData=reply.get("img").getStringInputStream();
			if(imageData.available() > 0) {
				Debuglog.debugln("size:"+imageData.available()+" ");
				Bitmap ret;
				try {
					ret=BitmapFactory.decodeStream(imageData);
				} catch (Exception e) {
					Debuglog.debugln("getImageCached exception:"+e.toString());
					/*
					System.out.println("[0]="+imageData.charAt(0)+" [1]="+imageData.charAt(1)+
				" [2]="+imageData.charAt(2));
					byte data[]=imageData.getBytes();
					System.out.println("imgdump: ("+data.length+"bytes)");
					for(int i=0; i < data.length; i++) {
						System.out.print(data[i]+" ");
					}
					*/
					System.out.println("\n"+imageData);
					ret = null;
				}
				Debuglog.debugln("getimg: ["+ret.getWidth()+"/"+ret.getHeight()+"]");
				imgCache.put(imageName,ret);
				return ret;
			} else {
				return null;
			}
		} catch(Exception e) {
			Debuglog.debugln("getImageCached exception:"+e.toString());
			return null;
		}
	}
	

	int lastDirProgress=-1;
	public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
		if(!fromUser) return;

		if(seekBar.getProgress() == this.lastDirProgress) return; // wenn sich nix geändert hat gleich raus, bis die antwort kommt und repaint aufgerufen wird dauerts immer ein bissl;
		this.lastDirProgress=progress;
		if(seekBar.getProgress() == 1) {
			seekBar.setProgress(2); // wemma von 0-1 nimmt ist 0-99%=0, 100%=1
		}
		this.onClickButton(seekBar);
		this.repaint();
	}

	public void onStartTrackingTouch(SeekBar seekBar) {
	}

	public void onStopTrackingTouch(SeekBar seekBar) {
	}
	
    
}
