package org.ferbar.btserver;

import android.util.Log;

import protocol.FBTCtlMessage;
import protocol.MessageLayouts;

/**
 * Created by chris on 31.12.16.
 */
public class BTcommThread extends org.ferbar.btcontrol.BTcommThread {
    final String TAG="BTcommThread";

    int clientID=0;

    int currspeed=0;
    int currDir=0;

    /**
     * Creates a new instance of BTcommThread
     *
     * @param BTStreamConnection für MIDP/android spezifisches connect
     */
    public BTcommThread(PlattformStream BTStreamConnection) {
        super(BTStreamConnection);
    }


    @Override
    public void run() {

        try {
            this.connect();
            System.out.printf("%d:socket accepted sending welcome msg\n", this.clientID);
            FBTCtlMessage heloReply = new FBTCtlMessage();

            heloReply.setType(MessageLayouts.messageTypeID("HELO"));

            heloReply.get("name").set("my bt server");
            heloReply.get("version").set("0.9");
            heloReply.get("protohash").set(MessageLayouts.hash);
            // heloReply.dump();
            sendMessage(heloReply);
            System.out.printf("%d:hello done, enter main loop\n", this.clientID);
            while(true) {
                FBTCtlMessage cmd=this.receiveMessage();
                System.out.println("new message:" + MessageLayouts.messageTypeName(cmd.getType()) );
                if(cmd.isType("PING")) {
                    sendStatusReply();
                } else if(cmd.isType("ACC")) {
                    int addr=cmd.get("addr").getIntVal();
                    this.currspeed+=5;
                    if(this.currspeed > 255)
                        this.currspeed=255;
                    sendStatusReply();
                    String ret=MainActivity.usbArduino.sendCommand(String.format("M%02x", this.currspeed));
                    Log.d(TAG, "ACC ret:" + ret);
                } else if(cmd.isType("BREAK")) {
                    int addr=cmd.get("addr").getIntVal();
                    this.currspeed-=5;
                    if(this.currspeed < 0)
                        this.currspeed=0;
                    sendStatusReply();
                    String ret=MainActivity.usbArduino.sendCommand(String.format("M%02x", this.currspeed));
                    Log.d(TAG, "ACC ret:" + ret);
                } else if(cmd.isType("DIR")) {
                    int addr=cmd.get("addr").getIntVal();
                    int dir=cmd.get("dir").getIntVal();
                    System.out.println("new dir:"+dir);
                    if(dir != 1 && dir != -1) {
                        throw new Exception("invalid dir");
                    }
                    if(this.currspeed != 0) {
                        throw new Exception("speed != 0");
                    }
                    this.currDir=dir;
                    sendStatusReply();

                    String ret=MainActivity.usbArduino.sendCommand("D"+this.currDir);
                    Log.d(TAG, "ACC ret:" + ret);
                } else if(cmd.isType("POWER")) {
                    FBTCtlMessage reply = new FBTCtlMessage(); reply.setType(MessageLayouts.messageTypeID("POWER_REPLY"));
                    reply.get("value").set(-1);
                    sendMessage(reply);
                    String ret=MainActivity.usbArduino.sendCommand("M00");
                    Log.d(TAG, "STOP ret:" + ret);
                } else if(cmd.isType("STOP")) {
                    int addr=cmd.get("addr").getIntVal(); // müsste immer 3 sein
                    this.currspeed=0;
                    sendStatusReply();
                    String ret=MainActivity.usbArduino.sendCommand("M00");
                    Log.d(TAG, "STOP ret:" + ret);
                } else if(cmd.isType("GETLOCOS")) { // liste mit eingetragenen loks abrufen, format: <name>;<adresse>;...\n
                    FBTCtlMessage reply = new FBTCtlMessage(); reply.setType(MessageLayouts.messageTypeID("GETLOCOS_REPLY"));
                    int i=0;
                    reply.get("info").get(0).get("addr").set(3);
                    reply.get("info").get(0).get("name").set("android");
                    reply.get("info").get(0).get("imgname").set("");
                    reply.get("info").get(0).get("speed").set(this.currspeed == 0 ? this.currDir : this.currspeed*this.currDir);
                    reply.get("info").get(0).get("functions").set(1);

                    /*
                        reply["info"][i]["imgname"]=lokdef[i].imgname;
                        if(lokdef[i].currspeed==0) {
                            reply["info"][i]["speed"]=lokdef[i].currdir;
                        } else {
                            reply["info"][i]["speed"]=lokdef[i].currspeed * lokdef[i].currdir;
                        }
                        */
                    // reply.dump();
                    sendMessage(reply);
                } else if(cmd.isType("GETFUNCTIONS")) {
                    int addr=cmd.get("addr").getIntVal(); // müsste immer 3 sein

                    FBTCtlMessage reply = new FBTCtlMessage(); reply.setType(MessageLayouts.messageTypeID("GETFUNCTIONS_REPLY"));
                    int i=0;
                    reply.get("info").get(0).get("name").set("testfunc");
                    reply.get("info").get(0).get("value").set(1);
                    reply.get("info").get(0).get("imgname").set("");

                    sendMessage(reply);
                } else {
                    System.out.println("message not yet implemented:"); cmd.dump();
                    throw new Exception("invalid message");
                }
            }


        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            this.close(true);
        }
    }

    private void sendStatusReply() throws Exception {
        FBTCtlMessage reply = new FBTCtlMessage();
        reply.setType(MessageLayouts.messageTypeID("STATUS_REPLY"));
        setLokStatus(reply);
        // reply.dump();
        sendMessage(reply);
    }

    int lastSpeed=-1;
    private void setLokStatus(FBTCtlMessage reply) throws Exception {
        reply.get("info").get(0).get("addr").set(3);
        reply.get("info").get(0).get("speed").set(this.currspeed == 0 ? this.currDir : this.currspeed*this.currDir);
        reply.get("info").get(0).get("functions").set(1);
    }

}
