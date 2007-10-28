/*
 * HelloMidlet.java
 *
 * Created on 15. Juli 2007, 14:47
 */

package btcontroll;
import java.util.Hashtable;
		
import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.bluetooth.*;

import javax.microedition.io.Connector;
import javax.microedition.io.StreamConnection;
//import javax.microedition.io.*; // input stream, output stream

// import java.io.DataInputStream;
// import java.io.DataOutputStream;

// fürn timer:
// import java.util.*;

	
/**
 *
 * @author chris
 */
public class HelloMidlet extends MIDlet implements CommandListener, iMyMessages, PrintClient.iAddAvailService {
    

	
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
	private Command okCommand1;
	private Command screenCommand1;
	private Command backCommand2;
	private Command screenCommand2;
//GEN-END:MVDFields
    
	private Canvas controllCanvas;
	BTcommThread btcomm;
	Hashtable availServices = new Hashtable(); // indexda = index im listServer
	private Command itemCommandShowServiceRecord=new Command("ServiceRecord", Command.ITEM, 1);

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
	
	public void debugServiceRecord(ServiceRecord sr) {
		helloForm.append(new javax.microedition.lcdui.Spacer(10,10));
            System.out.println("debugServiceRecord() done");
            if(sr == null) {
                // helloForm.setTitle("NO SR");
            } else {
                String connectionURL=sr.getConnectionURL(0,false);
                helloForm.append("debugServiceRecord ("+connectionURL+")");
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
                helloForm.append("attrlist len:"+ids.length+"\n");
                for(int i=0; i < ids.length; i++) {
                    DataElement attr=sr.getAttributeValue(ids[i]);
                    helloForm.append("[\""+ids[i]+"\"] type="+attr.getDataType());
                    debugDataElement(attr,0);
                }
            }
	}
	
