/**
 * 
 * @see http://stackoverflow.com/questions/13416879/show-a-dialog-in-thread-setdefaultuncaughtexceptionhandler
 */

package org.ferbar.btcontrol;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

public class UncaughtException extends Activity {
    protected static final String DOUBLE_LINE_SEP = "\n\n";
	protected static final String SINGLE_LINE_SEP = "\n";
	protected static final String EXTRA_CRASHED_FLAG = "crashed flag";
	protected static final String EXTRA_REPORT = "report";


	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		this.setContentView(R.layout.activity_uncaught_exception);
		Bundle extras = this.getIntent().getExtras();
		String report="???";
		if(extras != null) {
			report=(String) extras.get(EXTRA_REPORT);
		}
		TextView tv_report=(TextView) this.findViewById(R.id.textViewReport);
		tv_report.setText(report);
	}

	/*
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.uncaught_exception, menu);
		return true;
	}
	*/

	/*
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			return true;
		}
		return super.onOptionsItemSelected(item);
	}
	*/


	public static void setGlobalUncaughtExceptionHandler(final Activity activity) {
    	// Vorsicht: exception darf nicht im uithread vom AndroidMain auftreten!!! 
    	final Context context=activity.getBaseContext(); // getApplicationContext();
		final Thread.UncaughtExceptionHandler oldHandler = Thread.getDefaultUncaughtExceptionHandler();
		Thread.setDefaultUncaughtExceptionHandler(
            new Thread.UncaughtExceptionHandler() {
            	/*
                @Override
                public void uncaughtException(Thread thread, final Throwable ex) {
                	Log.e(TAG, "exception:"+ex.getMessage());
                	ex.printStackTrace();
                	runOnUiThread(new Runnable() {
						@Override
						public void run() {
							Toast.makeText(context, "exception!" + ex.toString(), Toast.LENGTH_LONG).show();
							
		                    AlertDialog.Builder builder = new AlertDialog.Builder(context);
		                    builder.setTitle("There is something wrong")
		                            .setMessage("Application will exit：" + ex.toString())
		                            .setPositiveButton("OK", new DialogInterface.OnClickListener() {
		
		                                @Override
		                                public void onClick(DialogInterface dialog, int which) {
		                                    // throw it again
		                                    throw (RuntimeException) ex;
		                                }
		                            })
		                            .show();
						}
                	});
                	oldHandler.uncaughtException(thread, ex);
                }
                */
			    /*
			     * (non-Javadoc)
			     * 
			     * @see
			     * java.lang.Thread.UncaughtExceptionHandler#uncaughtException(java.
			     * lang.Thread, java.lang.Throwable)
			     */
			    @Override
			    public void uncaughtException(Thread t, final Throwable e) {
			        StackTraceElement[] arr = e.getStackTrace();
			        final StringBuffer report = new StringBuffer(e.toString());
			        final String lineSeperator = "-------------------------------\n\n";
			        report.append(DOUBLE_LINE_SEP);
			        report.append("--------- Stack trace ---------\n\n");
			        for (int i = 0; i < arr.length; i++) {
			            report.append( "    ");
			            report.append(arr[i].toString());
			            report.append(SINGLE_LINE_SEP);
			        }
			        report.append(lineSeperator);
			        // If the exception was thrown in a background thread inside
			        // AsyncTask, then the actual exception can be found with getCause
			        report.append("--------- Cause ---------\n\n");
			        Throwable cause = e.getCause();
			        if (cause != null) {
			            report.append(cause.toString());
			            report.append(DOUBLE_LINE_SEP);
			            arr = cause.getStackTrace();
			            for (int i = 0; i < arr.length; i++) {
			                report.append("    ");
			                report.append(arr[i].toString());
			                report.append(SINGLE_LINE_SEP);
			            }
			        }
			        // get app version code / name
			        try {
			        	String packageName= context.getPackageName();
						PackageInfo pInfo = context.getPackageManager().getPackageInfo(packageName, 0);
						report.append(lineSeperator);
				        report.append("--------- App ---------\n\n");
				        report.append("Name: ");
				        report.append(packageName);
				        report.append(SINGLE_LINE_SEP);
				        report.append("Version: ");
				        report.append(pInfo.versionName + " - " + pInfo.versionCode);
				        report.append(SINGLE_LINE_SEP);
					} catch (NameNotFoundException e1) {
						// TODO Auto-generated catch block
						e1.printStackTrace();
					}
			        // Getting the Device brand,model and sdk verion details.
			        report.append(lineSeperator);
			        report.append("--------- Device ---------\n\n");
			        report.append("Brand: ");
			        report.append(Build.BRAND);
			        report.append(SINGLE_LINE_SEP);
			        report.append("Device: ");
			        report.append(Build.DEVICE);
			        report.append(SINGLE_LINE_SEP);
			        report.append("Model: ");
			        report.append(Build.MODEL);
			        report.append(SINGLE_LINE_SEP);
			        report.append("Id: ");
			        report.append(Build.ID);
			        report.append(SINGLE_LINE_SEP);
			        report.append("Product: ");
			        report.append(Build.PRODUCT);
			        report.append(SINGLE_LINE_SEP);
			        report.append(lineSeperator);
			        report.append("--------- Firmware ---------\n\n");
			        report.append("SDK: ");
			        report.append(Build.VERSION.SDK);
			        report.append(SINGLE_LINE_SEP);
			        report.append("Release: ");
			        report.append(Build.VERSION.RELEASE);
			        report.append(SINGLE_LINE_SEP);
			        report.append("Incremental: ");
			        report.append(Build.VERSION.INCREMENTAL);
			        report.append(SINGLE_LINE_SEP);
			        report.append(lineSeperator);
			
			        Log.e("Report ::", report.toString());
			        Intent crashedIntent = new Intent(activity, UncaughtException.class);
			        crashedIntent.putExtra(EXTRA_CRASHED_FLAG,  "Unexpected Error occurred.");
			        crashedIntent.putExtra(EXTRA_REPORT, report.toString());
			        crashedIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
			        crashedIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
			        activity.startActivity(crashedIntent);
			
			        System.exit(0);
			        // If you don't kill the VM here the app goes into limbo
			
			    }
            });
    }
    
	public void onClickOK(View v) {
		System.exit(0);
	}

	public void onClickSendMail(View v) {
		Intent i = new Intent(Intent.ACTION_SEND);
		i.setType("message/rfc822");
		i.putExtra(Intent.EXTRA_EMAIL  , new String[]{"chris@qnipp.com"});
		i.putExtra(Intent.EXTRA_SUBJECT, "crash report for ");
		String report = (String) ((TextView) this.findViewById(R.id.textViewReport)).getText();
		i.putExtra(Intent.EXTRA_TEXT, report);
		try {
		    startActivity(Intent.createChooser(i, "Send mail..."));
		} catch (android.content.ActivityNotFoundException ex) {
		    Toast.makeText(this, "There are no email clients installed.", Toast.LENGTH_SHORT).show();
		}
		System.exit(0);
	}
}
