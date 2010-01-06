/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package btcontroll;
import javax.microedition.lcdui.Form;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.StringItem;

/**
 *
 * @author chris
 */
public class Debuglog {
	static private Form debugForm;
	public Debuglog(Form debugForm1) {
		debugForm=debugForm1;
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
}
