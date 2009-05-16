/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package protocol;
import java.util.Vector;
import java.util.Enumeration;

/**
 *
 * speichert einen message aufbau
 */
public class MessageLayout {

	public int type;
	public String name;
	public Vector childLayout = new Vector();

	/**
	 * @return neuer pos pointer
	 * parst einen struct mit key=value dingern
	 */
	public int parse(String line, int pos) throws Exception {
		this.type=FBTCtlMessage.STRUCT;
		while(true) {
			int namestart=pos;
			while(!line.startsWith(":",pos)) {
				if(line.length() == pos) return pos;
				pos++;
			}
			String name=line.substring(namestart,pos);
			pos++;
			MessageLayout info = new MessageLayout();
			info.name=name;
			switch(line.charAt(pos)) {
				case 'I': info.type=FBTCtlMessage.INT;
					pos++;
					break;
				case 'S': info.type=FBTCtlMessage.STRING;
					pos++;
					break;
				case 'A': info.type=FBTCtlMessage.ARRAY;
					pos++;
					if(line.charAt(pos) != '{')
						throw new Exception("expected {");
					pos++;
					MessageLayout arrayinfo = new MessageLayout();
					pos = arrayinfo.parse(line,pos);
					info.childLayout.addElement(arrayinfo);
					break;
				default:
					throw new Exception("error parsing line (pos="+pos+")");
			}
			this.childLayout.addElement(info);
			if(line.length() == pos) break;
			switch(line.charAt(pos)) {
				case '}': return pos;
				case ',': pos++; continue;
				default : throw new Exception ("invalid char '"+line.charAt(pos)+"' @"+pos);
			}
		}
		
		return pos;
	}
	
	public void dump() throws Exception {
		System.out.println("MessageLayout dump");
		this.dump(0);
	}
	public void dump(int indent) throws Exception {
		System.out.println("type:"+MessageLayouts.messageTypeName(this.type));
		switch(this.type) {
			case FBTCtlMessage.UNDEF: System.out.println("UNDEF"); break;
			case FBTCtlMessage.INT: System.out.println("INT"); break;
			case FBTCtlMessage.STRING: System.out.println("STRING"); break;
			case FBTCtlMessage.ARRAY: {
				System.out.println("ARRAY");
				MessageLayout childLayout = (MessageLayout) this.childLayout.firstElement();
				childLayout.dump(indent);
				break; }
			default: {
				System.out.println("STRUCT");
				Enumeration e=this.childLayout.elements();
				while(e.hasMoreElements()) {
					MessageLayout childLayout = (MessageLayout) e.nextElement();
					System.out.println("["+childLayout.name+"]");
					childLayout.dump(indent);
				}
				break; }
		}
	}
}
