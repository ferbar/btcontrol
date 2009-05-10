package btcontroll;
import java.lang.*;
// import java.io.*;
import java.util.*;
// import javax.microedition.io.*;
import javax.bluetooth.*;

// fürs debug
// import javax.microedition.lcdui.*;


/**
 * This class shows a simple client application that performs device
 * and service
 * discovery and communicates with a print server to show how the Java
 * API for Bluetooth wireless technology works.
 */
public class PrintClient implements DiscoveryListener {


	public interface iAddAvailService {
		public void AddAvailService(ServiceRecord sr);
		public int CountAvailServices();
	}
	
			
    /**
     * The DiscoveryAgent for the local Bluetooth device.
     */
    private DiscoveryAgent agent;

    /**
     * The max number of service searches that can occur at any one time.
     */
    private int maxServiceSearches = 0;

    /**
     * The number of service searches that are presently in progress.
     */
    private int serviceSearchCount;

    /**
     * Keeps track of the transaction IDs returned from searchServices.
     */
    private int transactionID[];

    /**
     * Keeps track of the devices found during an inquiry.
     */
    private Vector deviceList;
    
    // private Displayable mydebug;
    private iMyMessages mydebug;
    private iAddAvailService addAvailService;


    /**
     * Creates a PrintClient object and prepares the object for device
     * discovery and service searching.
     *
     * @exception BluetoothStateException if the Bluetooth system could not be
     * initialized
     */
    public PrintClient(iMyMessages d, iAddAvailService s) throws BluetoothStateException {
        mydebug=d;
		addAvailService=s;

        /*
         * Retrieve the local Bluetooth device object.
         */
        LocalDevice local = LocalDevice.getLocalDevice();

        /*
         * Retrieve the DiscoveryAgent object that allows us to perform device
         * and service discovery.
         */
        agent = local.getDiscoveryAgent();

        /*
         * Retrieve the max number of concurrent service searches that can
         * exist at any one time.
         */
        try {
            maxServiceSearches = Integer.parseInt(
                LocalDevice.getProperty("bluetooth.sd.trans.max"));
        } catch (NumberFormatException e) {
            System.out.println("General Application Error");
            System.out.println("\tNumberFormatException: " + e.getMessage());
        }

        transactionID = new int[maxServiceSearches];

        // Initialize the transaction list
        for (int i = 0; i < maxServiceSearches; i++) {
            transactionID[i] = -1;
        }

        deviceList = new Vector();
        System.out.println("PrintClient() done");
    }

    /**
     * Adds the transaction table with the transaction ID provided.
     *
     * @param trans the transaction ID to add to the table
     */
    private void addToTransactionTable(int trans) {
        for (int i = 0; i < transactionID.length; i++) {
            if (transactionID[i] == -1) {
                transactionID[i] = trans;
                return;
            }
        }
    }

    /**
     * Removes the transaction from the transaction ID table.
     *
     * @param trans the transaction ID to delete from the table
     */
    private void removeFromTransactionTable(int trans) {
        for (int i = 0; i < transactionID.length; i++) {
            if (transactionID[i] == trans) {
                transactionID[i] = -1;
                return;
            }
        }
    }

