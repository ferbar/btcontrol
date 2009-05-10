/**
 * bastelt die protokoll packete zusammen und zerlegts wieder
 */


#include <string>
#include <map>
#include <vector>

#include <stdlib.h>

class MessageLayoutInfo;
typedef std::map<std::string , MessageLayoutInfo > MessageLayout;
class MessageLayoutInfo{
public:
	int type;
	MessageLayout childLayout;
};

class InputReader {
public:
	InputReader(const char *data, int len) : data(data), len(len), pos(0) {};
	int getInt() { int ret=atoi(&data[pos]); pos+=4; if(pos > len) throw "getInt übers ende gelesen";  return ret; };
	std::string getString() {
		unsigned char len=*((unsigned char*)data);
		std::string ret=std::string(&data[1],len); pos +=1+len;
		if(pos > len) throw "getString übers ende gelesen"; return ret; };
	const char *data;
	int len;
	int pos;
};


class FBTCtlMessage {
public:
	enum DataType {UNDEF=0, INT=1,STRING=2,ARRAY=3,STRUCT=4};

	FBTCtlMessage(const char *binMessage, int len);
	FBTCtlMessage(int i);
	FBTCtlMessage(const std::string &s);
	FBTCtlMessage(DataType type);

	FBTCtlMessage();
	// sollt alles kopierbar sein ... FBTCtlMessage(const FBTCtlMessage&msg);


	int getIntVal();
	std::string getStringVal();
	FBTCtlMessage &operator [](int i);
	FBTCtlMessage &operator [](const std::string &s);

	void readMessage(InputReader &in);
	void readMessage(InputReader &in, const MessageLayout &layout);

	std::string getBinaryMessage(const MessageLayout *layout = NULL);

	void dump(int indent=0, const MessageLayout *layout = NULL);

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



void loadMessageLayouts();
const MessageLayout &getMessageLayout(FBTCtlMessage::DataType type);
std::string messageTypeName(FBTCtlMessage::DataType type);

FBTCtlMessage::DataType messageTypeID(const std::string &name);

