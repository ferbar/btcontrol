/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package btcontroll;
import javax.microedition.lcdui.Form;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.StringItem;
import javax.microedition.lcdui.Display;

/**
 *
 * @author chris
 */
public class Debuglog {
	static private Form debugForm;
	static private Display display;
	public Debuglog(Form debugForm1, Display display1) {
		debugForm=debugForm1;
		display=display1;
	}
	static public void debugln(String text) {
		StringItem item=new StringItem("",text+"\n");
		debug(item);
		System.out.println(text);
	}
	static public void debug(String text) {
		StringItem item=new StringItem("",text);
		debug(item);
		System.out.print(text);
    }
	static public void debug(StringItem text) {
		text.setFont(Font.getFont(Font.FACE_PROPORTIONAL, Font.STYLE_PLAIN, Font.SIZE_SMALL));
        debugForm.append(text);
    }

	/**
	 * vibriert - blocking oder non-blocking ???
	 */
    static public void vibrate(int ms) {
		display.vibrate(ms);
/*
		try {
       // Nokia
       Class.forName("com.nokia.mid.sound.Sound");
       try
       {
           Class test=Class.forName("com.nokia.mid.ui.DeviceControl");
           com.nokia.mid.ui.DeviceControl.setLights(0,100);
       }
       catch(Exception ex2){}
   }
		catch(Exception ex){}
*/
	}
}
