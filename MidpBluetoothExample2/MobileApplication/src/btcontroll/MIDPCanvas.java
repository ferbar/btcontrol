/*
 * MIDPCanvas.java
 *
 * Created on 4. September 2007, 18:58
 */

package btcontroll;

import java.util.Timer;
import java.util.TimerTask;
import javax.microedition.lcdui.*;
import protocol.FBTCtlMessage;
import protocol.MessageLayouts;

/**
 *
 * @author  chris
 * @version
 */
public class MIDPCanvas extends Canvas implements CommandListener, BTcommThread.Callback {
	public BTcommThread btcomm;
	BTcommThread.DisplayOutput debugForm;
	String err="";
	private Command exitCommand = new Command("Exit", Command.BACK, 1);
	private Command emergencyStopCommand = new Command("STOP", Command.ITEM,1);
	private Command locoListCommand = new Command("loks", Command.ITEM, 5);
	private Command funcListCommand = new Command("Functions", Command.ITEM,6);
	private Command pwrOffCommand = new Command("Aus", Command.ITEM,7);
	private Command pwrOnCommand = new Command("Ein", Command.ITEM,8);
	
	// loco list
	private Command locoListCMDSelect = new Command("Select", Command.ITEM,1);
	private Command locoListCMDBack = new Command("back", Command.BACK,1);
			
	// func list (back=locoListCMDBack
	private Command funcListCMDOn = new Command("Ein", Command.ITEM,1);
	private Command funcListCMDOff = new Command("Aus", Command.ITEM,2);

	boolean inCommandAction=false;

	private ValueList selectList; // für die func und lok auswahl

	private Displayable backForm; // damit ma wieder zurück kommen

	private int currAddr=0; // 
	private int currSpeed=0; // wird von der callback func gesetzt
	private int currFuncBits=0;

	// ... damit eine gedrückte taste öfter gezählt wird
	Timer timer;
	Object timerwait;
	TimerTask task=null;
	public class HoldDownKeyTask extends TimerTask {
		MIDPCanvas parent;
		private FBTCtlMessage cmd;	// fürs releaseKey public
		public HoldDownKeyTask(FBTCtlMessage cmd, MIDPCanvas parent) {
			this.parent=parent;
			this.cmd=cmd;
		}
		public void run() {
			try {
				btcomm.addCmdToQueue(cmd,parent);
			} catch (Exception e) {
				err="HoldDown: Exception "+e.toString();
			}
		}
	}
	
	/**
	 * 
	 * wird für jedes PING- und INC- und DEC- reply aufgerufen
	 */
	public void BTCallback(FBTCtlMessage reply) {
		// TODO: optimieren *********
		try {
			if(reply == null) {
				throw new Exception("callback, no reply-disconnected?");
			} else {
				if(reply.isType("PING_REPLY") || reply.isType("ACC_REPLY") || reply.isType("BREAK_REPLY")
						|| reply.isType("STOP_REPLY") || reply.isType("SETFUNC_REPLY")) {
					// debugForm.debug("pingReply rx ");
					int an=reply.get("info").getArraySize();
					// debugForm.debug("asize:"+an+" ");
					for(int i=0; i < an; i++) {
						int addr= reply.get("info").get(i).get("addr").getIntVal();
						/*
						debugForm.debug("addr:"+addr+
							" speed: "+reply.get("info").get(i).get("speed").getIntVal()+
							" func: "+reply.get("info").get(i).get("functions").getIntVal());
						 */
						if(this.currAddr==addr) {
							this.currSpeed=reply.get("info").get(i).get("speed").getIntVal();
							this.currFuncBits=reply.get("info").get(i).get("functions").getIntVal();
						}
					}
				} else {
					throw new Exception("didn't receive REPLY");
				}
			}
		} catch (Exception e) {
			this.err=e.getMessage();
		}
		
		this.repaint();
	}
	
	/**
	 * constructor
	 */
	public MIDPCanvas(BTcommThread.DisplayOutput debugForm, BTcommThread btcomm) {
		this.debugForm=debugForm;
		backForm=HelloMidlet.display.getCurrent();
		update(btcomm);
		/* ------- steht im hello form!!!!!! 
		try {
			// Set up this canvas to listen to command events
			setCommandListener(this);
			// Add the Exit command
			this.addCommand(listCommand);
			addCommand(exitCommand);
		} catch(Exception e) {
			e.printStackTrace();
		}
		*/
		this.setCommandListener(this);
		this.addCommand(exitCommand);
		this.addCommand(emergencyStopCommand);
		this.addCommand(locoListCommand);
		this.addCommand(funcListCommand);
		this.addCommand(pwrOffCommand);
		this.addCommand(pwrOnCommand);

	}
	
