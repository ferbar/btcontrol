#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
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

#define CHECKVAL(_FMT, ...)	\
		if(!pos) {	\
			fprintf(stderr,"%s:%d " _FMT " \n",LOKDEF_FILENAME,n+1, ## __VA_ARGS__);	\
			fclose(flokdef);	\
			return false;	\
		}


/**
 * tut spaces am ende weg
 */
void strtrim(char *s)
{
	char *pos=s+strlen(s);
	while(pos > s) {
		pos--;
		if(isspace(*pos)) {
			*pos='\0';
		} else
			break;
	}
}

int str2decodertype(const char *pos)
{
	if(strncmp(pos,"F_DEC14",7) == 0) {
		return F_DEC14;
	}
	if(strncmp(pos,"F_DEC28",7) == 0) {
		return F_DEC28;
	}
	return F_DEFAULT;
}

char *getnext(char **pos)
{
	// printf("skipping value:");
	while(**pos && (strchr(",\r\n",**pos) == NULL)) {
		// printf("%c",**pos);
		(*pos)++;
	}
	// printf("\n");
	if(!**pos) {*pos=NULL; return NULL; }
	(*pos)++; // , überspringen
	// printf("skipping spaces:");
	while(**pos && (**pos == ' ') || (**pos == '\t')) {
		// printf("%c",**pos);
		(*pos)++;
	}
	// printf("\n");
	char *endpos=*pos;
	while(*endpos && (strchr(",\r\n",*endpos) == NULL)) endpos++;
	return endpos;
}

/**
 *
 * @return true = sussess
 */
bool readLokdef()
{
#define LOKDEF_FILENAME "lokdef.csv"
	FILE *flokdef=fopen(LOKDEF_FILENAME,"r");
	if(!flokdef) {
		fprintf(stderr,"error reading %s \n",LOKDEF_FILENAME);
		return false;
	}
	char buffer[1024];
	int n=0;
	int line=0;
	while(fgets(buffer,sizeof(buffer),flokdef)) {
		line++;
		// printf("line %d\n",line);
		if(buffer[0] =='#') continue;
		lokdef = (lokdef_t *) realloc(lokdef, sizeof(lokdef_t)*(n+1));
		bzero(&lokdef[n],sizeof(lokdef_t));
		lokdef[n].currdir=1;
		char *pos=buffer;
		char *pos_end;
		lokdef[n].addr=atoi(pos);
		pos_end=getnext(&pos);
		lokdef[n].flags=str2decodertype(pos);
		pos_end=getnext(&pos);
		strncpy(lokdef[n].name, pos,  MIN(sizeof(lokdef[n].name), pos_end-pos));
		strtrim(lokdef[n].name);
		pos_end=getnext(&pos);
		//TODO: trim
		strncpy(lokdef[n].imgname, pos, MIN(sizeof(lokdef[n].imgname), pos_end-pos));
		strtrim(lokdef[n].imgname);
		pos_end=getnext(&pos);
		CHECKVAL("error reading nfunc");
		lokdef[n].nFunc=atoi(pos);

		for(int i=0; i < lokdef[n].nFunc; i++) {
			pos_end=getnext(&pos);
			CHECKVAL("func i = %d, nfunc %d invalid? %d function names missing",i,lokdef[n].nFunc,lokdef[n].nFunc-i);
			strncpy(lokdef[n].func[i].name, pos, MIN(sizeof(lokdef[n].func[i].name), pos_end-pos));
			if(strchr(lokdef[n].func[i].name,'\n') != NULL) {
				fprintf(stderr,"%s:%d newline in funcname (%s)- irgendwas hats da\n",LOKDEF_FILENAME,n+1,lokdef[n].func[i].name);
				exit(1);
			}
		}

		n++;
	}
	fclose(flokdef);
	// listen-ende:
	lokdef = (lokdef_t *) realloc(lokdef, sizeof(lokdef_t)*(n+1));
	bzero(&lokdef[n],sizeof(lokdef_t));

	return true;
}

void dumpLokdef()
{
	int n=0;
	printf("----------------- dump lokdef -------------------------\n");
	while(lokdef[n].addr) {
		printf("addr:%d flags:%d, name:%s, img:%s, nFunc:%d,", lokdef[n].addr, lokdef[n].flags, lokdef[n].name, lokdef[n].imgname, lokdef[n].nFunc);
		for(int i=0; i < lokdef[n].nFunc; i++) {
			printf("%s:%d ",lokdef[n].func[i].name,lokdef[n].func[i].ison);
		}
		printf("currspeed:%d\n",lokdef[n].currspeed);
		n++;

	}
}
