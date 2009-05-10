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

void FBTCtlMessage::readMessage(InputReader &in, const MessageLayout *layout)
{
	printf("reading bin msg\n");

	this->type=(DataType) in.getByte();
	printf("datatype: %d\n",this->type);

	if(!layout && (this->type > STRUCT))
		layout=&getMessageLayout(this->type);

	if(!layout)
		throw "no message layout";

	MessageLayout::const_iterator it;
	for(it=layout->begin(); it != layout->end(); ++it) {
		DataType type=(DataType)in.getByte();
		if(type != it->second.type) {
			printf("unexpected type! %d, %d (%s)\n",type, it->second.type, messageTypeName((DataType)it->second.type).c_str());
			throw "unexpected DataType";
		}
		switch(it->second.type){
			case INT:
				printf("read int\n");
				this->structVal[it->first] = FBTCtlMessage(in.getInt()); break;
			case STRING:
				printf("read string\n");
				this->structVal[it->first] = FBTCtlMessage(in.getString()); break;
			case ARRAY: {
				int n=in.getByte();
				printf("read arraysize:%d\n",n);
				FBTCtlMessage tmparray(ARRAY);
				for(int i=0; i < n; i++) {
					FBTCtlMessage tmp;
					tmp.readMessage(in,&it->second.childLayout);
					tmparray[i]=tmp;
				}
				this->structVal[it->first] = tmparray;
				break; }
		/* ... wir sind in einem struct drinnen
			case STRUCT: {
				printf("struct\n");
				MessageLayout::const_iterator it_struct;
				for(it_struct = it->second.childLayout.begin(); it_struct != it->second.childLayout.end(); ++it_struct) {
					this->structVal[it_struct->first] = FBTCtlMessage(ARRAY);
				}
				break; }
		*/
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
	this->type=STRING;
	this->sval=s;
}

FBTCtlMessage::FBTCtlMessage(const char *s)
{
	this->type=STRING;
	this->sval=std::string(s);
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
		layout=&getMessageLayout(this->type);

	if(!layout)
		throw "no message layout";
	
	ret += std::string((char *) &this->type, 1);
	switch(this->type) {
		case INT:
			ret += std::string((char *) &this->ival, 4);
			break;
		case STRING: {
			int tmp=this->sval.size();
			ret += std::string((char *) &tmp, 1);
			ret += this->sval;
			break; }
		case ARRAY: {
			int tmp=this->arrayVal.size();
			ret += std::string((char *) &tmp, 1);
			/*
			printf("arraysize:%d string.size=%d:<",tmp,ret.size());
			fwrite(ret.data(),1,ret.size(),stdout);
			printf(">\n");
			*/
			for(int i=0; i < arrayVal.size(); i++) {
				ret += arrayVal[i].getBinaryMessage(layout);
			}
			break; }
		default: { // struct + message type
			MessageLayout::const_iterator it;
			for(it=layout->begin(); it != layout->end(); ++it) {
				if(structVal[it->first].type == it->second.type) {
					ret += structVal[it->first].getBinaryMessage(&it->second.childLayout);
				} else {
					throw "invalid type (try dump before";
				}
			}
			break; }
	}
	return ret;
		
}

#define INDENT() for(int ii=0; ii < indent; ii++) printf("  ");
void FBTCtlMessage::dump(int indent, const MessageLayout *layout) const
{
	
	if(!layout)
		layout=&getMessageLayout(this->type);

	if(!layout)
		throw "no message layout";

	// layout->dump();
	if(indent == 0) {
		printf("dumpMessage\n");
	}
	INDENT();
	printf("seqnum: %d, type:%d=%s\n",this->seqNum, this->type,  messageTypeName(this->type).c_str());
	switch(this->type) {
		case INT:
			INDENT();
			printf("(INT) %d\n",this->ival); break;
		case STRING:
			INDENT();
			printf("(STRING) %s\n",this->sval.c_str()); break;
		case ARRAY: {
			// INDENT(); printf("array\n");
			for(int i=0; i < arrayVal.size(); i++) {
				INDENT();
				printf("[%d]:\n",i);
				arrayVal[i].dump(indent+1,layout);
			}
			break; }
		default: { // struct + message type
			// INDENT(); printf("struct\n");
			std::map<std::string,FBTCtlMessage>::const_iterator it;
			for(it=this->structVal.begin(); it != this->structVal.end(); ++it) {
				INDENT();
				printf("[%s]:\n",it->first.c_str());
				MessageLayoutInfo *childLayoutInfo=&const_cast<MessageLayout *>(layout)->operator[](it->first);
				DataType expectedType=(DataType)childLayoutInfo->type;
				if(expectedType == it->second.type) {
					it->second.dump(indent+1, &childLayoutInfo->childLayout);
				} else {
					try {
						std::string typeName=messageTypeName(it->second.type);
						printf("type mismatch (expected %s, is: %s)\n",messageTypeName(expectedType).c_str(), typeName.c_str());
					} catch(const char *e) {
						printf("exception: %s ",e);
						printf("invalid field: %s\n",it->first.c_str());
						layout->dump();

					}
				}
				
			}
			break; }
	}
		
	if(indent == 0) {
		printf("dumpMessage done \n");
	}
}

class MessageLayoutAndType: public MessageLayout {
public:
	MessageLayoutAndType(const MessageLayout &in) : MessageLayout(in), type(FBTCtlMessage::UNDEF) {};
	MessageLayoutAndType():type(FBTCtlMessage::UNDEF) {};
	FBTCtlMessage::DataType type;
};

/**
 * zuordnung DataType -> name und wie schaut die message aus
 */
typedef std::map<std::string, MessageLayoutAndType> MessageLayouts;
MessageLayouts messageLayouts;
bool MessageLayoutsInited=false;

/**
 * nur für structs sinnvoll
 */
const MessageLayout& getMessageLayout(FBTCtlMessage::DataType type)
{
	if(type <= FBTCtlMessage::STRUCT) {
		printf("getMessageLayout: non-struct (%d)\n", type);
		throw "invalid type";
	}

	if(!MessageLayoutsInited) {
		loadMessageLayouts();
		MessageLayoutsInited=true;
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
		MessageLayoutInfo info;
		std::string name=std::string(namestart, pos-namestart);
		pos++;
		switch(*pos) {
			case 'I': info.type=FBTCtlMessage::INT;
				pos++;
				messageLayout[name]=info;
				break;
			case 'S': info.type=FBTCtlMessage::STRING;
				pos++;
				messageLayout[name]=info;
				break;
			case 'A': info.type=FBTCtlMessage::ARRAY;
				pos++;
				if(*pos != '{') throw "kein { nach A";
				pos++;
				info.childLayout=parseMessageLayout(pos);
				messageLayout[name]=info;
				break;
			case '\0': // ende/leer
				break;
			default:
				printf("invalid id: %s\n",pos);
				throw "error parsing protocol.dat";
		}
		if(*pos == '}' || *pos == '\0') {
			break;
		}
		printf("pos2=\"%s\"\n",pos);
		if(*pos != ',') {
			throw ", erwartet";
		}
		pos++;
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
		FBTCtlMessage::DataType type=(FBTCtlMessage::DataType) (n+FBTCtlMessage::STRUCT+1);
		messageLayout.type=type;
		messageLayouts[name] = messageLayout;
	}

	fclose(f);
	dumpMessageLayouts();
}

void MessageLayout::dump(int indent) const
{
	printf("sub:%d\n", this->size());
	MessageLayout::const_iterator it;
	for(it=this->begin(); it != this->end(); ++it) {
		INDENT();
		std::string typeName;
		try {
			typeName=messageTypeName((FBTCtlMessage::DataType)it->second.type);
		} catch( const char *e) {
			typeName=e;
		}
		printf("[%s]:[%s]\n",it->first.c_str(), typeName.c_str());
		if(it->second.type >= FBTCtlMessage::ARRAY) {
			it->second.childLayout.dump(indent+1);
		}
	}
}

void dumpMessageLayouts()
{
	printf("dumpMessageLayouts\n");
	MessageLayouts::const_iterator it;
	for(it=messageLayouts.begin(); it != messageLayouts.end(); ++it) {
		printf("%s=%d:\n",it->first.c_str(),it->second.type);
		it->second.dump();

	}

}


std::string messageTypeName(FBTCtlMessage::DataType type)
{
	switch(type) {
		case FBTCtlMessage::UNDEF: throw "undefined data type cant get type for this";
		case FBTCtlMessage::INT: return "(INT)";
		case FBTCtlMessage::STRING: return "(STRING)";
		case FBTCtlMessage::ARRAY: return "(ARRAY)";
		case FBTCtlMessage::STRUCT: return "(STRUCT)";
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
				throw "invalid id";
			}
			tmp += it->first;
			tmp +=")";
			return tmp;
			break; }
	}
}

FBTCtlMessage::DataType messageTypeID(const std::string &name)
{
	FBTCtlMessage::DataType type=messageLayouts[name].type;
	if(type == FBTCtlMessage::UNDEF)
		throw "invalid message name";
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
