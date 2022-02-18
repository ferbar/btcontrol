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
 */
/**
 * das dings da ist für user Eingabe zuständig
 * TODO: sendcommand in eine func
 * TODO: mehrfachsteuerung
 * TODO: schieber wurde mit drücken gesetzt, v!= schieber -> nach ein paar sekunden schieber auf v setzen
 * TODO: ConnectionManager ins display einbaun, ping als balken alle 0,5s updaten
 * TODO: shift taste richten
 * TODO: dec datei beim POM anzeigen
 * fix 20120211: home taste -> power off
 */

package org.ferbar.btcontrol;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
//import android.content.Context;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
// import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Rect;
import android.graphics.Shader.TileMode;
// import android.graphics.LinearGradient;
// import android.graphics.Shader.TileMode;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.RectShape;
// import android.graphics.drawable.ShapeDrawable;
// import android.graphics.drawable.shapes.RectShape;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.os.Looper;
import android.os.PowerManager;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.ContextMenu;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.Window;
import android.view.WindowManager;
//import android.view.ViewParent;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.EditText;
import android.widget.Toast;
import android.widget.VerticalSeekBar;
import android.graphics.PorterDuff;

// import android.widget.ToggleButton;
import protocol.FBTCtlMessage;
import protocol.MessageLayouts;

import org.ferbar.btcontrol.AndroidStream;
import org.ferbar.btcontrol.BTcommThread;
import org.ferbar.btcontrol.Debuglog;

public class ControlAction extends Activity implements BTcommThread.Callback, OnSeekBarChangeListener {
	final static String TAG ="btcontrol.ControlAction";
	private static final int ACTIVITY_SELECT_LOK=0;
	String infoMsg="";
	static ArrayList<Integer> currSelectedAddr=new ArrayList<Integer>();
	
	// damit sich der bildschirmschoner nicht einschaltet:
	PowerManager.WakeLock powerManager_wl;
	String [] funcNames=null;
	boolean [] funcStates=null; // beim repaint wird das geupdatet  

	// zuordnung adresse -> lokbezeichnung,bild
	static class AvailLocosListItem {public String name; public Bitmap img; public int speed; public int funcBits;
		public AvailLocosListItem(String name, Bitmap img, int speed, int funcBits) {
			this.name=name; this.img=img; this.speed=speed; this.funcBits=funcBits;} }
	// sollte nach jedem reconnect auf null gesetzt werden!
	static public Hashtable<Integer, AvailLocosListItem> availLocos;
	// private Hashtable imgCache=new Hashtable();
	
	final int repeatTimeout=250; // 4*/s senden
	// repeatTimeout=1000; <- zum debuggen
	
    boolean cfg_seekBarWorkaround=false;
	
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
    		if(powerMenuItem != null) {
    			powerMenuItem.setIcon(R.drawable.ic_power_on);
    			powerMenuItem.setTitle("Power (on)");
    		}
    	}
    };
    final Runnable mUpdatePowerMenuItemOff = new Runnable() {
    	public void run() {
    		// MenuItem power = (MenuItem) findViewById(R.id.menu_Power);
    		if(powerMenuItem != null) {
    			powerMenuItem.setIcon(R.drawable.ic_power_off);
    			powerMenuItem.setTitle("Power (off)");
    		}
    	}
    };
    
    
    // SeekBar seekBarDirection;
    int [] viewFunctions={R.id.bF0, R.id.bF1, R.id.bF2, R.id.bF3, R.id.bF4, R.id.bF5, R.id.bF6, R.id.bF7, R.id.bF8, R.id.bF9, 
    		R.id.bF10, R.id.bF11, R.id.bF12, R.id.bF13, R.id.bF14, R.id.bF15, R.id.bF16, R.id.bF17, R.id.bF18,  R.id.bF19,
    		R.id.bF20, R.id.bF21, R.id.bF22, R.id.bF23, R.id.bF24, R.id.bF25, R.id.bF26, R.id.bF27, R.id.bF28
    };

	
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
//		getWindow().requestFeature(Window.FEATURE_ACTION_BAR);
	    super.onCreate(savedInstanceState);
	    ControlAction.availLocos=new Hashtable<Integer, AvailLocosListItem>(); // bt.reconnect = liste wird neu übertragen
	    
		PowerManager pm = (PowerManager) getSystemService(ControlAction.POWER_SERVICE);
		powerManager_wl = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK, "My Tag");
		
		if(AndroidMain.btcomm == null) {
			// FIXME: da war auch einmal eine nullpointer exception weil btcomm = null war (warum???)
			Toast.makeText(this, "error: comm=null", Toast.LENGTH_LONG).show();
			this.finish();
			return;
		} else {
			synchronized(AndroidMain.btcomm) {
				// für den fall dasses gelockt ist ...
			}
			if(AndroidMain.btcomm == null) {
				Toast.makeText(this, "error: comm=null, wurde gefladert", Toast.LENGTH_LONG).show();
				this.finish();
				return;
			}
		}
		
		/* das bringt nix weil wird nur restored wenns gekillt wird während eine action drüber liegt 
	    if(savedInstanceState != null) {
	    	this.currSelectedAddr=savedInstanceState.getIntegerArrayList("currAddr");
	    }
	    */
        setContentView(R.layout.control);
        
        
        if(ControlAction.currSelectedAddr.size() == 0) {
        	// lok liste laden
        	Intent i = new Intent(this, ControlListAction.class);
        	startActivityForResult(i, ACTIVITY_SELECT_LOK);
        } else { // schon eine lok ausgewählt dann funk liste laden:
    		try {
        		FBTCtlMessage msg = new FBTCtlMessage();
    			msg.setType(MessageLayouts.messageTypeID("GETLOCOS"));
				AndroidMain.btcomm.addCmdToQueue(msg,this);
	    		msg = new FBTCtlMessage();
				msg.setType(MessageLayouts.messageTypeID("GETFUNCTIONS"));
		    	msg.get("addr").set(ControlAction.currSelectedAddr.get(0));
				AndroidMain.btcomm.addCmdToQueue(msg,this);
    		} catch (Exception e) {
    			e.printStackTrace();
    			Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
    		}
        }
		
		//this.seekBarDirection = (SeekBar)findViewById(R.id.seekBarDirection);
        VerticalSeekBar seekBarSpeed = (VerticalSeekBar)findViewById(R.id.seekBarSpeed2);
        seekBarSpeed.setOnSeekBarChangeListener(this);
        this.setSeekBarWorkaround();
      
        
        // bunter hintergrund:
