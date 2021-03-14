/**
 * mit -std=c++11 kompilieren !
 */
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>

#include "zsp.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "unquote.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "sound.h"
#include "utils.h"

const char *overrideConfigNames[] = {"sound.boil", "sound.brake", "sound.entwaessern", "sound.abfahrt", "sound.horn", NULL };

const char* ablNames[CFG_FUNC_SOUND_N] = {"Sieden", "Glocke", "Bremsquietschen", "Entwässern", "Anfahrpfiff", "??", "E-Motor", "??", "Schaltwerk",
	"??", "??", "??", "??", "??", "E Bremse", "Kurve", "??", "??"};

SoundType *cfg_soundFiles=NULL;
//SteamSoundType cfg_steamSoundFiles;

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
		printf("sound.name not set in config file\n");
		abort();
	}
	auto diSets=ZSPData.equal_range("DiSet");
	for(ZSPDataType::iterator it=diSets.first; it!=diSets.second; ++it) {
		//it->second->dump();
		std::string setName=it->second->getName();
		printf("DiSet SetName: %s\n", setName.c_str());
		if(setName == soundSetName) {
			printf("searching SCHWELLE, SAMPLE @ DiSet\n");
			cfg_soundFiles=it->second->parseDiSet();
			found=true;
			break;
		} else
			printf("no match\n");
	}
	if(found) {
	} else {
		diSets=ZSPData.equal_range("DSet");
		for(ZSPDataType::iterator it = diSets.first; it!=diSets.second; ++it) {
			std::string setName=it->second->getName();
			printf("DSet SetName: %s\n", setName.c_str());
			if(setName == soundSetName) {
				printf("searching SAMPLE @ DSet\n");
				cfg_soundFiles=it->second->parseDSet();
				found=true;
				break;
			}
		}
	}
	if(!found) {
		printf("Error: D*Set Name %s not found\n", soundSetName.c_str());
		abort();
	}

	/*
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
	*/

	// Ablauf Sounds laden:
	auto abl=ZSPData.equal_range("Abl");
	for(ZSPDataType::iterator it=abl.first; it!=abl.second; ++it) {
		it->second->dump();
		printf("Abl\n");
		SectionValuesPtr sp = it->second;
		if(utils::stoi(sp->operator[]("LOK")) == cfg_soundFiles->lok) {
			int nummer=utils::stoi(sp->operator[]("NUMMER"));
			if(nummer < 0 || nummer >= CFG_FUNC_SOUND_N) {
				printf("Error: invalid Abl::NUMMER:%d\n", nummer);
				abort();
			}	
			sp->dump();
			std::string filename = getSampleFilename(sp->operator[]("SAMPLE"));
			cfg_soundFiles->funcSound[nummer]=filename;
			printf(" ----------------- Ablauf %i => %s --------------\n", nummer, filename.c_str());
			cfg_soundFiles->funcSoundVolume[nummer]=utils::stoi(sp->operator[]("LAUTST"));
		}
	}
	// Ablauf Sound overrides:
	printf("Sound override config:\n");
	for(size_t i = 0 ; overrideConfigNames[i] ; i++) {
		std::string overrideConfig=config.get(overrideConfigNames[i]);
		if(overrideConfig != NOT_SET) {
			cfg_soundFiles->funcSound[i] = config.get(overrideConfigNames[i]);
		}
	}
	cfg_soundFiles->dump();
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

	/*
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
	try {
		cfg_funcSound[CFG_FUNC_SOUND_BOIL] = config.get("sound.boil");
	} catch(std::exception &e) {
		printf("unable to get config/sound.boil\n");
	}
	try {
		cfg_funcSound[CFG_FUNC_SOUND_BRAKE] = config.get("sound.brake");
	} catch(std::exception &e) {
		printf("unable to get config/sound.brake\n");
	}
	*/
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
DiSoundType *SectionValues::parseDiSet() {
	assert(cfg_soundFiles == NULL);
	DiSoundType *soundFiles=new DiSoundType();
	printf("SectionValues::parseDiSet()\n");
	soundFiles->nsteps=utils::stoi(this->operator[]("STUFEN"))+1;
	for(SectionValues::const_iterator it=this->begin(); it!=this->end(); it++) {
		printf(" -- '%s' '%s'\n", it->first.c_str(), it->second.c_str());
		if(it->first=="SAMPLE") {
			size_t komma=it->second.find_first_of(',');
			if(komma != std::string::npos) {
				std::string nr = it->second.substr(0,komma);
				std::string filename = getSampleFilename(it->second.substr(komma+1));
				int n=atol(nr.c_str());
				int fahrstufe=n/3;
				assert(fahrstufe < soundFiles->nsteps);
				printf(" -- Fahrstufe: %d/%d %s (curr limit %d) \n", fahrstufe,n%3, filename.c_str(), soundFiles->steps[fahrstufe].limit);
				switch(n%3) {
					case 0: soundFiles->steps[fahrstufe].up=filename; break;
					case 1: soundFiles->steps[fahrstufe].run=filename; break;
					case 2: soundFiles->steps[fahrstufe].down=filename; break;
				}
			}
		}
		if(it->first=="SCHWELLE") {
			size_t komma=it->second.find_first_of(',');
			int fahrstufe=atol(it->second.substr(0,komma).c_str());
			if(fahrstufe < soundFiles->nsteps+1) {
				size_t komma2=it->second.find_first_of(',',komma+1);
				soundFiles->steps[fahrstufe].limit=atol(it->second.substr(komma+1,komma2).c_str());
				printf("SCHWELLE: %d\n", soundFiles->steps[fahrstufe].limit);
			} else {
				printf(ANSI_RED "Error: invaid ZSP Fahrstufe/Schwelle [%d]\n" ANSI_DEFAULT, soundFiles->nsteps);
			}
		}
	}
	// Workaround: Fahrstufe 0 hat keine Schwelle 
	soundFiles->steps[0].limit=1;
	return soundFiles;
}
/**
 * "DSet"
"NUMMER",0
"NAME","BR01"
"STUFEN",2
"SLOTS",3             <- +1
(
slots+1 'ch' sample normal
slots+1 'ch' sample beim bremsen
slots+1 'ch' sample hoch last

zeit in ms
) STUFEN#

 */
