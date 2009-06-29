/*
 * Created on 28.10.2004
 *
 * if the program is called without any parameters, it does an inquiry. 
 * If it is calles with 1 parameter it does a service serach on the device specified (e.g. 000d9305170e). 
 * If it is called with 2 parameters, the second parameter is considered a UUID on which the service search is supposed to be restricted on.
 */
package de.avetana.bluetooth.test;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

import javax.bluetooth.*;

import de.avetana.bluetooth.sdp.RemoteServiceRecord;
import de.avetana.bluetooth.util.MSServiceRecord;

/**
 * @author gmelin
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public class InqTest implements DiscoveryListener {

	private static final boolean doException = false;
	
	private boolean searchCompleted = false;
	/* (non-Javadoc)
	 * @see javax.bluetooth.DiscoveryListener#deviceDiscovered(javax.bluetooth.RemoteDevice, javax.bluetooth.DeviceClass)
	 */
	public void deviceDiscovered(RemoteDevice btDevice, DeviceClass cod) {
		
		try {
			System.out.println ("device discovered " + btDevice + " name " + btDevice.getFriendlyName(false) + " majc " + cod.getMajorDeviceClass() + " minc " + cod.getMinorDeviceClass() + " sc " + cod.getServiceClasses());
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		if (doException) {
			byte[] b = new byte[0];
			b[1] = 0;
		}
		
	}

	/* (non-Javadoc)
	 * @see javax.bluetooth.DiscoveryListener#servicesDiscovered(int, javax.bluetooth.ServiceRecord[])
	 */
	public void servicesDiscovered(int transID, ServiceRecord[] servRecord) {
		// TODO Auto-generated method stub
		System.out.println (servRecord.length + " services discovered " + transID);
		
		if (doException) {
			byte[] b = new byte[0];
			b[1] = 0;
		}

		for (int i = 0;i < servRecord.length;i++) {
			try {
				System.out.println ("Record " + (i + 1));
				System.out.println ("" + servRecord[i]);
				} catch (Exception e) { e.printStackTrace(); }
				/*if (servRecord[0] instanceof RemoteServiceRecord) {
					RemoteServiceRecord rsr = (RemoteServiceRecord)servRecord[0];
					if (rsr.raw.length == 0) return;
					for (int j = 0; j < rsr.raw.length;j++) {
						System.out.print (" " + Integer.toHexString(rsr.raw[j] & 0xff));
					}
					System.out.println("\n----------------");
					
					ServiceRecord rsr2;
					try {
						rsr2 = RemoteServiceRecord.createServiceRecord("000000000000", new byte[0][0], new int[] { 0,1,2,3,4,5,6,7,8,9,10, 256 }, rsr.raw);
						byte[] raw2 = MSServiceRecord.getByteArray (rsr2);
						for (int j = 0; j < raw2.length;j++) {
							System.out.print (" " + Integer.toHexString(raw2[j] & 0xff));
						}
						System.out.println();

					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					
				}*/
 		}
		
	}

	/* (non-Javadoc)
	 * @see javax.bluetooth.DiscoveryListener#serviceSearchCompleted(int, int)
	 */
	public void serviceSearchCompleted(int transID, int respCode) {
		// TODO Auto-generated method stub
		System.out.println ("Service search completed " + transID + " / " + respCode);
		searchCompleted = true;
	}

	/* (non-Javadoc)
	 * @see javax.bluetooth.DiscoveryListener#inquiryCompleted(int)
	 */
	public void inquiryCompleted(int discType) {
		System.out.println ("Inquiry completed " + discType);
		searchCompleted = true;

	}

	public InqTest (String addr, String uuid) throws Exception {
		System.out.println ("Getting discoveryAgent");
		DiscoveryAgent da = LocalDevice.getLocalDevice().getDiscoveryAgent();
		
		if (addr == null) {
			da.startInquiry(DiscoveryAgent.GIAC, this);
		} else {
			UUID uuids[];
			System.out.println ("Setting up uuids");
			if (uuid == null) uuids = new UUID[] { };
			else if (uuid.length() == 32) uuids = new UUID[] { new UUID (uuid, false) };
			else uuids = new UUID[] { new UUID (uuid, true) };
			System.out.println ("Starting search");
			int transID = da.searchServices(new int[] { 0x05, 0x06, 0x07, 0x08, 0x09, 0x100, 0x303 }, uuids, new RemoteDevice (addr), this);
			System.out.println ("Started " + transID);
			//BufferedReader br = new BufferedReader (new InputStreamReader (System.in));
			//br.readLine();
			//if (!searchCompleted) da.cancelServiceSearch(transID);
			//System.out.println ("Canceled");

/*			Thread.currentThread().sleep(100);
			
			System.out.println ("Interrupting service search " + transID);
			da.cancelServiceSearch(transID);
			Thread.currentThread().sleep(1500);
			System.out.println ("Restarting service search");
			searchCompleted = false;
			transID = da.searchServices(new int[] { 0x05, 0x06, 0x07, 0x08, 0x09, 0x100, 0x303 }, uuids, new RemoteDevice ("000E0799107C"), this);
			*/
		}
		while (!searchCompleted) {
			synchronized (this) { wait(100); }
		}
		
		
		
		System.exit(0);
	}
	
	public static void main(String[] args) throws Exception {
		new InqTest(args.length == 0 ? null : args[0], args.length <= 1 ? null : args[1]);
	}
}
