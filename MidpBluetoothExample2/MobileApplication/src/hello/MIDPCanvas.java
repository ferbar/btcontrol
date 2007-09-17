/*
 * MIDPCanvas.java
 *
 * Created on 4. September 2007, 18:58
 */

package hello;

import javax.microedition.lcdui.*;

/**
 *
 * @author  chris
 * @version
 */
public class MIDPCanvas extends Canvas implements CommandListener {
	private int n=0;
	public BTcommThread btcomm;
	String err="";
			
	/**
	 * constructor
	 */
	public MIDPCanvas(BTcommThread btcomm) {
		this.btcomm=btcomm;
		try {
			// Set up this canvas to listen to command events
			setCommandListener(this);
			// Add the Exit command
			addCommand(new Command("Exit", Command.EXIT, 1));
		} catch(Exception e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * paint
	 */
	public void paint(Graphics g) {
		g.setColor(0xffffff);
		g.fillRect(0, 0, getWidth(), getHeight());
		g.setColor(0x0000ff);
		g.drawString("Sample Text ("+n+")",0,20,Graphics.TOP|Graphics.LEFT);
		g.drawString("isDoubleBuffered: "+this.isDoubleBuffered(),0,40,Graphics.TOP|Graphics.LEFT);
		if(err!="") {
			g.drawString("error: "+err,0,60,Graphics.TOP|Graphics.LEFT);
		}
	}
	
	/**
	 * Called when a key is pressed.
	 */
	protected  void keyPressed(int keyCode) {
		if(btcomm!=null) {
			btcomm.addCmdToQueue("tralala");
		} else {
			err="no btcomm";
		}
		this.n++;
		this.repaint();
	}
	
	/**
	 * Called when a key is released.
	 */
	protected  void keyReleased(int keyCode) {
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
	}
	
}
