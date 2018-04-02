/*
 *  This file is part of btcontrol
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
 *
 * baut messages zusammen
 * 
 * FBTCtlMessage f(msgtype);
 * f.get("info").get(0).get("addr").set(1);
 * 
 */

package protocol;

//import java.util.Vector;
import java.util.Hashtable;
import java.util.Enumeration;
import java.io.ByteArrayInputStream;
import java.io.InputStream;

import org.ferbar.btcontrol.Debuglog;

// warnings ausschalten geht im java 1.3 noch nicht, midp braucht 1.3
// @SuppressWarnings({"rawtypes","unchecked"}) // Hashtable<> gibts im midp nicht

/**
 *
 */
public class FBTCtlMessage {
	String name;
	int ival; // integer value oder string-offset im byte[] array
	int slen;
	byte buffer[];
	int type;
	Hashtable arrayVal; // struct:key=string + array:key=int
	
	public static final int UNDEF = 0;
	public static final int INT = 1;
	public static final int STRING = 2;
	public static final int ARRAY = 3;
	public static final int STRUCT = 4;

	/// int
	public FBTCtlMessage(int i) {
		type=INT;
		ival=i;
	}
			
	/// string
	public FBTCtlMessage(byte []buffer, int off, int len) {
		this.type=STRING;
		this.buffer=buffer;
		this.ival=off;
		this.slen=len;
	}
	
	public FBTCtlMessage() {
		type=UNDEF;
	}
	
	public void set(int i) throws Exception {
		if(this.type == UNDEF) {
			this.type=INT;
		}
		if(this.type != INT) {
			throw new Exception("setInt invalid type ("+this.type+")");
		}
		this.ival=i;
	}
	
	public void set(String s) throws Exception {
		if(this.type == UNDEF) {
			this.type=STRING;
		}
		if(this.type != STRING) {
			throw new Exception("setString invalid type ("+this.type+")");
		}
		this.buffer=s.getBytes();
		this.ival=0;
		this.slen=this.buffer.length;
	}
	
	public void setType(int type) throws Exception {
		if(this.type != UNDEF) 
			throw new Exception("type already set");

		if(type >= ARRAY)
			this.arrayVal=new Hashtable();

		this.type = type;
	}
	
	
			
	/**
	 * liest eine bin message
	 */
	public void readMessage(byte []inbuffer) throws Exception {
		this.readMessage(new InputReader(inbuffer),null);
	}
	public void readMessage(InputReader in, MessageLayout layout) throws Exception {
		System.out.println("reading bin msg");
		this.type=in.getByte(); // type muss >= STRUCT sein
		if(layout == null && (this.type > STRUCT)) {
			layout = MessageLayouts.getLayout(this.type);
		}
		if(layout == null) {
			throw new Exception("readMessage no layout");
		}
		this.arrayVal=new Hashtable();
		Enumeration e=layout.childLayout.elements();
		while(e.hasMoreElements()) {
			int elementType=in.getByte();
			MessageLayout childLayout = (MessageLayout) e.nextElement();
			System.out.println("reading [\""+childLayout.name+"\"]: type "+elementType);
			if(elementType != childLayout.type) {
				throw new Exception("readMessage type("+elementType+") != expected type("+childLayout.type+")");
			}
			switch(childLayout.type) {
				case INT:
					this.arrayVal.put(childLayout.name, new FBTCtlMessage(in.getInt()));
					break;
				case STRING:
					this.arrayVal.put(childLayout.name, new FBTCtlMessage(in.inbuffer,in.getStrOff(),in.getStrLen()));
					break;
				case ARRAY: {
					int n=in.getByte();
					System.out.println("read array["+n+"]");
					FBTCtlMessage tmparray = new FBTCtlMessage();
					tmparray.setType(ARRAY);
					MessageLayout arrayChildLayout = (MessageLayout) childLayout.childLayout.firstElement();
					for(int i=0; i < n; i++) {
						FBTCtlMessage tmp = new FBTCtlMessage();
						tmp.readMessage(in,arrayChildLayout);
						tmparray.arrayVal.put(new Integer(i), tmp);
					}
					this.arrayVal.put(childLayout.name, tmparray);
					break; }
				default : {
					throw new Exception("invalid type");
				}
			}
			
		}
	}
	
	/**
	 * wandelt die aktuelle message in einen bin - string um
	 * Vorsicht: zusätzliche Felder die im MessageLayout nicht vorkommen werden stillschweigend ignoriert
	 */
	public OutputWriter getBinaryMessage() throws Exception  {
		MessageLayout layout = MessageLayouts.getLayout(this.type);
		OutputWriter out=new OutputWriter();
		this.getBinaryMessage(layout, out);
		return out;
	}
	
