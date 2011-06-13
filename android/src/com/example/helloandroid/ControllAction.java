/**
 * das dings da ist für user eingabe zuständig
 */

package com.example.helloandroid;

import java.util.Hashtable;


import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import protocol.FBTCtlMessage;
import protocol.MessageLayouts;
import btcontroll.BTcommThread;

public class ControllAction extends Activity implements BTcommThread.Callback, OnSeekBarChangeListener {
	private static final int ACTIVITY_SELECT_LOK=0;
	String infoMsg="";
	int currAddr=3;
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
    
    SeekBar seekBarDirection;

	
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
	    super.onCreate(savedInstanceState);
        setContentView(R.layout.controll);
	    // TODO Auto-generated method stub
        
        
        // lok liste laden
		Intent i = new Intent(this, ControllListAction.class);
		startActivityForResult(i, ACTIVITY_SELECT_LOK);
		
		this.seekBarDirection = (SeekBar)findViewById(R.id.seekBarDirection);
        this.seekBarDirection.setOnSeekBarChangeListener(this);
	}
	
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
        super.onActivityResult(requestCode, resultCode, intent);
        this.currAddr=intent.getIntExtra("currAddr", -1);
        this.repaint();
    }
    
	public void BTCallback(FBTCtlMessage reply) {
		// TODO Auto-generated method stub
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
			while(true) {
				try {
					synchronized(AndroidMain.btcomm.statusChange) {
						// Debuglog.debugln("starting NotifyStatusChange locked");
						AndroidMain.btcomm.statusChange.wait();
					}
					repaint();
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
			// Image img = selectList.getImage(i);
			ControllAction.availLocos.put(addr,new AvailLocosListItem(name,/* img,*/ null, speed,func));
		}
	}

	
	int lastDirProgress=-1;
	public void onProgressChanged(SeekBar seekBar, int progress,
			boolean fromUser) {

		if(seekBar.getProgress() == this.lastDirProgress) return; // wenn sich nix geändert hat gleich raus, bis die antwort kommt und repaint aufgerufen wird dauerts immer ein bissl;
		this.lastDirProgress=progress;
		// TODO Auto-generated method stub
		if(seekBar.getProgress() == 1) {
			seekBar.setProgress(2); // wemma von 0-1 nimmt ist 0-99%=0, 100%=1
		}
		this.onClickButton(seekBar);
		this.repaint();
	}

	public void onStartTrackingTouch(SeekBar seekBar) {
		// TODO Auto-generated method stub
		
	}

	public void onStopTrackingTouch(SeekBar seekBar) {
		// TODO Auto-generated method stub
		
	}
	
    
}
