#include <stdio.h>
#include <boost/algorithm/string.hpp>
#include "utils.h"


Config config("conf/btserver.conf");

Config::Config(const std::string confFilename) {
	FILE *f=fopen(confFilename.c_str(),"r");
	if(!f) {
		fprintf(stderr,"no config file ... skipping\n");
		return;
	}
	char buffer[1024];
	while(fgets(buffer,sizeof(buffer),f)) {
		std::string line(buffer);
		std::string key, value;
		size_t komma=line.find_first_of('=');
		if(komma != std::string::npos) {
			key=line.substr(0,komma);
			value=line.substr(komma+1);
			boost::algorithm::trim(key);
			boost::algorithm::trim(value);
		} else {
			key=line;
			value="";
		}
		this->data[key] = value;
	}
}

std::string Config::get(const std::string key) {
	return this->data[key];
}