	public void getBinaryMessage(MessageLayout layout, OutputWriter out) throws Exception {
		if(layout == null) {
			layout = MessageLayouts.getLayout(this.type);
		}
		if(layout == null) {
			throw new Exception("getBinaryMessage no layout");
		}
		
		out.putByte(this.type);
		switch(this.type) {
			case INT:
				out.putInt(this.ival); break;
			case STRING: {
				out.putString(this.buffer,this.ival,this.slen); break;
			}
			case ARRAY: {
				int n=this.arrayVal.size();
				out.putByte(n);
				MessageLayout childLayout = (MessageLayout) layout.childLayout.firstElement();
				for(int i=0; i < n; i++) {
					FBTCtlMessage childNode=(FBTCtlMessage)this.arrayVal.get(new Integer(i));
					childNode.getBinaryMessage(childLayout, out);
				}
				break; }
			default: { // struct oder messageLayoutID
				
				if(layout.childLayout==null)
					throw new Exception("no child Layout");
				Enumeration e=layout.childLayout.elements();
				while(e.hasMoreElements()) {
					MessageLayout childLayout = (MessageLayout) e.nextElement();
					try {
						FBTCtlMessage childNode=(FBTCtlMessage)this.arrayVal.get(childLayout.name);
						if(childNode == null) {
							throw new Exception("getBinMsg ["+childLayout.name+"] missing");
						}
						if(childNode.type != childLayout.type)
							throw new Exception("getBinaryMessage invalid datatype");
						childNode.getBinaryMessage(childLayout, out);
					} catch(NullPointerException ex) {
						throw new Exception("getBinMsg NullPointerEx ["+childLayout.name+"]");
					}
				}
				break; }
		}
	}

	/**
	 * zugriff auf einen struct node, legt subnode an wenns den nicht gibt
	 */
	public FBTCtlMessage get(String index) throws Exception  {
		if(this.type == UNDEF) {
			this.type=STRUCT;
			this.arrayVal=new Hashtable();
		}
		if(this.type < STRUCT) {
			throw new Exception("FBTCtlMessage get not a struct ("+this.type+")");
		}
		if(!this.arrayVal.containsKey(index)) {
			this.arrayVal.put(index, new FBTCtlMessage());
		}
		return (FBTCtlMessage) this.arrayVal.get(index);
	}
	
	public FBTCtlMessage get(int index) throws Exception {
		if(this.type == UNDEF) {
			this.type=ARRAY;
			this.arrayVal=new Hashtable();
		}
		if(this.type != ARRAY) {
			throw new Exception("FBTCtlMessage get not a struct");
		}
		if(!this.arrayVal.containsKey(new Integer(index))) {
			this.arrayVal.put(new Integer(index), new FBTCtlMessage());
		}
		return (FBTCtlMessage) this.arrayVal.get(new Integer(index));
	}
	
	public int getIntVal() throws Exception {
		if(this.type != INT) throw new Exception("invalid type (not an int)");
		return ival;
	}
	
	public String getStringVal() throws Exception {
		if(this.type != STRING) throw new Exception("invalid type (not a string)");
		return new String(this.buffer,this.ival,this.slen,"utf-8");
	}
	
	public InputStream getStringInputStream() throws Exception {
		if(this.type != STRING) throw new Exception("invalid type (not a string)");
		return new ByteArrayInputStream(this.buffer,this.ival,this.slen);
	}
	
	public int getArraySize() throws Exception {
		if(this.type != ARRAY) throw new Exception("invalid type (not an array)");
		return this.arrayVal.size();
	}

	public int getType() {
		return this.type;
	}
	
	public void dump() throws Exception {
		Debuglog.debugln("dump msg********");
		this.dump(0,null);
		Debuglog.debugln("dump msg done***");
	}
	public void dump(int indent, MessageLayout layout) throws Exception{
		if(layout == null) {
			layout = MessageLayouts.getLayout(this.type);
			layout.dump();
		}
		if(layout == null)
			throw new Exception("no message layout");
		if(indent == 0)
			Debuglog.debugln("dumpMessage");
		Debuglog.debugln("type:"+this.type+" "+MessageLayouts.messageTypeName(this.type));
		switch(this.type) {
			case UNDEF: Debuglog.debugln("undefined"); break;
			case INT: Debuglog.debugln("int:"+this.ival); break;
			case STRING:
				String tmp;
				try {
					tmp=new String(this.buffer,this.ival,this.slen);
				} catch (Exception e) {
					tmp="not printable characters";
				}
				 Debuglog.debugln("string:"+tmp); break;
			case ARRAY: {
				Enumeration e=arrayVal.keys();
				while(e.hasMoreElements()) {
					Integer key = (Integer) e.nextElement();
					Debuglog.debugln("["+key+"]:");
					FBTCtlMessage sub = (FBTCtlMessage) arrayVal.get(key);
					sub.dump(indent+1, (MessageLayout)layout.childLayout.firstElement());
				}
				break; }
			default: {
				if(this.arrayVal == null) {
					System.out.println("no subnodes");
				} else {
					Enumeration e=layout.childLayout.elements();
					Hashtable validNodes=new Hashtable();
					while(e.hasMoreElements()) {
						MessageLayout childLayout= (MessageLayout) e.nextElement();
						System.out.print("[\""+childLayout.name+"\"]:\n");
						FBTCtlMessage sub = (FBTCtlMessage) this.arrayVal.get(childLayout.name);
						
						if(sub == null) {
							Debuglog.debugln(" NULL ");
						} else {
							sub.dump(indent+1, childLayout);
						}
						validNodes.put(childLayout.name, new Integer(1));
					}
					// checken ob elemente drinnen stehn die nicht rein gehören:
					e=this.arrayVal.keys();
					while(e.hasMoreElements()) {
						String name=(String) e.nextElement();
						if(!validNodes.containsKey(name)) {
							Debuglog.debugln("[\""+name+"\"] gehört nicht daher!!!!!!!");
						}
					}
				}
				break; }
		}
	}
	
	public boolean isType(String typeName) {
		 int type;
		 try{
			 type=MessageLayouts.messageTypeID(typeName);
		 } catch(Exception e) {
			 return false;
		 }
		 if(this.type == type) {
			 return true;
		 } else {
			 return false;
		 }
	}
}
