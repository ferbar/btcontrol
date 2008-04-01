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
import javax.microedition.io.StreamConnection;

import javax.microedition.lcdui.*;

/**
 *
 * @author chris
 */
public class BTcommThread extends Thread{
	interface DisplayOutput {
		public void debug(String text);
		public void debug(StringItem text);
		public void vibrate(int ms);
	}
	DataInputStream iStream;
	DataOutputStream oStream;
	StreamConnection BTStreamConnection;
	
	private DisplayOutput debugForm;  
	public int speed=0; // schreibta da rein, ruft notiry auf
	interface Callback {
		public void BTCallback();
	}
	// wenn daten empfangen werden wird notify() aufgerufen
	public Callback notifyObject=null;
	
	
	String my_queue[];
	
	// sachen fürn timeout:
	boolean timeout=false;
	Timer timeoutTimer;
	TimeoutTask timeoutTask;
	public class TimeoutTask extends TimerTask {
		boolean called=false;
		public TimeoutTask() {
			
		}
		public void run() {
			System.out.println( "TimeoutTask" );
			called=true;
			timeout=true;
			if(notifyObject != null) {
				try {
					notifyObject.BTCallback();
				} catch (Exception e) {
					debugForm.debug("BTCallback exception: "+e.toString()+"\n");
				}
			}
			debugForm.vibrate(100);
		}
	}
	

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
	public BTcommThread(DisplayOutput debugForm, StreamConnection BTStreamConnection) throws java.io.IOException {
		this.debugForm=debugForm;
		this.BTStreamConnection=BTStreamConnection;
		this.iStream=BTStreamConnection.openDataInputStream();
		this.oStream=BTStreamConnection.openDataOutputStream();
	}
	
	protected void close()
	{
		debugForm.debug("BTcommThread::close");
		try {
			this.iStream.close();
			this.oStream.close();
		} catch(Exception e) {
			debugForm.debug("BTcommThread::close exception ("+e.toString()+")");
		}
		try {
			BTStreamConnection.close();
		} catch(Exception e) {
			debugForm.debug("BTcommThread::close2 exception ("+e.toString()+")");
		}
		debugForm.debug("BTcommThread::close done");
		debugForm.vibrate(1000);
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
		int pingTimeout=2000;
		timer.schedule(task, pingTimeout, pingTimeout);
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
			debugForm.debug("writeChars done\n");
			buffer=new byte[50];
			int commandNr=0;
			StringItem pingtext=new StringItem("pingtext","");
			StringItem sendtext=new StringItem("sendtext","");
			StringItem readtext=new StringItem("readtext","");
			StringItem pingstat=new StringItem("pingstat","");
			debugForm.debug(pingtext);
			debugForm.debug(sendtext);
			debugForm.debug(readtext);
			debugForm.debug(pingstat);
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
						debugForm.debug("wait exception ("+e.toString()+')');
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
				timeoutTimer = new Timer();
				timeoutTask = new TimeoutTask();
				timeoutTimer.schedule(timeoutTask,1000); // timeout 1s
				oStream.write(s.getBytes()); oStream.flush();
				// helloForm.append("sent: "+s);
				sendtext.setText("sent: "+s);
				boolean found=false;
				while(!found) { // so lange lesen bis zeile mit cmdnr anfängt
					int n =myReadLn(iStream,buffer);
					s=new String(buffer,0,n);
					// helloForm.append("read done ("+n+"): "+s+'\n');
					readtext.setText("read done ("+n+"): "+s+'\n');
					if(n<=1) {
						debugForm.debug("read 0 bytes?!?!? ("+n+")");
						return;
					}
					String sreply_commandNr="";
					String sreply="";
					n=0;
					char c;

					while(n < s.length() && Character.isDigit(c=s.charAt(n))) {
						sreply_commandNr+=c;
						n++;
					}
					// skip space
					while(n < s.length() && (c=s.charAt(n))==' ') {
						n++;
					}
					while(n < s.length()) {
						c=s.charAt(n);
						sreply+=c;
						n++;
					}
					try {
						this.speed=Integer.parseInt(sreply);
					} catch (Exception e) {
						debugForm.debug("parseint:"+ e.toString()+"\n");
					}
					
					if(notifyObject != null) {
						try {
							notifyObject.BTCallback();
						} catch (Exception e) {
							debugForm.debug("BTCallback exception: "+e.toString()+"\n");
						}
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
				if(!timeoutTask.called) {
					this.timeout=false;
				}
				timeoutTimer.cancel();
			}
		} catch (java.io.IOException e) {
			debugForm.debug("IO exception("+e.toString()+")\n");
		} catch (Exception e) {
			debugForm.debug("exception("+e.toString()+")\n");
		}
		debugForm.debug("BT comm thread ended\n");
		close();
		timer.cancel(); // ping timer stoppen sonst gibts eventuell irgendwann 2 davon ...
	}
}
