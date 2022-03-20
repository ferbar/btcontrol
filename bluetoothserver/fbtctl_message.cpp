/**
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

#define NODEBUG

#include <string.h>
#include <stdio.h>
#include "fbtctl_message.h"
#include "utils.h"
#include <assert.h>

#define TAG "FBTCtlMessage"

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
	this->clear();
	if(cfg_debug) {
		ERRORF("FBTCtlMessage::readMessage");
	}

	this->type=(MessageLayout::DataType) in.getByte();
	if(cfg_debug) {
		DEBUGF("datatype: %d",this->type);
	}

	if(!layout && (this->type > MessageLayout::STRUCT)) {
		layout=&getMessageLayout(this->type);
		if(cfg_debug) {
			printf("messageLayout for type: %d\n",this->type);
			layout->dump();
		}
	}

	if(!layout)
		throw std::runtime_error("no message layout");

	std::vector<MessageLayout>::const_iterator it;
	for(it=layout->childLayouts.begin(); it != layout->childLayouts.end(); ++it) {
		MessageLayout::DataType type=(MessageLayout::DataType)in.getByte();
		if(type != it->type) {
			ERRORF("unexpected type! %d, %d (%s)\n",type, it->type, messageTypeName(it->type).c_str());
			throw std::runtime_error("unexpected DataType");
		}
		switch(it->type){
			case MessageLayout::INT:
				this->operator[](it->name) = FBTCtlMessage(in.getInt());
				DEBUGF("read int message size: %zd", this->operator[](it->name).getSize());
				break;
			case MessageLayout::STRING:
				this->operator[](it->name) = FBTCtlMessage(in.getString());
				DEBUGF("read string message size: %zd", this->operator[](it->name).getSize());
				break;
			case MessageLayout::ARRAY: {
				int n=in.getByte();
				DEBUGF("read arraysize:%d",n);
				FBTCtlMessage tmparray(MessageLayout::ARRAY);
				for(int i=0; i < n; i++) {
					FBTCtlMessage tmp;
					tmp.readMessage(in,&*it);
					tmparray[i]=tmp;
					// PRINT_FREE_HEAP("readmessage");
				}
				this->operator[](it->name) = tmparray;
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
	this->data=std::make_shared<data_int_t>(i);
}

FBTCtlMessage::FBTCtlMessage(const std::string &s)
{
	this->type=MessageLayout::STRING;
	this->data=std::make_shared<data_string_t>(s);
}

FBTCtlMessage::FBTCtlMessage(const char *s)
{
	this->type=MessageLayout::STRING;
	this->data=std::make_shared<data_string_t>(s);
}

FBTCtlMessage::FBTCtlMessage() : type(MessageLayout::UNDEF)
{


}

FBTCtlMessage::FBTCtlMessage(MessageLayout::DataType type) : type(type)
{
	if(type == MessageLayout::ARRAY) {
		this->data=std::make_shared<data_array_t>();
	} else if(type >= MessageLayout::STRUCT) {
		this->data=std::make_shared<data_struct_t>();
	} else {
		ERRORF("invalid init type %d - use specialized constructor", type);
		abort();
	}
}

void FBTCtlMessage::clear()
{
	this->type=MessageLayout::UNDEF;
	this->data=NULL;
}

int FBTCtlMessage::getIntVal()
{
	if(type != MessageLayout::INT) throw std::runtime_error("invalid type(int)");
	data_int_t &d=static_cast<data_int_t &>(*data);
	return d.ival;
}

std::string FBTCtlMessage::getStringVal()
{
	if(this->type != MessageLayout::STRING) throw std::runtime_error("invalid type(string)");
	data_string_t &d=static_cast<data_string_t &>(*data);
	return d.sval;
}

int FBTCtlMessage::getArraySize()
{
	if(this->type != MessageLayout::ARRAY) throw std::runtime_error("invalid type(no array)");
	data_array_t &d=static_cast<data_array_t &>(*data);
	return d.arrayVal.size();
}

FBTCtlMessage &FBTCtlMessage::operator [](int i)
{
	// DEBUGF("[%d]", i);
	if(this->type == MessageLayout::UNDEF) {
		DEBUGF("init to array");
		this->type=MessageLayout::ARRAY;
		this->data=std::make_shared<data_array_t>();
	}
	if(this->type != MessageLayout::ARRAY) throw std::runtime_error(utils::format("invalid type=%d(no array)", type));
	data_array_t *d=static_cast<data_array_t *>(&*data);
	assert(d);
	if((signed)d->arrayVal.size() <= i) {
		d->arrayVal.push_back(FBTCtlMessage(MessageLayout::STRUCT));
	}
	return d->arrayVal[i];
}

FBTCtlMessage &FBTCtlMessage::operator [](const std::string &s)
{
	// DEBUGF("[%s]", s.c_str());
	if(this->type < MessageLayout::STRUCT) throw std::runtime_error(utils::format("invalid type=%d (no struct)",this->type));
	data_struct_t *d=static_cast<data_struct_t *>(&*data);
	if(!d) {
		DEBUGF("FBTCtlMessage::operator [string] init");
		data=std::make_shared<data_struct_t>();
		d=static_cast<data_struct_t *>(&*data);
	}
	return d->structVal[s];
}

const FBTCtlMessage &FBTCtlMessage::operator [](const std::string &s) const
{
	// DEBUGF("[%s]", s.c_str());
	if(this->type < MessageLayout::STRUCT) throw std::runtime_error(utils::format("invalid type=%d (no struct)",this->type));
	data_struct_t *d=static_cast<data_struct_t *>(&*data);
	if(!d) {
		throw std::runtime_error("operator[] const, structVal not initialized");
	}
	return d->structVal[s];
}

std::string FBTCtlMessage::getBinaryMessage(const MessageLayout *layout) const
{
	std::string ret;
	if(!layout)
		layout=&getMessageLayout(this->type);

	if(!layout)
		throw std::runtime_error("no message layout");
	
	ret += std::string((char *) &this->type, 1);
	switch(this->type) {
		case MessageLayout::INT: {
			data_int_t &d=static_cast<data_int_t &>(*data);
			// The Straight Answer: ESP32 is Little Endian
			// unsigned char data[4]={ival & 0xff, (ival << 8) & 0xff, (ival << 16) & 0xff, ival << 24 };
			ret += std::string((char *) &d.ival, 4);
			break; }
		case MessageLayout::STRING: {
			data_string_t &d=static_cast<data_string_t &>(*data);
			int tmp=d.sval.size();
			if(tmp > 0x10000) {
				throw std::runtime_error("string too big");
			}
			ret += std::string((char *) &tmp, 2);
			ret += d.sval;
			break; }
		case MessageLayout::ARRAY: {
			data_array_t &d=static_cast<data_array_t &>(*data);
			int tmp=d.arrayVal.size();
			ret += std::string((char *) &tmp, 1);
			/*
			printf("arraysize:%d string.size=%d:<",tmp,ret.size());
			fwrite(ret.data(),1,ret.size(),stdout);
			printf(">\n");
			*/
			for(size_t i=0; i < d.arrayVal.size(); i++) {
				ret += d.arrayVal[i].getBinaryMessage(layout);
			}
			break; }
		default: { // struct + message type
			data_struct_t &d=static_cast<data_struct_t &>(*data);
			std::vector<MessageLayout>::const_iterator it;
			for(it=layout->childLayouts.begin(); it != layout->childLayouts.end(); ++it) {
				auto element=d.structVal.find(it->name);
				if(element != d.structVal.end()) {
					if(element->second.type == it->type) {
						ret += element->second.getBinaryMessage(&*it);
					} else {
						throw std::runtime_error("invalid type (try dump before)");
					}
				} else {
					throw std::runtime_error(std::string("struct \"")+messageTypeName(this->type)+ " value \""+it->name+"\" - field not set");
				}
			}
			break; }
	}
	return ret;
		
}