    /**
     * Completes a service search on each remote device in the list until all
     * devices are searched or until a printer is found that this application
     * can print to.
     *
     * @param devList the list of remote Bluetooth devices to search
     *
     * @return true if a printer service is found; otherwise false if
     * no printer service was found on the devList provided
     */
    private boolean searchServices(RemoteDevice[] devList) {
        UUID[] searchList = new UUID[2];
System.out.println("searchServices() Length = " + devList.length);

        /*
         * Add the UUID for L2CAP to make sure that the service record
         * found will support L2CAP.  This value is defined in the
         * Bluetooth Assigned Numbers document.
         */
        // [chris] das is der ID für L2CAP 
        searchList[0] = new UUID(0x0100);

        /*
         * Add the UUID for the printer service that we are going to use to
         * the list of UUIDs to search for. (a fictional printer service UUID)
         */
        // searchList[1] = new UUID("11111111111111111111111111111111", false);

        // serial connection: (btspp services only)
        searchList[1] = new UUID(0x1101);
		
		// [chris] nur welche mit port Nr 27
		// searchList[1] = new UUID(27);
		
		/*
         * Start a search on as many devices as the system can support.
         */
        for (int i = 0; i < devList.length; i++) {

            try {
System.out.println("Starting Service Search on " + devList[i].getBluetoothAddress());
                int trans = agent.searchServices(null, searchList, devList[i], this);
System.out.println("Starting Service Search " + trans);
                addToTransactionTable(trans);
            } catch (BluetoothStateException e) {
System.out.println("BluetoothStateException: " + e.getMessage());
                /*
                 * Failed to start the search on this device, try another
                 * device.
                 */
            }

            /*
             * Determine if another search can be started.  If not, wait for
             * a service search to end.
             */
            synchronized (this) {
                serviceSearchCount++;
System.out.println("maxServiceSearches = " + maxServiceSearches);
System.out.println("serviceSearchCount = " + serviceSearchCount);
                if (serviceSearchCount == maxServiceSearches) {
System.out.println("Waiting");
                    try {
                        this.wait();
                    } catch (Exception e) {
                    }
                }
System.out.println("Done Waiting " + serviceSearchCount);
            }
        }


        /*
         * Wait until all the service searches have completed.
         */
        while (serviceSearchCount > 0) {
            synchronized (this) {
                try {
                    this.wait();
                } catch (Exception e) {
                }
            }
        }
		return addAvailService.CountAvailServices() > 0;
    }

    /**
     * Finds the first printer that is available to print to.
     * @param fullSearch true => cache ignorieren
	 *
     * @return the service record of the printer that was found; null if no
     * printer service was found
     */
    public boolean findPrinter(boolean fullSearch) {
		RemoteDevice[] devList;
		if(!fullSearch) {
			/*
			 * If there are any devices that have been found by a recent inquiry,
			 * we don't need to spend the time to complete an inquiry.
			 */
			devList = agent.retrieveDevices(DiscoveryAgent.CACHED);
			if (devList != null) {
				if (searchServices(devList)) {
					System.out.println("findPrinter() return cached");
					mydebug.updateStatus(devList.length+" devices cached");
					return true;
				}
			}

			/*
			 * Did not find any printer services from the list of cached devices.
			 * Will try to find a printer service in the list of pre-known
			 * devices.
			 */
			devList = agent.retrieveDevices(DiscoveryAgent.PREKNOWN);
			if (devList != null) {
				if (searchServices(devList)) {
					mydebug.updateStatus(devList.length+" devices preknown");
					System.out.println("findPrinter() preknown");
					return true;
				}
			}
		}

        /*
         * Did not find a printer service in the list of pre-known or cached
         * devices.  So start an inquiry to find all devices that could be a
         * printer and do a search on those devices.
         */
        /* Start an inquiry to find a printer   */
        try {
            System.out.println("findPrinter() startInquiry");

            agent.startInquiry(DiscoveryAgent.GIAC, this);

            /*
             * Wait until all the devices are found before trying to start the
             * service search.
             */
            synchronized (this) {
                try {
                    this.wait();
                } catch (Exception e) {
                }
            }

        } catch (BluetoothStateException e) {

            System.out.println("Unable to find devices to search");
			mydebug.debug("Unable to find devices to search");
			mydebug.updateStatus("Unable to search (ex)");
        }

        System.out.println("findPrinter() size="+deviceList.size());
		mydebug.updateStatus(deviceList.size()+" devices found");
       
       
        if(deviceList.size() > 0) {
			mydebug.debug("found "+deviceList.size()+" Devices");
            devList = new RemoteDevice[deviceList.size()];
            deviceList.copyInto(devList);
            if (searchServices(devList)) {
                mydebug.debug("findPrinter found service");
                return true;
            }
			mydebug.updateStatus("no service found");
        } else {
       		mydebug.updateStatus("no devices found");
        }

        return false;
    }

