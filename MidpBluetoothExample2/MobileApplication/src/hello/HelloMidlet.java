/*
 * HelloMidlet.java
 *
 * Created on 15. Juli 2007, 14:47
 */

package hello;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.bluetooth.*;

import javax.microedition.io.Connector;
import javax.microedition.io.StreamConnection;
//import javax.microedition.io.*; // input stream, output stream

import java.io.DataInputStream;
import java.io.DataOutputStream;

// fürn timer:
import java.util.*;

/**
 *
 * @author chris
 */
public class HelloMidlet extends MIDlet implements CommandListener {
    
    /** Creates a new instance of HelloMidlet */
    public HelloMidlet() {
    }
    
	private Form helloForm;//GEN-BEGIN:MVDFields
	private Command exitCommand;
	private Command helpCommand1;
	private Alert help;
	private Form controllForm;
	private List listServer;
	private Command itemCommandDetail;
	private Command itemCommandSelect;
	private Command okCommand1;
	private Command screenCommand1;
	private Command backCommand2;//GEN-END:MVDFields
    

	/**
	 * tut zeile lesen
	 * @param buffer bitte gross genug machen sonst gibts exception
	 * @return nchars
	 *
	 */
	public int myReadLn(DataInputStream iStream, byte[] buffer) throws java.io.IOException 
	{
		int n=0;
		byte b;
		while((b=iStream.readByte()) != '\n') {
			buffer[n]=b;
			n++;
		}
		return n;
	}
	
	public class PingDing extends TimerTask {
		Object notifyObject;
		public PingDing(Object notifyObject) {
			this.notifyObject=notifyObject;
		}
		public void run() {
			System.out.println( "Running the task" );
			// HelloMidlet h =
			synchronized(this.notifyObject) {
				// helloForm.append("timer ");
				this.notifyObject.notify();
				// helloForm.append("done\n");
			}
		}
	}
//GEN-LINE:MVDMethods
    
    private void debugDataElement(DataElement attr, int level)
    {
        try {
            for(int i=0; i < level; i++)
                helloForm.append("  ");
            switch(attr.getDataType()) {
                case DataElement.DATSEQ: {
                    java.util.Enumeration en = (java.util.Enumeration) attr.getValue();
                    helloForm.append("(DATASEQ)\n");
                    int n=0;
                    while (en.hasMoreElements())
                    {
                        DataElement e = (DataElement) en.nextElement();
                        debugDataElement(e,level+1);
                        
                    }
                    helloForm.append("(/DATASEQ)\n");
                    break; }
                case DataElement.INT_1:
                case DataElement.U_INT_1:
                case DataElement.INT_4:
                case DataElement.U_INT_4: {
                    long l=attr.getLong();
                    helloForm.append("(INT)"+l);
                    break; }
                case DataElement.UUID:
                    helloForm.append("(UUID) ");
                default: {
                    helloForm.append(" '"+attr.getValue().toString()+"'");
                    break; }
                
            }
        } catch(ClassCastException e) {
            helloForm.append(" exception:"+e.toString());
        }
        helloForm.append("\n");
        
    }
    
