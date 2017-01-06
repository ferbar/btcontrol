package org.ferbar.btserver;

import android.content.Context;
import android.net.nsd.NsdServiceInfo;

import java.io.IOException;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;

import ferbar.org.btserver.R;
import protocol.MessageLayouts;

/**
 * Created by chris on 31.12.16.
 */
public class BTcommServer extends Thread {
    public static final int SERVERPORT = 3030;
    // private OnMessageReceived messageListener;
    private boolean running = false;
    private PrintWriter mOut;

    private BTcommServerMDNS mdns = new BTcommServerMDNS();
    ServerSocket serverSocket=null;

    public BTcommServer(Context context) throws Exception {
        MessageLayouts messageLayouts = new MessageLayouts();
        messageLayouts.load(context.getResources().openRawResource(R.raw.protocol));
        this.mdns.registerService(context, BTcommServer.SERVERPORT);
    }

    public void close() throws IOException {
        if(this.serverSocket != null) {
            this.serverSocket.close();
        }
        if(this.mdns != null) {
            this.mdns.unregisterService();
        }
    }


    @Override
    public void run() {
        super.run();

        running = true;

        System.out.println("S: Listening...");

        // try(ServerSocket serverSocket = new ServerSocket(SERVERPORT)) { erst ab min Api version 19 ...
        try {
            serverSocket = new ServerSocket(SERVERPORT);
            //create a server socket. A server socket waits for requests to come in over the network.

            while(true) {
                //create client socket... the method accept() listens for a connection to be made to this socket and accepts it.
                System.out.println("S: accepting connections...");
                Socket client = serverSocket.accept();
                System.out.println("S: Receiving...");

                BTcommThread clientThread = new BTcommThread(new AndroidStream(client));
                clientThread.start();
                /*
                //sends the message to the client
                mOut = new PrintWriter(new BufferedWriter(new OutputStreamWriter(client.getOutputStream())), true);

                //read the message received from client
                BufferedReader in = new BufferedReader(new InputStreamReader(client.getInputStream()));

                //in this while we wait to receive messages from client (it's an infinite loop)
                //this while it's like a listener for messages
                while (running) {
                    String message = in.readLine();

                    if (message != null && messageListener != null) {
                        //call the method messageReceived from ServerBoard class
                        messageListener.messageReceived(message);
                    }
                }
                */
            }
        } catch (Exception e) {
            System.out.println("S: Error");
            e.printStackTrace();

        } finally {
            System.out.println("S: Done.");
            if(serverSocket != null) {
                try {
                    this.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }

    }

    /*
    public void sendMessage(String message){
        if (mOut != null && !mOut.checkError()) {
            mOut.println(message);
            mOut.flush();
        }
    }
    */
}
