package org.ferbar.btcontrol;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.List;

import org.ferbar.btcontrol.ControlAction.AvailLocosListItem;
import org.ferbar.btcontrol.ControlAction.CallbackProgressRunnable;


// import com.example.helloandroid.R;
// import com.example.helloandroid.R.id;
// import com.example.helloandroid.R.layout;

import protocol.FBTCtlMessage;
import protocol.MessageLayouts;
import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.view.LayoutInflater;

public class ControlListAction extends ListActivity {

	ArrayAdapter<AvailLocosListItemAddr> listAdapter=null;
	Object listAdapter_notify=new Object();
	private LayoutInflater mInflater;
	
	private boolean selectMehrfachsteuerung=false;
	private Drawable orgStartMultiButtonImage=null;
	List<Integer> selectedLocosPos;
	
	
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        this.selectedLocosPos = new ArrayList<Integer>();
        
    	mInflater = (LayoutInflater)getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        setContentView(R.layout.list);
        registerForContextMenu(getListView());
        
        Bundle bundle = getIntent().getExtras();
        if(bundle != null) {
        	this.selectMehrfachsteuerung=bundle.getBoolean("Mehrfachsteuerung",false);
        	ImageButton ib = (ImageButton) this.findViewById(R.id.imageButtonStartMulti);
        	this.orgStartMultiButtonImage = ib.getDrawable();
        } else {
        	this.findViewById(R.id.linearLayoutMulti).setVisibility(View.GONE);
        }
        
