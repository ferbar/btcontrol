/**
 * bastelt die protokoll packete zusammen und zerlegts wieder
 */


#include <string>
#include <map>
#include <vector>

#include <stdlib.h>

/**
 * speichert den aufbau einer einzigen message
 */
class MessageLayoutInfo;
/**
 * speichert den aufbau der messages, wird aus protocol.dat geladen
 */
class MessageLayout : public std::map<std::string , MessageLayoutInfo > {
public:
	void dump(int indent=0) const;
};

// TODO: das da ins MessageLayout tun, dann kamma auch die MessageLayouts typedef killen ***********
class MessageLayoutInfo{
public:
	int type;
	MessageLayout childLayout;
};

/**
 * zum einlesen von binary messages
 */
class InputReader {
public:
	InputReader(const char *data, int len) : data(data), len(len), pos(0) {};
	unsigned char getByte() { 
		unsigned char ret=*((unsigned char *)(&this->data[this->pos])); this->pos+=1;
		printf("getByte ret=%d = %#x\n",(int) ret, ret);
		if(this->pos > this->len) throw "getInt übers ende gelesen";
		return ret; };
	int getInt() { 
		int ret=*((int *)(&this->data[this->pos])); this->pos+=4;
		printf("getInt ret=%d\n",ret);
		if(this->pos > this->len) throw "getInt übers ende gelesen";
		return ret; };
	std::string getString() {
		unsigned char len=this->getByte();
		std::string ret=std::string(&this->data[this->pos],len); this->pos +=len;
		printf("getString %s\n",ret.c_str());
		if(this->pos > this->len) throw "getString übers ende gelesen"; return ret; };
	const char *data;
	int len;
	int pos;
};


/**
 * zum zusammenbaun/auslesen einer message
 */
class FBTCtlMessage {
public:
	enum DataType {UNDEF=0, INT=1,STRING=2,ARRAY=3,STRUCT=4};

	FBTCtlMessage(const char *binMessage, int len);
	FBTCtlMessage(int i);
	FBTCtlMessage(const std::string &s);
	FBTCtlMessage(const char *s);
	FBTCtlMessage(DataType type);

	FBTCtlMessage();
	// sollt alles kopierbar sein ... FBTCtlMessage(const FBTCtlMessage&msg);


	int getIntVal();
	std::string getStringVal();
	FBTCtlMessage &operator [](int i);
	FBTCtlMessage &operator [](const std::string &s);

	void readMessage(InputReader &in, const MessageLayout *layout=NULL);

	std::string getBinaryMessage(const MessageLayout *layout = NULL);

	void dump(int indent=0, const MessageLayout *layout = NULL) const;

private:
	int seqNum;

	std::string name;

	DataType type;
	int ival;
	std::string sval;
	std::vector<FBTCtlMessage> arrayVal;
	std::map<std::string,FBTCtlMessage> structVal;

};

// typedef std::auto_ptr<FBTCtlMessage> FBTCtlMessagePtr;


// ladet die protocol.dat
void loadMessageLayouts();
// liefert das layout für eine message (type)
const MessageLayout &getMessageLayout(FBTCtlMessage::DataType type);
std::string messageTypeName(FBTCtlMessage::DataType type);

FBTCtlMessage::DataType messageTypeID(const std::string &name);

void dumpMessageLayouts();
