/**
 * mit -std=c++11 kompilieren !
 */
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "unquote.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "sound.h"
#include "utils.h"

SoundType cfg_soundFiles[10];
std::string cfg_funcSound[2];
SteamSoundType cfg_steamSoundFiles;

ZSPDataType ZSPData;

std::string soundsetPath;

/**
 * sucht nach dem dateinamen für sample NUMBER=1234
 * @param number
 * @return filename
 */
std::string getSampleFilename(std::string number) {
	// printf(" ---- %s \n",number.c_str());
	for(ZSPDataType::iterator it = ZSPData.find("Sample"); it!=ZSPData.end(); it++) {
		// it->second->dump();
		// printf("sampe\n");
		SectionValuesPtr sp = it->second;
		std::string sampleNumber = sp->operator[]("NUMMER");
			// printf("   sample -- number: %s\n", spIt->second.c_str());
			if(sampleNumber == number) {
				// printf(" ----------------- sample --------------\n");
				// sp->dump();
				std::string tmp = sp->operator[]("PFAD");
				std::string pfad=boost::algorithm::unquote(tmp,'"','\b');
				// printf(" %s/", pfad.c_str());
				tmp = sp->operator[]("NAME");
				std::string name=boost::algorithm::unquote(tmp,'"','\b');
				// printf(" %s\n", name.c_str());
				printf("getSampleFilename(%s) = '%s/%s'\n",number.c_str(), pfad.c_str(),name.c_str());
				return soundsetPath+"/"+boost::replace_all_copy(pfad, "\\", "/") + name;
			}
	}
	throw std::runtime_error("invalid sample number ("+number+")");
}


SoundType *loadZSP() {
	// std::multimap< int,std::string > map_data;
	// map_data.emplace(5,"sdfsdf");
	// map_data[7]["hello"] = 3.1415926;

	for(int i=0; i < 10; i++) {
		cfg_soundFiles[i].up=NOT_SET;
		cfg_soundFiles[i].run=NOT_SET;
		cfg_soundFiles[i].down=NOT_SET;
	}

	printf("zimo sound projekt test\n");
	std::string soundsetFile;
	soundsetFile=config.get("soundset");
	if(soundsetFile == NOT_SET) {
		printf("no soundset configured ... disabling sound\n");
		return NULL;
	}
	size_t slash=soundsetFile.find_last_of('/');
	if(slash != std::string::npos) {
		soundsetPath=soundsetFile.substr(0,slash);
	}
	FILE *f=fopen(soundsetFile.c_str(), "r");
	if(!f) {
		fprintf(stderr,"error loading sound fileset [%s]\n", soundsetFile.c_str());
		abort();
	}
	char buffer[1024];
	std::string section="";
	SectionValuesPtr sp;
	while(fgets(buffer, sizeof(buffer), f)) {
		std::string line=buffer;
		boost::algorithm::trim(line);
		if(boost::starts_with(line,"\"/")) {
			// printf("end section\n");
			ZSPData.insert(std::pair<std::string, SectionValuesPtr>(section,sp) );
			section="";
			continue;
		}
		if(section == "" ) {
			line=boost::algorithm::unquote(line);
			if(line == "") {
				continue;
			}
			sp.reset(new SectionValues()); // macht der ein auto free?
			printf("new section:%s\n",line.c_str());
			section=line;
			continue;
		}

		std::string subsection, value;
		size_t komma=line.find_first_of(',');
		if(komma != std::string::npos) {
			subsection=line.substr(0,komma);
			subsection=boost::algorithm::unquote(subsection);
			value=line.substr(komma+1);
		} else {
			subsection=line;
			value="";
		}
		if(sp == NULL) {
			printf("sp == null skipping\n");
			continue;
		}
		// printf("add %s=%s\n", subsection.c_str(), value.c_str());
		sp->push_back(std::pair<std::string, std::string>(subsection, value) );

		continue;
		/*
		try {    
			if (boost::starts_with(argv[1], "--foo="))
			int foo_value = boost::lexical_cast<int>(argv[1]+6);
		} catch (boost::bad_lexical_cast) {
			// bad parameter
		}
//		data[ */
	}
	bool found=false;
	std::string soundSetName=config.get("sound.name");
	if(soundSetName == NOT_SET) {
		printf("sound.name not set\n");
		abort();
	}
	auto diSets=ZSPData.equal_range("DiSet");
	for(ZSPDataType::iterator it=diSets.first; it!=diSets.second; ++it) {
		//it->second->dump();
		std::string setName=it->second->getName();
		printf("DiSet SetName: %s\n", setName.c_str());
		if(setName == soundSetName) {
			printf("searching SCHWELLE, SAMPLE @ DiSet\n");
			it->second->parseDiSet();
			found=true;
		} else
			printf("no match\n");
	}
	if(found) {
		for(int i=0; i < 10; i++) {
			printf("Fahrstufe %d ",i);
			cfg_soundFiles[i].dump();
		}
	} else {
		diSets=ZSPData.equal_range("DSet");
		for(ZSPDataType::iterator it = diSets.first; it!=diSets.second; ++it) {
			std::string setName=it->second->getName();
			printf("DSet SetName: %s\n", setName.c_str());
			if(setName == soundSetName) {
				printf("searching SAMPLE @ DSet\n");
				it->second->parseDSet();
				found=true;
			}
		}
	}
	if(!found) {
		printf("Error: D*Set Name %s not found\n", soundSetName.c_str());
		abort();
	}

	printf("searching Function sounds\n");
	auto range = ZSPData.equal_range("Func");
	for(ZSPDataType::iterator it = range.first; it!=range.second; ++it) {
		printf("############### [%s]\n",it->first.c_str());
		it->second->dump();
 		std::string tmp=it->second->operator[]("SAMPLE");
		printf("  SAMPLE=%s\n",tmp.c_str());
		std::string fileName=getSampleFilename(tmp);
		printf("  filename=%s\n", fileName.c_str());
	}

	/*
	Sound s(cfg_soundFiles);
	s.init();
	s.run();
	sleep(1);
	s.setFahrstufe(0);
	sleep(2);
	s.setFahrstufe(1);
	sleep(13);
	s.setFahrstufe(2);
	sleep(10);
	s.setFahrstufe(0);
	sleep(10);
	s.setFahrstufe(-1);
	sleep(15);
	*/

	/*
	die Func sounds sind bei jedem Soundset bei einer anderen Fx Taste, daher verwenden wirs nicht
	"Func"
		"LOK",32
		"NUMMER",2
		"SAMPLE",16
		"LAUTST",0
		"LOOP",0
		"SHORT",0
	"/Func"
	"Sample"
		"NUMMER",16
		"PFAD",""
		"NAME","Hupe_kurz.wav"
		"ART",12
		"SIZE",18128
		"L1",0
		"L2",18127
		"SR",2
		"INFO","Pfeife kurz"
		"LOOP",0
		"SHORT",0
		"FNR",34
	"/Sample"
	*/
	try {
		cfg_funcSound[CFG_FUNC_SOUND_HORN] = config.get("sound.horn");
	} catch(std::exception &e) {
		printf("unable to get config/sound.horn\n");
	}
	try {
		cfg_funcSound[CFG_FUNC_SOUND_ABFAHRT] = config.get("sound.abfahrt");
	} catch(std::exception &e) {
		printf("unable to get config/sound.abfahrt\n");
	}
	return cfg_soundFiles;
}