	public void update(BTcommThread btcomm)  {
		this.btcomm=btcomm;
		this.btcomm.pingCallback=this;
	}
	
	/**
	 * paint
	 */
	public void paint(Graphics g) {
		try {
			int width=getWidth();
			Font f=g.getFont();
			int lineHeight=f.getHeight();
			g.setColor(0xffffff);
			g.fillRect(0, 0, width, getHeight());
			g.setColor(0x00ff00);
			int speed_len=(Math.abs(this.currSpeed) * width/2 / 255);
			if(this.currSpeed >= 0) {
				g.fillRect(width/2, 20, speed_len, lineHeight);
				g.setColor(0x0000ff);
				g.drawString("Speed:"+this.currSpeed,width/2,20,Graphics.TOP|Graphics.LEFT);
			} else {
				g.fillRect(width/2 - speed_len, 20, speed_len, lineHeight);
				g.setColor(0x0000ff);
				g.drawString("Speed:"+this.currSpeed,0,20,Graphics.TOP|Graphics.LEFT);
			}
			// funcbits ausgeben:
			String tmp=""; //+btcomm.currFuncBits+":";
			for(int i=11; i >=0; i--) {
				if((this.currFuncBits & (1<<i)) > 0) {
					tmp+=(i+1)+" ";
				}
			}
			g.drawString(tmp,0,40,Graphics.TOP|Graphics.LEFT);
			//g.drawString("isDoubleBuffered: "+this.isDoubleBuffered(),0,40,Graphics.TOP|Graphics.LEFT);
			if(btcomm.timeout) {
				g.setColor(0xff0000);
				g.fillRect(0, 20+lineHeight, getWidth(), lineHeight);
				g.setColor(0x000000);
				g.drawString("timeout",0,20+lineHeight,Graphics.TOP|Graphics.LEFT);
			}
			if(btcomm.isAlive()==false) {
				g.setColor(0xff0000);
				g.fillRect(0, 20+lineHeight*2, getWidth(), lineHeight);
				g.setColor(0x000000);
				g.drawString("disconnected",0,20+lineHeight*2,Graphics.TOP|Graphics.LEFT);
			}
			if(err.compareTo("")!=0) {
				g.setFont(Font.getFont(Font.FACE_PROPORTIONAL, Font.STYLE_PLAIN, Font.SIZE_SMALL));
				g.drawString("error: "+err,0,20+lineHeight*2,Graphics.TOP|Graphics.LEFT);
			}
			if(this.inCommandAction) {
				g.drawString("in cmdAction",0,80,Graphics.TOP|Graphics.LEFT);
			}
		} catch (Exception e) {
			g.drawString("canvas::paint exception("+e.toString()+")",0,20,Graphics.TOP|Graphics.LEFT);
		}
	}
	
	/**
	 * Called when a key is pressed.
	 * note: stop = commandAction
	 */
	protected  void keyPressed(int keyCode) {
		if(btcomm!=null) {
			FBTCtlMessage msg = new FBTCtlMessage();
			int repeatTimeout=1000; // ms
			boolean sendCmd = true;
			try {
				int func=-1;
				switch(keyCode) {
					case Canvas.KEY_NUM0: func=9 ; break;
					case Canvas.KEY_NUM1: func=0 ; break;
					case Canvas.KEY_NUM2: func=1 ; break;
					case Canvas.KEY_NUM3: func=2 ; break;
					case Canvas.KEY_NUM4: func=3 ; break;
					case Canvas.KEY_NUM5: func=4 ; break;
					case Canvas.KEY_NUM6: func=5 ; break;
					case Canvas.KEY_NUM7: func=6 ; break;
					case Canvas.KEY_NUM8: func=7 ; break;
					case Canvas.KEY_NUM9: func=8 ; break;
					case Canvas.KEY_STAR: 
					case Canvas.KEY_POUND: sendCmd=false; break;

					default: 
						repeatTimeout=250; // 4*/s senden
						int gameAction =this.getGameAction( keyCode );
						switch ( gameAction )
						{
							case Canvas.UP:
								msg.setType(MessageLayouts.messageTypeID("ACC"));
								break;
							case Canvas.DOWN:
								msg.setType(MessageLayouts.messageTypeID("BREAK"));
								break;
							/* - mittlere taste wird als kommando aufgerufen!
							case Canvas.FIRE:
								msg.setType(MessageLayouts.messageTypeID("STOP"));
								break;
							 */
							/*
							case Canvas.LEFT:
								cmd="left";
								break;
							case Canvas.RIGHT:
								cmd="right";
								break;
							 */
							default:
								sendCmd=false;
								break;
						}
				}
				if(sendCmd) {
					if(func >= 0) {
						msg.setType(MessageLayouts.messageTypeID("SETFUNC"));
						msg.get("funcnr").set(func);
						msg.get("value").set(1);
					}
					msg.get("addr").set(this.currAddr);

					btcomm.addCmdToQueue(msg,this);
				}
			} catch (Exception e) {
				err="keyPressed: Ex"+e.toString();
			}
			// init holdDownKey
			if(task != null) {
				timer.cancel();
			}
			timer = new Timer();
			timerwait=new Object();
			task = new HoldDownKeyTask(msg,this);
			
			timer.schedule(task, repeatTimeout, repeatTimeout);
		} else {
			err="no btcomm";
		}
		this.repaint();
	}