	/** This method initializes UI of the application.//GEN-BEGIN:MVDInitBegin
	 */
	private void initialize() {//GEN-END:MVDInitBegin
        // Insert pre-init code here
        System.out.println("dlub");
        
		getDisplay().setCurrent(get_helloForm());//GEN-LINE:MVDInitInit
        helloForm.setTitle("bt scan...");
        // Insert post-init code here
        try {
            PrintClient client=new PrintClient(helloForm);
            ServiceRecord sr=client.findPrinter();
            System.out.println("findPrinter() done");
            if(sr == null) {
                helloForm.setTitle("NO SR");
            } else {
                String connectionURL=sr.getConnectionURL(0,false);
                helloForm.append("scan done ("+connectionURL+")");
                try {
                    String tmpDevName = sr.getHostDevice().getFriendlyName(false);
                    if (tmpDevName != null && tmpDevName.length() > 0) {
                        helloForm.append("name: "+tmpDevName+")");
                    }
                } catch (java.io.IOException e) { e.printStackTrace(); }
                int serviceNameOffset = 0x0000;
                int primaryLanguageBase = 0x0100;
                DataElement de = sr.getAttributeValue(primaryLanguageBase + serviceNameOffset);
                String srvName = null;
                if(de != null && de.getDataType() == DataElement.STRING) {
                        srvName = (String)de.getValue();
                        helloForm.append("service: "+srvName+")");
                } else {
					helloForm.append("no service name ");
				}
                helloForm.append("\n");
                
                int ids[]=sr.getAttributeIDs();
                helloForm.append(" "+ids.length);
                for(int i=0; i < ids.length; i++) {
                    DataElement attr=sr.getAttributeValue(ids[i]);
                    helloForm.append("[\""+ids[i]+"\"] type="+attr.getDataType());
                    debugDataElement(attr,0);
                }
                try {
                    StreamConnection connection = (StreamConnection)Connector.open(connectionURL);
                    // DataInputStream input = (InputConnection) connection.openDataInputStream();
                    // DataOutputStream output = connection.openDataOutputStream();
					DataInputStream iStream;
					DataOutputStream oStream;
					iStream = connection.openDataInputStream();
					oStream = connection.openDataOutputStream();
					// geht nicht: oStream.writeChars("hallo vom handy...\n");
					String s="hallo vom handy...\n";
					byte[] buffer = s.getBytes();
					oStream.write(buffer);
					/*
					for (int n = 0; n < stream.length; ++n) {
						oStream.writeByte(stream[n]);
					} */
					oStream.flush();
					helloForm.append("writeChars done\n");
					Timer timer = new Timer();
					Object timerwait=new Object();
					TimerTask task = new PingDing(timerwait);
					int timeout=1000;
					timer.schedule(task, timeout, timeout);
					buffer=new byte[50];
					int commandNr=0;
					StringItem pingtext=new StringItem("pingtext","");
					StringItem sendtext=new StringItem("sendtext","");
					StringItem readtext=new StringItem("readtext","");
					StringItem pingstat=new StringItem("pingstat","");
					helloForm.append(pingtext);
					helloForm.append(sendtext);
					helloForm.append(readtext);
					helloForm.append(pingstat);
					int minTPing=10000;
					int maxTPing=0;
					int avgTPing=0;
					while(true) {
						synchronized(timerwait) {
							try {
								// helloForm.append("wait...");
								timerwait.wait();
								// helloForm.append("...wait done\n");
							} catch (Exception e) {
								helloForm.append("wait exception ("+e.toString()+')');
								return;
							}
						}
						long startTime = System.currentTimeMillis();
						s=commandNr+" nop\n";
						oStream.write(s.getBytes()); oStream.flush();
						// helloForm.append("sent: "+s);
						sendtext.setText("sent: "+s);
						boolean found=false;
						while(!found) {
							int n =myReadLn(iStream,buffer);
							s=new String(buffer,0,n);
							// helloForm.append("read done ("+n+"): "+s+'\n');
							readtext.setText("read done ("+n+"): "+s+'\n');
							if(n<=1) {
								helloForm.append("read 0 bytes?!?!?");
								return;
							}
							String sreply_commandNr="";
							n=0;
							char c;
							
							while(n < s.length() && Character.isDigit(c=s.charAt(n))) {
								sreply_commandNr+=c;
								n++;
							}
							// helloForm.append("rx: "+sreply_commandNr+'\n');
							pingtext.setText("rx: "+sreply_commandNr+'\n');
							if(sreply_commandNr.length() == 0)
								continue;
							int reply_commandNr=Integer.parseInt(sreply_commandNr);
							if(reply_commandNr==commandNr)
								found=true;
						}
						commandNr++;
						long endTime = System.currentTimeMillis();
						int t=(int) (endTime-startTime);
						if(t < minTPing) minTPing = t;
						if(t > maxTPing) maxTPing = t;
						avgTPing+=t;
						pingstat.setText("min: "+minTPing+" max:"+maxTPing+" avg:"+(avgTPing/commandNr));
						
					}
                } catch (java.io.IOException e) {
					helloForm.append("exception("+e.toString()+")\n");
                }
            }
        } catch (BluetoothStateException e) {
            helloForm.append("exception: ("+e.toString()+')');
            e.printStackTrace();
        }
	}//GEN-LINE:MVDInitEnd
    