void FBTCtlMessage::dump(int indent, const MessageLayout *layout) const
{
	
	if(!layout) {
		try {
			layout=&getMessageLayout(this->type);
		} catch(std::exception &e) {
			printf("unable to get layout for %d\n", this->type);
		}
	}

	// layout->dump();
	if(indent == 0) {
		printf("dumpMessage\n");
	}
	INDENT();
	printf(/* "seqnum: %d, */ "type:%d=%s size=%zu ", /* this->seqNum,*/ this->type,  messageTypeName(this->type).c_str(),
		this->getSize() );
	switch(this->type) {
		case MessageLayout::UNDEF: {
			printf("undefined\n");
			break; }
		case MessageLayout::INT: {
			data_int_t &d=static_cast<data_int_t &>(*data);
			// INDENT();
			// printf("(INT) %d\n",this->ival); break;
			printf("value=%d\n",d.ival); break; }
		case MessageLayout::STRING: {
			data_string_t &d=static_cast<data_string_t &>(*data);
			// INDENT();
			// printf("(STRING) %s\n",this->sval.c_str()); break;
			printf("len=%d value=%s\n", (int) d.sval.length(), d.sval.c_str()); break; }
		case MessageLayout::ARRAY: {
			data_array_t &d=static_cast<data_array_t &>(*data);
			// INDENT(); 
			// printf("(ARRAY)\n");
			printf("\n");
			for(size_t i=0; i < d.arrayVal.size(); i++) {
				INDENT();
				printf("[%zu]:\n",i);
				d.arrayVal[i].dump(indent+1,layout);
			}
			break; }
		default: { // struct + message type
			data_struct_t *d=static_cast<data_struct_t *>(&*data);
			// INDENT();
			// printf("(STRUCT)\n");
			printf("\n");
			std::map<std::string,bool> validEntries;
			if(layout) {
				for(auto it=layout->childLayouts.begin(); it != layout->childLayouts.end(); ++it) {
					INDENT();
					printf("  [%s]:\n",it->name.c_str());
					if(!d || d->structVal.count(it->name) == 0) { // element gibts nicht
						INDENT()
						printf("  NULL\n");
					} else {
						// map::operator[] geht anscheinend nicht mit const keys grrr ...
						const FBTCtlMessage &sub=this->operator[](it->name);
						if(sub.type == it->type) {
							sub.dump(indent+2, &*it);
							validEntries[it->name] = true;
						} else {
							try {
								std::string typeName=messageTypeName(sub.type);
								printf("type mismatch (expected %s, is: %s)\n",messageTypeName(it->type).c_str(), typeName.c_str());
							} catch(const char *e) {
								printf("exception: %s ",e);
								printf("invalid field: %s\n",it->name.c_str());
								layout->dump();
							}
						}
					}
					
				}
			} else {
				printf("struct -- no message layout");
			}
			if(d) {
				// std::map<std::string,FBTCtlMessage>::const_iterator itCheck;
				for(auto itCheck=d->structVal.begin(); itCheck != d->structVal.end(); ++itCheck) {
					if(!validEntries[itCheck->first]) {
						printf("**** unused extra field: \"%s\"\n",itCheck->first.c_str());
					}
				}
			} else {
				printf("**** NULL\n");
			}
			break; }
	}
		
	if(indent == 0) {
		printf("dumpMessage done \n");
	}
}

