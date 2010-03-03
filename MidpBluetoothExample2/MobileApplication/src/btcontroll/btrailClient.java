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
 * main file
 *
 * Created on 15. Juli 2007, 14:47
 */

package btcontroll;
// import java.util.Hashtable;
		
import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.bluetooth.*;
import javax.microedition.rms.RecordStore;


// import java.io.InputStream; // für load resource

// import protocol.FBTCtlMessage;
import protocol.MessageLayouts;
 
//import javax.microedition.io.*; // input stream, output stream

// import java.io.DataInputStream;
// import java.io.DataOutputStream;

// fürn timer:
// import java.util.*;

	
/**
 *
 * @author chris
 */
public class btrailClient extends MIDlet implements CommandListener, PrintClient.iAddAvailService
{
    
	static public Display display;
	
	private int orgDiscoverable;
	private float btApiVersion;

	private Image imgListWait, imgListScanning, imgListNoService, imgListService;
	
    /** Creates a new instance of HelloMidlet */
    public btrailClient() throws Exception
	{
		this.imgListWait=Image.createImage("/icons/listWait.png");
		this.imgListScanning=Image.createImage("/icons/listScanning.png");
		this.imgListNoService=Image.createImage("/icons/listNoService.png");
		this.imgListService=Image.createImage("/icons/listService.png");

		display=this.getDisplay();

		// ladet die protocol.dat
		MessageLayouts messageLayouts = new MessageLayouts();
		messageLayouts.load();
		/*
		try {

			FBTCtlMessage test = new FBTCtlMessage();
			/ *
			test.setType(MessageLayouts.messageTypeID("PING_REPLY"));
			test.get("info").get(0).get("addr").set(1);
			test.get("info").get(0).get("speed").set(192);
			test.get("info").get(0).get("functions").set(0xffffff);
			 * /
			test.setType(MessageLayouts.messageTypeID("HELO"));
			test.get("name").set("testclient");
			test.get("version").set("0.0.1");
			test.get("protoversion").set(1);
			test.dump();
			System.out.println("---------- -> bin -----------------------------");
			byte binMessage[]=test.getBinaryMessage();
			// System.out.println(new String(binMessage));
			FBTCtlMessage test2 = new FBTCtlMessage();
			System.out.println("---------- bin -> struct ----------------------");
			test2.readMessage(binMessage);
			test2.dump();
			System.out.print("server:" + test2.get("name").getStringVal());
			System.out.println("test done");
			System.exit(1);
		} catch(Exception e) {
			e.printStackTrace();
			System.out.println("exception: " + e.toString());
		}
		 */
		// */
				
		// test.get("info").get(0).get("addr").set(1);
		
    }
    
	private Form debugForm;
	private Command exitCommand;
	private Command helpCommand1;
	private Alert help;
	// private Canvas controllForm;
	private ValueList listServer;
	private Form newTCPConnForm;
	private TextField newTCPConnForm_host;

	private Command itemCommandDetail;
	private Command okCommand1;
	private Command screenCommand1 = new Command("ShowList", Command.SCREEN, 1);
	private Command back2ListCommand;
	// private Command itemCommandBlah1;
	private Command clearCommand=new Command("clear", Command.SCREEN, 1);
	private Command btScanCommand=new Command("erneute BT suche", Command.SCREEN, 2);
	private Command screenCommand_startControllCanvas=new Command("connect", Command.ITEM, 1);
	private Command newTCPConn=new Command("TCP connect", Command.ITEM, 5);

	private Command debugScreenCommand = new Command("debugscreen", Command.SCREEN,9);
	
	private Canvas controllCanvas;
	BTcommThread btcomm;
	// Hashtable availServices = new Hashtable(); // indexda = index im listServer
	private Command itemCommandShowServiceRecord=new Command("ServiceRecord", Command.ITEM, 1);

//GEN-LINE:MVDMethods
    
