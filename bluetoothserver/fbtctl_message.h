#ifndef FBTCTL_MESSAGE_H
#define FBTCTL_MESSAGE_H
/**
 * bastelt die protokoll pakete zusammen und zerlegts wieder
 */


#include <string>
#include <map>
#include <vector>

#include <stdlib.h>
#include <stdio.h>
#include <stdexcept>
#include <memory>

#include "message_layout.h"

/**
 * zum einlesen von binary messages
 */
class InputReader {
public:
	InputReader(const char *data, int len) : data(data), len(len), pos(0) {};
	unsigned char getByte() { 
		unsigned char ret=*((unsigned char *)(&this->data[this->pos])); this->pos+=1;
		// printf("getByte ret=%d = %#x\n",(int) ret, ret);
		if(this->pos > this->len) throw std::runtime_error("getByte übers ende gelesen");
		return ret; };
	int getInt() { 
		int ret=*((int *)(&this->data[this->pos])); this->pos+=4;
		// printf("getInt ret=%d\n",ret);
		if(this->pos > this->len) throw std::runtime_error("getInt übers ende gelesen");
		return ret; };
	std::string getString() {
		int len=this->getByte() | (this->getByte() << 8);
		if(this->pos+len > this->len) { throw std::runtime_error("getString length > end of message"); }
		std::string ret=std::string(&this->data[this->pos],len); this->pos +=len;
		// printf("getString %s\n",ret.c_str());
		return ret; };
	const char *data;
	int len;
	int pos;
};


/**
 * zum zusammenbaun/auslesen einer message
 */
class FBTCtlMessage {
public:

	FBTCtlMessage(const char *binMessage, int len);
	FBTCtlMessage(int i);
	FBTCtlMessage(const std::string &s);
	FBTCtlMessage(const char *s);
	FBTCtlMessage(MessageLayout::DataType type);

	FBTCtlMessage();
	// sollt alles kopierbar sein ... FBTCtlMessage(const FBTCtlMessage&msg);

	MessageLayout::DataType getType() const { return this->type; } ;


	int getIntVal();
	std::string getStringVal();
	int getArraySize();
	FBTCtlMessage &operator [](int i);
	FBTCtlMessage &operator [](const std::string &s);
	const FBTCtlMessage &operator [](const std::string &s) const;

	void readMessage(InputReader &in, const MessageLayout *layout=NULL);

	std::string getBinaryMessage(const MessageLayout *layout = NULL) const;

	void dump(int indent=0, const MessageLayout *layout = NULL) const;

	void clear();

	bool isType(const char *);

	size_t getSize() const;

private:
//	int seqNum; -> wird nirgendwo verwendet

	MessageLayout::DataType type;
	struct data_t {
		virtual ~data_t() = default;
	};
	struct data_int_t : data_t {
		int ival;
		data_int_t(int ival) : ival(ival) {};
	};
	struct data_string_t : data_t {
		std::string sval;
		data_string_t(const std::string &sval) : sval(sval) {};
	};
	struct data_array_t : data_t {
		typedef	std::vector<FBTCtlMessage> ArrayVal;
		ArrayVal arrayVal;
		data_array_t() {};
	};
	struct data_struct_t : data_t {
		typedef std::map<std::string,FBTCtlMessage> StructVal;
		StructVal structVal;
		data_struct_t() {};
	};

	std::shared_ptr<data_t> data;
};

// typedef std::auto_ptr<FBTCtlMessage> FBTCtlMessagePtr;


#endif
