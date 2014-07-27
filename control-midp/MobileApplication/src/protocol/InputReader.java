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
 * zum messages parsen
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
