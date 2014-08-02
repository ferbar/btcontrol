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
 * liste, speichert zusätzlich noch ein object
 */

package org.ferbar.btcontrol;
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