    /**
     * This is the main method of this application.  It will print out
     * the message provided to the first printer that it finds.
     *
     * @param args[0] the message to send to the printer
     * /
    public static void main(String[] args) {
        PrintClient client = null;

        // Validate the proper number of arguments exist when starting this application.
        if ((args == null) || (args.length != 1)) {
            System.out.println("usage: java PrintClient message");
            return;
        }

        // Create a new PrintClient object.
        Screen s=new Form(null);
        try {
            client = new PrintClient(s);
        } catch (BluetoothStateException e) {
            System.out.println("Failed to start Bluetooth System");
            System.out.println("\tBluetoothStateException: " +
                                                     e.getMessage());
        }

        // Find a printer in the local area
        ServiceRecord printerService = client.findPrinter();

        if (printerService != null) {

            // Determine if this service will communicate over RFCOMM or L2CAP by retrieving the connection string.
            String conURL = printerService.getConnectionURL(
                ServiceRecord.NOAUTHENTICATE_NOENCRYPT, false);

            int index= conURL.indexOf(':');
            String protocol= conURL.substring(0, index);
            if (protocol.equals("btspp")) {

                // Since this printer service uses RFCOMM, create an RFCOMM connection and send the data over RFCOMM.

                RFCOMMPrinterClient printer = new RFCOMMPrinterClient(conURL);
                printer.printJob(args[0]);

            } else if (protocol.equals("btl2cap")) {

                    // Since this service uses L2CAP, create an L2CAP connection to the service and send the data to the service over L2CAP.

                    L2CAPPrinterClient printer = new L2CAPPrinterClient(conURL);
                    printer.printJob(args[0]);

            } else {
                System.out.println("Unsupported Protocol");

            }

        } else {
            System.out.println("No Printer was found");
        }
    }
	 */

    /**
     * Called when a device was found during an inquiry.  An inquiry
     * searches for devices that are discoverable.  The same device may
     * be returned multiple times.
     *
     * @see DiscoveryAgent#startInquiry
     *
     * @param btDevice the device that was found during the inquiry
     *
     * @param cod the service classes, major device class, and minor
     * device class of the remote device being returned
     *
     */
    public void deviceDiscovered(RemoteDevice btDevice, DeviceClass cod) {
System.out.println("Found device = " + btDevice.getBluetoothAddress());
// [chris] alle devices in die liste
        /*
         * Since service search takes time and we are already forced to
         * complete an inquiry, we will not do a service
         * search on any device that is not an Imaging device.
         * The device class of 0x600 is Imaging as
         * defined in the Bluetooth Assigned Numbers document.
         */
//        if (cod.getMajorDeviceClass() == 0x600) {
            /*
             * Imaging devices could be a display, camera, scanner, or
             * printer. If the imaging device is a printer,
             * then bit 7 should be set from its minor device
             * class according to the Bluetooth Assigned
             * Numbers document.
             */
//            if ((cod.getMinorDeviceClass() & 0x80) != 0) {
                /*
                 * Now we know that it is a printer.  Now we will verify that
                 * it has a rendering service on it.  A rendering service may
                 * allow us to print.  We will have to do a service search to
                 * get more information if a rendering service exists. If this
                 * device has a rendering service then bit 18 will be set in
                 * the major service classes.
                 */
//                if ((cod.getServiceClasses() & 0x40000) != 0) {
                    deviceList.addElement(btDevice);
//                }
//            }
//        }
    }

