/*
 *  This file is part of btcontroll
 *  btcontroll is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  btcontroll is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with btcontroll.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Grafische ausgabe
 *
 * Created on 4. September 2007, 18:58
 */

package org.ferbar.btcontrol;

import java.util.Timer;
import java.util.TimerTask;
import javax.microedition.lcdui.*;
import java.util.Hashtable;
import java.io.InputStream;

import protocol.FBTCtlMessage;
import protocol.MessageLayouts;

/**
 *
 * @author  chris
 * @version
 */
public class MIDPCanvas extends Canvas implements CommandListener, BTcommThread.Callback {
	public BTcommThread btcomm;
	String infoMsg=null;
	String lastInfoMsg=null;
	private final Image pfeilRechts;
	private final Image pfeilLinks;
				
	private Command exitCommand = new Command("Exit", Command.BACK, 1);
	private Command emergencyStopCommand = new Command("STOP", Command.ITEM,1);
	private Command locoListCommand = new Command("loks", Command.ITEM, 5);
	private Command funcListCommand = new Command("Functions", Command.ITEM,6);
	private Command pwrOffCommand = new Command("Aus", Command.ITEM,7);
	private Command pwrOnCommand = new Command("Ein", Command.ITEM,8);

	private Command BTScanCommand = new Command("btcontroll push", Command.SCREEN,9);
	private Command BTPushCommand = new Command("obex push", Command.ITEM, 1);
	private Command BTGammuPushCommand = new Command("gammu push", Command.ITEM, 2);
	
	// loco list
	private Command locoListCMDSelect = new Command("Select", Command.ITEM,1);
	private Command locoListCMDBack = new Command("back", Command.BACK,1);
	private Command locoListMultiCMDSelect = new Command("go", Command.ITEM,1);
			
	// func list (back=locoListCMDBack
	private Command funcListCMDOn = new Command("Ein", Command.ITEM,1);
	private Command funcListCMDOff = new Command("Aus", Command.ITEM,2);
	
	private Command sendPOMCommand = new Command("POM", Command.ITEM,9);
	private Command multiControllCommand = new Command("Mehrfachsteuerung", Command.ITEM, 10);

	private Command POMCMDGo = new Command("go", Command.OK,1);
	private Command POMCMDToggleBitMode = new Command("bitmode", Command.ITEM,10);
			
	boolean inCommandAction=false;

	private ValueList selectList; // für die func und lok auswahl
	private Form POMform=null;
	private boolean POMBitMode=false;
	TextField textField1=new TextField("CV","",3,TextField.NUMERIC);
	TextField textFieldPOMBit=new TextField("Bit# (0-7)","",1,TextField.NUMERIC);
	TextField textFieldPOMVal=new TextField("Val","",3,TextField.NUMERIC);
	TextField textFieldPOMBinVal=new TextField("Val (bit 0 - 7)","",8,TextField.NUMERIC);

	private Displayable backForm; // damit ma wieder zurück kommen

	private int[] currMultiAddr=null; // für mehrfachsteuerung
	private int currAddr=0; // 
	
	// zuordnung adresse -> lokbezeichnung,bild
	class AvailLocosListItem {public String name; public Image img; public int speed; public int funcBits;
		public AvailLocosListItem(String name, Image img, int speed, int funcBits) {
			this.name=name; this.img=img; this.speed=speed; this.funcBits=funcBits;} }
	private Hashtable availLocos=new Hashtable();
	private Hashtable imgCache=new Hashtable();

	// FillThread speichert da die letzte liste
	FBTCtlMessage lastReply;
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
				infoMsg="err: HoldDown: Exception "+e.toString();
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
						AvailLocosListItem item=(AvailLocosListItem)this.availLocos.get(new Integer(addr));
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
		
