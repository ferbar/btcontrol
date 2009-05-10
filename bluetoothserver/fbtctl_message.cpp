/**
 * bastelt die protokoll packete zusammen und zerlegts wieder
 *
 * message:
 *			int len
 *			byte msgtype
 *			daten ...
 *
 * daten:	int: einfach intval
 *			string: byte len, daten
 *			array: byte anzahl
 * 
 *
 * message parsen mit FBTCtlMessage(char *, int len);
 * message zusammenbaun mit:
 * FBTCtlMessage f(msgtype);
 * f["info"][0]["addr"]=1
 * f["info"][0]["speed"]=50
 * f["info"][0]["functions"]=50
 */

#include <string.h>
#include "fbtctl_message.h"

/**
 * parst eine message
 * pointer muss auf 1. byte nach len zeigen
 */
FBTCtlMessage::FBTCtlMessage(const char *binMessage, int len)
{
	InputReader in(binMessage, len);
	this->readMessage(in);
}

void FBTCtlMessage::readMessage(InputReader &in)
{
	this->type=(DataType) in.getInt();

	MessageLayout layout=getMessageLayout(this->type);
	this->readMessage(in, layout);
}

void FBTCtlMessage::readMessage(InputReader &in, const MessageLayout &layout)
{
	MessageLayout::const_iterator it;
	
	for(it=layout.begin(); it != layout.end(); ++it) {
		switch(it->second.type){
			case INT: this->structVal[it->first] = FBTCtlMessage(in.getInt()); break;
			case STRING: this->structVal[it->first] = FBTCtlMessage(in.getString()); break;
			case ARRAY: {
				int n=in.getInt();
				this->structVal[it->first] = FBTCtlMessage(ARRAY);
				for(int i=0; i < n; i++) {
					FBTCtlMessage tmp;
					tmp.readMessage(in,it->second.childLayout);
					this->structVal[it->first][i]=tmp;
				}
				break; }
			default: {
				printf("invalid type (%d)\n",it->second.type);
				break; }
		}
	}

}

FBTCtlMessage::FBTCtlMessage(int i)
{
	this->type=INT;
	this->ival=i;
}

FBTCtlMessage::FBTCtlMessage(const std::string &s)
{
	this->type=INT;
	this->sval=s;
}

FBTCtlMessage::FBTCtlMessage() : type(UNDEF)
{


}

FBTCtlMessage::FBTCtlMessage(DataType type) : type(type)
{


}


int FBTCtlMessage::getIntVal()
{
	if(type != INT) throw "invalid type(int)";
	return ival;
}

std::string FBTCtlMessage::getStringVal()
{
	if(this->type != STRING) throw "invalid type(string)";
	return sval;
}

FBTCtlMessage &FBTCtlMessage::operator [](int i)
{
	if(this->type == UNDEF) this->type=ARRAY;
	if(this->type != ARRAY) throw "invalid type(array)";
	if(arrayVal.size() <= i) {
		arrayVal.push_back(FBTCtlMessage(STRUCT));
	}
	return arrayVal[i];
}

FBTCtlMessage &FBTCtlMessage::operator [](const std::string &s)
{
	if(this->type < STRUCT) throw "invalid type(stuct)";
	return structVal[s];
}

std::string FBTCtlMessage::getBinaryMessage(const MessageLayout *layout)
{
	std::string ret;
	if(!layout)
		layout=getMessageLayout(this->type);

	if(!layout)
		throw "no message layout";
	
	ret += std::string((char *) &this->type, 4);
	switch(this->type) {
		case INT:
			ret += std::string((char *) &this->type, 4);
			break;
		case STRING: {
			int tmp=this->sval.size();
			ret += std::string((char *) &tmp, 1);
			ret += this->sval;
			break; }
		case ARRAY: {
			int tmp=this->arrayVal.size();
			ret += std::string((char *) &tmp, 1);
			for(int i=0; i < arrayVal.size(); i++) {
				ret += arrayVal[i].getBinaryMessage();
			}
			break; }
		default: { // struct + message type
			MessageLayout::iterator it;
			for(it=layout->begin(); it != layout->end(); ++it) {
				ret += structVal[it->first].getBinaryMessage(&it->second.childLayout);
			}
			break; }
	}
		
}

#define INDENT() for(int ii=0; ii < indent; ii++) printf("  ");
void FBTCtlMessage::dump(int indent, const MessageLayout *layout)
{
	
	if(!layout)
		layout=getMessageLayout(this->type);

	if(!layout)
		throw "no message layout";

	if(indent == 0) {
		printf("dump\n");
	}
	INDENT();
	printf("seqnum: %d, type:%s\n",this->seqNum, MessageTypeName(this->type));
	switch(this->type) {
		case INT:
			INDENT();
			printf("(INT) %d\n",this->ival); break;
		case STRING:
			INDENT();
			printf("(STRING) %s\n",this->sval.c_str()); break;
		case ARRAY: {
			for(int i=0; i < arrayVal.size(); i++) {
				INDENT();
				printf("[%d]:\n",i);
				arrayVal[i].dump(indent+1);
			}
			break; }
		default: { // struct + message type
			std::map<std::string,FBTCtlMessage>::iterator it;
			for(it=this->structVal.begin(); it != this->structVal.end(); ++it) {
				INDENT();
				printf("[%s]:\n",it->first.c_str());
				DataType expectedType=layout->operator[](it->first.type).type;
				if(expectedType != it->second.type) {
					printf("type mismatch (expected %s, is: %s)\n",messageTypeName(expectedType).c_str(), messageTypeName(it->second.type).c_str());
				}
				it->second.dump(indent+1);
			}
			break; }
	}
		
}

