package com.example.helloandroid;

import java.util.Comparator;
import java.util.Enumeration;

import com.example.helloandroid.ControllAction.AvailLocosListItem;

import btcontroll.Debuglog;
import protocol.FBTCtlMessage;
import protocol.MessageLayouts;
import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.view.ViewGroup;
import android.view.LayoutInflater;

public class ControllListAction extends ListActivity {

	ArrayAdapter<AvailLocosListItemAddr> listAdapter=null;
	Object listAdapter_notify=new Object();
	private LayoutInflater mInflater;
	
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
    	mInflater = (LayoutInflater)getSystemService(Context.LAYOUT_INFLATER_SERVICE); // TODO: rausfinden was das ist ;-)
        super.onCreate(savedInstanceState);
        setContentView(R.layout.list);
        registerForContextMenu(getListView());

        
        startFillData();
        // String[] mStrings = new String[]{"Android", "Google", "Eclipse"};
		// this.setListAdapter(new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, mStrings));
    }
	@Override
	public void onResume() {
		super.onResume();
		System.out.println("ControllAction::onResume");
		AndroidMain.plusActivity();
	}
	@Override
	public void onPause() {
		super.onPause();
		System.out.println("ControllAction::onPause isFinishing:"+this.isFinishing());
		AndroidMain.minusActivity();
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
	}

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        super.onListItemClick(l, v, position, id);

        Intent i=new Intent();
        i.putExtra("currAddr", this.listAdapter.getItem(position).addr);
        setResult(RESULT_OK,i);
        finish();
    }
    
    void fillData() {
    	// R.layout.list_item, strings) {
    	//,ControllAction.availLocos
    	this.listAdapter=new ArrayAdapter<AvailLocosListItemAddr>(this, android.R.layout.simple_list_item_1) {
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
		        	} else {
		        		holder = (ViewHolder) convertView.getTag();
	        		}
        			AvailLocosListItemAddr item=getItem(position);
        			System.out.println("set list item name:"+item.name);
	        		holder.name.setText(item.name);
        			System.out.println("set list item addr:"+item.addr);
	        		holder.addr.setText(""+item.addr);
	        		holder.img.setImageBitmap(item.img);
	        		// convertView.setBackgroundColor(Color.parseColor(getItem(position)));
	        		return convertView;
	        	}
    	
    	};
    	Enumeration<Integer> e = ControllAction.availLocos.keys();
    	while(e.hasMoreElements()) {
    		Integer addr=e.nextElement();
    		AvailLocosListItem i=(AvailLocosListItem)ControllAction.availLocos.get(addr);
    		this.listAdapter.add(new AvailLocosListItemAddr(addr,i.name,i.img, i.speed,i.funcBits));
    	}
    	this.listAdapter.sort(new Comparator<AvailLocosListItemAddr>() {
			public int compare(AvailLocosListItemAddr object1, AvailLocosListItemAddr object2) {
				// TODO Auto-generated method stub
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

		Thread th = new FillListThread(new ControllAction.CallbackProgressRunnable() {
			public void run() { // neues runnable objekt welches immer im UIThread rennen muss
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						loadProgressDialog.setProgress(progress+1);					
					}
				});
			}
		},
		new ControllAction.CallbackProgressRunnable() {
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
		ControllAction.CallbackProgressRunnable callbackProgress;
		ControllAction.CallbackProgressRunnable callbackTotal;
		private FBTCtlMessage cmd;
		public FillListThread(ControllAction.CallbackProgressRunnable runnable, ControllAction.CallbackProgressRunnable total, FBTCtlMessage cmd) {
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
				ControllAction.setAvailLocos(this.callbackProgress, this.callbackTotal, reply);
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
