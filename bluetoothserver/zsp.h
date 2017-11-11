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

class SoundType {
	public:
	SoundType() {};
	virtual void dump() {};
	virtual void loadSoundFiles() {};
};

class DiSoundStepType {
public:
	DiSoundStepType() : up(NOT_SET), down(NOT_SET), run(NOT_SET) {};
	std::string up;
	std::string down;
	std::string run;
	int limit;
	void dump() {
		printf("up: %s, down: %s, run: %s\n", this->up != NOT_SET ? this->up.c_str() : "", this->down != NOT_SET ? this->down.c_str() : "", this->run != NOT_SET ? this->run.c_str() : "");
	}
};
class DiSoundType : public SoundType {
public:
	DiSoundType() {};
	virtual ~DiSoundType() {};
	static const int maxSteps=10;
	int nsteps;
	DiSoundStepType steps[maxSteps];
	virtual void dump() {
		for(int step=0; step < this->nsteps; step++) {
			printf("Fahrstufe %d ",step);
			this->steps->dump();
		}
	};
	virtual void loadSoundFiles();
};

extern SoundType *cfg_soundFiles;

struct SteamSoundStepType {
	static const int maxSlots=6;
	std::string ch[3][maxSlots]; // H M L
	int ms;
};

class SteamSoundType : public SoundType {
public:
	SteamSoundType() {};
	virtual ~SteamSoundType() {};
	static const int maxSteps=5;
	int nsteps;
	int nslots; // +1 im verzeich zum zsp
	SteamSoundStepType steps[maxSteps];
	virtual void dump() {
		for(int step=0; step < this->nsteps; step++) {
			for(int hml=0; hml < 3; hml ++) {
				for(int slot=0; slot < this->nslots; slot++) {
					printf("ch: %s\n", this->steps[step].ch[hml][slot] != NOT_SET ? this->steps[step].ch[hml][slot].c_str() : "");
				}
			}
		}
	}
	virtual void loadSoundFiles();
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
	DiSoundType *parseDiSet();
	SteamSoundType *parseDSet();
	std::string getName();
};
typedef boost::shared_ptr<SectionValues> SectionValuesPtr;
typedef std::multimap<std::string, SectionValuesPtr > ZSPDataType;


#define CFG_FUNC_SOUND_ABFAHRT 0
#define CFG_FUNC_SOUND_HORN 1
#define CFG_FUNC_SOUND_BRAKE 2
#define CFG_FUNC_SOUND_BOIL 3

extern std::string cfg_funcSound[4];

SoundType *loadZSP();

#endif
