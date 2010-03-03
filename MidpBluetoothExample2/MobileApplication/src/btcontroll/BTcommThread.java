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
 *
 * BTcommThread.java
 *
 * Created on 16. September 2007, 10:46
 *
 * macht die kommunikation mit dem BT server
 *	- AddCmdQueue -tut ein kommando in die queue
 * run wartet auf ein commando in der queue und sendet das dann, ruft callback auf
 */
package btcontroll;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
// import java.lang.Exception;
import java.util.Timer;
import java.util.TimerTask;
import javax.microedition.io.Connector;
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
			
	// me4se kann das nicht:
	// DataInputStream iStream;
	// DataOutputStream oStream;
	InputStream iStream;
	OutputStream oStream;
	StreamConnection BTStreamConnection;
	String connectionURL;
	
	// private boolean err=false;
	public boolean connError() {return (this.connState != STATE_CONNECTED && this.connState != STATE_TIMEOUT); } ;
	boolean doupdate=false;
	private Object connectedNotifyObject;

	// doClient neu starten wenn verbindung abgebrochen?
	boolean stop=false;

	public static final int STATE_DISCONNECTED = 0;
	public static final int STATE_OPENPORT = 1;
	public static final int STATE_CONNECTING = 2;
	public static final int STATE_CONNECTED = 3;
	public static final int STATE_TIMEOUT = 4; // connected aber gerade timeout
	public static final int STATE_OPENERROR = 5;

	public int connState;

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

	public final Object statusChange = new Object();

	// FIXME: nextMessage = 1 element queue
	NextMessage nextMessage=null;
	boolean my_queue_returnReply=false;
	private final Object queue_wait=new Object(); // thread macht nachn senden ein notify -> kamma neues reinschreiben
	
	// sachen fürn timeout:
	Timer timeoutTimer;
	TimeoutTask timeoutTask;
	public class TimeoutTask extends TimerTask {
		boolean called=false;
		public TimeoutTask() {
			
		}
		public void run() {
			Debuglog.debugln("TimeoutTask");
			called=true;
			connState=STATE_TIMEOUT;
			if(pingCallback != null) {
				try {
					pingCallback.BTCallback(null);
				} catch (Exception e) {
					Debuglog.debugln("BTCallback exception: "+e.toString());
				}
			}
			Debuglog.vibrate(30);
		}
	}
	

	// sachen fürs pingding:
	Timer timer;
	private final Object timerwait = new Object();
	TimerTask task;
	public class PingDing extends TimerTask {
		Object notifyObject;
		public PingDing(Object notifyObject) {
			this.notifyObject=notifyObject;
		}
		public void run() {
			System.out.println( "Running the ping task" );
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
	 *
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
	 */
	
	
	/**
	 * Creates a new instance of BTcommThread
	 * @param iStream
	 * @param oStream
	 */
	public BTcommThread(String connectionURL, Object connectedNotifyObject) {
		this.connectionURL = connectionURL;
		this.connectedNotifyObject=connectedNotifyObject;
	}

	public void connect() throws java.io.IOException {
		this.BTStreamConnection = (StreamConnection)Connector.open(this.connectionURL);
		this.iStream=BTStreamConnection.openInputStream();
		this.oStream=BTStreamConnection.openOutputStream();
		System.out.print("iStream: "+this.iStream+" oStream:"+this.oStream);
		System.out.print("str: i:"+this.iStream.toString()+" o:"+this.oStream.toString());
	}

	private void setStatus(int connState) {
		this.connState=connState;
		synchronized(this.statusChange) {
			this.statusChange.notifyAll();
		}
	}

	protected void close()
	{
		this.setStatus(STATE_DISCONNECTED);

		Debuglog.debugln("BTcommThread::close");
		try {
			this.iStream.close();
			this.oStream.close();
		} catch(Exception e) {
			Debuglog.debugln("BTcommThread::close exception ("+e.toString()+")");
		}
		try {
			BTStreamConnection.close();
		} catch(Exception e) {
			Debuglog.debugln("BTcommThread::close2 exception ("+e.toString()+")");
		}
		Debuglog.debugln("BTcommThread::close done");
		Debuglog.vibrate(500);
	}
	
	public void addCmdToQueue(FBTCtlMessage message) throws Exception {
		addCmdToQueue(message,null);
	}
	
	
	public void addCmdToQueue(FBTCtlMessage message, Callback callback) throws Exception {
		if(this.connError()) {
			throw new Exception("addCmdToQueue: no comm");
		}
		synchronized(queue_wait) {
			if(this.nextMessage != null) { // queue nicht leer
				//try {
					Debuglog.debugln("addCmdToQueue need wait");
					queue_wait.wait(5000); // ... dann auf das nächste notify warten - mit 10s timeout
					if(this.nextMessage != null) { // timeout 
						throw new Exception("addCmdToQueue: timeout");
					}
				/*
				} catch (Exception e) {
					debugForm.debug("addCmdToQueue exception ("+e.toString()+")");
				}
				 */
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
			Debuglog.debugln("execCmd already in use!!!!");
			throw new Exception("execCmd already in use");
		}
		this.callbackExecCmd.inUse=true;
		// debugForm.debug("execCmd "+message.toString()+"\n");
		if(this.callbackExecCmd.reply != null) { // ganz schlecht
			Debuglog.debugln("execCmd: reply scho gesetzt!");
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
					Debuglog.debugln("execCmd: wait exception");
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
			Debuglog.debugln("txMsg getBin Exception: "+e.getMessage());
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
		int rc=iStream.read(msgSizeBuffer,0,4);
		if( rc != 4) {
			throw new Exception("error reading msgSize ("+rc+")");
		}
		msgSizeIn.reset();
		int msgSize = msgSizeIn.getInt();
		byte[] buffer = new byte[msgSize];
		// debugForm.debug("rxMsg: size: "+msgSize);
		int readBytes=0;
		while(readBytes < msgSize) {
			rc=iStream.read(buffer,readBytes,msgSize-readBytes);
			if(rc <= 0) { // rc = -1 wenn EOF
				throw new Exception("error reading msg (rc="+rc+", size:"+msgSize+" read:"+readBytes);
			}
			if(rc != msgSize) {  // sonyericsson bleibt hängen wenn da kein debug ist (vermutlich täts ein sleep auch tun)
				Debuglog.debugln("xx reading msg (rc="+rc+", size:"+msgSize+" read:"+readBytes);
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
	public void run() {
		this.setStatus(STATE_DISCONNECTED);
		long defaultWait=10000;
		long wait=0; // 10s warten wenn connect error 0ms gebraucht hat, sofort connecten wenn connect länger 10s gebraucht hat
		while(!this.stop) {
			// warten damit der server nicht mit requests bombardiert wird bzw dem handy der speicher ausgeht
			try {
				Thread.sleep(100);
				this.setStatus(STATE_DISCONNECTED);
				Thread.sleep(wait);
				wait=defaultWait;
			} catch(Exception e) {
				Debuglog.debugln("sleep error:" + e.toString());
			}

			this.setStatus(STATE_OPENPORT);
			long l=java.lang.System.currentTimeMillis();
			try {
				this.connect();
			} catch (java.io.IOException e) {
				l=java.lang.System.currentTimeMillis()-l;
				wait=l<defaultWait?defaultWait-l:defaultWait;
				Debuglog.debugln("BT comm thread: error connecting("+e.toString()+"), error after"+l+"ms");
				this.setStatus(STATE_OPENERROR);
				Debuglog.debugln("try to reconnect (connect failed)");
				continue;
			}
			this.setStatus(STATE_CONNECTING);

			doClient();
			if(this.doupdate) {
				return;
			}
			l=java.lang.System.currentTimeMillis()-l;
			wait=l<defaultWait?defaultWait-l:defaultWait;
			Debuglog.debugln("try to reconnect (conn lost)");
		}
	}

	/**
	 *
	 */
	public void doClient(){
		// init pingding
		timer = new Timer();
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
			Debuglog.debugln("server:"+heloMsg.get("name").getStringVal()+
				" version:"+heloMsg.get("version").getStringVal()+
				" protoHash:"+heloMsg.get("protohash").getIntVal());
			int protocolHash=heloMsg.get("protohash").getIntVal();
			if(MessageLayouts.hash != protocolHash) {
				Object notifyObject=new Object();
				YesNoAlert yesNo=new YesNoAlert("old version","update java midlet? (altes bitte löschen)",notifyObject);
				btrailClient.display.setCurrent(yesNo);
				synchronized(notifyObject) {
					notifyObject.wait();
				}
				FBTCtlMessage heloReply = new FBTCtlMessage();
				heloReply.setType(MessageLayouts.messageTypeID("HELO_ERR"));
				heloReply.get("protohash").set(MessageLayouts.hash);
				if(yesNo.yes) {
					heloReply.get("doupdate").set(1);
					this.doupdate=true;
				} else
					heloReply.get("doupdate").set(0);
				this.sendMessage(heloReply);
				Debuglog.debugln("invalid protocol.dat hash (server:"+protocolHash+" me:"+MessageLayouts.hash+")");
				this.close();
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
			synchronized(connectedNotifyObject) {
				this.connectedNotifyObject.notify();
			}
			this.setStatus(STATE_CONNECTED);

			int commandNr=0;
			StringItem pingtext=new StringItem("pingtext","");
			StringItem sendtext=new StringItem("sendtext","");
			StringItem readtext=new StringItem("readtext","");
			StringItem pingstat=new StringItem("pingstat","");
			Debuglog.debug(pingtext);
			Debuglog.debug(sendtext);
			Debuglog.debug(readtext);
			Debuglog.debug(pingstat);
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
							Debuglog.debugln("wait exception ("+e.toString()+')');
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
				pingstat.setText("curr:"+t+" min: "+minTPing+" max:"+maxTPing+" avg:"+(avgTPing/commandNr));
				if(!timeoutTask.called) {
					this.setStatus(STATE_CONNECTED);
				}
				timeoutTimer.cancel();
				// debugForm.debug("done ("+commandNr+")\n");
			}
		} catch (java.io.IOException e) {
			Debuglog.debugln("BT comm thread: IO exception("+e.toString()+")");
		} catch (Exception e) {
			Debuglog.debugln("BT comm thread: exception("+e.toString()+")");
			e.printStackTrace();
		}
		/*
		if(my_queue_returnReply){
			synchronized(this.reply_sync) { // sicher is sicher ...
				this.reply_sync.notify();
			}
		}
		*/
		Debuglog.debugln("BT comm thread: ended\n");
		close();
		timer.cancel(); // ping timer stoppen sonst gibts eventuell irgendwann 2 davon ...
		synchronized(queue_wait) {
			queue_wait.notifyAll(); // damit das prog nicht in einem queue wait hängen bleibt
		}
		synchronized(connectedNotifyObject) {
			connectedNotifyObject.notify(); // zur sicherheit damit nix hängen bleibt
		}
	}
}