    /**
     * The following method is called when a service search is completed or
     * was terminated because of an error.  Legal status values
     * include:
     * <code>SERVICE_SEARCH_COMPLETED</code>,
     * <code>SERVICE_SEARCH_TERMINATED</code>,
     * <code>SERVICE_SEARCH_ERROR</code>,
     * <code>SERVICE_SEARCH_DEVICE_NOT_REACHABLE</code>, and
     * <code>SERVICE_SEARCH_NO_RECORDS</code>.
     *
     * @param transID the transaction ID identifying the request which
     * initiated the service search
     *
     * @param respCode the response code which indicates the
     * status of the transaction; guaranteed to be one of the
     * aforementioned only
     *
     */
    public void serviceSearchCompleted(int transID, int respCode) {
System.out.println("serviceSearchCompleted(" + transID + ", " + respCode + ")");

       /*
        * Removes the transaction ID from the transaction table.
        */
        removeFromTransactionTable(transID);

        serviceSearchCount--;

        synchronized (this) {
            this.notifyAll();
        }
    }

    /**
     * Called when service(s) are found during a service search.
     * This method provides the array of services that have been found.
     *
     * @param transID the transaction ID of the service search that is
     * posting the result
     *
     * @param service a list of services found during the search request
     *
     * @see DiscoveryAgent#searchServices
     */
    public void servicesDiscovered(int transID, ServiceRecord[] servRecord) {

        /*
         * If this is the first record found, then store this record
         * and cancel the remaining searches.
         */
System.out.println("Found a service " + transID);
System.out.println("Length of array = " + servRecord.length);
if (servRecord.length == 0) { // zerst war da servRecord[0] == 0
    System.out.println("kein service record (?)");
	return;
}
System.out.println("After this");
mydebug.debug("servicesDiscovered n:"+servRecord.length+"\n");
	for(int i=0; i < servRecord.length; i++) {
		int primaryLanguageBase = 0x0100;
		ServiceRecord sr = servRecord[i];
		DataElement de = sr.getAttributeValue(primaryLanguageBase);
		if(de != null && de.getDataType() == DataElement.STRING) {
			String srvName = (String)de.getValue();
			mydebug.debug("servicesDiscovered["+i+"] "+srvName+"\n");
		} else {
			mydebug.debug("servicesDiscovered["+i+"] "+sr.getConnectionURL(0,false)+"\n");
		}
		if(isBtControllService(sr)) {
			addAvailService.AddAvailService(sr);
		}
	}


            /*
             * Cancel all the service searches that are presently
             * being performed.
			*/
            for (int i = 0; i < transactionID.length; i++) {
                if (transactionID[i] != -1) {
// [chris] dont stop search .... System.out.println(agent.cancelServiceSearch(transactionID[i]));
                }
            }
    }

    /**
     * Called when a device discovery transaction is
     * completed. The <code>discType</code> will be
     * <code>INQUIRY_COMPLETED</code> if the device discovery
     * transactions ended normally,
     * <code>INQUIRY_ERROR</code> if the device
     * discovery transaction failed to complete normally,
     * <code>INQUIRY_TERMINATED</code> if the device
     * discovery transaction was canceled by calling
     * <code>DiscoveryAgent.cancelInquiry()</code>.
     *
     * @param discType the type of request that was completed; one of
     * <code>INQUIRY_COMPLETED</code>, <code>INQUIRY_ERROR</code>
     * or <code>INQUIRY_TERMINATED</code>
     */
    public void inquiryCompleted(int discType) {
        synchronized (this) {
            try {
                this.notifyAll();
            } catch (Exception e) {
            }
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
			mydebug.debug("isBtControllService?");
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
										mydebug.debug("attr[0][1][1] != INT ("+attr2.getDataType()+")");
										return false;
									}
								}
								n++;
							}
							mydebug.debug("isBtControllService fuk!\n");
							return false;
						} else {
							mydebug.debug("attr[0][1] != DATASEQ\n");
							return false;
						}
					}
					n++;
				}
			} else {
				mydebug.debug("attr[0] != DATASEQ\n");
				return false;
			}
		} catch (Exception e) {
			mydebug.debug("exception: "+e.toString()+"\n");
		}
		mydebug.debug("isBtControllService fuk2\n");
		return false;
	}

}



