package btcontrol;

import java.io.IOException;
import java.net.Socket;
import java.io.InputStream;
import java.io.OutputStream;

import btcontrol.BTcommThread.PlattformStream;

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
