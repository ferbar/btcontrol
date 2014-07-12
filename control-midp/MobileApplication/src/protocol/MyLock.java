package protocol;

public class MyLock{
	  
	  private boolean isLocked = false;
	  
	  public synchronized void lock()
	  throws InterruptedException{
	    while(isLocked){
	      super.wait();
	    }
	    isLocked = true;
	  }
	  
	  public synchronized void unlock(){
	    isLocked = false;
	    super.notify();
	  }
	}