SteamSoundType *SectionValues::parseDSet() {
	SteamSoundType *soundFiles=new SteamSoundType();
	printf("SectionValues::parseDSet()\n");
	soundFiles->nslots=utils::stoi(this->operator[]("SLOTS"))+1;
	assert(soundFiles->nslots <= SteamSoundStepType::maxSlots);
	soundFiles->nsteps=utils::stoi(this->operator[]("STUFEN"));
	assert(soundFiles->nsteps <= SteamSoundType::maxSteps);
	soundFiles->lok=utils::stoi(this->operator[]("NUMMER"));
	printf("     nslots:%d, nsteps:%d, lok #%d\n", soundFiles->nslots, soundFiles->nsteps, soundFiles->lok);
	SectionValues::const_iterator it=this->begin();
	while(true) {
		if(it->second=="") {
			break;
		}
		++it;
	}
	printf("stufen: %d slots:%d\n", soundFiles->nsteps, soundFiles->nslots);
	for(int step = 0 ; step < soundFiles->nsteps; step++) {
		for(int hml = 0 ; hml < 3 ; hml ++) {
			for(int slot = 0 ; slot < soundFiles->nslots ; slot++) {
			// SectionValues::const_iterator it=this->begin(); it!=this->end(); it++) {
				printf(" -- '%s' '%s'\n", it->first.c_str(), it->second.c_str());
				try {
					//check if it->first is an int
					utils::stoi(it->first);
					std::string filename = getSampleFilename(it->first);
					soundFiles->steps[step].ch[hml][slot] = filename;
				} catch(...) {
					printf("<<< invalid sound\n");
				}
				++it;
			}
		}
		printf("reading ms\n");
		soundFiles->steps[step].ms=utils::stoi(it->first);
		++it;
	}

	return soundFiles;
}

