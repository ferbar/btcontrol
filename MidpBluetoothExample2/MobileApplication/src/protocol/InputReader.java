/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package protocol;

/**
 *
 * @author chris
 */
public class InputReader {
	public byte inbuffer[];
	private int pos=0;
	private int lastLen=0;
	
	public InputReader(byte inbuffer[]) {
		this.inbuffer=inbuffer;
	}
	public void reset() {
		pos=0;
	}
	
	public int getByte() {
		return 0xff & ((int) this.inbuffer[pos++]);
	}
	
	public int getInt() {
		int ret=(0xff & (int)this.inbuffer[pos++]) | ((0xff & (int)this.inbuffer[pos++]) << 8) | 
				((0xff & (int)this.inbuffer[pos++]) << 16) | ((0xff & (int)this.inbuffer[pos++]) << 24);
		return ret;
	}
	
	/**
	 * String s = new String(in.inbuffer, in.getStrOff(), in.getStrLen());
	 * länge = getStrLen 
	 * offset = getStr
	 */
	public int getStrOff() {
		this.lastLen=getByte() | (getByte() << 8);
		
		return this.pos;
	}
	public int getStrLen() {
		this.pos+=this.lastLen;
		/*
		System.out.println("getString: len="+len);
		if(len > 2) {
			System.out.println("[0]="+this.inbuffer[this.pos]+" [1]="+this.inbuffer[this.pos+1]+
				" [2]="+this.inbuffer[this.pos+2]);
			System.out.println("[0]="+ret.charAt(0)+" [1]="+ret.charAt(1)+
				" [2]="+ret.charAt(2));
		}*/
		return this.lastLen;
	}
}
