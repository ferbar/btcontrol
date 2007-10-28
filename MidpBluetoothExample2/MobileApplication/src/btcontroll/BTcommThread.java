/*
 * BTcommThread.java
 *
 * Created on 16. September 2007, 10:46
 *
 * macht die kommunikation mit dem BT server
 *	- AddCmdQueue -tut ein kommando in die queue
 */
package btcontroll;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.util.Timer;
import java.util.TimerTask;

import javax.microedition.lcdui.*;

/**
 *
 * @author chris
 */
public class BTcommThread extends Thread{
	DataInputStream iStream;
	DataOutputStream oStream;
	
	private Form debugForm;  
	
	String my_queue[];

	// sachen fürs pingding:
	Timer timer;
	Object timerwait;
	TimerTask task;
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
	
	
	/**
	 * tut zeile lesen
	 * @param buffer bitte gross genug machen sonst gibts exception
	 * @return nchars
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
	
	
	/**
	 * Creates a new instance of BTcommThread
	 * @param iStream
	 * @param oStream
	 */
	public BTcommThread(Form debugForm, DataInputStream iStream, DataOutputStream oStream) {
		this.debugForm=debugForm;
		this.iStream=iStream;
		this.oStream=oStream;
	}
	
	public void addCmdToQueue(String cmd) {
		synchronized(my_queue) {
			my_queue[0]=cmd;
		}
		synchronized(this.timerwait) {
			this.timerwait.notify();
		}
	}
	
	/**
	 * der thread
	 */
	public void run(){
		// init pingding
		timer = new Timer();
		timerwait=new Object();
		task = new PingDing(timerwait);
		int timeout=5000;
		timer.schedule(task, timeout, timeout);
		my_queue=new String[2];
		
		try {
					// geht nicht: oStream.writeChars("hallo vom handy...\n");
					String s="hallo vom handy...\n";
					byte[] buffer = s.getBytes();
					oStream.write(buffer);
					/*
					for (int n = 0; n < stream.length; ++n) {
						oStream.writeByte(stream[n]);
					} */
					oStream.flush();
					debugForm.append("writeChars done\n");
					buffer=new byte[50];
					int commandNr=0;
					StringItem pingtext=new StringItem("pingtext","");
					StringItem sendtext=new StringItem("sendtext","");
					StringItem readtext=new StringItem("readtext","");
					StringItem pingstat=new StringItem("pingstat","");
					debugForm.append(pingtext);
					debugForm.append(sendtext);
					debugForm.append(readtext);
					debugForm.append(pingstat);
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
								debugForm.append("wait exception ("+e.toString()+')');
								return;
							}
						}
						long startTime = System.currentTimeMillis();
						synchronized(my_queue) {
							if(my_queue[0]!= "") {
								s=commandNr+" "+my_queue[0]+"\n";
								my_queue[0]="";
							} else {
								s=commandNr+" nop\n";
							}
						}
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
								debugForm.append("read 0 bytes?!?!?");
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
			debugForm.append("exception("+e.toString()+")\n");
		}
		debugForm.append("BT comm thread ended\n");
	}
}
