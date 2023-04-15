#include <string.h>
#include <strings.h>
#include <assert.h>
#include "lokdef.h"
#include "fbtctl_message.h"
#include "utils.h"

#define TAG "REMOTELOKDEF"

bool refreshLokdef=false;

lokdef_t *initLokdef(FBTCtlMessage &reply)
{
	DEBUGF("initLokdef");
	PRINT_FREE_HEAP("initLokdef");
	lokdef_t *tmplokdef=NULL;
	int nLocos=reply["info"].getArraySize();
	DEBUGF("initLokdef n=%d+1, need: %dB", nLocos, sizeof(lokdef_t) * (nLocos+1));
	tmplokdef = (lokdef_t *) calloc(sizeof(lokdef_t),nLocos+1);
	if(!tmplokdef) {
		ERRORF("failed to allocate lokdef (%dB)", sizeof(lokdef_t) * (nLocos+1));
		throw std::runtime_error("failed to allocate lokdef");
	}

	for(int i=0; i < nLocos; i++) {
		DEBUGF("initLokdef %d, name: %s", i, reply["info"][i]["name"].getStringVal().c_str());
		tmplokdef[i].currdir=0;
		tmplokdef[i].addr=reply["info"][i]["addr"].getIntVal();
		strncpy(tmplokdef[i].name, reply["info"][i]["name"].getStringVal().c_str(), sizeof(lokdef[i].name));
		strncpy(tmplokdef[i].imgname, reply["info"][i]["imgname"].getStringVal().c_str(), sizeof(lokdef[i].imgname));
		int speed=reply["info"][i]["speed"].getIntVal();
		tmplokdef[i].currspeed=abs(speed);
		tmplokdef[i].currdir=speed >=0 ? true : false;
		int functions = reply["info"][i]["functions"].getIntVal();
		for(int f=0; f < MAX_NFUNC; f++) {
			tmplokdef[i].func[f].ison=((1 << f) & functions ) ? true : false;
		}

	}
	refreshLokdef=true;
	DEBUGF("init lokdef done");
	PRINT_FREE_HEAP("initLokdef done");
	return tmplokdef;
}

void initLokdefFunctions(lokdef_t *lokdef, int loknum, FBTCtlMessage &reply)
{
	DEBUGF("initLokdefFunctions");
	int nFunc=reply["info"].getArraySize();
	DEBUGF("initLokdefFunctions n=%d", nFunc);
	assert(nFunc < MAX_NFUNC);
	lokdef[loknum].nFunc=nFunc;
	for(int i=0; i < nFunc; i++) {
		DEBUGF("initLokdefFunc %d", i);
		strncpy(lokdef[loknum].func[i].name,    reply["info"][i]["name"].getStringVal().c_str(), sizeof(lokdef[loknum].func[i].name));
#ifndef DISABLE_FUNC_IMGNAME
		strncpy(lokdef[loknum].func[i].imgname, reply["info"][i]["imgname"].getStringVal().c_str(), sizeof(lokdef[loknum].func[i].imgname));
#endif
		int functions = reply["info"][i]["value"].getIntVal();
		for(int f=0; f < MAX_NFUNC; f++) {
			lokdef[loknum].func[i].ison=((1 << f) & functions ) ? true : false;
		}

	}
	DEBUGF("init lokdefFunctions done");
}
