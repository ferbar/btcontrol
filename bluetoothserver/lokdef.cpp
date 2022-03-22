/*
 *  This file is part of btcontrol
 *
 *  Copyright (C) Christian Ferbar
 *
 *  btcontrol is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  btcontrol is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with btcontrol.  If not, see <http://www.gnu.org/licenses/>.
 *
 * read loco list with address + features
 */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sstream>
#include <ctype.h>
#include "lokdef.h"
#include "utils.h"
#include <stdexcept>

lokdef_t *lokdef=NULL; 
#define TAG "lokdef"

/**
 * liefert den index
 * @return < 0 wenn lok nicht gefunden
 * @throws invalid address 
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
	throw std::runtime_error("getAddrIndex: invalid address " + utils::to_string(addr));
}

#define CHECKVAL(_FMT, ...)	\
		if(!pos) {	\
			fprintf(stderr,"%s:%d " _FMT " \n",LOKDEF_FILENAME, lineNo, ## __VA_ARGS__);	\
			throw std::runtime_error(utils::format(_FMT, ## __VA_ARGS__));	\
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

const char *getnext(const char **pos)
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
	while(**pos && ((**pos == ' ') || (**pos == '\t'))) {
		// printf("%c",**pos);
		(*pos)++;
	}
	// printf("\n");
	const char *endpos=*pos;
	while(*endpos && (strchr(",\r\n",*endpos) == NULL)) endpos++;
	return endpos;
}

/**
 *
 * @return true = sussess
 */
bool readLokdef()
{
#define LOKDEF_FILENAME "conf/lokdef.csv"
	std::string lokdefCsv = readFile(LOKDEF_FILENAME);
	if(lokdefCsv == "") {
		throw new std::runtime_error("lokdev.csv is empty");
	}
	std::string line;
	const char *buffer;
	std::istringstream f(lokdefCsv);

	int n=0;
	int lineNo=0;
	while (std::getline(f, line)) {
		buffer = line.c_str();
		lineNo++;
		// printf("line %d\n",line);
		if(buffer[0] == '#') continue;
		lokdef = (lokdef_t *) realloc(lokdef, sizeof(lokdef_t)*(n+1));
		bzero(&lokdef[n],sizeof(lokdef_t));
		lokdef[n].currdir=1;
		const char *pos=buffer;
		const char *pos_end;
		lokdef[n].addr=atoi(pos);
		pos_end=getnext(&pos);
		if(pos==NULL) {
			continue;
		}
		lokdef[n].flags=str2decodertype(pos);
		pos_end=getnext(&pos);
		strncpy(lokdef[n].name, pos,  MIN((signed)sizeof(lokdef[n].name)-1, pos_end-pos));
		if(pos_end-pos >= (signed)sizeof(lokdef[n].name)) {
			ERRORF("readLokdef warning line %d: lokdef.name > size: \"%.*s\", allowed max:%d", lineNo, (int)(pos_end-pos),pos, (int) sizeof(lokdef[n].name));
			lokdef[n].name[sizeof(lokdef[n].name)-1]='\0';
		}
		strtrim(lokdef[n].name);
		pos_end=getnext(&pos);

		strncpy(lokdef[n].imgname, pos, MIN((signed)sizeof(lokdef[n].imgname)-1, pos_end-pos));
		if(pos_end-pos >= (signed)sizeof(lokdef[n].imgname)) {
			ERRORF("readLokdef warning line %d: lokdef.imgname > size: \"%.*s\", allowed max:%d", lineNo, (int)(pos_end-pos),pos, (int) sizeof(lokdef[n].imgname));
			lokdef[n].imgname[sizeof(lokdef[n].imgname)-1]='\0';
		}
		strtrim(lokdef[n].imgname);
		pos_end=getnext(&pos);
		CHECKVAL("error reading nfunc");
		lokdef[n].nFunc=atoi(pos)+1;
		if(lokdef[n].nFunc > MAX_NFUNC) {
			throw std::runtime_error(utils::format("%s:%d Error: lokdef.nFunc > MAX_NFUNC\n", LOKDEF_FILENAME, lineNo));
		}
		strcpy(lokdef[n].func[0].name,"lHeadlight");
		lokdef[n].func[0].ison=true;

		for(int i=1; i < lokdef[n].nFunc; i++) {
			pos_end=getnext(&pos);
			CHECKVAL("func i = %d, nfunc %d invalid? %d function names missing",i,lokdef[n].nFunc,lokdef[n].nFunc-i);
			// =1 suchen
			int funcNameLen=pos_end-pos;
			// DEBUGF("[%d] function name: [%.*s], funcNameLen=%d", i, funcNameLen, pos, funcNameLen);
			if(memmem(pos, funcNameLen, "=1", 2)) {
				DEBUGF("startval=1 for %.*s", funcNameLen, pos);
				funcNameLen-=2;
				lokdef[n].func[i].ison=true;
			}
			strncpy(lokdef[n].func[i].name, pos, MIN((signed)sizeof(lokdef[n].func[i].name), funcNameLen));
			if(strchr(lokdef[n].func[i].name,'\n') != NULL) {
				throw std::runtime_error(utils::format("%s:%d Error: newline in funcname (%s) - error parsing lokdef\n", LOKDEF_FILENAME, lineNo, lokdef[n].func[i].name));
			}
		}
		if(*pos_end != '\0') {
			throw std::runtime_error(utils::format("%s:%d Error: too many function names [%s]", LOKDEF_FILENAME, lineNo, pos_end));
		}

		n++;
	}
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
		printf("currspeed:%d currdir:%d\n",lokdef[n].currspeed,lokdef[n].currdir);
		n++;

	}
}
