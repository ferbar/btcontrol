/*
 *  This file is part of btcontrol
 *
 *  Copyright (C) Christian Ferbar
 *
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
 */
/*
 * android TCP IO wrapper
 */
package org.ferbar.btcontrol;

import java.io.IOException;
import java.net.Socket;
import java.io.InputStream;
import java.io.OutputStream;

import org.ferbar.btcontrol.BTcommThread.PlattformStream;


public class AndroidStream implements PlattformStream {

	public AndroidStream(String server, int port) {
		this.server=server;
		this.port=port;
	}
	public void close() throws IOException {
		this.socket.close();
	}

	public InputStream openInputStream() throws IOException {
		return this.socket.getInputStream();
	}

	public OutputStream openOutputStream() throws IOException {
		return this.socket.getOutputStream();
	}

	public void connect() throws IOException {
		System.out.println("connecting:"+this.server+":"+this.port);
		this.socket = new Socket(this.server, this.port);
	}
	
	public String toString() {
		return "Server:"+this.server+":"+this.port+" connected:"+((this.socket != null) ? this.socket.isConnected() : "0");
	}

	public String server;
	public int port;
	private Socket socket;
}
