package org.ferbar.btserver;

import android.annotation.TargetApi;
import android.content.Context;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdServiceInfo;
import android.os.Build;
import android.util.Log;

import java.net.Inet4Address;
import java.net.InetAddress;

/**
 * Created by chris on 06.01.17.
 * FIXME: das ins android eingebaute MDNS meldet sich immer mit Android.local. Workaround: das jmdns verwenden (siehe SSHelper app z.b.)
 */
public class BTcommServerMDNS {
    private NsdManager mNsdManager;
    public final String TAG = "BTcommServerMDNS";
    private NsdManager.RegistrationListener mRegistrationListener;


    public void registerService(Context context, int port) throws Exception {
        // Create the NsdServiceInfo object, and populate it.
        NsdServiceInfo serviceInfo  = null;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.JELLY_BEAN) {

            this.mRegistrationListener = new NsdManager.RegistrationListener() {

                @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
                @Override
                public void onServiceRegistered(NsdServiceInfo NsdServiceInfo) {
                    // Save the service name.  Android may have changed it in order to
                    // resolve a conflict, so update the name you initially requested
                    // with the name Android actually used.
                    String mServiceName = NsdServiceInfo.getServiceName();
                    Log.i(TAG, "onServiceRegistered "+mServiceName);
                }

                @Override
                public void onRegistrationFailed(NsdServiceInfo serviceInfo, int errorCode) {
                    // Registration failed!  Put debugging code here to determine why.
                    Log.w(TAG, "onRegistrationFailed errorCode:" + errorCode);
                }

                @Override
                public void onServiceUnregistered(NsdServiceInfo arg0) {
                    // Service has been unregistered.  This only happens when you call
                    // NsdManager.unregisterService() and pass in this listener.
                    Log.i(TAG, "onServiceUnregistered");
                }

                @Override
                public void onUnregistrationFailed(NsdServiceInfo serviceInfo, int errorCode) {
                    // Unregistration failed.  Put debugging code here to determine why.
                    Log.w(TAG, "onUnregistrationFailed errorCode:" + errorCode);
                }
            };

            serviceInfo = new NsdServiceInfo();

            // The name is subject to change based on conflicts
            // with other services advertised on the same network.
            serviceInfo.setServiceName("android - btcontrol");
            serviceInfo.setServiceType("_btcontrol._tcp");
            serviceInfo.setPort(port);
            // serviceInfo.setAttribute("hostname","test.local");

            mNsdManager = (NsdManager) context.getSystemService(Context.NSD_SERVICE);

            mNsdManager.registerService(
                    serviceInfo, NsdManager.PROTOCOL_DNS_SD, mRegistrationListener);
        } else { // das müsst ma mit dem jmdns machen, aber hab kein < 4.1 handy das usb otg kann
            throw new Exception("MDNS not implemented in "+ android.os.Build.VERSION.SDK_INT );
        }
    }

    public void unregisterService() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            mNsdManager.unregisterService(mRegistrationListener);
            // mNsdManager.stopServiceDiscovery(mDiscoveryListener);
        }
    }
}