    private String debugDataElement(DataElement attr, int level)
    {
		String ret="";
        try {
            for(int i=0; i < level; i++)
                ret += "  ";
            switch(attr.getDataType()) {
                case DataElement.DATSEQ: {
                    java.util.Enumeration en = (java.util.Enumeration) attr.getValue();
                    ret += "(DATASEQ)\n";
                    int n=0;
                    while (en.hasMoreElements())
                    {
                        DataElement e = (DataElement) en.nextElement();
                        ret += debugDataElement(e,level+1);
                        
                    }
                    ret += "(/DATASEQ)\n";
                    break; }
                case DataElement.INT_1:
                case DataElement.U_INT_1:
                case DataElement.INT_4:
                case DataElement.U_INT_4: {
                    long l=attr.getLong();
                    ret += "(INT)"+l;
                    break; }
                case DataElement.UUID:
                    ret += "(UUID) ";
                default: {
                    ret += " '"+attr.getValue().toString()+"'";
                    break; }
                
            }
        } catch(ClassCastException e) {
            ret += " exception:"+e.toString();
        }
        ret += "\n";
        return ret;
    }
	
	public String debugServiceRecord(ServiceRecord sr) {
		String ret="";
		// helloForm.append(new javax.microedition.lcdui.Spacer(10,10));
            System.out.println("debugServiceRecord() done");
            if(sr == null) {
                // helloForm.setTitle("NO SR");
            } else {
                String connectionURL=sr.getConnectionURL(0,false);
                ret += "debugServiceRecord ("+connectionURL+")\n";
                try {
                    String tmpDevName = sr.getHostDevice().getFriendlyName(false);
                    if (tmpDevName != null && tmpDevName.length() > 0) {
                        ret += "name: "+tmpDevName+")";
                    }
                } catch (java.io.IOException e) { e.printStackTrace(); }
                int serviceNameOffset = 0x0000;
                int primaryLanguageBase = 0x0100;
                DataElement de = sr.getAttributeValue(primaryLanguageBase + serviceNameOffset);
                String srvName = null;
                if(de != null && de.getDataType() == DataElement.STRING) {
					srvName = (String)de.getValue();
					ret += "service: "+srvName+")\n";
                } else {
					ret += "no service name\n";
				}
                
                int ids[]=sr.getAttributeIDs();
                ret += "attrlist len:"+ids.length+"\n";
                for(int i=0; i < ids.length; i++) {
                    DataElement attr=sr.getAttributeValue(ids[i]);
                    ret += "[\""+ids[i]+"\"] type="+attr.getDataType();
                    ret += debugDataElement(attr,0);
                }
            }
		return ret;
	}
	
	public void updateStatus(String text) {
		this.get_listServer().setTitle(text);
	}
/*
    public void debug(String text) {
		StringItem item=new StringItem("",text);
		debug(item);
		/ *
		text.setFont(Font.getFont(Font.SIZE_SMALL));
        helloForm.append(text);
		try {
			// this.btcomm.addCmdToQueue("debug "+text+"\n");
		}catch(Exception e) {
		}
		 * /
		System.out.print(text);
    }
	public void debug(StringItem text) {
		text.setFont(Font.getFont(Font.FACE_PROPORTIONAL, Font.STYLE_PLAIN, Font.SIZE_SMALL));
        helloForm.append(text);
    }
 */
	
	
	/** This method initializes UI of the application.//GEN-BEGIN:MVDInitBegin
	 */
	private void initialize() {//GEN-END:MVDInitBegin
        // Insert pre-init code here
        System.out.println("System.out.println");
        
		getDisplay().setCurrent(this.get_listServer());//GEN-LINE:MVDInitInit

		
		Debuglog d=new Debuglog(this.getDebugForm(),this.getDisplay());
		
		
		String version = null;
		try {
			version = LocalDevice.getProperty("bluetooth.api.version");
		} catch(Exception e) {
			
		}
		if(version == null) {
			version="0";
		}
		try {
			String versionNeu="";
			for(int i=0; i < version.length(); i++) {
				char c=version.charAt(i);
				if((c >= '0' && c <= '9') || c =='.') {
					versionNeu+=c;
				}
			}
			version=versionNeu;
			this.btApiVersion = Float.parseFloat(version);
			debugForm.append("bt api version "+this.btApiVersion);
		} catch(Exception e) {
			debugForm.append("error parsing bt version("+version+") "+e.toString());
		}
		
		// me4se und api version 1.0 kann kein isPowerOn()
		if(this.btApiVersion >= 1.1) {
			try {
				if(!LocalDevice.isPowerOn()) {
					debugForm.setTitle("bt off");
					debugForm.append("please enable bluetooth!");
				}
			} catch(java.lang.Error e) {
				Debuglog.debugln("bt.isPowerOn exception");
				// e.printStackTrace();
			}
		}
		
		LocalDevice local;
		try {
			local = LocalDevice.getLocalDevice();
		} catch (BluetoothStateException e) {
			debugForm.setTitle("no bt?");
			debugForm.append("no bluetooth?");
			return;
		}
		

		
		// Insert post-init code here
		try {
			// wenn BT verbindong offen ist und die services von diesem handy abgefragt werden ist eventuell die verbindung weg - deswegen auf unsichtbar setzen
			this.orgDiscoverable=local.getDiscoverable();
			local.setDiscoverable(DiscoveryAgent.NOT_DISCOVERABLE);
		} catch (BluetoothStateException e) {
			Debuglog.debugln("bt.setDiscoverable(NOT_DISCOVERABLE) not supported");
		}
		SearchBTServerThread s=new SearchBTServerThread(this);
		s.start();
		// Debuglog.debugln("init done");
	}//GEN-LINE:MVDInitEnd

