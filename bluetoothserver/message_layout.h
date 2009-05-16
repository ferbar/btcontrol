#ifndef MESSAGE_LAYOUT_H
#define MESSAGE_LAYOUT_H

#include <string>
#include <map>
#include <vector>
#include <string>

#define INDENT() for(int ii=0; ii < indent; ii++) printf("  ");

/**
 * speichert den aufbau einer einzigen message
 */
// class MessageLayoutInfo;
/**
 * speichert den aufbau der messages, wird aus protocol.dat geladen
 */
class MessageLayout {
public:
	enum DataType {UNDEF=0, INT=1,STRING=2,ARRAY=3,STRUCT=4};
	void dump(int indent=0) const;
	DataType type;
	std::string name;
	std::vector < MessageLayout > childLayouts;
};

// ladet die protocol.dat
void loadMessageLayouts();
// liefert das layout für eine message (type)
const MessageLayout &getMessageLayout(MessageLayout::DataType type);
std::string messageTypeName(MessageLayout::DataType type);

MessageLayout::DataType messageTypeID(const std::string &name);

void dumpMessageLayouts();

#endif