size_t FBTCtlMessage::getSize() const
{
	size_t ret=sizeof(FBTCtlMessage);
	// string
	switch(this->type) {
		case MessageLayout::UNDEF: {
			ret+=0;
			break; }
		case MessageLayout::INT: {
			// data_int_t &d=static_cast<data_int_t &>(*data);
			ret+=sizeof(data_int_t);
			break; }
		case MessageLayout::STRING: {
			data_string_t &d=static_cast<data_string_t &>(*data);
			ret+=sizeof(data_string_t) + d.sval.capacity();
			break; }
		case MessageLayout::ARRAY: {
			data_array_t &d=static_cast<data_array_t &>(*data);
			ret+=sizeof(std::vector<FBTCtlMessage>);
			for(auto &it : d.arrayVal)
				ret+=it.getSize();
			break; }
		default: { // STRUCT etc
			data_struct_t *d=static_cast<data_struct_t *>(&*data);
			if(d) {
				ret+=sizeof(data_struct_t) + (sizeof(std::string)) * d->structVal.size();
				  // das wird unten im getSize mitgerechnet+ (sizeof(FBTCtlMessage)) * structVal.size();
				for(auto &it : d->structVal)
					ret+=it.first.capacity() + it.second.getSize();
			}
			break; }
	}
	return ret;
}

bool FBTCtlMessage::isType(const char *name)
{
	if(this->type == messageTypeID(name)) {
		return true;
	} else {
		return false;
	}
}