	class SearchBTServerThread extends Thread {
		btrailClient h;
		SearchBTServerThread(btrailClient h) {
			this.h=h;
		}
		public void run() {
			PrintClient client;
			try {
				try {
					debugForm.setTitle("bt scan...");
					client=new PrintClient(h);
				} catch (BluetoothStateException e) {
					debugForm.append("exception: ("+e.toString()+") bluetooth disabled?");
					e.printStackTrace();
					return;
				}
				client.findPrinter(false);
			} catch (Exception e) {
				Debuglog.debugln("SearchBTServerThread:" + e.toString());
			}
		}
	}
    
	class ConnectThread extends Thread {
		String connectionURL;
		ConnectThread(String connectionURL) {
			this.connectionURL=connectionURL;
		}
		public void run() {
			try {
				System.out.print("connecting:"+this.connectionURL);

				Object connectedNotifyObject = new Object();
				btcomm = new BTcommThread(this.connectionURL, connectedNotifyObject);
						//Thread t = new Thread(btcomm); t.start(); -> da is isAlive auf einmal nicht gesetzt
				getDisplay().setCurrent(get_controllCanvas(btcomm));
				btcomm.start();

				// update check:
				synchronized(connectedNotifyObject) {
					connectedNotifyObject.wait();
				}
				if(!btcomm.connError()) {
					// debugForm.setTitle("connected");
					
				} else if (btcomm.doupdate) {
					exitMIDlet();
				}
			} catch(Exception e) {
				Alert alert;
				alert = new Alert("commandActioon","Exception:"+e.toString(),null,null);
				alert.setTimeout(Alert.FOREVER);
				getDisplay().setCurrent(alert);
			}
		}
	}
	
