/*
 *  This file is part of btcontrol
 *
 *  Copyright (C) Christian Ferbar
 *
 *  btcontrol is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  btcontrol is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with btcontrol.  If not, see <http://www.gnu.org/licenses/>.
 */
/**
 * debugging
 */
package org.ferbar.btcontrol;

/**
*
* @author chris
*/
public class Debuglog {

	static public String pingstat="";


	//	static private Form debugForm;
//	static private Display display;
	public Debuglog() { //Form debugForm1, Display display1) {
		/*
		debugForm=debugForm1;
		display=display1;
		*/
	}
	static public void debugln(String text) {
//		StringItem item=new StringItem("",text+"\n");
//		debug(item);
		System.out.println(text);
	}
	static public void debug(String text) {
//		StringItem item=new StringItem("",text);
		// debug(item);
		System.out.print(text);
   }
	
	static public void debug(StringItem text) {
//		text.setFont(Font.getFont(Font.FACE_PROPORTIONAL, Font.STYLE_PLAIN, Font.SIZE_SMALL));
//       debugForm.append(text);
   }

	/**
	 * vibriert - blocking oder non-blocking ???
	 */
   static public void vibrate(int ms) {
//		display.vibrate(ms);
	   System.out.println("vibrate");
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

class StringItem {
	   StringItem(String text, String value) {this.text=text;};
	   void setText(String value) {
		   if(this.text.compareTo("pingstat")==0) {Debuglog.pingstat=value;}
	   }
	   private String text;
}

class YesNoAlert {
	public YesNoAlert(String title, String text, Object notifyObject) {
		yes=false;
	}
	public boolean yes;
}

// FakeBTrailClient
class btrailClient {
	class FakeDisplay {
		void setCurrent(YesNoAlert yna) {};
	}
	
	public static FakeDisplay display;
}


