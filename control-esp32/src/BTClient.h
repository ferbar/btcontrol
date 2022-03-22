#ifndef BTCLIENT_H
#define BTCLIENT_H

#include <BTAddress.h>
#include <BluetoothSerial.h>

#include <string>
#include <map>
#include "ClientStream.h"

class BTClient : public ClientStream, public BluetoothSerial {
public:
	BTClient() :remoteAddress("") {};
	void connect(const BTAddress &address, int channel);
	virtual void close();
	virtual ~BTClient();
	virtual void readSelect(int timeout);
	virtual void prepareMessage();
	virtual void flushMessage();
	virtual std::string getRemoteAddr();

	virtual ssize_t read(void *buf, size_t count);
	virtual int read() { return BluetoothSerial::read(); };
	virtual ssize_t write(const void *buf, size_t count);

/* => BluetoothSerial
    begin(name);
	void discoverAsync(std::function< void(BTAdvertisedDevice* pDevice) >& deviceFoundCallback);
	void discoverAsyncStop(); => BluetoothSerial
*/

	// channel number : service name
	// typedef std::map<int, const std::string> SDP_SPP_Channels;
	// void getSPPChannels(const Bluetooth::BTAddress &addr, std::function< void(SDP_SPP_Channels)>& SDPCallback );
	virtual bool isConnected();
private:
	BTAddress remoteAddress;
};

extern BTClient btClient;

#endif