	/** Called by the system to indicate that a command has been invoked on a particular displayable.//GEN-BEGIN:MVDCABegin
	 * @param command the Command that ws invoked
	 * @param displayable the Displayable on which the command was invoked
	 */
	public void commandAction(Command command, Displayable displayable) {//GEN-END:MVDCABegin
        // Insert global pre-action code here
		if (displayable == helloForm) {//GEN-BEGIN:MVDCABody
			if (command == exitCommand) {//GEN-END:MVDCABody
                // Insert pre-action code here
				exitMIDlet();//GEN-LINE:MVDCAAction3
                // Insert post-action code here
			} else if (command == helpCommand1) {//GEN-LINE:MVDCACase3
				// Insert pre-action code here
				getDisplay().setCurrent(get_help(), get_helloForm());//GEN-LINE:MVDCAAction8
				// Insert post-action code here
			} else if (command == screenCommand1) {//GEN-LINE:MVDCACase8
				// Insert pre-action code here
				getDisplay().setCurrent(get_listServer());//GEN-LINE:MVDCAAction18
				// Insert post-action code here
			} else if (command == okCommand1) {//GEN-LINE:MVDCACase18
				// Insert pre-action code here
				// Do nothing//GEN-LINE:MVDCAAction16
				// Insert post-action code here
			}//GEN-BEGIN:MVDCACase16
		} else if (displayable == listServer) {
			if (command == backCommand2) {//GEN-END:MVDCACase16
				// Insert pre-action code here
				getDisplay().setCurrent(get_helloForm());//GEN-LINE:MVDCAAction21
				// Insert post-action code here
			}//GEN-BEGIN:MVDCACase21
		}//GEN-END:MVDCACase21
        // Insert global post-action code here
}//GEN-LINE:MVDCAEnd
    
    /**
     * This method should return an instance of the display.
     */
    public Display getDisplay() {//GEN-FIRST:MVDGetDisplay
        return Display.getDisplay(this);
    }//GEN-LAST:MVDGetDisplay
    
    /**
     * This method should exit the midlet.
     */
    public void exitMIDlet() {//GEN-FIRST:MVDExitMidlet
        getDisplay().setCurrent(null);
        destroyApp(true);
        notifyDestroyed();
    }//GEN-LAST:MVDExitMidlet
    
	/** This method returns instance for helloForm component and should be called instead of accessing helloForm field directly.//GEN-BEGIN:MVDGetBegin2
	 * @return Instance for helloForm component
	 */
	public Form get_helloForm() {
		if (helloForm == null) {//GEN-END:MVDGetBegin2
            // Insert pre-init code here
			helloForm = new Form(null, new Item[0]);//GEN-BEGIN:MVDGetInit2
			helloForm.addCommand(get_exitCommand());
			helloForm.addCommand(get_helpCommand1());
			helloForm.addCommand(get_screenCommand1());
			helloForm.setCommandListener(this);//GEN-END:MVDGetInit2
            // Insert post-init code here
		}//GEN-BEGIN:MVDGetEnd2
		return helloForm;
	}//GEN-END:MVDGetEnd2
    
    
	/** This method returns instance for exitCommand component and should be called instead of accessing exitCommand field directly.//GEN-BEGIN:MVDGetBegin5
	 * @return Instance for exitCommand component
	 */
	public Command get_exitCommand() {
		if (exitCommand == null) {//GEN-END:MVDGetBegin5
            // Insert pre-init code here
			exitCommand = new Command("Exit", Command.EXIT, 1);//GEN-LINE:MVDGetInit5
            // Insert post-init code here
		}//GEN-BEGIN:MVDGetEnd5
		return exitCommand;
	}//GEN-END:MVDGetEnd5

	/** This method returns instance for helpCommand1 component and should be called instead of accessing helpCommand1 field directly.//GEN-BEGIN:MVDGetBegin6
	 * @return Instance for helpCommand1 component
	 */
	public Command get_helpCommand1() {
		if (helpCommand1 == null) {//GEN-END:MVDGetBegin6
			// Insert pre-init code here
			helpCommand1 = new Command("Help", Command.HELP, 1);//GEN-LINE:MVDGetInit6
			// Insert post-init code here
		}//GEN-BEGIN:MVDGetEnd6
		return helpCommand1;
	}//GEN-END:MVDGetEnd6

	/** This method returns instance for help component and should be called instead of accessing help field directly.//GEN-BEGIN:MVDGetBegin9
	 * @return Instance for help component
	 */
	public Alert get_help() {
		if (help == null) {//GEN-END:MVDGetBegin9
			// Insert pre-init code here
			help = new Alert(null, "Hilfe dingens ...\n", null, null);//GEN-BEGIN:MVDGetInit9
			help.setTimeout(-2);//GEN-END:MVDGetInit9
			// Insert post-init code here
		}//GEN-BEGIN:MVDGetEnd9
		return help;
	}//GEN-END:MVDGetEnd9

