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

	static public int hash;
	/// name = MessageLayout subnode
	static Hashtable messageLayouts = new Hashtable();

	public static String myReadLn(InputStream iStream) throws java.io.IOException 
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
			ret+=(char)i;
			n++;
		}
		if(n==0)
			return null;
		return ret;
	}
	
	/**
	 * ladet die protocol.dat
	 * setzt this.hash: hashwert von protocol.dat (zum vergleichen ob server protocol.dat und handy protocol.dat stimmen
	 */
	public void load() throws Exception {
		InputStream protocolFile = this.getClass().getResourceAsStream("protocol.dat");
		String line;
		this.hash=0;
		int typeNr = FBTCtlMessage.STRUCT + 1;
		while((line=myReadLn(protocolFile)) != null) {
			if(line.startsWith("#"))
				continue;
			
			int pos=0;
			for(int i=0; i < line.length(); i++) {
				int c=line.charAt(i);
				this.hash += (c &0xff);
			}
			pos=0;
			while(line.startsWith(" ",pos)) pos++;
			if(pos==line.length()) continue;
			int namestart=pos;
			while(!line.startsWith("=",pos)) pos++;
			if(pos==line.length()) continue;
			String name=line.substring(namestart, pos);
			pos++;
			System.out.print("name: " + name);
			MessageLayout layout = new MessageLayout();
			layout.parse(line,pos);
			System.out.println();
			layout.type=typeNr;
			MessageLayouts.messageLayouts.put(name, layout);
			typeNr++;
			
		}
		System.out.println("---------------------------hash)"+this.hash);
	}
	
	public static MessageLayout getLayout(int type) throws Exception {
				
		Enumeration e = MessageLayouts.messageLayouts.elements();
		while(e.hasMoreElements()) {
			MessageLayout layout = (MessageLayout) e.nextElement();
			if(layout.type==type)
				return layout;
		}
		throw new Exception("getLayout type "+type+" invalid");
	}
	
	public static void dump() { MessageLayouts.dump(0);}
	public static void dump(int indent) {
		Enumeration e = messageLayouts.elements();
		while(e.hasMoreElements()) {
			String name = (String) e.nextElement();
			MessageLayout layout = (MessageLayout) messageLayouts.get(name);
			String messageTypeName;
			try {
				messageTypeName=MessageLayouts.messageTypeName(layout.type);
			} catch(Exception ex) {
				messageTypeName="exception:"+ex;
			}
			System.out.print("["+name+"]:["+messageTypeName+"]");
		}
	}
	
	public static String messageTypeName(int type) throws Exception {
		/* - als 1. funk wird immer getMessageLayout aufgerufen (msg vom server lesen)
		if(!MessageLayouts.loaded) {
			MessageLayouts.load();
			MessageLayouts.loaded=true;
		}*/
		switch(type) {
			case FBTCtlMessage.UNDEF: throw new Exception("undefined data type");
			case FBTCtlMessage.INT: return "(INT)";
			case FBTCtlMessage.STRING: return "(STRING)";
			case FBTCtlMessage.ARRAY: return "(ARRAY)";
			case FBTCtlMessage.STRUCT: return "(STRUCT)";
			default: {
				String tmp = "(STRUCT ";
				Enumeration e = MessageLayouts.messageLayouts.keys();
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
