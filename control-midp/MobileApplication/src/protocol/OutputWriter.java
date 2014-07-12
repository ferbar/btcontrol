/*
 *  This file is part of btcontroll
 *  btcontroll is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  btcontroll is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with btcontroll.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * zum message zusammenbaun
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
