/*
 * Created on 27.10.2004
 *
 * TODO To change the template for this generated file go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
package de.avetana.bluetooth.obex;

import java.io.*;

import javax.microedition.io.*;
import javax.obex.*;

/**
 * @author gmelin
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */

public class SessionNotifierImpl implements SessionNotifier {

	private StreamConnectionNotifier locConNot;
	
	public SessionNotifierImpl (StreamConnectionNotifier locConNot) {
		this.locConNot = locConNot;
	}
	
	/* (non-Javadoc)
	 * 	 * @see javax.obex.SessionNotifier#acceptAndOpen(javax.obex.ServerRequestHandler)
	 */
	public synchronized  Connection acceptAndOpen(ServerRequestHandler handler)
			throws IOException {
		return this.acceptAndOpen(handler,null);
	}
	
	public synchronized  Connection acceptAndOpen(ServerRequestHandler handler, Authenticator auth)
		throws IOException {
		if (locConNot == null) throw new IOException ("ConnectionNotifier is null ! maybe it was closed previousely..");
		StreamConnection streamCon = locConNot.acceptAndOpen();
		SessionHandler sh = new SessionHandler (streamCon, auth, handler);
		sh.start();
		return sh;
	}
	
	public StreamConnectionNotifier getConnectionNotifier() {
		return (StreamConnectionNotifier)locConNot;
	}

	public void close() throws IOException {
		if (locConNot != null) locConNot.close();
		locConNot.close();
	}

}
