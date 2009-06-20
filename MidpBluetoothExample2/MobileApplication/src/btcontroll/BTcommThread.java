/*
 * BTcommThread.java
 *
 * Created on 16. September 2007, 10:46
 *
 * macht die kommunikation mit dem BT server
 *	- AddCmdQueue -tut ein kommando in die queue
 * run wartet auf ein commando in der queue und sendet das dann, ruft callback auf
 */
package btcontroll;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.lang.Exception;
import java.util.Timer;
import java.util.TimerTask;
import javax.microedition.io.StreamConnection;

import javax.microedition.lcdui.*;

import protocol.FBTCtlMessage;
import protocol.MessageLayouts;
import protocol.InputReader;
import protocol.OutputWriter;

/**
 *
 * @author chris
 */
public class BTcommThread extends Thread{
	public interface DisplayOutput {
		public void debug(String text);
		public void debug(StringItem text);
		public void vibrate(int ms);
	}
			
	DataInputStream iStream;
	DataOutputStream oStream;
	StreamConnection BTStreamConnection;
	
	private DisplayOutput debugForm;
	public interface Callback {
		public void BTCallback(FBTCtlMessage reply);
	}
	private class NextMessage {
		NextMessage(FBTCtlMessage message, Callback callback) {
			this.message=message;
			this.callback=callback;
		}
		FBTCtlMessage message;
		Callback callback;
	}
	// wenn daten empfangen werden wird notify() aufgerufen
	public Callback pingCallback=null;
	

