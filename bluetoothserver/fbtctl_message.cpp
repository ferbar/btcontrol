/**
 *  This file is part of btcontrol
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
 *
 *
 * bastelt die protokoll packete zusammen und zerlegts wieder
 *
 * message:
 *			int len
 *			byte msgtype
 *			daten ...
 *
 * daten:	int: einfach intval
 *			string: 2byte len, daten
 *			array: byte anzahl
 * 
 *
 * protocol mit loadMessageLayouts() laden
 *
 * message parsen mit FBTCtlMessage(char *, int len);
 * message zusammenbaun mit:
 * FBTCtlMessage f(msgtype);
 * f["info"][0]["addr"]=1
 * f["info"][0]["speed"]=50
 * f["info"][0]["functions"]=50
 */

#include <string.h>
#include <stdio.h>
#include "fbtctl_message.h"
#include "utils.h"

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
	clear();
	if(cfg_debug) {
		printf("reading bin msg\n");
	}

	this->type=(MessageLayout::DataType) in.getByte();
	if(cfg_debug) {
		printf("datatype: %d\n",this->type);
	}

	if(!layout && (this->type > MessageLayout::STRUCT)) {
		layout=&getMessageLayout(this->type);
		if(cfg_debug) {
			printf("messageLayout for type: %d\n",this->type);
			layout->dump();
		}
	}

	if(!layout)
		throw "no message layout";

	std::vector<MessageLayout>::const_iterator it;
	for(it=layout->childLayouts.begin(); it != layout->childLayouts.end(); ++it) {
		MessageLayout::DataType type=(MessageLayout::DataType)in.getByte();
		if(type != it->type) {
			printf("unexpected type! %d, %d (%s)\n",type, it->type, messageTypeName(it->type).c_str());
			throw "unexpected DataType";
		}
		switch(it->type){
			case MessageLayout::INT:
				printf("read int\n");
				this->structVal[it->name] = FBTCtlMessage(in.getInt()); break;
			case MessageLayout::STRING:
				printf("read string\n");
				this->structVal[it->name] = FBTCtlMessage(in.getString()); break;
			case MessageLayout::ARRAY: {
				int n=in.getByte();
				printf("read arraysize:%d\n",n);
				FBTCtlMessage tmparray(MessageLayout::ARRAY);
				for(int i=0; i < n; i++) {
					FBTCtlMessage tmp;
					tmp.readMessage(in,&*it);
					tmparray[i]=tmp;
				}
				this->structVal[it->name] = tmparray;
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
				printf("invalid type (%d)\n",it->type);
				break; }
		}
	}

}

FBTCtlMessage::FBTCtlMessage(int i)
{
	this->type=MessageLayout::INT;
	this->ival=i;
}

FBTCtlMessage::FBTCtlMessage(const std::string &s)
{
	this->type=MessageLayout::STRING;
	this->sval=s;
}

FBTCtlMessage::FBTCtlMessage(const char *s)
{
	this->type=MessageLayout::STRING;
	this->sval=std::string(s);
}

FBTCtlMessage::FBTCtlMessage() : type(MessageLayout::UNDEF)
{


}

FBTCtlMessage::FBTCtlMessage(MessageLayout::DataType type) : type(type)
{


}

void FBTCtlMessage::clear()
{
	this->type=MessageLayout::UNDEF;
	this->ival=0;
	this->sval="";
	this->structVal.clear();
	this->arrayVal.clear();
}

int FBTCtlMessage::getIntVal()
{
	if(type != MessageLayout::INT) throw "invalid type(int)";
	return ival;
}

std::string FBTCtlMessage::getStringVal()
{
	if(this->type != MessageLayout::STRING) throw "invalid type(string)";
	return sval;
}

int FBTCtlMessage::getArraySize()
{
	return arrayVal.size();
}

FBTCtlMessage &FBTCtlMessage::operator [](int i)
{
	if(this->type == MessageLayout::UNDEF) this->type=MessageLayout::ARRAY;
	if(this->type != MessageLayout::ARRAY) throw "invalid type(array)";
	if((signed)arrayVal.size() <= i) {
		arrayVal.push_back(FBTCtlMessage(MessageLayout::STRUCT));
	}
	return arrayVal[i];
}

