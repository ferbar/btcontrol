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

DataType data;

std::string soundsetPath;

std::string getSampleFilename(std::string number) {
	// printf(" ---- %s \n",number.c_str());
	for(DataType::iterator it = data.find("Sample"); it!=data.end(); it++) {
		// it->second->dump();
		// printf("sampe\n");
		SectionValuesPtr sp = it->second;
		SectionType::iterator spIt = sp->data.find("NUMMER");
		if(spIt!=sp->data.end()) {
			// printf("   sample -- number: %s\n", spIt->second.c_str());
			if(spIt->second == number) {
				// printf(" ----------------- sample --------------\n");
				// sp->dump();
				spIt = sp->data.find("PFAD");
				std::string pfad=boost::algorithm::unquote(spIt->second,'"','\b');
				printf(" %s/", pfad.c_str());
				spIt = sp->data.find("NAME");
				std::string name=boost::algorithm::unquote(spIt->second,'"','\b');
				printf(" %s\n", name.c_str());
				printf(" ----------------- /sample -------------- %s %s\n",pfad.c_str(),name.c_str());
				return soundsetPath+"/"+boost::replace_all_copy(pfad, "\\", "/") + name;
			}
		}
	}
	throw "invalid number";
}


void loadZSP() {
	// std::multimap< int,std::string > map_data;
	// map_data.emplace(5,"sdfsdf");
	// map_data[7]["hello"] = 3.1415926;

	printf("zimo sound projekt test\n");
	const std::string soundsetFile=config.get("soundset");
	if(soundsetFile == "") {
		return;
	}
	size_t slash=soundsetFile.find_last_of('/');
	if(slash != std::string::npos) {
		soundsetPath=soundsetFile.substr(0,slash);
	}
	FILE *f=fopen(soundsetFile.c_str(), "r");
	assert(f);
	char buffer[1024];
	std::string section="";
	SectionValuesPtr sp;
	while(fgets(buffer, sizeof(buffer), f)) {
		std::string line=buffer;
		boost::algorithm::trim(line);
		if(boost::starts_with(line,"\"/")) {
			// printf("end section\n");
			data.insert(std::pair<std::string, SectionValuesPtr>(section,sp) );
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
		printf("add %s=%s\n", subsection.c_str(), value.c_str());
		sp->data.insert(std::pair<std::string, std::string>(subsection, value) );

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
	DataType::iterator it = data.find("DiSet");
	if(it!=data.end()) {
		it->second->dump();
	}
	for(int i=0; i < 10; i++) {
		printf("%d ",i);
		cfg_soundFiles[i].dump();
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
}

void SectionValues::dump() {
		for(SectionType::const_iterator it=data.begin(); it!=data.end(); it++) {
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
				printf("schwelle: %d\n", cfg_soundFiles[fahrstufe].limit);
			}
		}
	}
