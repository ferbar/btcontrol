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
	String err="";
	private Command exitCommand = new Command("Exit", Command.EXIT, 1);
	
	// ... damit eine gedrückte taste öfter gezählt wird
	Timer timer;
	Object timerwait;
	TimerTask task=null;
	public class HoldDownKeyTask extends TimerTask {
		Object notifyObject;
		private String cmd;
		public HoldDownKeyTask(String cmd, Object notifyObject) {
			this.notifyObject=notifyObject;
			this.cmd=cmd;
		}
		public void run() {
			btcomm.addCmdToQueue(cmd);
		}
	}
	
	public void BTCallback() {
		this.repaint();
	}
	
	/**
	 * constructor
	 */
	public MIDPCanvas(BTcommThread btcomm) {
		update(btcomm);
		/*
		try {
			// Set up this canvas to listen to command events
			setCommandListener(this);
			// Add the Exit command
			addCommand(exitCommand);
		} catch(Exception e) {
			e.printStackTrace();
		}
		 */
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
			g.setColor(0xffffff);
			g.fillRect(0, 0, getWidth(), getHeight());
			g.setColor(0x0000ff);
			g.drawString("Speed:"+btcomm.speed,0,20,Graphics.TOP|Graphics.LEFT);
			g.drawString("isDoubleBuffered: "+this.isDoubleBuffered(),0,40,Graphics.TOP|Graphics.LEFT);
			if(err!="") {
				g.drawString("error: "+err,0,60,Graphics.TOP|Graphics.LEFT);
			}
		} catch (Exception e) {
			g.drawString("canvas::paint exception("+e.toString()+")",0,20,Graphics.TOP|Graphics.LEFT);
		}
	}
	
	/**
	 * Called when a key is pressed.
	 */
	protected  void keyPressed(int keyCode) {
		if(btcomm!=null) {
			String cmd;
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
			btcomm.addCmdToQueue(cmd + " ("+keyCode+")");
			// init holdDownKey
			if(task != null) {
				timer.cancel();
			}
			timer = new Timer();
			timerwait=new Object();
			task = new HoldDownKeyTask(cmd,timerwait);
			int timeout=250; // 4*/s senden
			timer.schedule(task, timeout, timeout);
		} else {
			err="no btcomm";
		}
		this.repaint();
	}
	
	/**
	 * Called when a key is released.
	 */
	protected  void keyReleased(int keyCode) {
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
	 * Called when action should be handled
	 */
	public void commandAction(Command command, Displayable displayable) {
		System.out.println( "commandAction ("+command.toString()+")" );
		if(command==exitCommand) {
			System.out.println("exitCommand");
		}
	}
	
	protected void showNotify(){
		System.out.println( "showNotify" );
	}
	protected void hideNotify(){
		System.out.println( "hideNotify" );
	}
	
}
