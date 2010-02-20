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
 * ja/nein abfrage - fenster
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
