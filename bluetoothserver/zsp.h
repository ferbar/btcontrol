#ifndef ZSP_H
#define ZSP_H
#include <map>
#include <unordered_map>
#include <string>
#if __cplusplus >= 201103L
#include <memory>
#else
#include <boost/shared_ptr.hpp>
#endif
#include "utils.h"
#include <vector>

// zpr => ABL
#define CFG_FUNC_SOUND_BOIL 0

#define CFG_FUNC_SOUND_BRAKE 2
#define CFG_FUNC_SOUND_ENTWAESSERN 3
#define CFG_FUNC_SOUND_ABFAHRT 4
#define CFG_FUNC_SOUND_HORN 5
#define CFG_FUNC_SOUND_EMOTOR 6
#define CFG_FUNC_SOUND_N 18
extern const char* ablNames[CFG_FUNC_SOUND_N];

extern const char *overrideConfigNames[];

#define STEAM_SLOT_NORMAL 0
#define STEAM_SLOT_BRAKE 1
#define STEAM_SLOT_ACC 2

class ZSP {
public:
	ZSP(const char file){};
	~ZSP();
	void getDiSet(int number){};
};

class Sample {
public:
	Sample() {};
	Sample(std::string fileName, int loopStartPos, int loopEndPos) : fileName(fileName), loopStartPos(loopStartPos), loopEndPos(loopEndPos) {};
	operator bool() const { return this->fileName != NOT_SET; };
	operator std::string() const = delete;
	void load(int volumeLevel);

	const std::string loopStart() { if( this->wav != NOT_SET ) return this->wav.substr(0, this->loopStartPos); throw std::runtime_error("file " + this->fileName + " not loaded"); }
	const std::string loop() { if( this->wav != NOT_SET ) return this->wav.substr(this->loopStartPos, this->loopEndPos - this->loopStartPos); throw std::runtime_error("file " + this->wav + " not loaded"); }
	const std::string loopEnd() { if( this->wav != NOT_SET ) return this->wav.substr(this->loopEndPos, this->wav.length() - this->loopEndPos); throw std::runtime_error("file not loaded"); }

	std::string fileName{NOT_SET};
	std::string wav{NOT_SET};
	int loopStartPos{0};
	int loopEndPos{0};
};

Sample getSample(std::string number);

class SoundType {
public:
	// SoundType() { for(int i =0; i < CFG_FUNC_SOUND_N; i++) { this->funcSound[i]=NOT_SET; this->funcSoundVolume[i]=0; }};
	SoundType() {};
	virtual void dump() {};
	virtual void loadSoundFiles() {};
	// ID vom soundset
	int lok;
	Sample funcSound[CFG_FUNC_SOUND_N];
	int funcSoundVolume[CFG_FUNC_SOUND_N];
};

class DiSoundStepType {
public:
	DiSoundStepType() : limit(255) {};
	Sample up;
	Sample down;
	Sample run;
	int limit; // max speed für diese Stufe, wenn darüber => nächste stufe
	void dump() {
		printf("up: %s, down: %s, run: %s, limit: %d\n",
			this->up ? this->up.fileName.c_str() : "",
			this->down ? this->down.fileName.c_str() : "",
			this->run ? this->run.fileName.c_str() : "", this->limit);
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
			this->steps[step].dump();
		}
	};
	virtual void loadSoundFiles();
};

extern SoundType *cfg_soundFiles;

struct SteamSoundStepType {
	static const int maxSlots=6;
	Sample ch[3][maxSlots]; // H M L
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
					printf("ch: %s\n", this->steps[step].ch[hml][slot] ? this->steps[step].ch[hml][slot].fileName.c_str() : "");
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
#if __cplusplus >= 201103L
typedef std::shared_ptr<SectionValues> SectionValuesPtr;
#else
typedef boost::shared_ptr<SectionValues> SectionValuesPtr;
#endif
typedef std::multimap<std::string, SectionValuesPtr > ZSPDataType;



SoundType *loadZSP();

#endif