	/** This method returns instance for controllForm component and should be called instead of accessing controllForm field directly.//GEN-BEGIN:MVDGetBegin10
	 * @return Instance for controllForm component
	 */
	public Form get_controllForm() {
		if (controllForm == null) {//GEN-END:MVDGetBegin10
			// Insert pre-init code here
			controllForm = new Form(null, new Item[0]);//GEN-LINE:MVDGetInit10
			// Insert post-init code here
		}//GEN-BEGIN:MVDGetEnd10
		return controllForm;
	}//GEN-END:MVDGetEnd10

	/** This method returns instance for listServer component and should be called instead of accessing listServer field directly.//GEN-BEGIN:MVDGetBegin11
	 * @return Instance for listServer component
	 */
	public List get_listServer() {
		if (listServer == null) {//GEN-END:MVDGetBegin11
			// Insert pre-init code here
			listServer = new List("gefundene Server", Choice.IMPLICIT, new String[0], new Image[0]);//GEN-BEGIN:MVDGetInit11
			listServer.addCommand(get_backCommand2());
			listServer.setCommandListener(this);
			listServer.setSelectedFlags(new boolean[0]);
			listServer.setSelectCommand(get_itemCommandSelect());//GEN-END:MVDGetInit11
			// Insert post-init code here
		}//GEN-BEGIN:MVDGetEnd11
		return listServer;
	}//GEN-END:MVDGetEnd11

	/** This method returns instance for itemCommandDetail component and should be called instead of accessing itemCommandDetail field directly.//GEN-BEGIN:MVDGetBegin13
	 * @return Instance for itemCommandDetail component
	 */
	public Command get_itemCommandDetail() {
		if (itemCommandDetail == null) {//GEN-END:MVDGetBegin13
			// Insert pre-init code here
			itemCommandDetail = new Command("Details", Command.ITEM, 1);//GEN-LINE:MVDGetInit13
			// Insert post-init code here
		}//GEN-BEGIN:MVDGetEnd13
		return itemCommandDetail;
	}//GEN-END:MVDGetEnd13

	/** This method returns instance for itemCommandSelect component and should be called instead of accessing itemCommandSelect field directly.//GEN-BEGIN:MVDGetBegin14
	 * @return Instance for itemCommandSelect component
	 */
	public Command get_itemCommandSelect() {
		if (itemCommandSelect == null) {//GEN-END:MVDGetBegin14
			// Insert pre-init code here
			itemCommandSelect = new Command("Select", Command.ITEM, 1);//GEN-LINE:MVDGetInit14
			// Insert post-init code here
		}//GEN-BEGIN:MVDGetEnd14
		return itemCommandSelect;
	}//GEN-END:MVDGetEnd14

	/** This method returns instance for okCommand1 component and should be called instead of accessing okCommand1 field directly.//GEN-BEGIN:MVDGetBegin15
	 * @return Instance for okCommand1 component
	 */
	public Command get_okCommand1() {
		if (okCommand1 == null) {//GEN-END:MVDGetBegin15
			// Insert pre-init code here
			okCommand1 = new Command("Ok", Command.OK, 1);//GEN-LINE:MVDGetInit15
			// Insert post-init code here
		}//GEN-BEGIN:MVDGetEnd15
		return okCommand1;
	}//GEN-END:MVDGetEnd15

	/** This method returns instance for screenCommand1 component and should be called instead of accessing screenCommand1 field directly.//GEN-BEGIN:MVDGetBegin17
	 * @return Instance for screenCommand1 component
	 */
	public Command get_screenCommand1() {
		if (screenCommand1 == null) {//GEN-END:MVDGetBegin17
			// Insert pre-init code here
			screenCommand1 = new Command("ShowList", Command.SCREEN, 1);//GEN-LINE:MVDGetInit17
			// Insert post-init code here
		}//GEN-BEGIN:MVDGetEnd17
		return screenCommand1;
	}//GEN-END:MVDGetEnd17

	/** This method returns instance for backCommand2 component and should be called instead of accessing backCommand2 field directly.//GEN-BEGIN:MVDGetBegin20
	 * @return Instance for backCommand2 component
	 */
	public Command get_backCommand2() {
		if (backCommand2 == null) {//GEN-END:MVDGetBegin20
			// Insert pre-init code here
			backCommand2 = new Command("Back", Command.BACK, 1);//GEN-LINE:MVDGetInit20
			// Insert post-init code here
		}//GEN-BEGIN:MVDGetEnd20
		return backCommand2;
	}//GEN-END:MVDGetEnd20
    
    public void startApp() {
        initialize();
    }
    
    public void pauseApp() {
    }
    
    public void destroyApp(boolean unconditional) {
    }
    
}
