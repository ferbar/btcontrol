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
	private byte inbuffer[];
	private int pos=0;
	
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
	public String getString() {
		int len=getByte() | (getByte() << 8);
		String ret=new String(this.inbuffer,this.pos,len);
		this.pos+=len;
		return ret;
	}
}
