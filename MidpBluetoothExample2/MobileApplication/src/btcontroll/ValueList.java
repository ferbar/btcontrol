/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package btcontroll;
import java.util.Vector;
import javax.microedition.lcdui.Font;

/**
 *
 * @author chris
 */
public class ValueList extends javax.microedition.lcdui.List {
	Vector values=new Vector();
	public ValueList(String title, int listType) {
		super(title, listType); //, new String[0], new Image[0]);
	}
	
	public int append(String stringPart, javax.microedition.lcdui.Image imagePart, Object value) {
		this.values.addElement(value);
		return append(stringPart, imagePart);
	}

	public Object getValue(int i) {
		return this.values.elementAt(i);
	}

	public void setValue(int i, Object o) {
		this.values.setElementAt(o, i);
	}

	public void setDisabled(int i) {
		Font f=this.getFont(i);
		this.setFont(i, Font.getFont(f.getFace(), Font.STYLE_ITALIC, f.getSize()));
	}
}
