/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package protocol;

/**
 *
 * @author chris
 */
public class OutputWriter {
	final int avail=32; // max message len - sollte fürs handy -> btserver reichen
	public int pos=0;
	public byte ret[]=new byte[avail];
	
	public void putByte(int i) {
		this.ret[this.pos]  =(byte)(i & 0xff);
		this.pos++;
	}
	
	public void putInt(int i) {
		this.ret[this.pos]  =(byte)(i & 0xff);
		this.ret[this.pos+1]=(byte)((i >> 8) & 0xff);
		this.ret[this.pos+2]=(byte)((i >> 16) & 0xff);
		this.ret[this.pos+3]=(byte)((i >> 24) & 0xff);
		this.pos+=4;
	}
	public void putString(byte buffer[], int off, int len) {
		this.putByte(len & 0xff);
		this.putByte(len >> 8);
		for(int i=0; i < len; i++) {
			this.ret[pos++] = buffer[i+off];
		}
		
	}

	public byte[] getBytes() {
		return this.ret;
	}
	public int getSize() {
		return this.pos;
	}
}
