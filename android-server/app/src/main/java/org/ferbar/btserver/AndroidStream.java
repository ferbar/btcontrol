package org.ferbar.btserver;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.SocketException;

/**
 * Created by chris on 05.01.17.
 */
public class AndroidStream implements org.ferbar.btcontrol.BTcommThread.PlattformStream {
    Socket client=null;
    public AndroidStream(Socket client) {
        this.client=client;
        try {
            this.client.setSoTimeout(10000); // ms
        } catch (SocketException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void close() throws IOException {
        this.client.close();
    }

    @Override
    public InputStream openInputStream() throws IOException {
        return this.client.getInputStream();
    }

    @Override
    public OutputStream openOutputStream() throws IOException {
        return this.client.getOutputStream();
    }

    @Override
    public void connect() throws IOException {
        // nix machen, ham schon eine verbindung
    }
}
