#include <stdio.h>
#include <boost/algorithm/string.hpp>
#include "utils.h"
#include <stdexcept>
#include <sstream>
#include <iostream>

// fürs backtrace:
#include <execinfo.h>
// fürs demangle:
#include <cxxabi.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// dirname
#include <libgen.h>

const std::string NOT_SET="__NOT_SET";

Config config;

Config::Config() {

}

void Config::init(const std::string &confFilename) {
	std::string data=readFile(confFilename);
	std::istringstream f(data);
	std::string line;    
	while (std::getline(f, line)) {
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
		this->data.insert( std::pair<std::string, std::string>(key, value) );
	}
}

const std::string Config::get(const std::string key) {
	try {
		std::multimap<std::string, std::string>::const_iterator it = this->data.find(key);
		if(it == this->end()) { // raspi 20161004 macht keine exception wenn nicht gefunden
			return NOT_SET;
		}
		return it->second;
	} catch(std::out_of_range &e) {
		throw std::out_of_range("key " + key + " not found");
	}
}

int utils::stoi(const std::string &in)	{
	if(in == NOT_SET) {
		throw std::runtime_error("NOT SET");
	}
	size_t end=0;
	int ret=std::stoi(in, &end, 0);
	if(end != in.length()) {
		throw std::runtime_error("error converting number");
	}
	return ret;
}

bool utils::startsWith(const std::string &str, const std::string &with) {
	return str.find(with) == 0;
}

bool utils::startsWith(const std::string &str, const char *with) {
	return str.find(with) == 0;
}

#undef runtime_error
std::RuntimeExceptionWithBacktrace::RuntimeExceptionWithBacktrace(const std::string &what)
	: std::runtime_error(what)
{
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr das kamma mit addr2line noch anschaun dann:
	fprintf(stderr, "Error: %s size=%zd\n", what.c_str(), size);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	/*
	messages = backtrace_symbols(array, size);

	// skip first stack frame (points here)
	for (i = 1; i < size && messages != NULL; ++i) {
		fprintf(stderr, "[bt]: (%d) %s\n", i, messages[i]);
	}
	free(messages);

	*/

	// http://stackoverflow.com/questions/77005/how-to-generate-a-stacktrace-when-my-gcc-c-app-crashes

	char ** messages = backtrace_symbols(array, size);    

	// skip first stack frame (points here)
	for (size_t i = 1; i < size && messages != NULL; ++i) {
		char *mangled_name = 0, *offset_begin = 0, *offset_end = 0;

		// find parantheses and +address offset surrounding mangled name
		for (char *p = messages[i]; *p; ++p) {
			if (*p == '(') {
				mangled_name = p; 
			} else if (*p == '+') {
				offset_begin = p;
			} else if (*p == ')') {
				offset_end = p;
				break;
			}
		}

		// if the line could be processed, attempt to demangle the symbol
		if (mangled_name && offset_begin && offset_end && mangled_name < offset_begin) {
			*mangled_name++ = '\0';
			*offset_begin++ = '\0';
			*offset_end++ = '\0';

			int status;
			char * real_name = abi::__cxa_demangle(mangled_name, 0, 0, &status);

			// if demangling is successful, output the demangled function name
			if (status == 0) {    
				std::cerr << "[bt]: (" << i << ") " << messages[i] << " : " 
					<< real_name << "+" << offset_begin << offset_end 
					<< std::endl;

			}
			// otherwise, output the mangled function name
			else {
				std::cerr << "[bt]: (" << i << ") " << messages[i] << " : " 
					<< mangled_name << "+" << offset_begin << offset_end 
					<< std::endl;
			}
			free(real_name);
		}
		// otherwise, print the whole line
		else {
			std::cerr << "[bt]: (" << i << ") " << messages[i] << std::endl;
		}
	}
	std::cerr << std::endl;

	free(messages);
}

std::RuntimeExceptionWithBacktrace::~RuntimeExceptionWithBacktrace() throw ()
{

}

/**
 * liest eine komplette Datei, wenn nicht gefunden dann relativ zum bin dir
 * @param filename - das was eingelesen wird, kopie(!!)
 */
std::string readFile(std::string filename)
{
	std::string ret;
	struct stat buf;
	if(stat(filename.c_str(), &buf) != 0) {
		char execpath[MAXPATHLEN];
		if(readlink("/proc/self/exe", execpath, sizeof(execpath)) <= 0) {
			printf("error reading /proc/self/exe\n");
			abort();
		}
		char *linkpath=dirname(execpath);
		filename.insert(0,std::string(linkpath) + '/');
		if(stat(filename.c_str(), &buf) != 0) {
			fprintf(stderr,"error stat file %s\n",filename.c_str());
			throw std::runtime_error("error stat file");
		}
	}
	ret.resize(buf.st_size,'\0');
	FILE *f=fopen(filename.c_str(),"r");
	if(!f) {
		fprintf(stderr,"error reading file %s\n",filename.c_str());
		throw std::runtime_error("error reading file");
	} else {
		const char *data=ret.data(); // mutig ...
		fread((void*)data,1,buf.st_size,f);
		fclose(f);
		printf("%s:%lu bytes\n",filename.c_str(),buf.st_size);
	}
	return ret;
}

/**
 * das muss am ende der Datei sein!!!
 * @throws exception bei einem fehler / wenn size nicht gelesen werden konnte
 */
#undef read
int myRead(int so, void *data, size_t size) {
	int read=0;
	// printf("myRead: %zd\n",size);
	while(read < (int) size) {
		// printf("read: %zd\n",size-read);
		int rc=::read(so,((char *) data)+read,size-read);
		// printf("rc: %d\n",rc);
		if(rc < 0) {
			throw std::runtime_error("error reading data");
		} else if(rc == 0) { // stream is blocking -> sollt nie vorkommen
			throw std::runtime_error("nothing to read");
		}
		read+=rc;
	}
	return read;
}

