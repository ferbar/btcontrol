#ifndef BTUTILS_H
#define BTUTILS_H

#include <string>
#include "fbtctl_message.h"

namespace BTUtils {
	void BTPush(std::string addr);
	void BTScan(FBTCtlMessage &reply);
	std::string getRemoteAddr(int so);
}

#endif
