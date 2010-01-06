/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package btcontroll;
import javax.microedition.lcdui.*;

/**
 *
 * @author chris
 */
public class YesNoAlert extends Alert implements CommandListener {
	public boolean yes;
	private Object notifyObject;
	private final Command cyes = new Command("Yes", Command.OK, 1);
	private final Command cno = new Command("No", Command.CANCEL, 1);
	Displayable old;
	
	YesNoAlert(String title, String alertText, Object notifyObject) {
		super(title, alertText, null, AlertType.CONFIRMATION);
		this.notifyObject=notifyObject;
		this.setCommandListener(this);
		this.setTimeout(Alert.FOREVER);
		this.addCommand(cyes);
		this.addCommand(cno);
		old=btrailClient.display.getCurrent();
	}
	
	public void commandAction(Command command, Displayable displayable) {
		if (command == cyes)
			this.yes=true;
		else if (command == cno)
			this.yes=false;
		else
			this.yes=false;
		synchronized(notifyObject) {
			this.notifyObject.notify();
		}
		btrailClient.display.setCurrent(old);
	}
}