	/**
	 * Called when a key is released.
	 */
	protected  void keyReleased(int keyCode) {
		FBTCtlMessage cmd=((HoldDownKeyTask)task).cmd;
		if(cmd.isType("SETFUNC")) {
			try {
				cmd.get("value").set(0);
				btcomm.addCmdToQueue(cmd,this);
			} catch (Exception e) {
				err="keyReleased: Ex"+e.toString();
			}
		}
		timer.cancel();
	}
	
	/**
	 * Called when a key is repeated (held down).
	 */
	protected  void keyRepeated(int keyCode) {
	}
	
	/**
	 * Called when the pointer is dragged.
	 */
	protected  void pointerDragged(int x, int y) {
	}
	
	/**
	 * Called when the pointer is pressed.
	 */
	protected  void pointerPressed(int x, int y) {
	}
	
	/**
	 * Called when the pointer is released.
	 */
	protected  void pointerReleased(int x, int y) {
	}

	/**
	 * befüllt eine List View mit werten (keine ahnung mehr warum das async ist)
	 */
	class FillListThread extends Thread
	{
		private String title;
		private FBTCtlMessage cmd;
		private String fieldDisplay;
		private String fieldValue;
		public FillListThread(String title, FBTCtlMessage cmd, String fieldDisplay, String fieldValue) {
			this.title=title;
			this.cmd=cmd;
			this.fieldDisplay=fieldDisplay;
			this.fieldValue=fieldValue;
		}
		
