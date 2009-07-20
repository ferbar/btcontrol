#include "lokdef.h"

lokdef_t *lokdef; 

/**
 * liefert den index
 * @return < 0 wenn lok nicht gefunden
 *
 */
int getAddrIndex(int addr)
{
	int i=0;
	while(lokdef[i].addr) {
		if(lokdef[i].addr==addr) {
			return i;
		}
		i++;
	}
	return -1;
}