class MessageLayoutAndName : public MessageLayout {
public:
	MessageLayoutAndName(const MessageLayout &in) : MessageLayout(in) {};
	MessageLayoutAndName() {};
	std::string name;
};

/**
 * zuordnung DataType -> name und wie schaut die message aus
 */
std::map<std::string, MessageLayoutAndTypeName> MessageLayouts;
bool MessageLayoutsInited=false;

const MessageLayout& getMessageLayout(FBTCtlMessage::DataType type)
{
	if(type <= FBTCtlMessage::STRUCT) {
		printf("da hats was (%d)\n", type);
		throw "invalid type";
	}

	if(!MessageLayoutsInited) {
		loadMessageLayouts();
		MessageLayoutsInited=true;
	}
	MessageLayout tmp;
	return MessageLayouts[type];
}

/**
 * @param pos zeigt auf name:TYP
 */
MessageLayout parseMessageLayout(const char *&pos)
{
	printf("parse \"%s\"\n",pos);
	MessageLayout messageLayout;
	while(true) {
		const char *namestart=pos;
		while(*pos && *pos != ':') pos++;
		if(pos=='\0')
			throw "error finding name";
		printf("pos=\"%c\"\n",*pos);
		pos++;
		MessageLayoutInfo info;
		std::string name=std::string(namestart, pos-namestart);
		switch(*pos) {
			case 'I': info.type=FBTCtlMessage::INT;
				messageLayout[name]=info;
				pos++;
				break;
			case 'S': info.type=FBTCtlMessage::STRING;
				messageLayout[name]=info;
				pos++;
				break;
			case 'A': info.type=FBTCtlMessage::ARRAY;
				pos++;
				if(*pos != '{') throw "kein { nach A";
				pos++;
				info.childLayout=parseMessageLayout(pos);
				break;
		}
		if(*pos == '}' || *pos == '\0') {
			break;
		}
		printf("pos2=\"%s\"\n",pos);
		if(*pos != ',') {
			throw ", erwartet";
		}
	}
	return messageLayout;
}

/**
 * tut die protocol.dat laden
 */
void loadMessageLayouts()
{
	FILE *f=fopen("protocol.dat","r");
	if(!f) {
		printf("error reading protocol.dat\n");
		throw "error reading protocol.dat";
	}
	char buffer[1024];
	int n=0;
	while(!feof(f)) {
		fgets(buffer,sizeof(buffer)-1,f);
		int n=strlen(buffer);
		if((buffer[n-1] == '\n') || (buffer[n-1] == '\r')) {
			buffer[n-1]='\0';
		}
		if((buffer[n-2] == '\n') || (buffer[n-2] == '\r')) {
			buffer[n-2]='\0';
		}
		if(buffer[0]=='#') {
			continue;
		}
		const char *pos=buffer;
		while(*pos && isspace(*pos)) pos++;
		if(*pos=='\0')
			continue;
		printf("line: %s\n",buffer);
		const char *namestart=pos;
		while(*pos && *pos!='=') pos++;
		if(*pos=='\0') {
			throw "invalid identifier";
		}
		std::string name(namestart,pos-namestart);
		printf("identifier found: %s\n",name.c_str());
		pos++;

		MessageLayoutAndType messageLayout(parseMessageLayout(pos));
		messageLayout.name=name;
		DataType type=(FBTCtlMessage::DataType) (n+FBTCtlMessage::STRUCT+1);
		MessageLayouts[type] = messageLayout;
	}

	fclose(f);
}

std::string messageTypeName(FBTCtlMessage::DataType type)
{
	switch(type) {
		case FBTCtlMessage::INT: return "(INT)";
		case FBTCtlMessage::STRING: return "(STRING)";
		case FBTCtlMessage::ARRAY: return "(ARRAY)";
		default: {
			std::string tmp;
			tmp +="(STRUCT ";
			tmp += MessageLayouts[type].name;
			tmp +=")";
			break; }
	}
}

FBTCtlMessage::DataType messageTypeID(const std::string &name)
{
	
	std::map<std::string, MessageLayoutAndType>::const_iterator it;
	for(it=MessageLayouts.begin(); it != MessageLayouts.end(); ++it) {
		printf("searching for %s ... ?= %s\n",name.c_str(), it->first.c_str());
		if(name == it->second.nyme)
			return it->first;
	}
	throw "invalid message name";
}
