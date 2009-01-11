/*
 * MIDPCanvas.java
 *
 * Created on 4. September 2007, 18:58
 */

package btcontroll;

import java.util.Timer;
import java.util.TimerTask;
import javax.microedition.lcdui.*;

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

	private List selectList; // für die lok auswahl

	private Displayable backForm; // damit ma wieder zurück kommen
	
	// ... damit eine gedrückte taste öfter gezählt wird
	Timer timer;
	Object timerwait;
	TimerTask task=null;
	public class HoldDownKeyTask extends TimerTask {
		Object notifyObject;
		public String cmd;	// fürs releaseKey public
		public HoldDownKeyTask(String cmd, Object notifyObject) {
			this.notifyObject=notifyObject;
			this.cmd=cmd;
		}
		public void run() {
			try {
				btcomm.addCmdToQueue(cmd);
			} catch (Exception e) {
				err="HoldDown: Exception "+e.toString();
			}
		}
	}
	
	public void BTCallback() {
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
		this.btcomm.notifyObject=this;
	}
	
	/**
	 * paint
	 */
	public void paint(Graphics g) {
		try {
			int width=getWidth();
			g.setColor(0xffffff);
			g.fillRect(0, 0, width, getHeight());
			g.setColor(0x00ff00);
			int speed_len=(Math.abs(btcomm.currSpeed) * width/2 / 255);
			if(btcomm.currSpeed >= 0) {
				g.fillRect(width/2, 20, speed_len, 20);
				g.setColor(0x0000ff);
				g.drawString("Speed:"+btcomm.currSpeed,width/2,20,Graphics.TOP|Graphics.LEFT);
			} else {
				g.fillRect(width/2 - speed_len, 20, speed_len, 20);
				g.setColor(0x0000ff);
				g.drawString("Speed:"+btcomm.currSpeed,0,20,Graphics.TOP|Graphics.LEFT);
			}
			// funcbits ausgeben:
			String tmp=""; //+btcomm.currFuncBits+":";
			for(int i=11; i >=0; i--) {
				if((btcomm.currFuncBits & (1<<i)) > 0) {
					tmp+=(i+1)+" ";
				}
			}
			g.drawString(tmp,0,40,Graphics.TOP|Graphics.LEFT);
			//g.drawString("isDoubleBuffered: "+this.isDoubleBuffered(),0,40,Graphics.TOP|Graphics.LEFT);
			if(btcomm.timeout) {
				g.setColor(0xff0000);
				g.fillRect(0, 40, getWidth(), 20);
				g.setColor(0x000000);
				g.drawString("timeout",0,40,Graphics.TOP|Graphics.LEFT);
			}
			if(btcomm.isAlive()==false) {
				g.setColor(0xff0000);
				g.fillRect(0, 60, getWidth(), 20);
				g.setColor(0x000000);
				g.drawString("disconnected",0,60,Graphics.TOP|Graphics.LEFT);
			}
			if(err.compareTo("")!=0) {
				g.drawString("error: "+err,0,60,Graphics.TOP|Graphics.LEFT);
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
			String cmd;
			int repeatTimeout=1000; // ms
			switch(keyCode) {
				case Canvas.KEY_NUM0: cmd="func 10 down"; break;
				case Canvas.KEY_NUM1: cmd="func 1 down"; break;
				case Canvas.KEY_NUM2: cmd="func 2 down"; break;
				case Canvas.KEY_NUM3: cmd="func 3 down"; break;
				case Canvas.KEY_NUM4: cmd="func 4 down"; break;
				case Canvas.KEY_NUM5: cmd="func 5 down"; break;
				case Canvas.KEY_NUM6: cmd="func 6 down"; break;
				case Canvas.KEY_NUM7: cmd="func 7 down"; break;
				case Canvas.KEY_NUM8: cmd="func 8 down"; break;
				case Canvas.KEY_NUM9: cmd="func 9 down"; break;
				case Canvas.KEY_STAR: cmd="key_star down"; break;
				case Canvas.KEY_POUND: cmd="key_pound down"; break;
				default: 
					repeatTimeout=250; // 4*/s senden
					int gameAction =this.getGameAction( keyCode );
					switch ( gameAction )
					{
						case Canvas.UP:
							cmd="up";
							break;
						case Canvas.LEFT:
							cmd="left";
							break;
						case Canvas.DOWN:
							cmd="down";
							break;
						case Canvas.RIGHT:
							cmd="right";
							break;
						default:
							cmd="invalid_key";
							// leave unchanged
					}
			}
			try {
				btcomm.addCmdToQueue(cmd + " ("+keyCode+")");
			} catch (Exception e) {
				err="keyPressed: Ex"+e.toString();
			}
			// init holdDownKey
			if(task != null) {
				timer.cancel();
			}
			timer = new Timer();
			timerwait=new Object();
			task = new HoldDownKeyTask(cmd,timerwait);
			
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
		String cmd=((HoldDownKeyTask)task).cmd;
		if(cmd.startsWith("func ")) {
			cmd=cmd.substring(0,cmd.length()-4);
			try {
				btcomm.addCmdToQueue(cmd+"up");
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
	 * loks
	 */
	class fillListThread extends Thread
	{
		private String cmd;
		public fillListThread(String cmd) {
			this.cmd=cmd;
		}
		
		public void run()
		{
			try {
				selectList.setTitle("reading...");
				selectList.deleteAll();
				String list=btcomm.execCmd(cmd);
				int last=0;
				int pos;
				while( (pos=list.indexOf("\n",last) ) >= 0) {
					String line=list.substring(last,pos);
					int index = selectList.append(line,null);
					last = pos+1;
				}
				selectList.setTitle("auswählen");
			} catch(Exception e) {
				selectList.setTitle("ex:"+e.toString());
			}
		}
	}

	public List get_locoList()
	{
		selectList = new List("Loks", Choice.IMPLICIT, new String[0], new Image[0]);
		selectList.setCommandListener(this);
		//selectList.setSelectedFlags(new boolean[0]);
		selectList.setSelectCommand(locoListCMDSelect);
		selectList.addCommand(locoListCMDBack);
		return selectList;
	}  		

	
	public List get_funcList()
	{
		selectList = new List("Funktionen", Choice.IMPLICIT, new String[0], new Image[0]);
		selectList.setCommandListener(this);
		//selectList.setSelectedFlags(new boolean[0]);
		selectList.setSelectCommand(funcListCMDOn);
		selectList.addCommand(funcListCMDOff);
		selectList.addCommand(locoListCMDBack);
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
				btcomm.addCmdToQueue("pwr_off");
			} else if(command==this.pwrOnCommand) {
				this.setTitle("pwr on");
				btcomm.addCmdToQueue("pwr_on");
			} else if(command==this.emergencyStopCommand) {
				this.setTitle("stop");
				btcomm.addCmdToQueue("stop");
			} else if(command==this.locoListCommand) {
				HelloMidlet.display.setCurrent(get_locoList());
				Thread th = new fillListThread("list");
				th.start();
			} else if(command==this.locoListCMDSelect) {
				String line=selectList.getString(selectList.getSelectedIndex());
				int pos=line.indexOf(";");
				if(pos >= 0) {
					String saddr=line.substring(0,pos);
					debugForm.debug("locoListSelect: \""+saddr+"\"");
					int addr=Integer.parseInt(saddr);
					String ret=btcomm.execCmd("select "+addr);

					HelloMidlet.display.setCurrent(this);
					this.setTitle(ret);
			
				}
			} else if(command==this.locoListCMDBack) {
				HelloMidlet.display.setCurrent(this);
			} else if(command==this.funcListCommand) {
				HelloMidlet.display.setCurrent(get_funcList());
				Thread th = new fillListThread("funclist");
				th.start();
			} else if((command==this.funcListCMDOn) || (command==this.funcListCMDOff)) {
				String line=selectList.getString(selectList.getSelectedIndex());
				int pos=line.indexOf(";");
				if(pos >= 0) {
					String saddr=line.substring(0,pos);
					int addr=Integer.parseInt(saddr);
					String ret=btcomm.execCmd("func "+addr+" " + (command==this.funcListCMDOn ? "on" : "off"));

					HelloMidlet.display.setCurrent(this);
					this.setTitle(ret);
			
				}
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
	}
	protected void hideNotify(){
		System.out.println( "hideNotify" );
	}
	
}
