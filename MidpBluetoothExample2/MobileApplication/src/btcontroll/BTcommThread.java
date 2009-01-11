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
import java.lang.Exception;
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
	public int currAddr=0; // schreibta da rein, ruft notiry auf
	public int currSpeed=0;
	public int currFuncBits=0;
	interface Callback {
		public void BTCallback();
	}
	// wenn daten empfangen werden wird notify() aufgerufen
	public Callback notifyObject=null;
	

	// FIXME: riesen pfusch das !!!!!!!!!!
	String my_queue[];
	boolean my_queue_returnReply=false;
	Object queue_wait=new Object(); // thread macht nachn senden ein notify -> kamma neues reinschreiben
	String reply=null;
	Object reply_sync=new Object();
	
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
	
	public void addCmdToQueue(String cmd) throws Exception {
		addCmdToQueue(cmd,false);
	}
	
	
	public void addCmdToQueue(String cmd, boolean returnReply) throws Exception {
		synchronized(queue_wait) {
			if(this.my_queue[0] != null) { // queue nicht leer
				try {
					debugForm.debug("addCmdToQueue need wait");
					queue_wait.wait(); // ... dann auf das nächste notify warten
				} catch (Exception e) {
					debugForm.debug("addCmdToQueue exception ("+e.toString()+")");
				}
			}
			if(this.my_queue[0] != null) {
				throw new Exception("addCmdToQueue: queue not empty");
			}
			this.my_queue[0]=cmd;
			this.my_queue_returnReply=returnReply;
		}
		
		synchronized(this.timerwait) {
			this.timerwait.notify();
		}
	}
	
	/**
	 * sendet ein command und liefert die antwort zurück
	 */
	public String execCmd(String cmd) throws Exception {
		debugForm.debug("execCmd "+cmd+"\n");
		addCmdToQueue(cmd,true);
		if(this.reply != null) { // ganz schlecht
			debugForm.debug("execCmd: reply scho gesetzt!\n");
			throw new Exception("reply != null");
		}
		this.reply=new String();
		String ret="undef";

		debugForm.debug("execCmd: wait 4 reply\n");
		synchronized(this.reply_sync) {
	
			try {
				this.reply_sync.wait();
			} catch (InterruptedException ex) {
				debugForm.debug("execCmd: wait exception\n");
				ex.printStackTrace();
			}
	 
			debugForm.debug("execCmd: wait done\n");
			ret = this.reply;
			this.reply=null;
		}

		debugForm.debug("execCmd: return: \""+ret+"\"\n");
		return ret;
	
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
		my_queue[0]=null;	my_queue[1]=null;
		
		
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
				if(my_queue[0] == null) { // kein befehl in der queue -> auf timerwait warten (= entweder timeout/nop oder ein neuer befehl
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
				}
				long startTime = System.currentTimeMillis();
				boolean returnReply=false;
				synchronized(queue_wait) {
					
					if(my_queue[0] != null) {
						s=commandNr+" "+my_queue[0]+"\n";
						debugForm.debug("#"+commandNr+" = "+my_queue[0]+"\n");
						returnReply=my_queue_returnReply;
						my_queue_returnReply=false;
						my_queue[0]=null;
						queue_wait.notify();
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
					// debugForm.debug("readln for cmd "+commandNr+"\n");
					int n =myReadLn(iStream,buffer);
					s=new String(buffer,0,n);
					// helloForm.append("read done ("+n+"): "+s+'\n');
					if(s==null) {
						debugForm.debug("BT comm thread: s==null\n");
						return;
					}
					// debugForm.debug("read done ("+n+"): \""+s+"\"\n");
					readtext.setText("read done ("+n+"): \""+s+"\"\n");
					if(n<=1) {
						debugForm.debug("BT comm thread: read 0 bytes?!?!? ("+n+")");
						return;
					}
					String sreply_commandNr="";
					String sreply="";
					n=0;
					char c;

					// nr einlesen ...
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
					// debugForm.debug("cmd parse done\n");
					
					// --> mehrzeilige antwort mit spaces am anfang:
					if(sreply_commandNr.length() == 0) {
						debugForm.debug("#"+commandNr+" multiline reply: \""+s+"\"\n");
						if(returnReply) {
							// debugForm.debug("returnReply\n");
							this.reply += s.substring(1) + '\n'; // readline filterts ja raus ....
						}
						continue;
					}
					// sreply_commandNr angegeben -> cmd ende und speed steht auch da
					int reply_commandNr=Integer.parseInt(sreply_commandNr);
					if(reply_commandNr==commandNr) {
						found=true;
					} else {
						debugForm.debug("#"+commandNr+" invalidID:\""+reply_commandNr+"\"");
					}

					// debugForm.debug("read speed\n");
					// addr,speed,funcbits auslesen - gibts nur wenn commandNr angegeben wurde
					String tmp="-1";
					int p=0;
					try {
						int i=p+1;
						while((i < sreply.length()) && Character.isDigit(sreply.charAt(i))) { i++; }
						tmp=sreply.substring(p, i);
						p=i+1;
						this.currAddr=Integer.parseInt(tmp);
						
						i=p+1;
						while((i < sreply.length()) && Character.isDigit(sreply.charAt(i))) { i++; }
						tmp=sreply.substring(p, i);
						p=i+1;
						this.currSpeed=Integer.parseInt(tmp);
						
						i=p+1;
						while(i < sreply.length()) {
							int d=Character.digit(sreply.charAt(i),16);
							if(d>=0 && d < 16) {
							} else {
								break;
							}
							i++;
						}
						tmp=sreply.substring(p, i);
						p=i+1;
						this.currFuncBits=Integer.parseInt(tmp,16);
						
					} catch (Exception e) {
						debugForm.debug("parseret "+ e.toString()+" p:"+p+" tmp:\""+tmp+"\"\n");
					}
					
					// debugForm.debug("notify\n");
					if(this.notifyObject != null) {
						try {
							notifyObject.BTCallback();
						} catch (Exception e) {
							debugForm.debug("#"+commandNr+" BTCallback exception: "+e.toString()+"\n");
						}
					}
					// debugForm.debug("notify done\n");
					// helloForm.append("rx: "+sreply_commandNr+'\n');
					pingtext.setText("rx: "+sreply_commandNr+'\n');
				}
				if(returnReply){
					debugForm.debug("#"+commandNr+" done ret.len="+this.reply.length()+"\n");
					synchronized(this.reply_sync) {
						// debugForm.debug("reply.notify\n");
						this.reply_sync.notify();
					}
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
				// debugForm.debug("done ("+commandNr+")\n");
			}
		} catch (java.io.IOException e) {
			debugForm.debug("BT comm thread: IO exception("+e.toString()+")\n");
		} catch (Exception e) {
			debugForm.debug("BT comm thread: exception("+e.toString()+")\n");
		}
		if(my_queue_returnReply){
			synchronized(this.reply_sync) { // sicher is sicher ...
				this.reply_sync.notify();
			}
		}
		debugForm.debug("BT comm thread: ended\n");
		close();
		timer.cancel(); // ping timer stoppen sonst gibts eventuell irgendwann 2 davon ...
	}
}
