/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package protocol;

import java.util.Hashtable;
import java.util.Enumeration;
import java.io.InputStream;

/**
 *
 * @return len gelesene bytes, -1 bei eof
 */
public class MessageLayouts {
	
	static Hashtable messageLayouts;

	public String myReadLn(InputStream iStream) throws java.io.IOException 
	{
		String ret="";
		int i;
		int n=0;
		while((i=iStream.read()) != -1) {
			if(i == '\r') {
				i=iStream.read();
			}
			if(i == '\n') {
				return ret;
			}
			ret+=(byte)i;
			n++;
		}
		if(n==0)
			return null;
		return ret;
	}
	
	public void load()  throws java.io.IOException {
		InputStream f = this.getClass().getResourceAsStream("protocol.dat");
		String line;
		int typeNr = FBTCtlMessage.STRUCT + 1;
		while((line=myReadLn(f)) != null) {
			if(line.charAt(0) == '#')
				continue;
			try {
				int pos=0;
				while(line.charAt(pos) != ' ') pos++;
				int namestart=pos;
				while(line.charAt(pos) != '=') pos++;
				String name=line.substring(namestart, pos);
				pos++;
				System.out.print("name: " + name);
				MessageLayout layout = new MessageLayout();
				layout.parse(line,pos);
				this.messageLayouts.put(name, layout);
				typeNr++;
			} catch(Exception e) {
				System.out.println("Exception: "+e);
			}
			
		}
	}
	
	public static MessageLayout getLayout(int type) {
		MessageLayout ret = (MessageLayout) messageLayouts.get(new Integer(type));
		return ret;
	}
	
	public void dump() { this.dump(0);}
	public void dump(int indent) {
		Enumeration e = messageLayouts.elements();
		while(e.hasMoreElements()) {
			String name = (String) e.nextElement();
			MessageLayout layout = (MessageLayout) messageLayouts.get(name);
			String messageTypeName;
			try {
				messageTypeName=this.messageTypeName(layout.type);
			} catch(Exception ex) {
				messageTypeName="exception:"+ex;
			}
			System.out.print("["+name+"]:["+messageTypeName+"]");
		}
	}
	
	public static String messageTypeName(int type) throws Exception {
		switch(type) {
			case FBTCtlMessage.UNDEF: throw new Exception("undefined data type");
			case FBTCtlMessage.INT: return "(INT)";
			case FBTCtlMessage.STRING: return "(STRING)";
			case FBTCtlMessage.ARRAY: return "(ARRAY)";
			case FBTCtlMessage.STRUCT: return "(STRUCT)";
			default: {
				String tmp = "(STRUCT ";
				Enumeration e = messageLayouts.elements();
				while(e.hasMoreElements()) {
					String name = (String) e.nextElement();
					MessageLayout layout = (MessageLayout) messageLayouts.get(name);
					if(layout.type == type) {
						tmp += name;
						tmp +=")";
						return tmp;
					}
				}
				throw new Exception("invalid type ("+type+")"); }
		}
	}

	public static int messageTypeID(String name) {
		MessageLayout layout = (MessageLayout) messageLayouts.get(name);
		return layout.type;
	}
}
