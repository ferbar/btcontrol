/**
 * 
 * FBTCtlMessage f(msgtype);
 * f.get("info").get(0).get("addr").set(1);
 * 
 */

package protocol;

//import java.util.Vector;
import java.util.Hashtable;
import java.util.Enumeration;

/**
 *
 */
public class FBTCtlMessage {
	String name;
	int ival;
	String sval;
	int type;
	Hashtable arrayVal; // struct:key=string + array:key=int
	
	public static final int UNDEF = 0;
	public static final int INT = 1;
	public static final int STRING = 2;
	public static final int ARRAY = 3;
	public static final int STRUCT = 4;

	FBTCtlMessage(int i) {
		type=INT;
		ival=i;
	}
			
	FBTCtlMessage(String s) {
		type=STRING;
		sval=s;
	}
	
	FBTCtlMessage() {
		type=UNDEF;
	}
	
	
			
	/**
	 * liest eine bin message
	 */
	public void readMessage(MessageLayout layout) {
		***************** TODO ***************
	}
	
	/**
	 * zugriff auf einen struct node, legt subnode an wenns den nicht gibt
	 */
	public FBTCtlMessage get(String index) {
		if(this.type == UNDEF) {
			this.type=STRUCT;
			this.arrayVal=new Hashtable();
		}
		if(this.type != STRUCT) {
			throw new Exception("FBTCtlMessage get not a struct");
		}
		if(!this.arrayVal.containsKey(index)) {
			this.arrayVal.put(index, new FBTCtlMessage());
		}
		return (FBTCtlMessage) this.arrayVal.get(index);
	}
	
	public FBTCtlMessage get(int index) {
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
		return sval;
	}
	
	
	public String getBinaryMessage(MessageLayout layout) {
		String ret="";
		return ret;
	}
	
	public void dump() throws Exception {
		this.dump(0,null);
	}
	public void dump(int indent, MessageLayout layout) throws Exception{
		if(layout == null)
			layout = MessageLayouts.getLayout(this.type);
		if(layout == null)
			throw new Exception("no message layout");
		if(indent == 0)
			System.out.println("dumpMessage\n");
		System.out.println("type:"+this.type+" "+MessageLayouts.messageTypeName(this.type)+"\n");
		switch(this.type) {
			case UNDEF: System.out.println("undefined"); break;
			case INT: System.out.println("int:"+this.ival); break;
			case STRING: System.out.println("string:"+this.sval); break;
			case ARRAY: {
				Enumeration e=arrayVal.elements();
				while(e.hasMoreElements()) {
					Integer key = (Integer) e.nextElement();
					System.out.println("["+key+"]:");
					FBTCtlMessage sub = (FBTCtlMessage) arrayVal.get(key);
					sub.dump(indent+1, (MessageLayout)layout.childLayout.get(new Integer(0)));
				}
				break; }
			default: {
				Enumeration e=arrayVal.elements();
				while(e.hasMoreElements()) {
					String key = (String) e.nextElement();
					System.out.println("[\""+key+"\":");
					FBTCtlMessage sub = (FBTCtlMessage) arrayVal.get(key);
					sub.dump(indent+1, (MessageLayout)layout.childLayout.get(new Integer(0)));
				}
				break; }
		}
	}
}