	public void updateStatus(String text) {
		helloForm.setTitle(text);
	}
    public void debug(String text) {
        helloForm.append(text);
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
//			getKeyStates();
            PrintClient client=new PrintClient(this,this);
			client.findPrinter();
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
		try {
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
			} else if (command == screenCommand2) {//GEN-LINE:MVDCACase21
				if(btcomm == null) {
					helloForm.setTitle("connecting ...");
					try {
						String connectionURL=listServer.getString(listServer.getSelectedIndex());
						StreamConnection connection = (StreamConnection)Connector.open(connectionURL);
						// DataInputStream input = (InputConnection) connection.openDataInputStream();
						// DataOutputStream output = connection.openDataOutputStream();
						btcomm = new BTcommThread(helloForm, connection.openDataInputStream(), connection.openDataOutputStream());
						Thread t = new Thread(btcomm);
						t.start();
						helloForm.setTitle("connected");
					} catch (java.io.IOException e) {
						helloForm.append("exception("+e.toString()+")\n");
						helloForm.setTitle("connecting exception...");
					} 
				}
				// Insert pre-action code here
				getDisplay().setCurrent(get_controllCanvas());
				/*
				getDisplay().setCurrent(get_controllForm());//GEN-LINE:MVDCAAction23
				*/
				// Insert post-action code here
			} else if (command == itemCommandShowServiceRecord) {
				int index=listServer.getSelectedIndex();
				String url=listServer.getString(index);
				//debug("sr["+index+"="+url+"] -----------\n");
				Object o = this.availServices.get(url);
				//debug("sr o\n");
				debug("sr o="+o.toString());
				ServiceRecord sr = (ServiceRecord) o;
				//debug("sr cast ok\n");
				//debug("sr ----------"+sr.toString()+"\n");
				this.debugServiceRecord(sr);
				debug("sr done--------\n");
			}//GEN-BEGIN:MVDCACase23
		}//GEN-END:MVDCACase23
		else if(displayable == controllCanvas) {
			if(command == exitCommand) {
				getDisplay().setCurrent(get_helloForm());
			}
		}
		// Insert global post-action code here
		} catch(Exception e) {
			Alert alert;
			alert = new Alert("Exception",e.getMessage(),null,null);
			alert.setTimeout(Alert.FOREVER);
			getDisplay().setCurrent(alert);
		}
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
    
	public Canvas get_controllCanvas() {
		if(controllCanvas==null) {
			controllCanvas=new MIDPCanvas(btcomm);
			controllCanvas.setCommandListener(this);
			controllCanvas.addCommand(exitCommand);
		}
		return controllCanvas;
	}

    
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

	/** This method returns instance for listServer component and should be called instead of accessing listServer field directly.//GEN-BEGIN:MVDGetBegin11
	 * @return Instance for listServer component
	 */
	public List get_listServer() {
		if (listServer == null) {//GEN-END:MVDGetBegin11
			// Insert pre-init code here
			listServer = new List("gefundene Server", Choice.IMPLICIT, new String[0], new Image[0]);//GEN-BEGIN:MVDGetInit11
			listServer.addCommand(get_backCommand2());
			listServer.addCommand(get_screenCommand2());
			listServer.setCommandListener(this);
			listServer.setSelectedFlags(new boolean[0]);
			listServer.setSelectCommand(get_screenCommand2());//GEN-END:MVDGetInit11
			listServer.addCommand(itemCommandShowServiceRecord);
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

	/** This method returns instance for screenCommand2 component and should be called instead of accessing screenCommand2 field directly.//GEN-BEGIN:MVDGetBegin22
	 * @return Instance for screenCommand2 component
	 */
	public Command get_screenCommand2() {
		if (screenCommand2 == null) {//GEN-END:MVDGetBegin22
			// Insert pre-init code here
			screenCommand2 = new Command("controllForm", Command.SCREEN, 1);//GEN-LINE:MVDGetInit22
			// Insert post-init code here
		}//GEN-BEGIN:MVDGetEnd22
		return screenCommand2;
	}//GEN-END:MVDGetEnd22
//GEN-LINE:MVDGetEnd10
    public void startApp() {//GEN-LINE:MVDGetInit10
        initialize();//GEN-LINE:MVDGetBegin10
    }
    
    public void pauseApp() {
		// schätzometrisch tät das eh nur timeout machen ...
		btcomm=null;
    }
    
    public void destroyApp(boolean unconditional) {
    }

	public void AddAvailService(ServiceRecord sr) {
		if(isBtControllService(sr)) {
			String url=sr.getConnectionURL(0,false);
			int p=url.indexOf(";");
			url=url.substring(0,p);
			int index = this.get_listServer().append(url,null);
			availServices.put(url,sr);
		}
		
	}

	/**
	 *
	 * bissi mühsamer code damit ma sr["4"]=dataseq[1]=dataseq[1]=int bekommt 
	 * TODO: das attr[nummer] kapseln ...
	 *
	 */
	public boolean isBtControllService(ServiceRecord sr) {
		try {
			DataElement attr=sr.getAttributeValue(4);
			if(DataElement.DATSEQ == attr.getDataType()) {
				java.util.Enumeration en = (java.util.Enumeration) attr.getValue();
				int n=0;
				while (en.hasMoreElements()) {
					DataElement attr1 = (DataElement) en.nextElement();
					if(n==1) {
						if(DataElement.DATSEQ == attr1.getDataType()) {
							java.util.Enumeration en1 = (java.util.Enumeration) attr1.getValue();
							n=0;
							while (en1.hasMoreElements()) {
								DataElement attr2 = (DataElement) en1.nextElement();
								if(n==1) {
									if(DataElement.U_INT_1 == attr2.getDataType()){
										long port=attr2.getLong();
										// debug("isBtControllService port:"+port+"\n");
										if(port==30) // endlich meinen port gefunden !!!
											return true;
										else
											return false;
									} else {
										debug("attr[0][1][1] != INT ("+attr2.getDataType()+")");
										return false;
									}
								}
								n++;
							}
							debug("isBtControllService fuk!\n");
							return false;
						} else {
							debug("attr[0][1] != DATASEQ\n");
							return false;
						}
					}
					n++;
				}
			} else {
				debug("attr[0] != DATASEQ\n");
				return false;
			}
		} catch (Exception e) {
			debug("exception: "+e.toString()+"\n");
		}
		debug("isBtControllService fuk2\n");
		return false;
	}
}
