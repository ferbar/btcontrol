/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package protocol;
import java.util.Hashtable;

/**
 *
 * speichert einen message aufbau
 */
public class MessageLayout {

	public int type;
	Hashtable childLayout;

	/**
	 * @return neuer pos pointer
	 */
	public int parse(String line, int pos) throws Exception {
		this.type=FBTCtlMessage.STRUCT;
		while(true) {
			int namestart=pos;
			while(line.charAt(pos) != ':') pos++;
			String name=line.substring(namestart,pos);
			pos++;
			MessageLayout info = new MessageLayout();
			switch(line.charAt(pos)) {
				case 'I': info.type=FBTCtlMessage.INT;
					pos++;
					this.childLayout.put(name, info);
					break;
				case 'S': info.type=FBTCtlMessage.STRING;
					pos++;
					this.childLayout.put(name, info);
					break;
				case 'A': info.type=FBTCtlMessage.ARRAY;
					pos++;
					if(line.charAt(pos) != '{')
						throw new Exception("expected {");
					pos++;
					MessageLayout arrayinfo = new MessageLayout();
					pos = arrayinfo.parse(line,pos);
					info.childLayout.put(null, arrayinfo);
					this.childLayout.put(name, info);
				default: System.out.println("error parsing line \""+line+"\"");
					throw new Exception("error parsing line");
			}
			if(pos == line.length())
				break;
		}
		
		return pos;
	}
	
}
