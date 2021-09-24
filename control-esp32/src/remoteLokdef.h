#include "lokdef.h"

extern bool refreshLokdef;

lokdef_t *initLokdef(FBTCtlMessage &reply);
void initLokdefFunctions(lokdef_t *lokdef, int loknum, FBTCtlMessage &reply);
