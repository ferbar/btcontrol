/*
 * MIDPCanvas.java
 *
 * Created on 4. September 2007, 18:58
 */

package btcontroll;

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
public class MIDPCanvas extends Canvas implements CommandListener, ItemStateListener, BTcommThread.Callback {
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
	private Command locoListMultiCMDSelect = new Command("go", Command.ITEM,1);
			
	// func list (back=locoListCMDBack
	private Command funcListCMDOn = new Command("Ein", Command.ITEM,1);
	private Command funcListCMDOff = new Command("Aus", Command.ITEM,2);
	
	private Command sendPOMCommand = new Command("POM", Command.ITEM,9);
	private Command multiControllCommand = new Command("Mehrfachsteuerung", Command.ITEM, 10);

	private Command POMCMDGo = new Command("go", Command.OK,1);
			
	boolean inCommandAction=false;

	private ValueList selectList; // für die func und lok auswahl
	private Form form=null;
	TextField textField1=new TextField("CV","",3,TextField.NUMERIC);
	TextField textFieldPOMVal=new TextField("Val","",3,TextField.NUMERIC);
	TextField textFieldPOMBinVal=new TextField("Val (bin)","",8,TextField.NUMERIC);

	private Displayable backForm; // damit ma wieder zurück kommen

	private int[] currMultiAddr=null; // für mehrfachsteuerung
	private int currAddr=0; // 
	private int currSpeed=0; // wird von der callback func gesetzt
	private int currFuncBits=0;
	
	// zuordnung adresse -> lokbezeichnung,bild
	class availLocosListItem {public String name; public Image img; 
		public availLocosListItem(String name, Image img) { this.name=name; this.img=img; } }
	private Hashtable availLocos=new Hashtable();
	private Hashtable imgCache=new Hashtable();

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
				if(reply.isType("STATUS_REPLY")) {
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

		this.addCommand(sendPOMCommand);
		this.addCommand(multiControllCommand);
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
			if(this.currMultiAddr == null) {
				Image img=((availLocosListItem) this.availLocos.get(new Integer(this.currAddr))).img;
				if(img != null) {
					g.drawImage(img, 0, 20,0);
				} else {
					System.out.print("no image ["+this.currAddr+"]");
				}
			} else {
				for(int i=0; i < this.currMultiAddr.length; i++) {
					System.out.print("get image ["+this.currMultiAddr[i]+"]");
					Image img=((availLocosListItem) this.availLocos.get(new Integer(this.currMultiAddr[i]))).img;
					if(img != null) {
						g.drawImage(img, (16+2)*i, 20,0);
					} else {
						System.out.print("no image ["+this.currMultiAddr[i]+"]");
					}
				}
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
			e.printStackTrace();
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
								msg.setType(MessageLayouts.messageTypeID("STOP"));
								break;
							 
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
					if(func < 0 && this.currMultiAddr != null) {
						for(int i=0; i < this.currMultiAddr.length; i++) {
							msg.get("list").get(i).get("addr").set(this.currMultiAddr[i]);
						}
					} else {
						msg.get("addr").set(this.currAddr);
					}

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
				debugForm.debug("|||"+e.getMessage());
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
		if(this.form != null)
			return this.form;
		this.form = new Form("");
		this.form.setCommandListener(this);
		this.form.addCommand(POMCMDGo);
		this.form.addCommand(locoListCMDBack);
		this.form.append(textField1);
		ItemStateListener listener = new ItemStateListener() {
			public void itemStateChanged(Item item) {
				if(item == textFieldPOMVal) {
					String s="";
					int CVVal=Integer.parseInt(textFieldPOMVal.getString());
					for(int i=0; i < 8; i++) {
						s+=((CVVal & (1 << i)) == 0) ? "0" : "1";
					}
					textFieldPOMBinVal.setString(s);
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
					int CVVal=Integer.parseInt(s,2);
					textFieldPOMVal.setString(""+CVVal);
				}
			}
		};
		this.form.setItemStateListener(listener);
		/*
		textFieldPOMVal.setItemCommandListener(listener);
		textFieldPOMBinVal.setItemCommandListener(listener);
		 */
		this.form.append(textFieldPOMVal);
		this.form.append(textFieldPOMBinVal);
		return this.form;
	}
	
	public void itemStateChanged(Item item) {

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
				
			} else if(command==this.locoListCommand) {
				HelloMidlet.display.setCurrent(get_locoList());
			} else if(command==this.locoListCMDSelect) {
				this.setAvailLocos(selectList);
				int n=selectList.getSelectedIndex();
				Integer addr=(Integer) selectList.getValue(n);
				this.currAddr=addr.intValue();
				this.currMultiAddr=null;
				HelloMidlet.display.setCurrent(this);
				this.setTitle(selectList.getString(n));
				
			} else if(command==this.multiControllCommand) {
				HelloMidlet.display.setCurrent(get_locoListMultiControll());
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
					debugForm.debug("locoListMultiCMDSelect ("+listSize+")");
					String s="";
					for(int i=0; i < selectedIndexes.length; i++) {
						if(selectedIndexes[i]) {
							Integer addr=(Integer) selectList.getValue(i);
							this.currAddr=addr.intValue(); // damit irgendwas angezeigt wird
							this.currMultiAddr[n]=addr.intValue();
							// debugForm.debug("["+n+"]="+addr.intValue()+" ");
							s+=addr.intValue()+", ";
							n++;
						}
					}
					HelloMidlet.display.setCurrent(this);
					this.setTitle("Mehrfachsteuerung "+s);
				} else {
					Alert alert;
					alert = new Alert("Mehrfachsteuerung","bitte >= 2 loks auswählen ("+listSize+")",null,null);
					alert.setTimeout(Alert.FOREVER);
					HelloMidlet.display.setCurrent(alert);
				}
				
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
			} else if((command==this.sendPOMCommand)) {
				Form f=getPOMForm();
				f.setTitle("POM (addr:"+this.currAddr+")");
				HelloMidlet.display.setCurrent(f);
			} else if((command==this.POMCMDGo)) {
				FBTCtlMessage msg = new FBTCtlMessage();
				msg.setType(MessageLayouts.messageTypeID("POM"));
				msg.get("addr").set(this.currAddr);
				msg.get("cv").set(Integer.parseInt(textField1.getString()));
				msg.get("value").set(Integer.parseInt(textFieldPOMVal.getString()));
				btcomm.addCmdToQueue(msg);
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
			if(this.currAddr==0 && this.btcomm != null && this.btcomm.isAlive()) // wenn keine adresse gesetzt + verbunden
				HelloMidlet.display.setCurrent(get_locoList());
		} catch(Exception e) {
			err=e.toString();
			System.out.println( "Exception:" +e.toString() );
		}
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
		this.debugForm.debug("getImageCached ("+imageName+")");
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
				this.debugForm.debug("size:"+imageData.available()+" ");
				Image ret;
				try {
					ret=Image.createImage(imageData);
				} catch (Exception e) {
					this.debugForm.debug("getImageCached exception:"+e.toString());
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
			this.debugForm.debug("getImageCached exception:"+e.toString());
			return null;
		}
	}
	
	private void setAvailLocos(ValueList selectList) {
		int n=selectList.size();
		for(int i=0; i < n; i++) {
			Integer addr = (Integer) selectList.values.elementAt(i);
			String name = selectList.getString(i);
			Image img = selectList.getImage(i);
			this.availLocos.put(addr,new availLocosListItem(name,img));
		}
	}
}