	/** Called by the system to indicate that a command has been invoked on a particular displayable.//GEN-BEGIN:MVDCABegin
	 * @param command the Command that ws invoked
	 * @param displayable the Displayable on which the command was invoked
	 */
	public void commandAction(Command command, Displayable displayable) {//GEN-END:MVDCABegin
        // Insert global pre-action code here
		try {
// debugForm -----------------------------------------------------------------------------------------
		if (displayable == debugForm) {//GEN-BEGIN:MVDCABody
			if (command == helpCommand1) {//GEN-LINE:MVDCACase3
				// Insert pre-action code here
				getDisplay().setCurrent(get_help(), getDebugForm());//GEN-LINE:MVDCAAction8
				// Insert post-action code here
			} else if (command == back2ListCommand) {
				// Insert pre-action code here
				getDisplay().setCurrent(get_listServer());
				// Insert post-action code here

			} else if (command == screenCommand1) {
				// Insert pre-action code here
				getDisplay().setCurrent(get_listServer());
				// Insert post-action code here
			} else if (command == okCommand1) {
				// Insert pre-action code here
				// Do nothing//GEN-LINE:MVDCAAction16
				// Insert post-action code here
			} else if (command == clearCommand) {
				debugForm.deleteAll();
			}
// listServer -----------------------------------------------------------------------------------------
		} else if (displayable == listServer) {
			if (command == exitCommand) {
				exitMIDlet();
			} else if (command == btScanCommand) {  // neue BT suche
				debugForm.setTitle("bt scan...");
				Debuglog.debugln("bt scan started");

				SearchBTServerThread s=new SearchBTServerThread(this);
				s.start();
			} else if (command == debugScreenCommand) {
				getDisplay().setCurrent(getDebugForm());
			} else if(command == newTCPConn ) {
				getDisplay().setCurrent(getNewTCPConnForm());
			} else if(command == screenCommand_startControllCanvas) { // verbinden!
				if(btcomm != null && !btcomm.isAlive()) {
					debugForm.append("btcomm not alive -> deleting object");
					btcomm.close();
					btcomm=null;
				}
				if(btcomm == null) {
					debugForm.setTitle("connecting ...");

					String connectionURL=null;
					// me4se dürfte einen bug bei getSelectedIndex haben
					int index=this.listServer.getSelectedIndex();
					Object o = this.listServer.getValue(index);
					if(o != null) {
						if(o instanceof ServiceRecord) {
							ServiceRecord sr = (ServiceRecord) o;
							connectionURL=sr.getConnectionURL(0, false);
						} else if(o instanceof String) {
							connectionURL=(String) o;
						}
					}
					if(connectionURL == null) {
						Debuglog.debugln("no connection url");
					} else {
						ConnectThread connectThread = new ConnectThread(connectionURL);
						connectThread.start();
						// DataInputStream input = (InputConnection) connection.openDataInputStream();
						// DataOutputStream output = connection.openDataOutputStream();
					}

				} else {
					getDisplay().setCurrent(get_controllCanvas(btcomm));
				}
				// Insert pre-action code here
				/*
				getDisplay().setCurrent(get_controllForm());//GEN-LINE:MVDCAAction23
				*/
				// Insert post-action code here
			} else if (command == itemCommandShowServiceRecord) {
				// me4se dürfte einen bug bei getSelectedIndex haben
				int index=this.get_listServer().getSelectedIndex();
				Debuglog.debugln("sr["+index+"]=...");
				Object o = this.get_listServer().getValue(index);
				//debug("sr o\n");
				if(o != null) {
					Debuglog.debugln("sr o="+o.toString());
					// System.out.println("da ["+o.getClass().toString()+"] SR:["+)
					if(o instanceof ServiceRecord) {
						ServiceRecord sr = (ServiceRecord) o;
						String s=sr.getConnectionURL(0, false)+"\n";
						//debug("sr cast ok\n");
						//debug("sr ----------"+sr.toString()+"\n");
						s+=this.debugServiceRecord(sr);

						StringItem i = new StringItem("", s);
						i.setFont(Font.getFont(Font.FACE_PROPORTIONAL, Font.STYLE_PLAIN, Font.SIZE_SMALL));
						Form f = new Form("ServiceRecord", new Item[] { i });
						f.addCommand(getBack2ListCommand());
						f.setCommandListener(this);
						System.out.println("set display form");
						getDisplay().setCurrent(f);
					} else {
						Alert alert;
						alert = new Alert("no service",o.getClass().getName()+": "+o.toString(),null,null);
						alert.setTimeout(Alert.FOREVER);
						getDisplay().setCurrent(alert);
						Debuglog.debugln(o.getClass().getName()+": "+o.toString());
					}
				}
			}//GEN-BEGIN:MVDCACase23
		} else if (displayable == newTCPConnForm) {
			if(command == screenCommand_startControllCanvas) {
				// write post-init user code here
				try {
					RecordStore rs=javax.microedition.rms.RecordStore.openRecordStore("tcpservers", true);
					byte[] tmp=this.newTCPConnForm_host.getString().getBytes();
					rs.addRecord(tmp, 0, tmp.length);
				} catch (Exception e) {
					Debuglog.debugln("con action:"+e.toString());
				}
				if(btcomm != null && !btcomm.isAlive()) {
					debugForm.append("btcomm not alive -> deleting object");
					btcomm.close();
					btcomm=null;
				}
				if(btcomm == null) {
					debugForm.setTitle("connecting ...");

					String connectionURL;
					connectionURL = "socket://"+this.newTCPConnForm_host.getString()+":3030";
					ConnectThread connectThread = new ConnectThread(connectionURL);
					connectThread.start();

				} else {
					getDisplay().setCurrent(get_controllCanvas(btcomm));
				}

			} else if(command == back2ListCommand) {
				getDisplay().setCurrent(get_listServer());
			}
		}  else if(command == back2ListCommand) { // fürs ServiceRecord Form
			getDisplay().setCurrent(get_listServer());
		}
		
		// Insert global post-action code here
		} catch(Exception e) {
			Alert alert;
			alert = new Alert("commandActioon","Exception:"+e.toString(),null,null);
			alert.setTimeout(Alert.FOREVER);
			getDisplay().setCurrent(alert);
			e.printStackTrace();
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
        try {
			LocalDevice local=LocalDevice.getLocalDevice();
			local.setDiscoverable(this.orgDiscoverable);
		} catch(Exception e) {
		}
		getDisplay().setCurrent(null);
        destroyApp(true);
        notifyDestroyed();
    }//GEN-LAST:MVDExitMidlet
    
	/** This method returns instance for helloForm component and should be called instead of accessing helloForm field directly.                        
	 * @return Instance for helloForm component
	 */
	public Form getDebugForm() {
		if (debugForm == null) {
            // Insert pre-init code here
			debugForm = new Form(null, new Item[0]);
			debugForm.addCommand(getBack2ListCommand());
			debugForm.addCommand(screenCommand1);
			debugForm.addCommand(clearCommand);
			debugForm.addCommand(get_helpCommand1());
			debugForm.setCommandListener(this);
            // Insert post-init code here
			
		}
		return debugForm;
	}                    

	/**
	 * abfrage hostname für eine neue TCP connection
	 * @return
	 */
	public Form getNewTCPConnForm() {
		if (newTCPConnForm == null) {

			newTCPConnForm_host = new TextField("IP/hostname", null, 32, TextField.ANY);

			try {
				RecordStore rs=javax.microedition.rms.RecordStore.openRecordStore("tcpservers", true);
				javax.microedition.rms.RecordEnumeration re=rs.enumerateRecords(null, null, false);
				while (re.hasNextElement()) {
					String tmp = new String(re.nextRecord());
					newTCPConnForm_host.setString(tmp);
				}
			} catch (Exception e) {
				Debuglog.debugln("getNewTCPConnForm:"+e.toString());
			}
			newTCPConnForm = new Form("form", new Item[] { newTCPConnForm_host });
			newTCPConnForm.addCommand(getBack2ListCommand());
			newTCPConnForm.addCommand(screenCommand_startControllCanvas);
			newTCPConnForm.setCommandListener(this);

		}
		return newTCPConnForm;
	}


	public Canvas get_controllCanvas(BTcommThread btcomm) throws Exception {
		if(controllCanvas==null) {
			controllCanvas=new MIDPCanvas(btcomm);

		} else {
			((MIDPCanvas) controllCanvas).update(btcomm);
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

	/** This method returns instance for listServer component and should be called instead of accessing listServer field directly.                         
	 * @return Instance for listServer component
	 */
	public ValueList get_listServer() {
		if (listServer == null) {
			// Insert pre-init code here
			listServer = new ValueList("bt rail server list", Choice.IMPLICIT);
			listServer.addCommand(get_exitCommand());
			listServer.addCommand(screenCommand_startControllCanvas);
			listServer.addCommand(newTCPConn);
			listServer.setCommandListener(this);
			// macht exception am neuen sony .... listServer.setSelectedFlags(new boolean[0]);
			listServer.setSelectCommand(screenCommand_startControllCanvas);
			listServer.addCommand(itemCommandShowServiceRecord);
			listServer.addCommand(btScanCommand);
			listServer.addCommand(debugScreenCommand);
			// Insert post-init code here
		}
		return listServer;
	}                     

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

	/** This method returns instance for backCommand2 component and should be called instead of accessing backCommand2 field directly.//GEN-BEGIN:MVDGetBegin20
	 * @return Instance for backCommand2 component
	 */
	public Command getBack2ListCommand() {
		if (back2ListCommand == null) {//GEN-END:MVDGetBegin20
			// Insert pre-init code here
			back2ListCommand = new Command("Back", Command.BACK, 1);//GEN-LINE:MVDGetInit20
			// Insert post-init code here
		}//GEN-BEGIN:MVDGetEnd20
		return back2ListCommand;
	}//GEN-END:MVDGetEnd20

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

	public void addAvailService(ServiceRecord sr) {
/*
		this.debug("adding service...\n");
		
		String url=sr.getConnectionURL(0,false);
		int p=url.indexOf(";");
		if(p >= 0)
			url=url.substring(0,p);
		
		try {
*/		  
			// this.debug("service add: getlist");
		String hostname=null;
		try {
			hostname=sr.getHostDevice().getFriendlyName(false);
		} catch(Exception e) {}
		if(hostname == null) {
			hostname=sr.getHostDevice().getBluetoothAddress()+"???";
		}
		ValueList l=get_listServer();
		boolean set=false;
		for(int i=0; i < l.size(); i++) {
			Object o=l.getValue(i);
			if(o.getClass() == sr.getHostDevice().getClass()) {
				RemoteDevice lrd=(RemoteDevice) o;
				if(lrd.equals(sr.getHostDevice())) {
					l.set(i, l.getString(i), this.imgListService);
					l.setValue(i, sr);
					System.out.println("addAvailService - replaced value, set value["+i+"]="+sr.getClass().getName()+" "+sr.toString());
					set=true;
					break;
				}
			}
		}
		if(!set) {
			l.append(hostname, this.imgListService, sr);
			System.out.println("addAvailService - added server");
		}
/*			
		} catch(Exception e) {
			this.debug("service-add Exception: "+e.toString()+"\n");				
		}
 */
	}

	/**
	 * callback die aufgerufen wird wenn beim device suchvorgang ein device gefunden wird
	 * @param rd
	 */
	public void BTServerFound(RemoteDevice rd) {
		String s;
		try {
			s=rd.getFriendlyName(false);
		} catch(Exception e) {
			s=rd.getBluetoothAddress();
		}
		get_listServer().append(s,this.imgListWait,rd);
		Debuglog.debugln("found server "+ s);
	}

	/**
	 * callback die aufgerufen wird wenn searchService mit rd gestartet wird
	 * @param rd
	 */
	public void BTServerStartScanning(RemoteDevice rd) {
		String s;
		try {
			s=rd.getFriendlyName(false);
		} catch(Exception e) {
			s=rd.getBluetoothAddress();
		}
		ValueList l=get_listServer();
		for(int i=0; i < l.size(); i++) {
			Object o=l.getValue(i);
			if(o.getClass() == rd.getClass()) {
				RemoteDevice lrd=(RemoteDevice) o;
				if(lrd.equals(rd)) {
					l.set(i, l.getString(i), this.imgListScanning);
					break;
				}
			}
		}
		Debuglog.debugln("start scanning: "+rd.getBluetoothAddress());
	}

	/**
	 * callback die aufgerufen wird wenn searchservice(rd) fertig ist
	 * wenn rd noch in der liste ist wurde kein service gefunden -> bild auf x setzen, disable
	 * @param rd
	 */
	public void BTServerScanningDone(RemoteDevice rd) {
		String s;
		try {
			s=rd.getFriendlyName(false);
		} catch(Exception e) {
			s=rd.getBluetoothAddress();
		}
		ValueList l=get_listServer();
		for(int i=0; i < l.size(); i++) {
			Object o=l.getValue(i);
			if(o.getClass() == rd.getClass()) {
				RemoteDevice lrd=(RemoteDevice) o;
				if(lrd.equals(rd)) {
					l.set(i, l.getString(i), this.imgListNoService);
					l.setDisabled(i);
					break;
				}
			}
		}
		Debuglog.debugln(s + " scanned");
	}


}