void SectionValues::dump() {
	for(SectionValues::const_iterator it=this->begin(); it!=this->end(); it++) {
		printf(" -- '%s' '%s'\n", it->first.c_str(), it->second.c_str());
	}
}

std::string SectionValues::getName() {
	std::string ret=this->operator[]("NAME");
	if(ret != NOT_SET ) {
		return boost::algorithm::unquote(ret,'"','\b');
	} else 
		return NOT_SET;
}

/**
 * FIXME: das tut globale variablen setzen!!!!!!
 */
void SectionValues::parseDiSet() {
	printf("SectionValues::parseDiSet()\n");
	for(SectionValues::const_iterator it=this->begin(); it!=this->end(); it++) {
		printf(" -- '%s' '%s'\n", it->first.c_str(), it->second.c_str());
		if(it->first=="SAMPLE") {
			size_t komma=it->second.find_first_of(',');
			if(komma != std::string::npos) {
				std::string nr = it->second.substr(0,komma);
				std::string filename = getSampleFilename(it->second.substr(komma+1));
				int n=atol(nr.c_str());
				int fahrstufe=n/3;
				switch(n%3) {
					case 0: cfg_soundFiles[fahrstufe].up=filename; break;
					case 1: cfg_soundFiles[fahrstufe].run=filename; break;
					case 2: cfg_soundFiles[fahrstufe].down=filename; break;
				}
			}
		}
		if(it->first=="SCHWELLE") {
			size_t komma=it->second.find_first_of(',');
			int fahrstufe=atol(it->second.substr(0,komma).c_str());
			size_t komma2=it->second.find_first_of(',',komma+1);
			cfg_soundFiles[fahrstufe].limit=atol(it->second.substr(komma+1,komma2).c_str());
			printf("SCHWELLE: %d\n", cfg_soundFiles[fahrstufe].limit);
		}
	}
}
/**
 * "DSet"
"NUMMER",0
"NAME","BR01"
"STUFEN",2
"SLOTS",3             <- +1
(
slots+1 'ch' sample hoch last
slots+1 'ch' sample normal
slots+1 'ch' sample beim bremsen

zeit in ms
) STUFEN#

 */
void SectionValues::parseDSet() {
	printf("SectionValues::parseDSet()\n");
	cfg_steamSoundFiles.nslots=utils::stoi(this->operator[]("SLOTS"))+1;
	assert(cfg_steamSoundFiles.nslots <= SteamSoundSlotType::maxSlots);
	cfg_steamSoundFiles.nstufen=utils::stoi(this->operator[]("STUFEN"));
	assert(cfg_steamSoundFiles.nstufen <= SteamSoundType::maxStufen);
	SectionValues::const_iterator it=this->begin();
	while(true) {
		if(it->second=="") {
			break;
		}
		++it;
	}
	printf("stufen: %d slots:%d\n", cfg_steamSoundFiles.nstufen, cfg_steamSoundFiles.nslots);
	for(int stufe = 0 ; stufe < cfg_steamSoundFiles.nstufen; stufe++) {
		for(int hml = 0 ; hml < 3 ; hml ++) {
			for(int slot = 0 ; slot < cfg_steamSoundFiles.nslots ; slot++) {
			// SectionValues::const_iterator it=this->begin(); it!=this->end(); it++) {
				printf(" -- '%s' '%s'\n", it->first.c_str(), it->second.c_str());
				try {
					utils::stoi(it->first);
					std::string filename = getSampleFilename(it->first);
					cfg_steamSoundFiles.slots[stufe].ch[hml][slot] = filename;
				} catch(...) {
					printf("<<< invalid sound\n");
				}
				++it;
			}
		}
		printf("reading ms\n");
		cfg_steamSoundFiles.slots[stufe].ms=utils::stoi(it->first);
		++it;
	}
}

