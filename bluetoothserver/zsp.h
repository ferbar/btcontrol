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


struct SoundType {
	std::string up;
	std::string down;
	std::string run;
	int limit;
	void dump() {
		printf("up: %s, down: %s, run: %s\n", this->up != NOT_SET ? this->up.c_str() : "", this->down != NOT_SET ? this->down.c_str() : "", this->run != NOT_SET ? this->run.c_str() : "");
	}
};
extern SoundType cfg_soundFiles[10];

struct SteamSoundSlotType {
	static const int maxSlots=6;
	std::string ch[3][maxSlots]; // H M L
	int ms;
};

struct SteamSoundType {
	static const int maxStufen=5;
	int nstufen;
	int nslots; // +1 im verzeich zum zsp
	SteamSoundSlotType slots[maxStufen];
	void dump() {
		for(int i=0; i < this->nstufen; i++) {
			for(int hml=0; hml < 3; hml ++) {
				for(int j=0; j < this->nslots; j++) {
					printf("ch: %s\n", this->slots[i].ch[hml][j] != NOT_SET ? this->slots[i].ch[hml][j].c_str() : "");
				}
			}
		}
	}
};
extern SteamSoundType cfg_steamSoundFiles;


// typedef std::multimap<std::string, std::string > SectionType;
class SectionValues : public std::vector<std::pair<std::string, std::string > > {
public:
	SectionValues() {};
	//std::unordered_multimap<std::string,std::string> data;
	//SectionType data;
	// SectionValues::iterator find(std::string);
	std::string operator[] (const char * key) {
		for(SectionValues::iterator it = this->begin(); it!=this->end(); ++it) {
	        if(it->first == key) return it->second;
		}
		return NOT_SET;
		};
	void dump();
	void parseDiSet();
	void parseDSet();
	std::string getName();
};
typedef boost::shared_ptr<SectionValues> SectionValuesPtr;
typedef std::multimap<std::string, SectionValuesPtr > ZSPDataType;


#define CFG_FUNC_SOUND_ABFAHRT 0
#define CFG_FUNC_SOUND_HORN 1

extern std::string cfg_funcSound[2];

SoundType *loadZSP();

#endif