/**
 * The RFCOMMPrinterClient will make a connection using the connection string
 * provided and send a message to the server to print the data sent.
 *
class RFCOMMPrinterClient {

    **
     * Keeps the connection string in case the application would like to make
     * multiple connections to a printer.
     *
    private String serverConnectionString;

    **
     * Creates an RFCOMMPrinterClient that will send print jobs to a printer.
     *
     * @param server the connection string used to connect to the server
     *
    RFCOMMPrinterClient(String server) {
        serverConnectionString = server;
    }

    **
     * Sends the data to the printer to print.  This method will establish a
     * connection to the server and send the String in bytes to the printer.
     * This method will send the data in the default encoding scheme used by
     * the local virtual machine.
     *
     * @param data the data to send to the printer
     *
     * @return true if the data was printed; false if the data failed to be
     * printed
     *
    public boolean printJob(String data) {
        OutputStream os = null;
        StreamConnection con = null;

        try {
            // Open the connection to the server
           con =(StreamConnection)Connector.open(serverConnectionString);

           // Sends data to remote device
           os = con.openOutputStream();
           os.write(data.getBytes());

           // Close all resources
           os.close();
           con.close();
        } catch (IOException e2) {
            System.out.println("Failed to print data");
            System.out.println("IOException: " + e2.getMessage());
            return false;
        }

        return true;
   }
}
	 */

/**
 * The L2CAPPrinterClient will make a connection using the connection string
 * provided and send a message to the server to print the data sent.
 *
class L2CAPPrinterClient {

    **
     * Keeps the connection string in case the application would like to make
     * multiple connections to a printer.
     *
    private String serverConnectionString;

    **
     * Creates an L2CAPPrinterClient object that will allow an application to
     * send multiple print jobs to a Bluetooth printer.
     *
     * @param server the connection string used to connect to the server
     *
    L2CAPPrinterClient(String server) {
        serverConnectionString = server;
    }

    **
     * Sends a print job to the server.  The print job will print the message
     * provided.
     *
     * @param msg a non-null message to print
     *
     * @return true if the message was printed; false if the message was not
     * printed
     *
    public boolean printJob(String msg) {
        L2CAPConnection con = null;
        byte[] data = null;
        int index = 0;
        byte[] temp = null;

        try {
            // Create a connection to the server
            con = (L2CAPConnection)Connector.open(serverConnectionString);

            // Determine the maximum amount of data I can send to the server.
            int MaxOutBufSize = con.getTransmitMTU();
            temp = new byte[MaxOutBufSize];

            // Send as many packets as are needed to send the data
            data = msg.getBytes();

            while (index < data.length) {

                *
                 * Determine if this is the last packet to send or if there
                 * will be additional packets
                 *
                if ((data.length - index) < MaxOutBufSize) {
                    temp = new byte[data.length - index];
                    System.arraycopy(data, index, temp, 0, data.length - index);
                } else {
                    temp = new byte[MaxOutBufSize];
                    System.arraycopy(data, index, temp, 0, MaxOutBufSize);
                }

                con.send(temp);

                index += MaxOutBufSize;
            }

            // Close the connection to the server
            con.close();
        } catch (BluetoothConnectionException e) {
            System.out.println("Failed to print message");
            System.out.println("\tBluetoothConnectionException: " + e.getMessage());
            System.out.println("\tStatus: " + e.getStatus());
        } catch (IOException e) {
            System.out.println("Failed to print message");
            System.out.println("\tIOException: " + e.getMessage());
            return false;
        }

        return true;
  } // End of method printJob.

} // End of class L2CAPPrinterClient

*/