		this.repaint();
	}
	
	/**
	 * constructor
	 */
	public MIDPCanvas(BTcommThread btcomm) throws Exception {
		backForm=btrailClient.display.getCurrent();
		this.pfeilRechts=Image.createImage("/icons/pfeilRechts.png");
		this.pfeilLinks=Image.createImage("/icons/pfeilLinks.png");
		if(this.hasPointerEvents()) { // eventuell braucht der das dammit die pointer* events ausgelöst werden!!!
			Debuglog.debugln("touchscreen");
		}
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

		this.addCommand(sendPOMCommand);
		this.addCommand(multiControllCommand);

		this.addCommand(this.BTScanCommand);
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
					synchronized(btcomm.statusChange) {
						// Debuglog.debugln("starting NotifyStatusChange locked");
						btcomm.statusChange.wait();
					}
					repaint();
				} catch (InterruptedException ex) {
					return;
				}
			}
		}
	}

	public void update(BTcommThread btcomm)  {
		this.btcomm=btcomm;
		this.btcomm.pingCallback=this;
		// this.
		this.infoMsg=null;

		if(this.notifyStatusChange != null) this.notifyStatusChange.interrupt();
		this.notifyStatusChange = new NotifyStatusChange();
		this.notifyStatusChange.start();
	}
	
	/**
	 * paint
	 */
	final int speedYPos=20;
	int speedLineHeight=0;
	public void paint(Graphics g) {
		try {
			int width=getWidth();
			Font f=g.getFont();
			int lineHeight=f.getHeight();
			this.speedLineHeight=lineHeight;
			g.setColor(0xffffff);
			g.fillRect(0, 0, width, getHeight());
			g.setColor(0x00ff00);
			AvailLocosListItem item=(AvailLocosListItem)this.availLocos.get(new Integer(this.currAddr));
			int currSpeed=-1;
			int currFuncBits=-1;
			if(item != null) {
				currSpeed=item.speed;
				currFuncBits=item.funcBits;
			}
			int speed_len=(Math.abs(currSpeed) * width/2 / 255);
			int middle=width/2;
			if(currSpeed >= 0) {
				g.fillRect(middle, this.speedYPos, speed_len, this.speedLineHeight);
				g.setColor(0x0000ff);
				g.drawString(currSpeed+" ", middle, this.speedYPos, Graphics.TOP|Graphics.RIGHT);
				g.drawImage(this.pfeilRechts, middle, this.speedYPos, Graphics.TOP|Graphics.LEFT);
			} else {
				g.fillRect(middle - speed_len, this.speedYPos, speed_len, this.speedLineHeight);
				g.setColor(0x0000ff);
				g.drawString(" "+currSpeed,middle,this.speedYPos,Graphics.TOP|Graphics.LEFT);
				g.drawImage(this.pfeilLinks, middle, this.speedYPos, Graphics.TOP|Graphics.RIGHT);
			}
			if((this.currMultiAddr == null) && (item != null)) {
				Image img=item.img;
				if(img != null) {
					g.drawImage(img, 0, 20,0);
				} else {
					System.out.print("no image ["+this.currAddr+"]");
				}
			} else if(this.currMultiAddr != null) {
				int pos=0;
				for(int i=0; i < this.currMultiAddr.length; i++) {
					System.out.print("get image ["+this.currMultiAddr[i]+"]");
					Image img=((AvailLocosListItem) this.availLocos.get(new Integer(this.currMultiAddr[i]))).img;
					if(img != null) {
						g.drawImage(img, pos, 20,0);
						pos+=img.getWidth()+2;
					} else {
						System.out.print("no image ["+this.currMultiAddr[i]+"]");
					}
				}
			}
			// funcbits ausgeben:
			String tmp=""; //+btcomm.currFuncBits+":";
			for(int i=17; i >=0; i--) {
				if((currFuncBits & (1<<i)) > 0) {
					tmp+=(i)+" ";
				}
			}
			g.drawString(tmp,0,40,Graphics.TOP|Graphics.LEFT);
			//g.drawString("isDoubleBuffered: "+this.isDoubleBuffered(),0,40,Graphics.TOP|Graphics.LEFT);
			if(btcomm.connState != BTcommThread.STATE_CONNECTED) {
				g.setColor(0xff0000);
				g.fillRect(0, 20+lineHeight, getWidth(), lineHeight);
				g.setColor(0x000000);
				String msg;
				// TODO: das auf btcomm.statusText umbaun
				switch(btcomm.connState) {
					case BTcommThread.STATE_DISCONNECTED: msg="disconnected"; break;
					case BTcommThread.STATE_TIMEOUT: msg="timeout"; break;
					case BTcommThread.STATE_OPENPORT: msg="open port"; break;
					case BTcommThread.STATE_CONNECTING: msg="connecting"; break;
					case BTcommThread.STATE_OPENERROR: msg="error connecting"; break;
					default: msg="???"; break;
				}
				g.drawString(msg,0,20+lineHeight,Graphics.TOP|Graphics.LEFT);
			}
			if(btcomm.isAlive()==false) { // sollte nichtmehr vorkommen
				g.setColor(0xff0000);
				g.fillRect(0, 20+lineHeight*2, getWidth(), lineHeight);
				g.setColor(0x000000);
				g.drawString("conn dead",0,20+lineHeight*2,Graphics.TOP|Graphics.LEFT);
			}
			if(this.infoMsg != null && this.infoMsg.compareTo("")!=0) {
				g.setFont(Font.getFont(Font.FACE_PROPORTIONAL, Font.STYLE_PLAIN, Font.SIZE_SMALL));
				g.drawString(this.infoMsg,0,20+lineHeight*2,Graphics.TOP|Graphics.LEFT);
			}
			if(this.inCommandAction) {
				g.drawString("in cmdAction",0,80,Graphics.TOP|Graphics.LEFT);
			}
		} catch (Exception e) {
			g.drawString("canvas::paint exception("+e.toString()+")",0,20,Graphics.TOP|Graphics.LEFT);
			e.printStackTrace();
		}

		// lokauswahl - liste anzeigen wenn currAddr == 0
		try {
			if(this.currAddr==0 && this.btcomm != null && !this.btcomm.connError()) // wenn keine adresse gesetzt + verbunden
				btrailClient.display.setCurrent(get_locoList());
		} catch(Exception e) {
			this.infoMsg="paint err:"+e.toString();
			System.out.println( "Exception:" +e.toString() );
		}
	}

	boolean shiftKey=false;
	/**
	 * Called when a key is pressed.
	 * note: stop = commandAction
	 */
	protected  void keyPressed(int keyCode) {
		final String msgLockFunctionInfo="press * to lock Fn";
		if(btcomm!=null) {
			if(this.infoMsg != null) {
				if(this.lastInfoMsg == this.infoMsg) { // pointer vergleich is absicht da!!!
					this.infoMsg="";
					this.lastInfoMsg=null;
				} else {
					this.lastInfoMsg=this.infoMsg;
				}
			}
			FBTCtlMessage msg = new FBTCtlMessage();
			int repeatTimeout=-1; // ms

			try {
				int func=-1;
				switch(keyCode) {
					case Canvas.KEY_NUM0: func=0 ; this.infoMsg=msgLockFunctionInfo; break;
					case Canvas.KEY_NUM1: func=1 ; this.infoMsg=msgLockFunctionInfo; break;
					case Canvas.KEY_NUM2: func=2 ; this.infoMsg=msgLockFunctionInfo; break;
					case Canvas.KEY_NUM3: func=3 ; this.infoMsg=msgLockFunctionInfo; break;
					case Canvas.KEY_NUM4: func=4 ; this.infoMsg=msgLockFunctionInfo; break;
					case Canvas.KEY_NUM5: func=5 ; this.infoMsg=msgLockFunctionInfo; break;
					case Canvas.KEY_NUM6: func=6 ; this.infoMsg=msgLockFunctionInfo; break;
					case Canvas.KEY_NUM7: func=7 ; this.infoMsg=msgLockFunctionInfo; break;
					case Canvas.KEY_NUM8: func=8 ; this.infoMsg=msgLockFunctionInfo; break;
					case Canvas.KEY_NUM9: func=9 ; this.infoMsg=msgLockFunctionInfo; break;
					case Canvas.KEY_STAR:  msg=null; this.infoMsg=null; this.timer.cancel(); this.task=null; break;
					case Canvas.KEY_POUND: this.shiftKey=true; msg=null; break;

					default: 
						int gameAction =this.getGameAction( keyCode );
						switch ( gameAction )
						{
							case Canvas.GAME_A: func=0 ; break;
							case Canvas.GAME_B: func=1 ; break;
							case Canvas.GAME_C: func=2 ; break;
							case Canvas.GAME_D: func=3 ; break;
							case Canvas.UP:
								if(this.currMultiAddr == null)
									msg.setType(MessageLayouts.messageTypeID("ACC"));
								else
									msg.setType(MessageLayouts.messageTypeID("ACC_MULTI"));
								break;
							case Canvas.DOWN:
								if(this.currMultiAddr == null)
									msg.setType(MessageLayouts.messageTypeID("BREAK"));
								else
									msg.setType(MessageLayouts.messageTypeID("BREAK_MULTI"));
								break;
							// - mittlere taste wird bei nokia handies als kommando aufgerufen!
							case Canvas.FIRE:
								if(this.currMultiAddr == null)
									msg.setType(MessageLayouts.messageTypeID("STOP"));
								else
									msg.setType(MessageLayouts.messageTypeID("STOP_MULTI"));
								break;
							 
							
							case Canvas.LEFT: {
								AvailLocosListItem item = ((AvailLocosListItem) this.availLocos.get(new Integer(this.currAddr)));
								if(Math.abs(item.speed)<=1) {
									if(this.currMultiAddr == null)
										msg.setType(MessageLayouts.messageTypeID("DIR"));
									else
										msg.setType(MessageLayouts.messageTypeID("DIR_MULTI"));
									msg.get("dir").set(-1);
								} else {
									msg=null;
									this.infoMsg="Richtungsänderung nur wenn Lok steht!";
								}
								break; }
							case Canvas.RIGHT:   {
								AvailLocosListItem item = ((AvailLocosListItem) this.availLocos.get(new Integer(this.currAddr)));
								if(Math.abs(item.speed)<=1) {
									if(this.currMultiAddr == null)
										msg.setType(MessageLayouts.messageTypeID("DIR"));
									else
										msg.setType(MessageLayouts.messageTypeID("DIR_MULTI"));
									msg.get("dir").set(1);
								} else {
									msg=null;
									this.infoMsg="Richtungsänderung nur wenn Lok steht!";
								}
								break; }
							
							default:
								msg=null;
								Debuglog.debugln("invalid key ("+keyCode+"/"+gameAction+")");
								break;
						}
				}
				if(msg != null) {
					if(func >= 0) {
						repeatTimeout=1000; // ms
						if(this.shiftKey) {
							func+=10;
						}
						msg.setType(MessageLayouts.messageTypeID("SETFUNC"));
						msg.get("funcnr").set(func);
						msg.get("value").set(1);
					} else {
						repeatTimeout=250; // 4*/s senden
					}
					if(func < 0 && this.currMultiAddr != null) {
						for(int i=0; i < this.currMultiAddr.length; i++) {
							msg.get("list").get(i).get("addr").set(this.currMultiAddr[i]);
						}
						// msg.dump(debugForm);
					} else {
						msg.get("addr").set(this.currAddr);
					}

					btcomm.addCmdToQueue(msg,this);
				}
			} catch (Exception e) {
				this.infoMsg="keyPressed: Ex"+e.toString();
			}
			if(msg != null) {
				// init holdDownKey
				if(task != null) {
					timer.cancel();
				}
				timer = new Timer();
				timerwait=new Object();
				task = new HoldDownKeyTask(msg,this);

				timer.schedule(task, repeatTimeout, repeatTimeout);
			}
			this.updateTitle();
		} else {
			this.infoMsg="keyPressed err: no btcomm";
		}
		this.repaint();
	}

	void updateTitle()
	{
		String title=null;
		if(this.currMultiAddr == null) { 
			AvailLocosListItem item = ((AvailLocosListItem) this.availLocos.get(new Integer(this.currAddr)));
			if(item != null)
				title = item.name;
		} else { // mehrfachsteuerung
			title = "Mehrfachsteuerung ";
			for(int i=0; i < this.currMultiAddr.length; i++) {
				title+=this.currMultiAddr[i]+", ";
			}
		}
		if(title != null)
			this.setTitle(title);
	}
			
	/**
	 * Called when a key is released.
	 */
	protected  void keyReleased(int keyCode) {
		if(keyCode == Canvas.KEY_POUND) {
			this.shiftKey=false;
		} else if(task != null) {
			FBTCtlMessage cmd=((HoldDownKeyTask)task).cmd;
			if(cmd.isType("SETFUNC")) {
				try {
					cmd.get("value").set(0);
					btcomm.addCmdToQueue(cmd,this);
				} catch (Exception e) {
					this.infoMsg="keyReleased: Ex"+e.toString();
				}
			}
			timer.cancel();
		}
	}

	/**
	 * Called when a key is repeated (held down).
	 */
	protected  void keyRepeated(int keyCode) {
	}

	/**
	 * Helper function for pointer*
	 */
	protected void pointerSendCmd(int x, int y) {
		try {
			int width=getWidth();
			FBTCtlMessage msg = null;
			if(y >= this.speedYPos && y < this.speedYPos + this.speedLineHeight) {
				int newSpeed = ((x-(width/2))*255*2)/width;
				AvailLocosListItem item=(AvailLocosListItem)this.availLocos.get(new Integer(this.currAddr));
				// umpolen nicht mit schieber ... TODO: das besser machen (5s halten oder 2* schieben oder so)
				if(item.speed > 0 && newSpeed < 0) {
					newSpeed = 0;
				} else if(item.speed < 0 && newSpeed > 0) {
					newSpeed = 0;
				}
				int diffSpeed=newSpeed - item.speed;

				msg = new FBTCtlMessage();
				if(diffSpeed*newSpeed > 0) { // beschleunigen
					if (this.currMultiAddr == null) {
						msg.setType(MessageLayouts.messageTypeID("ACC"));
					} else {
						msg.setType(MessageLayouts.messageTypeID("ACC_MULTI"));
					}
				} else { // bremsen
					if (this.currMultiAddr == null) {
						msg.setType(MessageLayouts.messageTypeID("BREAK"));
					} else {
						msg.setType(MessageLayouts.messageTypeID("BREAK_MULTI"));
					}
				}
			} else { // TODO: func über touch

			}
			
			if(msg != null) {
						
				int func = -1;
				if (func < 0 && this.currMultiAddr != null) {
					for (int i = 0; i < this.currMultiAddr.length; i++) {
						msg.get("list").get(i).get("addr").set(this.currMultiAddr[i]);
					}
					// msg.dump(debugForm);
				} else {
					msg.get("addr").set(this.currAddr);
				}

				btcomm.addCmdToQueue(msg, this);
			}
		} catch (Exception e) {
			this.infoMsg = "keyPressed: Ex" + e.toString();
		}
	}

	/**
	 * Called when the pointer is dragged.
	 */
	protected  void pointerDragged(int x, int y) {
		Debuglog.debugln("pointerDragged @"+x+':'+y);
		pointerSendCmd(x, y);
	}
	
	/**
	 * Called when the pointer is pressed.
	 */
	protected  void pointerPressed(int x, int y) {
		Debuglog.debugln("pointerPressed @"+x+':'+y);
		pointerSendCmd(x, y);
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
		private String fieldImage;
		public FillListThread(String title, FBTCtlMessage cmd, String fieldDisplay, String fieldValue, String fieldImage) {
			this.title=title;
			this.cmd=cmd;
			this.fieldDisplay=fieldDisplay;
			this.fieldValue=fieldValue;
			this.fieldImage=fieldImage;
		}
		
		public void run()
		{
			try {
				selectList.setTitle("reading...");
				selectList.deleteAll(); // sollte eigentlich eh leer sein 
				FBTCtlMessage reply=btcomm.execCmd(cmd);
				lastReply=reply;
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
					Image image=null;
					if(this.fieldImage != null) {
						String imageName=reply.get("info").get(i).get(this.fieldImage).getStringVal();
						image=getImageCached(imageName);
					}
					selectList.append(reply.get("info").get(i).get(this.fieldDisplay).getStringVal(), image, o);
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
				Debuglog.debugln("|||"+e.getMessage());
			}
		}
	}

	/**
	 * startet einen eigenen thread zum befüllen der liste - blockiert sonst commandAction
	 */
	public List get_locoList() throws Exception
	{
		selectList = new ValueList("Loks", Choice.IMPLICIT);
		selectList.setCommandListener(this);
		//selectList.setSelectedFlags(new boolean[0]);
		selectList.setSelectCommand(locoListCMDSelect);
		selectList.addCommand(locoListCMDBack);
		FBTCtlMessage msg = new FBTCtlMessage();
		msg.setType(MessageLayouts.messageTypeID("GETLOCOS"));
		Thread th = new FillListThread("Loks",msg,"name","addr","imgname");
		th.start();
		return selectList;
	}  		

	public List get_locoListMultiControll() throws Exception
	{
		selectList = new ValueList("Loks", Choice.MULTIPLE);
		selectList.setCommandListener(this);
		//selectList.setSelectedFlags(new boolean[0]);
		selectList.addCommand(locoListMultiCMDSelect);
		selectList.addCommand(locoListCMDBack);
		FBTCtlMessage msg = new FBTCtlMessage();
		msg.setType(MessageLayouts.messageTypeID("GETLOCOS"));
		Thread th = new FillListThread("Loks",msg,"name","addr","imgname");
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
		Thread th = new FillListThread("funclist",msg,"name",null,"imgname");
		th.start();
		return selectList;
	}  

	private Form getPOMForm() {
		if(this.POMform != null)
			return this.POMform;
		this.POMform = new Form("");
		this.POMform.setCommandListener(this);
		this.POMform.addCommand(POMCMDGo);
		this.POMform.addCommand(locoListCMDBack);
		this.POMform.addCommand(POMCMDToggleBitMode);
		this.POMform.append(textField1);
		ItemStateListener listener = new ItemStateListener() {
			public void itemStateChanged(Item item) {
				if(item == textFieldPOMVal) {
					if(textFieldPOMVal.getString().equals("")) {
						
					} else {
						int CVVal=Integer.parseInt(textFieldPOMVal.getString());
						if(POMBitMode) {
							if(CVVal != 0 && CVVal != 1) {
								textFieldPOMVal.setString("");
							}
							textFieldPOMBinVal.setString(textFieldPOMVal.getString());
						} else {
							String s="";
							for(int i=0; i < 8; i++) {
								s+=((CVVal & (1 << i)) == 0) ? "0" : "1";
							}
							textFieldPOMBinVal.setString(s);
						}
					}
				} else if(item == textFieldPOMBinVal) {
					String s=textFieldPOMBinVal.getString();
					boolean wrongChar=false;
					// int cursor=textFieldPOMBinVal.getCaretPosition();
					int i=0;
					while(i < s.length()) {
						char c=s.charAt(i);
						if(c != '0' && c != '1') {
							s=s.substring(0, i)+s.substring(i+1);
							wrongChar=true;
							continue;
						}
						i++;
					}
					if(wrongChar)
						textFieldPOMBinVal.setString(s);
					if(s.equals("")) {
						textFieldPOMVal.setString("");
					} else {
						int CVVal=Integer.parseInt(s,2);
						textFieldPOMVal.setString(""+CVVal);
					}
				} else if(item == textFieldPOMBit) {
					int bitNr=Integer.parseInt(textFieldPOMBit.getString());
					if(bitNr < 0 || bitNr >= 8) {
						textFieldPOMBit.setString("");
					}
				}
			}
		};
		this.POMform.setItemStateListener(listener);
		/*
		textFieldPOMVal.setItemCommandListener(listener);
		textFieldPOMBinVal.setItemCommandListener(listener);
		 */
		this.POMform.append(textFieldPOMVal);
		this.POMform.append(textFieldPOMBinVal);
		return this.POMform;
	}
	
	/**
	 * startet ein btscan am server
	 */
	public List get_BTDevicesList() throws Exception
	{
		selectList = new ValueList("BTDevices", Choice.IMPLICIT);
		selectList.setCommandListener(this);
		//selectList.setSelectedFlags(new boolean[0]);
		selectList.setSelectCommand(this.BTPushCommand);
		selectList.addCommand(locoListCMDBack);
		selectList.addCommand(this.BTPushCommand);
		selectList.addCommand(this.BTGammuPushCommand);

		FBTCtlMessage msg = new FBTCtlMessage();
		msg.setType(MessageLayouts.messageTypeID("BTSCAN"));
		Thread th = new FillListThread("BTDevices",msg,"name","addr",null);
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
				btrailClient.display.setCurrent(backForm);
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
				if(this.currMultiAddr == null) {
					msg.setType(MessageLayouts.messageTypeID("STOP"));
					msg.get("addr").set(this.currAddr);
				} else {
					msg.setType(MessageLayouts.messageTypeID("STOP_MULTI"));
					for(int i=0; i < this.currMultiAddr.length; i++) {
						msg.get("list").get(i).get("addr").set(this.currMultiAddr[i]);
					}
				}
				btcomm.addCmdToQueue(msg,this);

			// btscan am server starten:
			} else if(command==this.BTScanCommand) {
				btrailClient.display.setCurrent(get_BTDevicesList());
			} else if((command==this.BTPushCommand) || (command==this.BTPushCommand)) {
				this.setTitle("obex push");
				FBTCtlMessage msg = new FBTCtlMessage();
				msg.setType(MessageLayouts.messageTypeID("BTPUSH"));
				int n=selectList.getSelectedIndex();
				msg.get("addr").set((String)this.selectList.getValue(n));
				msg.get("type").set((command==this.BTPushCommand) ? 0 : 1);
				btcomm.addCmdToQueue(msg,this);
				btrailClient.display.setCurrent(this);

			// loco list
			} else if(command==this.locoListCommand) {
				btrailClient.display.setCurrent(get_locoList());
			} else if(command==this.locoListCMDSelect) {
				btrailClient.display.getCurrent();
				this.setAvailLocos(this.lastReply,this.selectList);
				int n=selectList.getSelectedIndex();
				Integer addr=(Integer) this.selectList.getValue(n);
				this.currAddr=addr.intValue();
				this.currMultiAddr=null;
				btrailClient.display.setCurrent(this);
				this.updateTitle();
				
			} else if(command==this.multiControllCommand) {
				btrailClient.display.setCurrent(get_locoListMultiControll());
			} else if(command==this.locoListMultiCMDSelect) {
				boolean[] selectedIndexes=new boolean[selectList.size()];
				int listSize=selectList.getSelectedFlags(selectedIndexes);
				/*
				int n=0;
				debugForm.debug("locoListMultiCMDSelect ");
				for(int i=0; i < selectedIndexes.length; i++) {
					debugForm.debug("["+i+"]="+selectedIndexes[i]+" ");
					if(selectedIndexes[i]) {
						n++;
					}
				}*/
				if(listSize > 1) {
					this.currMultiAddr=new int[listSize];
					int n=0;
					Debuglog.debugln("locoListMultiCMDSelect ("+listSize+")");
					String s="";
					for(int i=0; i < selectedIndexes.length; i++) {
						if(selectedIndexes[i]) {
							Integer addr=(Integer) selectList.getValue(i);
							this.currAddr=addr.intValue(); // damit irgendwas angezeigt wird
							this.currMultiAddr[n]=addr.intValue();
							// debugForm.debug("["+n+"]="+addr.intValue()+" ");
							n++;
						}
					}
					btrailClient.display.setCurrent(this);
					this.updateTitle();
				} else {
					Alert alert;
					alert = new Alert("Mehrfachsteuerung","bitte >= 2 loks auswählen ("+listSize+")",null,null);
					alert.setTimeout(Alert.FOREVER);
					btrailClient.display.setCurrent(alert);
				}
				
			} else if(command==this.locoListCMDBack) {
				btrailClient.display.setCurrent(this);
			} else if(command==this.funcListCommand) {
				btrailClient.display.setCurrent(get_funcList());
			} else if((command==this.funcListCMDOn) || (command==this.funcListCMDOff)) {
				int funcnr=selectList.getSelectedIndex();
				btrailClient.display.setCurrent(this);
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
			} else if((command==this.sendPOMCommand)) {
				Form f=getPOMForm();
				f.setTitle("POM (addr:"+this.currAddr+")");
				btrailClient.display.setCurrent(f);
			} else if((command==this.POMCMDGo)) {
				FBTCtlMessage msg = new FBTCtlMessage();
				if(this.POMBitMode) {
					msg.setType(MessageLayouts.messageTypeID("POMBIT"));
					msg.get("addr").set(this.currAddr);
					msg.get("cv").set(Integer.parseInt(textField1.getString()));
					msg.get("bit").set(Integer.parseInt(this.textFieldPOMBit.getString()));
					msg.get("value").set(Integer.parseInt(textFieldPOMVal.getString()));
				} else {
					msg.setType(MessageLayouts.messageTypeID("POM"));
					msg.get("addr").set(this.currAddr);
					msg.get("cv").set(Integer.parseInt(textField1.getString()));
					msg.get("value").set(Integer.parseInt(textFieldPOMVal.getString()));
				}
				btcomm.addCmdToQueue(msg);
			} else if(command==this.POMCMDToggleBitMode) {
				if(this.POMBitMode) {
					this.POMform.delete(1);
					this.POMBitMode=false;
				} else {
					this.POMform.insert(1, this.textFieldPOMBit);
					this.POMBitMode=true;
				}
			} else {
				this.setTitle("invalid command");
			}
		} catch(Exception e) {
			btrailClient.display.setCurrent(this);	// canvas wieder setzen wenna in der liste is ....

			Alert alert;
			alert = new Alert("commandAction","Exception:" + e,null,null);
			alert.setTimeout(Alert.FOREVER);
			btrailClient.display.setCurrent(alert);

			this.infoMsg="commandAction err:"+e.toString();
			e.printStackTrace();
		}
		this.inCommandAction=false;
		// debug("canvas cmd done \""+controllCanvas.toString()+"\"\n");
	}
	
	protected void showNotify(){
		System.out.println( "showNotify" );
	}
	protected void hideNotify(){
		System.out.println( "hideNotify" );
	}
	
	/**
	 * ladet ein bild vom server, btcomm muss gerade frei sein, blocking
	 * @param imageName
	 * @return bild
	 */
	private Image getImageCached(String imageName) {
		FBTCtlMessage msg = new FBTCtlMessage();
		Debuglog.debugln("getImageCached ("+imageName+")");
		if(imageName.length() == 0) {
			return null;
		}
		try {
			Image cached = (Image) imgCache.get(imageName);
			if(cached != null)
				return cached;
			msg.setType(MessageLayouts.messageTypeID("GETIMAGE"));
			msg.get("imgname").set(imageName);
			FBTCtlMessage reply=btcomm.execCmd(msg);
			InputStream imageData=reply.get("img").getStringInputStream();
			if(imageData.available() > 0) {
				Debuglog.debugln("size:"+imageData.available()+" ");
				Image ret;
				try {
					ret=Image.createImage(imageData);
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

	/**
	 * @param reply antwort auf GETLOCOS - da steht auch aktueller speed + func der loks drinnen
	 * @param selectList liste die filllistthread erzeugt hat (wegen den bildern)
	 * @throws java.lang.Exception
	 */
	private void setAvailLocos(FBTCtlMessage reply, ValueList selectList) throws Exception {
		int n=reply.get("info").getArraySize();
		for(int i=0; i < n; i++) {
			Integer addr = new Integer(reply.get("info").get(i).get("addr").getIntVal());
			String name = reply.get("info").get(i).get("name").getStringVal();
			int speed = reply.get("info").get(i).get("speed").getIntVal();
			int func = reply.get("info").get(i).get("functions").getIntVal();
			Image img = selectList.getImage(i);
			this.availLocos.put(addr,new AvailLocosListItem(name,img,speed,func));
		}
	}
}