		public void run()
		{
			try {
				selectList.setTitle("reading...");
				selectList.deleteAll(); // sollte eigentlich eh leer sein 
				FBTCtlMessage reply=btcomm.execCmd(cmd);
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
					selectList.append(reply.get("info").get(i).get(this.fieldDisplay).getStringVal(), null, o);
					// debugForm.debug("radListD ("+i+")\n");
					/*
					debugForm.debug("addr:"+pingReply.get("info").get(i).get("addr").getIntVal()+
					" speed: "+pingReply.get("info").get(i).get("speed").getIntVal()+
					" func: "+pingReply.get("info").get(i).get("functions").getIntVal());
					 */
				}

				/*
				int last=0;
				int pos;
				while( (pos=list.indexOf("\n",last) ) >= 0) {
					String line=list.substring(last,pos);
					int index = selectList.append(line,null);
					last = pos+1;
				}
				*/
				selectList.setTitle(this.title);
			} catch(Exception e) {
				selectList.setTitle("ex:"+e.toString());
				debugForm.debug("|||"+e.getMessage());
			}
		}
	}

	public List get_locoList() throws Exception
	{
		selectList = new ValueList("Loks", Choice.IMPLICIT);
		selectList.setCommandListener(this);
		//selectList.setSelectedFlags(new boolean[0]);
		selectList.setSelectCommand(locoListCMDSelect);
		selectList.addCommand(locoListCMDBack);
		FBTCtlMessage msg = new FBTCtlMessage();
		msg.setType(MessageLayouts.messageTypeID("GETLOCOS"));
		Thread th = new FillListThread("Loks",msg,"name","addr");
		th.start();
		return selectList;
	}  		

	
	public List get_funcList() throws Exception
	{
		selectList = new ValueList("Funktionen", Choice.IMPLICIT);
		selectList.setCommandListener(this);
		//selectList.setSelectedFlags(new boolean[0]);
		selectList.setSelectCommand(funcListCMDOn);
		selectList.addCommand(funcListCMDOff);
		selectList.addCommand(locoListCMDBack);
		FBTCtlMessage msg = new FBTCtlMessage();
		msg.setType(MessageLayouts.messageTypeID("GETFUNCTIONS"));
		msg.get("addr").set(this.currAddr);
		Thread th = new FillListThread("funclist",msg,"name",null);
		th.start();
		return selectList;
	}  

	/**
	 * Called when action should be handled
	 *
	 */
	public void commandAction(Command command, Displayable displayable) {
		// debug( "commandAction ("+command.toString()+")" );
		this.inCommandAction=true;
		// debug("canvas cmd \""+command.toString()+"\"\n");
		try {
			if(command == exitCommand) {
				HelloMidlet.display.setCurrent(backForm);
			} else if(command==this.pwrOffCommand) {
				this.setTitle("pwr off");
				FBTCtlMessage msg = new FBTCtlMessage();
				msg.setType(MessageLayouts.messageTypeID("POWER"));
				msg.get("value").set(0);
				btcomm.addCmdToQueue(msg);
			} else if(command==this.pwrOnCommand) {
				this.setTitle("pwr on");
				FBTCtlMessage msg = new FBTCtlMessage();
				msg.setType(MessageLayouts.messageTypeID("POWER"));
				msg.get("value").set(1);
				btcomm.addCmdToQueue(msg);
			} else if(command==this.emergencyStopCommand) {
				this.setTitle("stop");
				FBTCtlMessage msg = new FBTCtlMessage();
				msg.setType(MessageLayouts.messageTypeID("STOP"));
				msg.get("addr").set(this.currAddr);
				btcomm.addCmdToQueue(msg,this);
			} else if(command==this.locoListCommand) {
				HelloMidlet.display.setCurrent(get_locoList());
			} else if(command==this.locoListCMDSelect) {
				int n=selectList.getSelectedIndex();
				Integer addr=(Integer) selectList.getValue(n);
				this.currAddr=addr.intValue();
				HelloMidlet.display.setCurrent(this);
				this.setTitle(selectList.getString(n));
				/*
				int pos=line.indexOf(";");
				if(pos >= 0) {
					String saddr=line.substring(0,pos);
					debugForm.debug("locoListSelect: \""+saddr+"\"");
					int addr=Integer.parseInt(saddr);
					String ret=btcomm.execCmd("select "+addr);

					HelloMidlet.display.setCurrent(this);
					this.setTitle(ret);
				}
				 */
			} else if(command==this.locoListCMDBack) {
				HelloMidlet.display.setCurrent(this);
			} else if(command==this.funcListCommand) {
				HelloMidlet.display.setCurrent(get_funcList());
			} else if((command==this.funcListCMDOn) || (command==this.funcListCMDOff)) {
				int funcnr=selectList.getSelectedIndex();
				HelloMidlet.display.setCurrent(this);
				FBTCtlMessage msg = new FBTCtlMessage();
				msg.setType(MessageLayouts.messageTypeID("SETFUNC"));
				msg.get("addr").set(this.currAddr);
				msg.get("funcnr").set(funcnr);
				msg.get("value").set(command==this.funcListCMDOn ? 1 : 0);
				btcomm.addCmdToQueue(msg,this);
				/*
				String line=selectList.getString(selectList.getSelectedIndex());
				int pos=line.indexOf(";");
				if(pos >= 0) {
					String saddr=line.substring(0,pos);
					int funcnr=Integer.parseInt(saddr);
					// String ret=btcomm.execCmd("func "+addr+" " + (command==this.funcListCMDOn ? "on" : "off"));

					HelloMidlet.display.setCurrent(this);
					// this.setTitle(ret);
			
				}
				 * */
			} else {
				this.setTitle("invalid command");
			}
		} catch(Exception e) {
			HelloMidlet.display.setCurrent(this);	// canvas wieder setzen wenna in der liste is ....

			Alert alert;
			alert = new Alert("commandActioon","Exception:" + e,null,null);
			alert.setTimeout(Alert.FOREVER);
			HelloMidlet.display.setCurrent(alert);

			err=e.toString();
		}
		this.inCommandAction=false;
		// debug("canvas cmd done \""+controllCanvas.toString()+"\"\n");
	}
	
	protected void showNotify(){
		System.out.println( "showNotify" );
		try {
			if(this.currAddr==0)
				HelloMidlet.display.setCurrent(get_locoList());
		} catch(Exception e) {
			err=e.toString();
		}
	}
	protected void hideNotify(){
		System.out.println( "hideNotify" );
	}
	
}