	// FIXME: nextMessage = 1 element queue
	NextMessage nextMessage=null;
	boolean my_queue_returnReply=false;
	Object queue_wait=new Object(); // thread macht nachn senden ein notify -> kamma neues reinschreiben
	
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
			if(pingCallback != null) {
				try {
					pingCallback.BTCallback(null);
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
	
	public void addCmdToQueue(FBTCtlMessage message) throws Exception {
		addCmdToQueue(message,null);
	}
	
	
	public void addCmdToQueue(FBTCtlMessage message, Callback callback) throws Exception {
		synchronized(queue_wait) {
			if(this.nextMessage != null) { // queue nicht leer
				try {
					debugForm.debug("addCmdToQueue need wait");
					queue_wait.wait(); // ... dann auf das nächste notify warten
				} catch (Exception e) {
					debugForm.debug("addCmdToQueue exception ("+e.toString()+")");
				}
			}
			if(this.nextMessage != null) {
				throw new Exception("addCmdToQueue: queue not empty");
			}
			this.nextMessage=new NextMessage(message,callback);
		}
		
		synchronized(this.timerwait) {
			this.timerwait.notify();
		}
	}
	
	/**
	 * callback func wird ausgeführt wenn antwort vom exec-cmd da ist
	 */
	class CallbackExecCmd implements Callback {
		public void BTCallback(FBTCtlMessage reply) {
			this.reply=reply;
			synchronized(this.reply_sync) {
				// debugForm.debug("reply.notify\n");
				this.reply_sync.notify();
			}
		}
		public FBTCtlMessage reply;
		public Object reply_sync=new Object();
		public boolean inUse=false;

	}
	// kann global sein weil queue = 1 command => der muss sowieso warten bis reingeschrieben werden darf
	CallbackExecCmd callbackExecCmd=new CallbackExecCmd();
	
	/**
	 * sendet ein command und liefert die antwort zurück
	 * @return antwort
	 */
	public FBTCtlMessage execCmd(FBTCtlMessage message) throws Exception {
		if(this.callbackExecCmd.inUse) {
			debugForm.debug("execCmd already in use!!!!\n");
			throw new Exception("execCmd already in use");
		}
		this.callbackExecCmd.inUse=true;
		// debugForm.debug("execCmd "+message.toString()+"\n");
		if(this.callbackExecCmd.reply != null) { // ganz schlecht
			debugForm.debug("execCmd: reply scho gesetzt!\n");
			throw new Exception("reply != null");
		}
		addCmdToQueue(message,callbackExecCmd);

		FBTCtlMessage ret;
		// debugForm.debug("execCmd: wait 4 reply\n");
		// message.dump(debugForm);
		synchronized(this.callbackExecCmd.reply_sync) {
			// schaut leicht blöd aus, kann aber schon vom thread gesetzt worden sein! - und dann warten wir hier bis in alle ehwigkeiten ...
			// wichtig ist dass schreiben und da auslesen mit selben object gelockt ist
			if(this.callbackExecCmd.reply==null) {

				try {
					this.callbackExecCmd.reply_sync.wait();
				} catch (InterruptedException ex) {
					debugForm.debug("execCmd: wait exception\n");
					ex.printStackTrace();
				}
			}
			// debugForm.debug("execCmd: wait done\n");
			ret=this.callbackExecCmd.reply;
			this.callbackExecCmd.reply=null;
		}

		// debugForm.debug("execCmd: return:\n");
		// message.dump(debugForm);
		this.callbackExecCmd.inUse=false;
		return ret;
	
	}

	/**
	 * sendet eine Message incl message header (size)
	 * TODO: da sind 2 new OutputWriter - optimieren
	 */
	private void sendMessage(FBTCtlMessage msg) throws Exception
	{
		// debugForm.debug("txMsg "); msg.dump(debugForm);
		OutputWriter out;
		try {
			out=msg.getBinaryMessage();
		} catch(Exception e) {
			debugForm.debug("txMsg getBin Exception: "+e.getMessage());
			throw e;
		}
		OutputWriter outSize=new OutputWriter();
		outSize.putInt(out.getSize());
		oStream.write(outSize.getBytes(),0,outSize.getSize());
		// debugForm.debug("txMsg send: "+out.getSize()+"B ");
		oStream.write(out.getBytes(),0,out.getSize());
		oStream.flush();
		// debugForm.debug("txMsg done");
	}
	
	/**
	 * liest eine message incl header (size) ein
	 *  TODO: byte[const länge], nicht jedes mal neu allocen!!!
	 * msgSizeBuffer = global -> nicht threadsave, sollt aber eh nur von einem thread aufgerufen werden (midp handys können nur eine connection machen)
	 */
	byte msgSizeBuffer[] = new byte[4];
	InputReader msgSizeIn = new InputReader(msgSizeBuffer);
	private FBTCtlMessage receiveMessage() throws Exception
	{
		// debugForm.debug("rxMsg: ");
		if(iStream.read(msgSizeBuffer,0,4) != 4) {
			throw new Exception("error reading msgSize");
		}
		msgSizeIn.reset();
		int msgSize = msgSizeIn.getInt();
		byte[] buffer = new byte[msgSize];
		// debugForm.debug("rxMsg: size: "+msgSize);
		int readBytes=0;
		while(readBytes < msgSize) {
			int rc=iStream.read(buffer,readBytes,msgSize-readBytes);
			if(rc <= 0) { // rc = -1 wenn EOF
				throw new Exception("error reading msg (rc="+rc+", size:"+msgSize+" read:"+readBytes);
			}
			if(rc != msgSize) {  // sonyericsson bleibt hängen wenn da kein debug ist (vermutlich täts ein sleep auch tun)
				debugForm.debug("xx reading msg (rc="+rc+", size:"+msgSize+" read:"+readBytes);
			}
			readBytes += rc;
		}
		// debugForm.debug("parsing: ");
		FBTCtlMessage msg = new FBTCtlMessage();
		msg.readMessage(buffer);
		// debugForm.debug("parsing done ");
		// debugForm.debug("rx Msg"); msg.dump(debugForm);
		return msg;
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
		
		try {
			// geht nicht: oStream.writeChars("hallo vom handy...\n");
			// String s; // ="hallo vom handy...\n";
			FBTCtlMessage heloMsg = this.receiveMessage();
			if(!heloMsg.isType("HELO")) {
				throw new Exception("didn't receive HELO");
			}
			debugForm.debug("server:"+heloMsg.get("name").getStringVal()+
				" version:"+heloMsg.get("version").getStringVal()+
				" protoHash:"+heloMsg.get("protohash").getIntVal());
			int protocolHash=heloMsg.get("protohash").getIntVal();
			if(MessageLayouts.hash != protocolHash) {
				debugForm.debug("invalid protocol.dat hash (server:"+protocolHash+" me:"+MessageLayouts.hash+")");
				throw new Exception("invalid protocol.dat hash (server:"+protocolHash+" me:"+MessageLayouts.hash+")");
			}
			
// ping test:
			/*
			FBTCtlMessage pingMsg = new FBTCtlMessage();
			pingMsg.setType(MessageLayouts.messageTypeID("PING"));
			this.sendMessage(pingMsg);
			FBTCtlMessage pingReply = this.receiveMessage();
			if(!pingReply.isType("PING_REPLY")) {
				throw new Exception("didn't receive HELO");
			}
			debugForm.debug("pingReply rx ");
			int an=pingReply.get("info").getArraySize();
			debugForm.debug("size:"+an+" ");
			for(int i=0; i < an; i++) {
				debugForm.debug("addr:"+pingReply.get("info").get(i).get("addr").getIntVal()+
					" speed: "+pingReply.get("info").get(i).get("speed").getIntVal()+
					" func: "+pingReply.get("info").get(i).get("functions").getIntVal());
			}
			*/
			
			/*
			for (int n = 0; n < stream.length; ++n) {
				oStream.writeByte(stream[n]);
			}
			oStream.flush();
			debugForm.debug("writeChars done\n");
			 */
			// byte []buffer=new byte[50];
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
				if(this.nextMessage == null) { // kein befehl in der queue -> auf timerwait warten (= entweder timeout/nop oder ein neuer befehl
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
				NextMessage currMessage=null;
				synchronized(queue_wait) {
					
					if(this.nextMessage != null) {
						/*
						s=commandNr+" "+my_queue[0]+"\n";
						debugForm.debug("#"+commandNr+" = "+my_queue[0]+"\n");
						 */
						currMessage=this.nextMessage;
						this.nextMessage=null;
						queue_wait.notify();
					} else {
						FBTCtlMessage pingMsg = new FBTCtlMessage();
						pingMsg.setType(MessageLayouts.messageTypeID("PING"));
						currMessage=new NextMessage(pingMsg, pingCallback);
					}
				}
				timeoutTimer = new Timer();
				timeoutTask = new TimeoutTask();
				timeoutTimer.schedule(timeoutTask,1000); // timeout 1s
				this.sendMessage(currMessage.message);
				/*
				oStream.write(s.getBytes()); oStream.flush();
				helloForm.append("sent: "+s);
				// sendtext.setText("sent: "+s);
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
				*/
				FBTCtlMessage reply = this.receiveMessage();
				if(currMessage.callback != null) {
					// debugForm.debug("#"+commandNr+" done ret.len="+reply.toString()+"\n");
					currMessage.callback.BTCallback(reply);
				}

				/*
				if(returnReply){
					synchronized(this.reply_sync) {
						// debugForm.debug("reply.notify\n");
						this.reply_sync.notify();
					}
				}
				 */
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
		/*
		if(my_queue_returnReply){
			synchronized(this.reply_sync) { // sicher is sicher ...
				this.reply_sync.notify();
			}
		}
		*/
		debugForm.debug("BT comm thread: ended\n");
		close();
		timer.cancel(); // ping timer stoppen sonst gibts eventuell irgendwann 2 davon ...
	}
}