FBTCtlMessage &FBTCtlMessage::operator [](const std::string &s)
{
	if(this->type < MessageLayout::STRUCT) throw "invalid type(stuct)";
	return structVal[s];
}

std::string FBTCtlMessage::getBinaryMessage(const MessageLayout *layout) const
{
	std::string ret;
	if(!layout)
		layout=&getMessageLayout(this->type);

	if(!layout)
		throw "no message layout";
	
	ret += std::string((char *) &this->type, 1);
	switch(this->type) {
		case MessageLayout::INT:
			ret += std::string((char *) &this->ival, 4);
			break;
		case MessageLayout::STRING: {
			int tmp=this->sval.size();
			ret += std::string((char *) &tmp, 2);
			ret += this->sval;
			break; }
		case MessageLayout::ARRAY: {
			int tmp=this->arrayVal.size();
			ret += std::string((char *) &tmp, 1);
			/*
			printf("arraysize:%d string.size=%d:<",tmp,ret.size());
			fwrite(ret.data(),1,ret.size(),stdout);
			printf(">\n");
			*/
			for(size_t i=0; i < arrayVal.size(); i++) {
				ret += arrayVal[i].getBinaryMessage(layout);
			}
			break; }
		default: { // struct + message type
			std::vector<MessageLayout>::const_iterator it;
			for(it=layout->childLayouts.begin(); it != layout->childLayouts.end(); ++it) {
				StructVal::const_iterator element=this->structVal.find(it->name);
				if(element != this->structVal.end()) {
					if(element->second.type == it->type) {
						ret += element->second.getBinaryMessage(&*it);
					} else {
						throw "invalid type (try dump before)";
					}
				} else {
					throw std::string("struct \"")+messageTypeName(this->type)+ " value \""+it->name+"\" - field not set";
				}
			}
			break; }
	}
	return ret;
		
}

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
		case MessageLayout::INT:
			INDENT();
			printf("(INT) %d\n",this->ival); break;
		case MessageLayout::STRING:
			INDENT();
			printf("(STRING) %s\n",this->sval.c_str()); break;
		case MessageLayout::ARRAY: {
			// INDENT(); printf("array\n");
			for(size_t i=0; i < arrayVal.size(); i++) {
				INDENT();
				printf("[%zu]:\n",i);
				arrayVal[i].dump(indent+1,layout);
			}
			break; }
		default: { // struct + message type
			// INDENT(); printf("struct\n");
			std::vector<MessageLayout>::const_iterator it;
			std::map<std::string,bool> validEntries;
			for(it=layout->childLayouts.begin(); it != layout->childLayouts.end(); ++it) {
				INDENT();
				printf("[%s]:\n",it->name.c_str());
				if(this->structVal.count(it->name) == 0) { // element gibts nicht
					printf("NULL\n");
				} else {
					// map::operator[] geht anscheinend nicht mit const keys grrr ...
					FBTCtlMessage *sub=&(const_cast<FBTCtlMessage*>(this))->structVal[it->name];
					if(sub->type == it->type) {
						sub->dump(indent+1, &*it);
						validEntries[it->name] = true;
					} else {
						try {
							std::string typeName=messageTypeName(sub->type);
							printf("type mismatch (expected %s, is: %s)\n",messageTypeName(it->type).c_str(), typeName.c_str());
						} catch(const char *e) {
							printf("exception: %s ",e);
							printf("invalid field: %s\n",it->name.c_str());
							layout->dump();
						}
					}
				}
				
			}
			std::map<std::string,FBTCtlMessage>::const_iterator itCheck;
			for(itCheck=this->structVal.begin(); itCheck != this->structVal.end(); ++itCheck) {
				if(!validEntries[itCheck->first]) {
					printf("**** invalid field %s\n",itCheck->first.c_str());
				}
			}
			break; }
	}
		
	if(indent == 0) {
		printf("dumpMessage done \n");
	}
}

bool FBTCtlMessage::isType(const char *name)
{
	if(this->type == messageTypeID(name)) {
		return true;
	} else {
		return false;
	}
}
