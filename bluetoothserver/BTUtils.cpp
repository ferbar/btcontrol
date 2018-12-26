#include <stdio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

#include "BTUtils.h"
#include "utils.h"

/**
 * liefert die remote Addr für eine verbindung
 * @param so: socket zum client (nicht der listen-socket)
 */
std::string BTUtils::getRemoteAddr(int so)
{
	// struct rfcomm_dev_req req;
	struct sockaddr_rc raddr;
	char dst[18];


	socklen_t addrlen=sizeof(raddr);
	if (getpeername(so, (struct sockaddr *)&raddr, &addrlen) < 0) {
		perror("Can't get RFCOMM socket name");
		exit(1);
	}

	ba2str(&raddr.rc_bdaddr, dst);
	return std::string(dst);
}

void BTUtils::BTPush(std::string addr)
{
	printf("BT send update: to %s\n", addr.c_str());
	std::string cmd;
	cmd+="./ussp-push " + addr + "@ btcontrol.jar btcontrol.jar";
	system(cmd.c_str());
}

void BTUtils::BTScan(FBTCtlMessage &reply)
{
	FILE *f=popen("hcitool scanning --flush","r");
	if(f) {
		char buffer[1024];
		if(fgets(buffer,sizeof(buffer),f) == NULL) {
			fprintf(stderr,"error reading header\n");
		} else if(!STREQ(buffer,"Scanning ...\n")) {
			fprintf(stderr,"wrong header\n");
		} else {
			int i=0;
			while(fgets(buffer,sizeof(buffer),f)) {
				int n=strlen(buffer);
				if(n > 1) {
					buffer[n-1]='\0';
				}
				char addr[1024];
				char name[1024];
				if(sscanf(buffer,"%s %12c",addr,name) == 2 ) {
					printf("addr: [%s], name: [%s]\n",addr,name);
					reply["info"][i]["addr"]=addr;
					reply["info"][i]["name"]=name;
					i++;
				} else {
					fprintf(stderr,"error scanning line %s\n",buffer);
				}

			}
		}
		int rc=pclose(f);
		if(rc) {
			fprintf(stderr,"hcitool error");
		}
	} else {
		perror("hcitool scanning --flush error");
	}
}

