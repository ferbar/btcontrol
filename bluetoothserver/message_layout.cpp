/*
 *  This file is part of btcontroll
 *  btcontroll is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  btcontroll is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with btcontroll.  If not, see <http://www.gnu.org/licenses/>.
 *
 * message generation
 */
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "message_layout.h"
#include "fbtctl_message.h"

/**
 * zuordnung DataType -> name und wie schaut die message aus
 */
typedef std::map<std::string, MessageLayout> MessageLayouts;
MessageLayouts messageLayouts;
bool MessageLayoutsInited=false;

/**
 * nur für structs sinnvoll
 */
const MessageLayout& getMessageLayout(MessageLayout::DataType type)
{
	if(type <= MessageLayout::STRUCT) {
		printf("getMessageLayout: non-struct (%d)\n", type);
		throw "invalid type";
	}

	MessageLayout tmp;
	MessageLayouts::const_iterator it;
	for(it=messageLayouts.begin(); it != messageLayouts.end(); ++it) {
		// printf("searching for %d ... ?= %s\n", type, it->first.c_str());
		if(type == it->second.type)
			return messageLayouts[it->first];
	}
	throw "invalid struct type";
}

/**
 * @param pos zeigt auf name:TYP
 * TODO: Das in die MessageLayout klasse tun *****************************
 */
MessageLayout parseMessageLayout(const char *&pos)
{
	// printf("parse \"%s\"\n",pos);
	MessageLayout messageLayout;
	if(*pos=='\0') { // leere zeile
		return messageLayout;
	}
	while(true) {
		const char *namestart=pos;
		while(*pos && *pos != ':') pos++;
		if(*pos=='\0')
			throw "error finding name";
		// printf("pos=\"%c\"\n",*pos);
		MessageLayout info;
		std::string name=std::string(namestart, pos-namestart);
		info.name=name;
		pos++;
		switch(*pos) {
			case 'I': info.type=MessageLayout::INT;
				pos++;
				break;
			case 'S': info.type=MessageLayout::STRING;
				pos++;
				break;
			case 'A': info.type=MessageLayout::ARRAY;
				pos++;
				if(*pos != '{') throw "kein { nach A";
				pos++;
				info=parseMessageLayout(pos);
				info.name=name;
				info.type=MessageLayout::ARRAY;
				break;
			case '\0': // ende/leer
				break;
			default:
				printf("invalid id: %s\n",pos);
				throw "error parsing protocol.dat";
		}
		messageLayout.childLayouts.push_back(info);
		if(*pos == '}' || *pos == '\0') {
			break;
		}
		// printf("pos2=\"%s\"\n",pos);
		if(*pos != ',') {
			throw ", erwartet";
		}
		pos++;
	}
	return messageLayout;
}

/**
 * tut die protocol.dat laden
 * @return hash von protocol.dat (zum vergleich ob das tel die selbe protocol.dat verwendet)
 *              kommentare werden ignoriert
 */
int loadMessageLayouts()
{
	int hash=0;
	if(MessageLayoutsInited==true)
		return 0;
	MessageLayoutsInited=true;
	FILE *f=fopen("protocol.dat","r");
	if(!f) {
		printf("error reading protocol.dat\n");
		throw "error reading protocol.dat";
	}
	char buffer[1024];
	int typeNr=MessageLayout::STRUCT+1;
	while(!feof(f)) {
		if(fgets(buffer,sizeof(buffer)-1,f) == NULL) {
			if(!feof(f)) {
				printf("error reading line (%s)\n",strerror(errno));
			}
			break;
		}
		int n=strlen(buffer);
		if((n>=1) && ((buffer[n-1] == '\n') || (buffer[n-1] == '\r'))) {
			buffer[n-1]='\0';
		}
		if((n>=2) && ((buffer[n-2] == '\n') || (buffer[n-2] == '\r'))) {
			buffer[n-2]='\0';
		}
		if(buffer[0]=='#') {
			continue;
		}
		{ // hash für die zeile berechnen
			int lineHash=0;
			const unsigned char *n=(unsigned char *) buffer;
			while(*n) { lineHash+=*n; n++;}
			hash+=lineHash;
		}

		const char *pos=buffer;
		while(*pos && isspace(*pos)) pos++;
		if(*pos=='\0')
			continue;
		// printf("line: %s\n",buffer);
		const char *namestart=pos;
		while(*pos && *pos!='=') pos++;
		if(*pos=='\0') {
			throw "invalid identifier";
		}
		std::string name(namestart,pos-namestart);
		// printf("identifier found: %s\n",name.c_str());
		pos++;

		MessageLayout messageLayout(parseMessageLayout(pos));
		MessageLayout::DataType type=(MessageLayout::DataType) (typeNr);
		messageLayout.type=type;
		// printf("messayLayout: type: %d line:%s\n",type,buffer);
		messageLayouts[name] = messageLayout;
		typeNr++;
	}

	fclose(f);
	return hash;
}

void MessageLayout::dump(int indent) const
{
	printf("sub:%zu\n", this->childLayouts.size());
	std::vector<MessageLayout>::const_iterator it;
	for(it=this->childLayouts.begin(); it != this->childLayouts.end(); ++it) {
		INDENT();
		std::string typeName;
		try {
			typeName=messageTypeName(it->type);
		} catch( const char *e) {
			typeName=e;
		}
		printf("[%s]:[%s]\n",it->name.c_str(), typeName.c_str());
		if(it->type >= MessageLayout::ARRAY) {
			it->dump(indent+1);
		}
	}
}

void dumpMessageLayouts()
{
	printf("dumpMessageLayouts\n");
	MessageLayouts::const_iterator it;
	for(it=messageLayouts.begin(); it != messageLayouts.end(); ++it) {
		printf("%s=%d:\n",it->first.c_str(),it->second.type);
		it->second.dump(1);

	}

}


std::string messageTypeName(MessageLayout::DataType type)
{
	switch(type) {
		case MessageLayout::UNDEF: throw "undefined data type cant get type for this";
		case MessageLayout::INT: return "(INT)";
		case MessageLayout::STRING: return "(STRING)";
		case MessageLayout::ARRAY: return "(ARRAY)";
		case MessageLayout::STRUCT: return "(STRUCT)";
		default: {
			std::string tmp;
			tmp +="(STRUCT ";
			MessageLayouts::const_iterator it;
			for(it=messageLayouts.begin(); it != messageLayouts.end(); ++it) {
				// printf("searching for %d (%s)\n",type, it->first.c_str());
				if(type == it->second.type)
					break;
			}
			if(it == messageLayouts.end()) {
				std::string tmp;
				tmp += "invalid id (";
				tmp += type;
				tmp += ")";
				throw tmp;
			}
			tmp += it->first;
			tmp +=")";
			return tmp;
			}
	}
	assert(!"no way out of the switch");
}

MessageLayout::DataType messageTypeID(const std::string &name)
{
	MessageLayout::DataType type=messageLayouts[name].type;
	if(type == MessageLayout::UNDEF)
		throw std::string("invalid message name: \"") + name + "\"";
	return type;
	/*
	MessageLayouts::const_iterator it;
	for(it=messageLayouts.begin(); it != messageLayouts.end(); ++it) {
		printf("searching for %s ... ?= %s\n",name.c_str(), it->second.name.c_str());
		if(name == it->second.name)
			return it->first;
	}
	*/
}
