#include <map>
#include <string>
#include <boost/shared_ptr.hpp>


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
		printf("up: %s, down: %s, run: %s\n", this->up.c_str(), this->down.c_str(), this->run.c_str());
	}
};

class SectionValues {
public:
	SectionValues() {};
	std::multimap<std::string,std::string> data;
	void dump();
};
typedef boost::shared_ptr<SectionValues> SectionValuesPtr;

typedef std::multimap<std::string, SectionValuesPtr > DataType;

extern SoundType cfg_soundFiles[10];

int loadZSP();
