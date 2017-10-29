#ifndef ZSP_H
#define ZSP_H
#include <map>
#include <unordered_map>
#include <string>
#include <boost/shared_ptr.hpp>
#include "utils.h"


class ZSP {
public:
	ZSP(const char file){};
	~ZSP();
	void getDiSet(int number){};
};

std::string getSampleFilename(std::string number);

typedef std::multimap<std::string, std::string > SectionType;

struct SoundType {
	std::string up;
	std::string down;
	std::string run;
	int limit;
	void dump() {
		printf("up: %s, down: %s, run: %s\n", this->up != NOT_SET ? this->up.c_str() : "", this->down != NOT_SET ? this->down.c_str() : "", this->run != NOT_SET ? this->run.c_str() : "");
	}
};

struct SteamSoundType {
	std::string ch[4];
	void dump() {
		for(unsigned int i=0; i < sizeof(ch); i++) {
			printf("ch: %s\n", this->ch[i] != NOT_SET ? this->ch[i].c_str() : "");
		}
	}
};

class SectionValues {
public:
	SectionValues() {};
	//std::unordered_multimap<std::string,std::string> data;
	SectionType data;
	void dump();
	void parseDiSet();
	void parseDSet();
	std::string getName();
};
typedef boost::shared_ptr<SectionValues> SectionValuesPtr;

typedef std::multimap<std::string, SectionValuesPtr > DataType;

extern SoundType cfg_soundFiles[10];
extern SteamSoundType cfg_steamSoundFiles;

#define CFG_FUNC_SOUND_ABFAHRT 0
#define CFG_FUNC_SOUND_HORN 1

extern std::string cfg_funcSound[2];

SoundType *loadZSP();

#endif