/*        LinearGradient test = new LinearGradient(0.f, 0.f, 300.f, 0.0f,  
        	      new int[] { 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
        	      0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF}, 
        	      null, TileMode.CLAMP);
        ShapeDrawable shape = new ShapeDrawable(new RectShape());
        shape.getPaint().setShader(test);
        seekBarSpeed.setProgressDrawable(shape);
        
        // hintergrund wegtun:
/* 
        int h=seekBarSpeed.getHeight();
        seekBarSpeed.setProgressDrawable(null);
        // seekBarSpeed.set
  //      seekBarSpeed.setSecondaryProgress(50);
*/
        seekBarSpeed.getProgressDrawable().setColorFilter(0x00000000, PorterDuff.Mode.MULTIPLY);
        
        
        this.update_btcomm();
        
        // TODO: geht das ned vielleicht gleich übers xml ?
        for(int i=0; i < this.viewFunctions.length; i++) {
        	ImageButton ib=(ImageButton) this.findViewById(this.viewFunctions[i]);
        	registerForContextMenu(ib);
        }
	}

    public void setSeekBarWorkaround() {
    	final VerticalSeekBar seekBarSpeed = (VerticalSeekBar)findViewById(R.id.seekBarSpeed2);
        cfg_seekBarWorkaround=getSharedPreferences(AndroidMain.PREFS_NAME, 0).getBoolean("seekbarWorkaround", false);
        if(cfg_seekBarWorkaround) {
        	seekBarSpeed.setOnTouchListener(new OnTouchListener() {
    			@Override
    			public boolean onTouch(View v, MotionEvent event) {
                    if(		/* event.getAction() == MotionEvent.ACTION_MOVE ||
                            event.getAction() == MotionEvent.ACTION_UP || */
                            event.getAction() == MotionEvent.ACTION_DOWN) {
                        // Rect seekBarThumbRect = seekBarSpeed.getThumb().getBounds();
                        int seekBarHeight = seekBarSpeed.getHeight();
                        int progress=(int) ((seekBarHeight-event.getY())*255/seekBarHeight);
                        Log.d(TAG, "---- seekbar touch: @"+event.getY()+"/"+seekBarHeight+" ="+progress);
                        seekBarSpeed.setProgress(progress);
                        /*
                        if(seekBarThumbRect.left - (seekBarThumbRect.right - seekBarThumbRect.left) / 2 < (Math.abs(seekBarHeight - event.getY())) &&
                                seekBarThumbRect.right + (seekBarThumbRect.right - seekBarThumbRect.left) / 2 > (Math.abs(seekBarHeight - event.getY())) &&
                                seekBarThumbRect.top < event.getX() &&
                                seekBarThumbRect.bottom > event.getX())
                                        return false;
                        */
                        ControlAction.this.onStartTrackingTouch(seekBarSpeed);
                    }
                    if(event.getAction() == MotionEvent.ACTION_UP) {
                    	ControlAction.this.onStopTrackingTouch(seekBarSpeed);
                    }
                    return true;
    			}
            });
        } else {
        	seekBarSpeed.setOnTouchListener(null);
        }
    }
	
	@Override
	public void onResume() {
		super.onResume();
		Log.d(TAG, "ControlAction::onResume");
		AndroidMain.plusActivity();
		powerManager_wl.acquire();
	}
	
	/**
	 * hint: beim 1. start wird sofort der Lok - Auswahl Dialog aufgerufen d.h. hier auch das onPause
	 */
	@Override
	public void onPause() {
		super.onPause();
		this.fullStop();
		PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		if (!pm.isScreenOn()) {
		   this.setPower(false);
		}
		Log.d(TAG, "ControlAction::onPause isFinishing:"+this.isFinishing());
		AndroidMain.minusActivity();
		powerManager_wl.release();
		// TODO: alle threads stoppen

	}
	/* - bringt nix
	@Override
	public void onSaveInstanceState(Bundle outState) {
		outState.putIntegerArrayList("currAddr", this.currSelectedAddr);
	}
	*/
	
	/*
	@Override
	public void onStop() {
		super.onStop();
		// @todo: btcom zumachen (ist die activity gepaused oder das ganze prog?)
		System.out.println("ControlAction::onStop");
		// AndroidMain.stopConnection();
	}
	@Override
	public void onStart() {
		super.onStart();
		System.out.println("ControlAction::onStart");
	}
	@Override
	public void onDestroy() {
		super.onStart();
		System.out.println("ControlAction::onDestroy");
	} */

	/**
	 * lok ausgewählt
	 */
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
        super.onActivityResult(requestCode, resultCode, intent);
        if(intent != null) {
        	ArrayList<Integer> tmp=intent.getIntegerArrayListExtra("currAddr");
        	if(tmp == null) { // programmfehler, darf ned passieren
        		throw new NullPointerException("onActivityResult: nullpointer");
        	}
        	ControlAction.currSelectedAddr = tmp;
        	// funcNames von der 1. Lok einlesen:
	    	FBTCtlMessage msg = new FBTCtlMessage();
	    	try {
				msg.setType(MessageLayouts.messageTypeID("GETFUNCTIONS"));
		    	msg.get("addr").set(ControlAction.currSelectedAddr.get(0));
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
    	try {
    		// aktuelle lok stoppen:
    		if(ControlAction.currSelectedAddr.size() > 0) {
	    		AvailLocosListItem lok=ControlAction.availLocos.get(ControlAction.currSelectedAddr.get(0));
	        	if((lok != null) && (lok.speed != 0)) {
	    			this.fullStop();
	    		}
    		}
        	// power status abfragen + icon setzen:
    		powerMenuItem = (MenuItem) menu.findItem(R.id.menu_Power);
    		FBTCtlMessage msg = new FBTCtlMessage();
			msg.setType(MessageLayouts.messageTypeID("POWER"));
	    	msg.get("value").set(-1);
			AndroidMain.btcomm.addCmdToQueue(msg,this);
			
			MenuItem seekbarWorkaroundMenuItem = (MenuItem) menu.findItem(R.id.menu_seekbarWorkaround);
			seekbarWorkaroundMenuItem.setTitle("Seekbar Workaround ("+(cfg_seekBarWorkaround ? "on" : "off" ) + ")"); 
		} catch (Exception e) {
			e.printStackTrace();
			Toast.makeText(this, "error checking power state: "+e.getMessage(), Toast.LENGTH_LONG).show();
		}
		return true;
    }
    
    @Override
    public void onOptionsMenuClosed(Menu menu) {
    	powerMenuItem=null;
    }
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.control_menu, menu);
        return true;
    }
    
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle item selection
        switch (item.getItemId()) {
        case R.id.menu_selectLok: { // lok auswahl starten
        	Intent i = new Intent(this, ControlListAction.class);
        	startActivityForResult(i, ACTIVITY_SELECT_LOK);
            return true; }
        case R.id.menu_Power: { // Power togglen
        	FBTCtlMessage msg = new FBTCtlMessage();
			try {
				msg.setType(MessageLayouts.messageTypeID("POWER"));
		    	msg.get("value").set(powerMenuItemState==0 ? 1 : 0);
				AndroidMain.btcomm.addCmdToQueue(msg,this);
			} catch (Exception e) {
				e.printStackTrace();
				Toast.makeText(this, "error changing power state: "+e.getMessage(), Toast.LENGTH_LONG).show();
			}
            return true; }
        case R.id.menu_functions: { // function dialog aufmachen
        	//List items

        	//Prepare the list dialog box
        	AlertDialog.Builder builder = new AlertDialog.Builder(this);
        	AvailLocosListItem lok=ControlAction.availLocos.get(ControlAction.currSelectedAddr.get(0));
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
// TODO: namen schön machen + icon anzeigen
        	builder.setMultiChoiceItems(this.funcNames, this.funcStates,  new DialogInterface.OnMultiChoiceClickListener() {
        		// Click listener

        		public void onClick(DialogInterface dialog, int item, boolean on) {

        	        // Toast.makeText(getApplicationContext(), funcNames[item], Toast.LENGTH_SHORT).show();

    	        	FBTCtlMessage msg = new FBTCtlMessage();
    	        	try {
        	        	msg.setType(MessageLayouts.messageTypeID("SETFUNC"));
        	        	msg.get("funcnr").set(item);
        	        	msg.get("value").set(on ? 1 : 0);
        	        	msg.get("addr").set(ControlAction.currSelectedAddr.get(0));
						AndroidMain.btcomm.addCmdToQueue(msg); // TODO: callback handler vom ControlAction aufrufen
					} catch (Exception e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
        	    }
        	}
        	);

        	AlertDialog alert = builder.create();

        	//display dialog box

        	alert.show();
        	return true; }
        case R.id.menu_help: {
        	  // Create the alert box
            AlertDialog.Builder alertbox = new AlertDialog.Builder(this);
            alertbox.setTitle("Hilfe");
            // Set the message to display
            alertbox.setMessage("Back(kurz) = power off, zurück zum Connect Dialog\n" +
            	"Home (kurz) = power off, App zu\n" +
            	"Menu = aktueller Zug stop\n" +
            	"Slider drücken = V wird langsam gesetzt\n" +
            	"Vol +/- gedrückt halten = beschleunigen/bremsen\n" +
            	"im menü wird aktueller power-status richtig angezeigt");

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
        case R.id.menu_POM_Neu: { // pom neu mit jmri dialog starten [test]
        	Intent i = new Intent(this, PomAction.class);
        	startActivityForResult(i, ACTIVITY_SELECT_LOK);
            return true; }
        case R.id.menu_POM: { // Programming on the main dialog
        	// Context mContext = getApplicationContext();
    		AvailLocosListItem lok=ControlAction.availLocos.get(ControlAction.currSelectedAddr.get(0));
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
		        		int vi=0;
		        		try{
		        			vi=Integer.parseInt(value);
			        		if(vi > 255) {
			        			v.getBackground().setColorFilter(0xFFFF0000, PorterDuff.Mode.MULTIPLY);
			        		} else {
    							v.getBackground().setColorFilter(null);
		    		        	if(v.hasFocus()) {
		    		        		String valueBits="";
		    		        		for(int i=0; i < 8; i++) {
		    		        			valueBits=(((vi >> i) & 1) == 1 ? 1 : 0 ) + valueBits;
		    		        		}
		    		        		EditText editTextValueBits=(EditText) dialog.findViewById(R.id.editTextValueBits);
		    		        		editTextValueBits.setText(valueBits);
	    		        		}
			        		}
		        		} catch(Exception e) {
							v.getBackground().setColorFilter(0xFFFF0000, PorterDuff.Mode.MULTIPLY);
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
    		        		try {
    		        			vbi=Integer.parseInt(valueBits,2);
    							v.getBackground().setColorFilter(null);
    		        		} catch(Exception e) {
    		        			vbi=0;
    							v.getBackground().setColorFilter(0xFFFF0000, PorterDuff.Mode.MULTIPLY);
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
		        	public void onClick(View goButton) {
		        		try {
		        			EditText v=(EditText) dialog.findViewById(R.id.editTextCV);
		        			String sCV=v.getText().toString();
		        			if(sCV.length()==0) {
		        				Toast.makeText(dialog.getContext(), "CV number empty", Toast.LENGTH_LONG).show();
		        				return;
		        			}
			    			int cv=Integer.parseInt(sCV);

			    			v=(EditText) dialog.findViewById(R.id.editTextValue);
		        			String sValue=v.getText().toString();
		        			int value;
		        			if(sValue.length() == 0) {
		        				value=-1;
		        			} else {
		        				value=Integer.parseInt(sValue);
		        			}
		        		
		        			FBTCtlMessage msg = new FBTCtlMessage();
			    			msg.setType(MessageLayouts.messageTypeID("POM"));
			    	    	msg.get("addr").set(ControlAction.currSelectedAddr.get(0));
			    	    	msg.get("cv").set(cv);
			    	    	msg.get("value").set(value);
			    	    	
			    	    	FBTCtlMessage reply = AndroidMain.btcomm.execCmd(msg);
			    	    	if(reply != null) {
			    	    		Toast.makeText(dialog.getContext(), "CV "+cv+" = "+value+" gesendet", Toast.LENGTH_LONG).show();
			    	    		v.setText(""+reply.get("value").getIntVal());
			    	    	}
			    		} catch (Exception e) {
			    			// TODO Auto-generated catch block
			    			Log.e(TAG, "POM Exception", e);
			    			Toast.makeText(dialog.getContext(), "POM Exception "+e.getMessage(), Toast.LENGTH_SHORT).show();
			    		}

		        	}
		        });
            	dialog.setOwnerActivity(this);
            	dialog.show();
    		} else {
    			Toast.makeText(this,"keine lok", Toast.LENGTH_LONG).show();
    		}
        	return true;
        }
        case R.id.menu_multi: { // Mehrfachsteuerung lok auswahl starten
        	Intent i = new Intent(this, ControlListAction.class);
        	i.putExtra("Mehrfachsteuerung", true);
        	startActivityForResult(i, ACTIVITY_SELECT_LOK);
        	return true;
        }
        case R.id.menu_exitNoPowerOff: {
        	Intent startMain = new Intent(Intent.ACTION_MAIN);
        	startMain.addCategory(Intent.CATEGORY_HOME);
        	startMain.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        	startActivity(startMain);
        	return true;
        }
        case R.id.menu_seekbarWorkaround: {
        	cfg_seekBarWorkaround=!cfg_seekBarWorkaround;
        	final SharedPreferences settings = getSharedPreferences(AndroidMain.PREFS_NAME, 0);
        	SharedPreferences.Editor editor = settings.edit();
			editor.putBoolean("seekbarWorkaround", cfg_seekBarWorkaround);
			editor.commit();
			this.setSeekBarWorkaround();
        	return true;
        }
        default:
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
     * @param pos slider auf eine pos setzen
     * @param acc 1 => acc, -1 =>break, 0 = pos verwenden
     */
    private void updateSpeedSlider(final int pos, final int acc) {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				SeekBar speedSeekBar=(SeekBar)findViewById(R.id.seekBarSpeed2);
				int pos2=pos;
				if(acc!=0) {
					AvailLocosListItem item=ControlAction.availLocos.get(ControlAction.currSelectedAddr.get(0));
					if(item != null) {
						pos2=Math.abs(item.speed)+acc*5;
					}
				}
				speedSeekBar.setProgress(pos2);
			}
		});
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
				this.setMessageAddrField(msg, "BREAK");
				updateSpeedSlider(0,-1);
				break;
			case R.id.buttonACC:
				this.setMessageAddrField(msg, "ACC");
				updateSpeedSlider(0,1);
				break;
			case R.id.buttonDirLeft:
			case R.id.buttonDirRight: {
				int main_addr=ControlAction.currSelectedAddr.get(0);
				if(Math.abs(availLocos.get(main_addr).speed) > 1) {
					Log.e(TAG, "error: changing dir only when stopped ("+availLocos.get(main_addr).speed+")");
					Toast.makeText(this, "geht nur bei v=0", Toast.LENGTH_LONG).show();
					return;
				}
				int val=(view.getId() == R.id.buttonDirRight) ? 1 : -1;
				int currDir=availLocos.get(main_addr).speed < 0 ? -1 : 1;
				if(val == currDir) { // wenn sich die richtugn nicht geändert hat dann stop senden
					this.setMessageAddrField(msg, "STOP");
				} else {
					this.setMessageAddrField(msg, "DIR");
					msg.get("dir").set(val);
				}
				break; }
			/* gibts nichtmehr
			case R.id.seekBarDirection:
				if(Math.abs(availLocos.get(this.currAddr).speed) > 1) {
					System.out.println("error: changing dir only when stopped ("+availLocos.get(this.currAddr).speed+")");
					Toast.makeText(this, "geht nur bei v=0", 0).show();
					return;
				}
				msg.setType(MessageLayouts.messageTypeID("DIR"));
				int val=0;
				SeekBar sb=(SeekBar) view;
				val=(sb.getProgress() == 0) ? -1 : 1;
				int currDir=availLocos.get(this.currAddr).speed < 0 ? -1 : 1;
				if(val == currDir) return; // nur senden wenn sichs geändert hat
				msg.get("dir").set(val);
				break; */
			default: {
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
					Toast toast = Toast.makeText(this, funcname, Toast.LENGTH_SHORT);
					toast.getView().buildDrawingCache(true); // resolveSize(size, measureSpec);
					int top=this.getRelativeTop(view)-view.getHeight(); // view=button - stimmt zwar nicht ganz aber kann ned so daneben sein
					int left=this.getRelativeLeft(view);
					// toast.setGravity(Gravity.TOP|Gravity.LEFT,this.getRelativeLeft(v),this.getRelativeTop(v));
					toast.setGravity(Gravity.TOP|Gravity.LEFT,left,top);
					// toast.setText(funcname);
					// toast.setDuration(Toast.LENGTH_LONG);
					toast.show();

					this.setMessageAddrField(msg, "SETFUNC");
					msg.get("funcnr").set(func);
					int value;

					value=this.funcStates[func] ? 0 : 1; // togglen
					msg.get("value").set(value);
					break;
				}
				// andere taste -> stop
				this.setMessageAddrField(msg, "STOP");
	    		this.updateSpeedSlider(0,0);
				break; }
			}
			
			// message für mehrfachsteuerung umbaun:

			AndroidMain.btcomm.addCmdToQueue(msg,this);
		} catch (Exception e) {
			// Toast.makeText(this, "error sending cmd:" + e.toString(), Toast.LENGTH_LONG).show();
			e.printStackTrace();
			this.handleBtCommException(e);
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
				if(AndroidMain.btcomm.nextMessage == null) { // nur was neues in die queue schieben wenns leer is
					AndroidMain.btcomm.addCmdToQueue(cmd,parent);
		    		updateSpeedSlider(0,cmd.isType("ACC") ? 1 : -1);
				}
			} catch (Exception e) {
				infoMsg="err: HoldDown: Exception "+e.toString();
			}
		}
	}

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch(keyCode) {
        case KeyEvent.KEYCODE_VOLUME_DOWN:
        case KeyEvent.KEYCODE_VOLUME_UP: {
	        	if(event.getRepeatCount()==0) { // beim ersten durchgang den timer starten
		        	FBTCtlMessage msg = new FBTCtlMessage();
		        	try {
		    	    	this.setMessageAddrField(msg, keyCode == KeyEvent.KEYCODE_VOLUME_DOWN ? "BREAK" : "ACC");
		    			AndroidMain.btcomm.addCmdToQueue(msg,this);
		    		} catch (Exception e) {
		    			// TODO Auto-generated catch block
		    			e.printStackTrace();
		    			this.handleBtCommException(e);
		    		}
		    		this.updateSpeedSlider(0,msg.isType("ACC") ? 1 : -1);
		
		    		// init holdDownKey
					if(task != null) {
						timer.cancel();
					}
					timer = new Timer();
					timerwait=new Object();
					task = new HoldDownKeyTask(msg,this);
					timer.schedule(task, repeatTimeout, repeatTimeout);
		        	Log.e(TAG, "key down ("+event.toString()+")");
	        	}
	            return true;
        	}
        case KeyEvent.KEYCODE_POWER: { // das funktioniert da nicht !! siehe onPause
        	this.fullStop();
        	this.setPower(false);
        	break;  // unten dann normales KEYCODE_POWER aufrufen
        }
        // das geht nur mit der onAttachedToWindow-anomalie:
        case KeyEvent.KEYCODE_HOME: {
        	this.fullStop();
        	break;
        }
        } // switch
        return super.onKeyDown(keyCode, event);
    }
    
	@Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if((keyCode == KeyEvent.KEYCODE_VOLUME_DOWN || keyCode == KeyEvent.KEYCODE_VOLUME_UP)) {
            // to your stuff here
        	Log.d(TAG, "key up ("+event.toString()+")");
        	if(timer != null) // wenn die app zum onKeyDown Zeitpunkt noch nicht gelaufen ist stürzts ab ...
        		timer.cancel();
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }
    
    /**
     * < Android 4.0: mit dem funktioniert dann das KeyEvent.KEYCODE_HOME (keyevent_home)
     *
    @Override
    public void onAttachedToWindow() {
        this.getWindow().setType(WindowManager.LayoutParams.TYPE_KEYGUARD);

        super.onAttachedToWindow();
    }
    */
    

    /**
     * strom abdrehn wenn back gedrück wurde
     */
    @Override
    public void onBackPressed() {
    	this.fullStop();
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
					Log.d(TAG, "BTCallback STATUS_REPLY");
					// debugForm.debug("pingReply rx ");
					int an=reply.get("info").getArraySize();
					// debugForm.debug("asize:"+an+" ");
					for(int i=0; i < an; i++) {
						int addr= reply.get("info").get(i).get("addr").getIntVal();
						
						Log.d(TAG, "STATUS_REPLY ["+i+"] addr:"+addr+
							" speed: "+reply.get("info").get(i).get("speed").getIntVal()+
							" func: "+reply.get("info").get(i).get("functions").getIntVal());
						AvailLocosListItem item=(AvailLocosListItem)ControlAction.availLocos.get(Integer.valueOf(addr));
						if(item != null) {
							item.speed=reply.get("info").get(i).get("speed").getIntVal();
							item.funcBits=reply.get("info").get(i).get("functions").getIntVal();
						} else {
							Log.d(TAG, "addr:"+addr+"not in list!!!");
						}
					}
				} else if(reply.isType("GETFUNCTIONS_REPLY")) {
					int n=reply.get("info").getArraySize();
			        this.funcNames=new String[n];
			        this.funcStates=new boolean[n];
					for(int i=0; i < n; i++) {
						this.funcNames[i]=reply.get("info").get(i).get("name").getStringVal();
					}
				} else if(reply.isType("GETLOCOS_REPLY")) {
					ControlAction.setAvailLocos(null, null, reply);
				} else if(reply.isType("POWER_REPLY")) {
		    		powerMenuItemState=reply.get("value").getIntVal();
					if(powerMenuItemState > 0) {
						this.runOnUiThread(mUpdatePowerMenuItemOn);
					} else {
						this.runOnUiThread(mUpdatePowerMenuItemOff);
					}
					return;
				} else {
					throw new Exception("didn't receive REPLY");
				}
			}
		} catch (Exception e) {
			this.infoMsg="callback err:"+e.getMessage();
		}
		
		this.runOnUiThread(mUpdateResults);
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
					ControlAction.this.runOnUiThread(mUpdateResults);
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
		// check obs uithread is
		if(Looper.getMainLooper().getThread() != Thread.currentThread()) {
			Log.d(TAG, "drawDealers wrong thread");
			throw new NullPointerException("error: not called from UI thread");
		}
		if(!this.hasWindowFocus()) {
			// Activity is im hintergrund
			return;
		}
		AvailLocosListItem item = null;
		if(ControlAction.currSelectedAddr.size() > 0) {
			int main_addr=ControlAction.currSelectedAddr.get(0);
			item=ControlAction.availLocos.get(main_addr);
		}
		if(item != null) {
			ProgressBar seekBar = (ProgressBar) findViewById(R.id.seekBarSpeed);
			// seekBar.setMax(255);
			seekBar.setProgress(Math.abs(item.speed));
			
			/* seekBarDir gibts nimma
			seekBar = (SeekBar) findViewById(R.id.seekBarDirection);
			int currProgress=seekBar.getProgress();
			int currDir=item.speed < 0 ? 0 : 2;
			if(currProgress != currDir) { // nur updaten wenn was anderes angezeigt wird als die lok grad hat
				this.lastDirProgress=-1;
				seekBar.setProgress(item.speed < 0 ? 0 : 2);
			}
			*/
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
			
			// Status zeile mit der aktuellefahrenden loks updaten:
			LinearLayout ll=(LinearLayout) this.findViewById(R.id.linearLayoutCurrLok);
			ll.removeAllViews();
			String text="";
			for(Integer addr : ControlAction.currSelectedAddr) {
				AvailLocosListItem currItem = ControlAction.availLocos.get(addr);
				ImageView iv = new ImageView(this);
				iv.setImageBitmap(currItem.img);
				ll.addView(iv);
				if(text.length() > 0) text+=", ";
				text+=currItem.name;
			}
			TextView tv=new TextView(this);
			tv.setId(R.id.textView1);
			tv.setText(text);
			ll.addView(tv);
			
			// richtungswechesl buttons nur bei speed=0 anzeigen, stop nur bei speed != 0 
			Button bSTOP = (Button) findViewById(R.id.buttonSTOP);
			Button bDirLeft = (Button) findViewById(R.id.buttonDirLeft);
			Button bDirRight = (Button) findViewById(R.id.buttonDirRight);
			Button bBREAK = (Button) findViewById(R.id.buttonBREAK);
			if(Math.abs(item.speed) <= 1) {
				bSTOP.setVisibility(View.GONE);
				bDirLeft.setVisibility(View.VISIBLE);
				bDirRight.setVisibility(View.VISIBLE);
				// bDirLeft.setBackgroundDrawable(bSTOP.getBackground());
				// bDirRight.setBackgroundDrawable(bSTOP.getBackground());
				if(item.speed >= 0) {
					bDirLeft.getBackground().setColorFilter(null);
					// MULTIPY sieht man bei > 4.0 fast nicht
					bDirRight.getBackground().setColorFilter(0xFF00FF00, PorterDuff.Mode.SRC);
				} else {
					bDirLeft.getBackground().setColorFilter(0xEEEE0000, PorterDuff.Mode.SRC);
					bDirRight.getBackground().setColorFilter(null);
				}
				bBREAK.setEnabled(false);
			} else {
				bSTOP.setVisibility(View.VISIBLE);
				bDirLeft.setVisibility(View.GONE);
				bDirRight.setVisibility(View.GONE);
				bBREAK.setEnabled(true);
			}

			
		} else {
			TextView tv=(TextView)this.findViewById(R.id.textView1);
			tv.setText("invalid Lok addr:"+ControlAction.currSelectedAddr.toString());
		}
		
		String title="btcontrol ";
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
		// Status leiste updaten:
		{
			TextView tv=(TextView)this.findViewById(R.id.textViewStatus);
			if(AndroidMain.btcomm != null && AndroidMain.btcomm.connState == BTcommThread.STATE_CONNECTED) {
				info=Debuglog.pingstat;
				tv.setBackgroundColor(Color.TRANSPARENT);
			} else {
				tv.setBackgroundColor(Color.RED);
			}
			tv.setText(info);
			// TODO: wlan empfang? ???
		}
		
		// func buttons neu zeichnen TODO: nur nach änderung machen!!!
		if(this.funcNames != null && item != null) { // add item != null, beim starten kanns sein dass die liste noch leer ist
			int n=this.funcNames.length;
			for(int i=0; i < this.viewFunctions.length; i++) {
				ImageButton ib=(ImageButton)this.findViewById(this.viewFunctions[i]);
				if(i >= n) { // lok hat weniger als 10 funktions -> button disablen
					// ib.setEnabled(false);
					ib.setVisibility(View.GONE);
					continue;
				}
				// ib.setEnabled(true);
				ib.setVisibility(View.VISIBLE);
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
					if(ib != null) {
						if(i < n) {
							String funcName=this.funcNames[i];
							char c=funcName.length() > 0 ? funcName.charAt(0) : 'x';
							int imageID=0;
							String imgName=funcName;
							int p=funcName.indexOf('|');
							if(p==-1) {
								imgName=funcName.substring(p+1);
							}
							switch(c) {
							case 's': // lokdef.csv.sample updaten!!!
								if(imgName.startsWith("sDurchsage") || imgName.startsWith("sAnsage")) {
									imageID=R.drawable.image_button_sound_durchsage;
								} else if(imgName.startsWith("sHorn")) {
									imageID=R.drawable.image_button_sound_horn;
								} else if(imgName.startsWith("sTrillerpfeife")) {
									imageID=R.drawable.image_button_sound_trillerpfeife;
								} else if(imgName.startsWith("sPfeife")) {
									imageID=R.drawable.image_button_sound_pfeife;
								} else if(imgName.startsWith("sSound ein/aus")) {
									imageID=R.drawable.image_button_sound_on_off;
								} else {
									imageID=R.drawable.image_button_sound;									
								}
								break;
							case 'l':
								if(imgName.startsWith("lRauchfang")) {
									imageID=R.drawable.image_button_rauchfang;
								} else if(imgName.startsWith("lFernlicht")) {
										imageID=R.drawable.image_button_headlight;
								} else {
									imageID=R.drawable.image_button_light;
								}
								break;
							case 'p':
								imageID=R.drawable.image_button_pantograph;
								break;
							default:
								//tb.setBackgroundResource(android.R.drawable.btn_star);
							}
							if(imageID != 0) {
								Drawable drawablePantograph=this.getResources().getDrawable(imageID);
								ib.setImageDrawable(drawablePantograph);
								ib.setImageLevel(this.funcStates[i] ? 1 : 0);
							}
						} else {
							// tb.setBackgroundResource(android.R.drawable.btn_star);
						}
					}
					
				// }
			}
		}
		
		// restliche fahrende Loks anzeigen:
		LinearLayout statusOther = (LinearLayout)this.findViewById(R.id.linearLayoutStatusOther);
		statusOther.removeAllViews();
		Enumeration<Integer> e = ControlAction.availLocos.keys();
		while(e.hasMoreElements()) {
			int addr=e.nextElement();
			if(!ControlAction.currSelectedAddr.contains(addr)) {
				item = ControlAction.availLocos.get(addr);
				if((item.speed < 0) || (item.speed > 1)) {
					ImageView iv=new ImageView(this);
					iv.setImageBitmap(item.img);
					statusOther.addView(iv);
					TextView tv=new TextView(this);
					tv.setText(""+item.speed);
					statusOther.addView(tv);
				}
			}
		}

		ConnectivityManager conMan = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
		android.net.NetworkInfo.State wifi = conMan.getNetworkInfo(1).getState();
		if(wifi == NetworkInfo.State.CONNECTED) {
		} else if(wifi == NetworkInfo.State.CONNECTING) {
			
		}
	}
	
	abstract static class CallbackProgressRunnable implements Runnable {
		public int progress=0;
		public abstract void run();
	}
	
	/**
	 * @param reply antwort auf GETLOCOS - da steht auch aktueller speed + func der loks drinnen
	 * @param selectList liste die filllistthread erzeugt hat (wegen den bildern)
	 * @throws java.lang.Exception
	 */
	public static void setAvailLocos(CallbackProgressRunnable callbackProgress, CallbackProgressRunnable callbackTotal, FBTCtlMessage reply) throws Exception {
		int n=reply.get("info").getArraySize();
		if(callbackProgress != null) {
			callbackProgress.progress=0; callbackProgress.run();
			callbackTotal.progress=n; callbackTotal.run();
		}
		for(int i=0; i < n; i++) {
			Integer addr = Integer.valueOf(reply.get("info").get(i).get("addr").getIntVal());
			String name = reply.get("info").get(i).get("name").getStringVal();
			int speed = reply.get("info").get(i).get("speed").getIntVal();
			int func = reply.get("info").get(i).get("functions").getIntVal();
			String imgname=reply.get("info").get(i).get("imgname").getStringVal();
			Bitmap img=ControlAction.getImageCached(imgname);
			ControlAction.availLocos.put(addr,new AvailLocosListItem(name, img, speed,func));
			if(callbackProgress != null) {
				callbackProgress.progress=i+1; callbackProgress.run();
			}
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
					ret=Bitmap.createScaledBitmap(ret, ret.getWidth()*4, ret.getHeight()*4, true );
				} catch (Exception e) {
					Log.d(TAG, "getImageCached exception:"+e.toString());
					/*
					System.out.println("[0]="+imageData.charAt(0)+" [1]="+imageData.charAt(1)+
				" [2]="+imageData.charAt(2));
					byte data[]=imageData.getBytes();
					System.out.println("imgdump: ("+data.length+"bytes)");
					for(int i=0; i < data.length; i++) {
						System.out.print(data[i]+" ");
					}
					*/
					Log.d(TAG, "\n"+imageData);
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
	

	//int lastDirProgress=-1;
	/**
	 * 
	 */
	public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
		/* das war vom seekBarDir:
		if(!fromUser) return;

		if(seekBar.getProgress() == this.lastDirProgress) return; // wenn sich nix geändert hat gleich raus, bis die antwort kommt und repaint aufgerufen wird dauerts immer ein bissl;
		this.lastDirProgress=progress;
		if(seekBar.getProgress() == 1) {
			seekBar.setProgress(2); // wemma von 0-1 nimmt ist 0-99%=0, 100%=1
		}
		this.onClickButton(seekBar);
		this.repaint();
		*/
	}
	
	public class HoldDownSliderTask extends TimerTask {
		BTcommThread.Callback parent;
		private FBTCtlMessage msgAcc;
		private FBTCtlMessage msgBreak;
		public HoldDownSliderTask(BTcommThread.Callback parent) {
			this.parent=parent;
			this.msgAcc=new FBTCtlMessage();
			this.msgBreak=new FBTCtlMessage();
        	try {
        		ControlAction.this.setMessageAddrField(msgAcc, "ACC");
        		ControlAction.this.setMessageAddrField(msgBreak,"BREAK");
    		} catch (Exception e) {
    			Log.e(TAG,"HoldDownSliderTask", e);
    			Toast.makeText(ControlAction.this, "exception:"+e.getMessage(), Toast.LENGTH_LONG).show();
    		}
		}
		public void run() {
			try {
				if(AndroidMain.btcomm.nextMessage == null) { // nur was neues in die queue schieben wenns leer is
					SeekBar seekBarSpeed = (SeekBar)findViewById(R.id.seekBarSpeed2);
					AvailLocosListItem item=ControlAction.availLocos.get(ControlAction.currSelectedAddr.get(0));
					if(item != null) {
						if(Math.abs(item.speed) < seekBarSpeed.getProgress()) {
							AndroidMain.btcomm.addCmdToQueue(msgAcc,parent);
						} else {
							AndroidMain.btcomm.addCmdToQueue(msgBreak,parent);
						}
					}
				}
			} catch (Exception e) {
				infoMsg="err: HoldDown: Exception "+e.toString();
			}
		}
	}

	public void onStartTrackingTouch(SeekBar seekBar) {
		if(seekBar.getId() == R.id.seekBarSpeed2) {
			// init holdDownKey
			if(task != null) {
				timer.cancel();
			}
			timer = new Timer();
			timerwait=new Object();
			task = new HoldDownSliderTask(this);
			timer.schedule(task, repeatTimeout, repeatTimeout);
		}
	}

	public void onStopTrackingTouch(SeekBar seekBar) {
		if(seekBar.getId() == R.id.seekBarSpeed2) {
			timer.cancel();
		}
	}
	
	public void setMessageAddrField(FBTCtlMessage msg, String msgType) throws Exception {
		if(!msgType.equals("SETFUNCTION") && (ControlAction.currSelectedAddr.size() > 1)) {
			msg.setType(MessageLayouts.messageTypeID(msgType+"_MULTI"));
			for(int i=0; i < ControlAction.currSelectedAddr.size(); i++) {
				Integer addr = ControlAction.currSelectedAddr.get(i);
				msg.get("list").get(i).get("addr").set(addr);
			}
		} else {
			msg.setType(MessageLayouts.messageTypeID(msgType));
			msg.get("addr").set(ControlAction.currSelectedAddr.get(0));
		}
	}
	
	/**
	 * notaus
	 */
	public void fullStop() {
    	FBTCtlMessage msg = new FBTCtlMessage();
    	try {
    		this.setMessageAddrField(msg, "STOP");
    		AndroidMain.btcomm.addCmdToQueue(msg,this);
    	} catch (Exception e) {
			e.printStackTrace();
			this.handleBtCommException(e);
		}
		this.updateSpeedSlider(0,0);
	}
	
	public void setPower(boolean powerState) {
    	FBTCtlMessage msg = new FBTCtlMessage();
    	try {
        	msg.setType(MessageLayouts.messageTypeID("POWER"));
	    	msg.get("value").set(powerState ? 1 : 0);
			AndroidMain.btcomm.addCmdToQueue(msg,this);
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	private void handleBtCommException(Exception e) {
		Toast.makeText(this, "error sending command", Toast.LENGTH_LONG).show();
		if(AndroidMain.btcomm == null) {
			final ProgressDialog loadProgressDialog=AndroidMain.createConnectingProgressDialog(this);
	        
	    	// Create runnable for posting
	        Runnable mUpdateProgressDialog = new Runnable() {
	            public void run() {
	                AndroidMain.repaint(loadProgressDialog, null);
	            }
	        };
			AndroidMain.restartConnection(loadProgressDialog, mUpdateProgressDialog);
		}
	}


}
