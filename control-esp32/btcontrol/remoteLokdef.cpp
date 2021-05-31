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
	lokdef_t *tmplokdef=NULL;
	int nLocos=reply["info"].getArraySize();
	DEBUGF("initLokdef n=%d", nLocos);
	tmplokdef = (lokdef_t *) calloc(sizeof(lokdef_t),nLocos+1);

	for(int i=0; i < nLocos; i++) {
		DEBUGF("initLokdef %d", i);
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
		strncpy(lokdef[loknum].func[i].imgname, reply["info"][i]["imgname"].getStringVal().c_str(), sizeof(lokdef[loknum].func[i].imgname));
		int functions = reply["info"][i]["value"].getIntVal();
		for(int f=0; f < MAX_NFUNC; f++) {
			lokdef[loknum].func[i].ison=((1 << f) & functions ) ? true : false;
		}

	}
	DEBUGF("init lokdefFunctions done");
}