        startFillData();
        // String[] mStrings = new String[]{"Android", "Google", "Eclipse"};
		// this.setListAdapter(new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, mStrings));
    }
	@Override
	public void onResume() {
		super.onResume();
		System.out.println("ControlAction::onResume");
		AndroidMain.plusActivity();
	}
	@Override
	public void onPause() {
		super.onPause();
		System.out.println("ControlAction::onPause isFinishing:"+this.isFinishing());
		AndroidMain.minusActivity();
	}

	/**
	 * das rumgefummel mit onKeyDown ist weil sich das ControlAction sonst gerne auch beendet weils das onBackPressed auch bekommt ....
	 * @see android.app.Activity#onKeyDown(int, android.view.KeyEvent)
	 */
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if(keyCode == KeyEvent.KEYCODE_BACK)
			return true; 
		return super.onKeyDown(keyCode, event);
	}
	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if(keyCode == KeyEvent.KEYCODE_BACK) {
			this.finish();
			return true; 
		}
		return super.onKeyDown(keyCode, event);
	}
	
    class AvailLocosListItemAddr extends AvailLocosListItem {
    	public AvailLocosListItemAddr(int addr, String name, Bitmap img, int speed, int funcBits) {
    		super(name, img, speed, funcBits);
    		this.addr=addr;
    	}
    	int addr;
    }
    public static class ViewHolder {
    	ImageView img;
		TextView addr;
		TextView name;
		CheckBox cb;
	}

    /**
     * wenn ein button oder checkbox in der liste ist wird das hier nicht "normal" aufgerufen
     */
    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        super.onListItemClick(l, v, position, id);
        if(this.selectMehrfachsteuerung) {
        	CheckBox cb = (CheckBox) v.findViewById(R.id.list_item_checkBox);
        	cb.toggle();
        	// lok in die liste eintragen:
        	if(cb.isChecked()) {
        		if(!this.selectedLocosPos.contains(position)) {
        			this.selectedLocosPos.add(position);
        		}
        	} else {
        		this.selectedLocosPos.remove(new Integer(position));
        	}
        	
        	// ausgewählte loks in der image button anzeigen:
		    List <Bitmap>images=new ArrayList<Bitmap>();
		    int width=0;
		    int height=0;
        	for(int loco_pos : this.selectedLocosPos) {
       			Bitmap bm = this.listAdapter.getItem(loco_pos).img;
       			if(bm != null) {
       				images.add(bm);
       				width+=bm.getWidth();
       				if(height < bm.getHeight()) height = bm.getHeight();
       			}
        	}

        	ImageButton ib = (ImageButton) this.findViewById(R.id.imageButtonStartMulti);
        	if(images.size() == 0) { // kein bild ausgewählt: button disable, default icon anzeigen
        		ib.setImageDrawable(this.orgStartMultiButtonImage);
        		ib.setEnabled(false);
        	} else {
    		    Bitmap out = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
    		    int wpos=0;
    		    Canvas comboImage = new Canvas(out);
    		    for(Bitmap pic : images) {
    		    	comboImage.drawBitmap(pic, wpos, 0f, null);
    		    	wpos+=pic.getWidth();
    		    }
    		    ib.setImageBitmap(out);
        		ib.setEnabled(true);
        	}
        	
        } else { // normale lok auswahl:
	        Intent intent=new Intent();
	        ArrayList<Integer> value = new ArrayList<Integer>();
        	value.add(this.listAdapter.getItem(position).addr);
	        intent.putIntegerArrayListExtra("currAddr", value);
	        intent.putExtra("currAddr", value);
	        setResult(RESULT_OK, intent);
	        finish();
        }
    }
    
    public void onClickMultiButton(View v) {
        Intent intent=new Intent();
        ArrayList<Integer> value = new ArrayList<Integer>();
        for(int position : this.selectedLocosPos) {
        	value.add(this.listAdapter.getItem(position).addr);
        }
        intent.putIntegerArrayListExtra("currAddr", value);
        intent.putExtra("currAddr", value);
        setResult(RESULT_OK, intent);
        finish();    	
    }
    
    void fillData() {
    	// R.layout.list_item, strings) {
    	//,ControlAction.availLocos
    	this.listAdapter=new ArrayAdapter<AvailLocosListItemAddr>(this, R.layout.list_item) {
    		@Override
    		public View getView(int position, View convertView, ViewGroup parent) {
    				ViewHolder holder;
        			if(convertView == null) {
		        		convertView = mInflater.inflate(R.layout.list_item, null);
		        		holder = new ViewHolder();
		        		holder.name = (TextView) convertView.findViewById(R.id.list_item_name);
		        		holder.addr = (TextView) convertView.findViewById(R.id.list_item_addr);
		        		holder.img =  (ImageView) convertView.findViewById(R.id.list_item_img);
		        		convertView.setTag(holder);
		        		holder.cb =  (CheckBox) convertView.findViewById(R.id.list_item_checkBox);
		        	} else {
		        		holder = (ViewHolder) convertView.getTag();
	        		}
        			AvailLocosListItemAddr item=getItem(position);
        			System.out.println("set list item name:"+item.name);
	        		holder.name.setText(item.name);
        			System.out.println("set list item addr:"+item.addr);
	        		holder.addr.setText(""+item.addr);
	        		holder.img.setImageBitmap(item.img);
	        		if(ControlListAction.this.selectMehrfachsteuerung) {
	        			holder.cb.setChecked(ControlListAction.this.selectedLocosPos.contains(position));
	        		} else {
		        		// cb.setVisibility(View.GONE); -> dann funktioniert das alignLeftOf CheckBox nichtmehr !!!
		        		// cb.setVisibility(View.INVISIBLE);
		        		holder.cb.setWidth(0);
		        		holder.cb.setPadding(0,0,0,0);
	        		}
	        		// convertView.setBackgroundColor(Color.parseColor(getItem(position)));
	        		return convertView;
	        	}
    	
    	};
    	Enumeration<Integer> e = ControlAction.availLocos.keys();
    	while(e.hasMoreElements()) {
    		Integer addr=e.nextElement();
    		AvailLocosListItem i=(AvailLocosListItem)ControlAction.availLocos.get(addr);
    		this.listAdapter.add(new AvailLocosListItemAddr(addr,i.name,i.img, i.speed,i.funcBits));
    	}
    	this.listAdapter.sort(new Comparator<AvailLocosListItemAddr>() {
			public int compare(AvailLocosListItemAddr object1, AvailLocosListItemAddr object2) {
				return object1.name.compareTo(object2.name);
			}
    	});
		this.setListAdapter(this.listAdapter);
    }


	// Progress Dialog anlegen:
	ProgressDialog loadProgressDialog;
    
    public void startFillData() {
		FBTCtlMessage msg = new FBTCtlMessage();
		try {
			msg.setType(MessageLayouts.messageTypeID("GETLOCOS"));
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		loadProgressDialog = new ProgressDialog(this);
        loadProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        loadProgressDialog.setMessage("Loading...");
        loadProgressDialog.setCancelable(false);
        loadProgressDialog.setProgress(0);
        loadProgressDialog.show();

		Thread th = new FillListThread(new ControlAction.CallbackProgressRunnable() {
			public void run() { // neues runnable objekt welches immer im UIThread rennen muss
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						loadProgressDialog.setProgress(progress+1);					
					}
				});
			}
		},
		new ControlAction.CallbackProgressRunnable() {
			public void run() { // neues runnable objekt welches immer im UIThread rennen muss
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
				      	loadProgressDialog.setMax(progress+1);
					}
				});
			}
		}, msg);
		th.start();
    }
	
	/**
	 * ladet die lok-liste, async damit das UI nicht steht wenns länger dauert
	 */
	class FillListThread extends Thread
	{
		ControlAction.CallbackProgressRunnable callbackProgress;
		ControlAction.CallbackProgressRunnable callbackTotal;
		private FBTCtlMessage cmd;
		public FillListThread(ControlAction.CallbackProgressRunnable runnable, ControlAction.CallbackProgressRunnable total, FBTCtlMessage cmd) {
			this.callbackProgress=runnable;
			this.callbackTotal=total;
			this.cmd=cmd;
		}
		
		public void run()
		{
			try {
		    	// ArrayAdapter li=new ArrayAdapter(this.context, android.R.id.list);
		    	// ArrayAdapter li=new ArrayAdapter(this.context, android.R.layout.simple_list_item_1);
		    	//setTitle("reading...");
				// li.clear(); // sollte eigentlich eh leer sein 
				FBTCtlMessage reply=AndroidMain.btcomm.execCmd(cmd);
				this.callbackProgress.progress=0;this.callbackProgress.run();
				ControlAction.setAvailLocos(this.callbackProgress, this.callbackTotal, reply);
				// lastReply=reply;
				/*
				int n=reply.get("info").getArraySize();
				for(int i=0; i < n; i++) {
					Object o;
					if(this.fieldValue != null) {
						// debugForm.debug("radListA ("+i+")\n");
						FBTCtlMessage tmp=reply.get("info").get(i).get(this.fieldValue);
						if(tmp==null) {
							throw new Exception("readList: invalid fieldValue");
						}
						// debugForm.debug("radListB ("+i+")\n");
						if(tmp.getType() == FBTCtlMessage.INT) {
							o=new Integer(tmp.getIntVal());
						} else if(tmp.getType() == FBTCtlMessage.STRING) {
							o=tmp.getStringVal();
						} else {
							throw new Exception("readList: invalid field");
						}
						// debugForm.debug("radListC ("+i+")\n");
					} else {
						o = null;
					}
					// Image image=null;
					if(this.fieldImage != null) {
						String imageName=reply.get("info").get(i).get(this.fieldImage).getStringVal();
						// image=getImageCached(imageName);
					}
					// selectList.append(reply.get("info").get(i).get(this.fieldDisplay).getStringVal(), image, o);

					li.add(reply.get("info").get(i).get(this.fieldDisplay).getStringVal());
					
					
					// debugForm.debug("radListD ("+i+")\n");
					/*
					debugForm.debug("addr:"+pingReply.get("info").get(i).get("addr").getIntVal()+
					" speed: "+pingReply.get("info").get(i).get("speed").getIntVal()+
					" func: "+pingReply.get("info").get(i).get("functions").getIntVal());
					 * /
				}

				/*
				int last=0;
				int pos;
				while( (pos=list.indexOf("\n",last) ) >= 0) {
					String line=list.substring(last,pos);
					int index = selectList.append(line,null);
					last = pos+1;
				}
				*
				// setTitle(this.title);
				listAdapter=li;
				*/
		        runOnUiThread(new Runnable() {
		        	public void run() {
		        		fillData();
		        	}
		        });
			} catch(Exception e) {
				//setTitle("ex:"+e.toString());
				Debuglog.debugln("FillListThread|||"+e.getMessage());
			} finally {
				runOnUiThread(new Runnable() {
					public void run() {
				        loadProgressDialog.dismiss();
					}
				});
			}
		}
	}
}
