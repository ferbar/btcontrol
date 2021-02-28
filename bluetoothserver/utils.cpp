#include <stdio.h>
#ifdef ESP_PLATFORM
// am ESP32 geht das std::stoi sonst ned: - muss vorm utils.h includet sein
#define _GLIBCXX_USE_C99 1
#endif
//#include <stdexcept>
//#include <sstream>
#include <iostream>
#include <memory>
#include <stdarg.h>
#include <cctype>
#include <algorithm>
#include <string.h>

#include "Thread.h"
#include "utils.h"

#ifdef HAVE_EXECINFO
// fürs backtrace:
#include <execinfo.h>
// fürs demangle:
#include <cxxabi.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// dirname
#include <libgen.h>

// intptr_t
#include <inttypes.h>

#ifdef ESP_PLATFORM
// fürs Serial
#include <Arduino.h>
#endif

#define TAG "utils"

namespace utils {
	utils::Log log;
};

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
			utils::trim(key);
			utils::trim(value);
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
#if __cplusplus > 199711L
	size_t end=0;
	int ret=std::stoi(in, &end, 0);
	if(end != in.length()) {
#else
	char *endp=NULL;
	int ret=::strtol(in.c_str(), &endp, 0);
	if(endp != NULL) {
#endif
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

bool utils::endsWith(const std::string &str, const char *with) {
	return str.rfind(with) == str.length()-strlen(with);
}

/**
 * tut spaces am ende weg
 */
void strtrim(char *s)
{
	char *pos=s+strlen(s);
	while(pos > s) {
		pos--;
		if(isspace(*pos)) {
			*pos='\0';
		} else
			break;
	}
}

#ifdef HAVE_EXECINFO
void utils::dumpBacktrace() {
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr das kamma mit addr2line noch anschaun dann:
	//fprintf(stderr, "Error: %s size=%zd\n", what.c_str(), size);
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
#endif

#undef runtime_error
std::RuntimeExceptionWithBacktrace::RuntimeExceptionWithBacktrace(const std::string &what)
	: std::runtime_error(what)
{
	printf("Error: %s\n", what.c_str());
	utils::dumpBacktrace();
}

std::RuntimeExceptionWithBacktrace::~RuntimeExceptionWithBacktrace() throw ()
{

}

/**
 * liest eine komplette Datei, wenn nicht gefunden dann relativ zum bin dir
 * @param filename - das was eingelesen wird, kopie(!!)
 */
#if not defined ESP_PLATFORM
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
#endif

/* is template im utils.h
std::string utils::format(const char *fmt, ...) {
	size_t size = 0;
	va_list ap;
	char *buf=NULL;
	va_start(ap, fmt);
	size=vasprintf(&buf, fmt, ap );
	va_end(ap);
	if(buf==NULL) {
		DEBUGF("format failed: fmt:%s",fmt);
		return std::string("format failed");
	}
	printf("format result size:%zu string:%s\n", size, buf);
	std::string ret=std::string( buf, size ); // We don't want the '\0' inside
	free(buf);
	return ret;
}
*/

/**
 * das muss am ende der Datei sein!!!
 * @throws exception bei einem fehler / wenn size nicht gelesen werden konnte
 */
#undef read
ssize_t myRead(int so, void *data, size_t size) {
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

bool utils::isDir(const char *filename) {
	struct stat buffer;
	if(stat(filename, &buffer) != 0) {
		return false;
	}
	if(S_ISDIR(buffer.st_mode)) {
		return true;
	} else {
		return false;
	}
}

ThreadSpecific threadSpecificClientID;
ThreadSpecific threadSpecificMessageID;
void utils::setThreadClientID(int clientID) {
	intptr_t tmp = clientID;
	threadSpecificClientID.set((void*) tmp);
}
void utils::setThreadMessageID(int messageID) {
	intptr_t tmp = messageID;
	threadSpecificMessageID.set((void*) tmp);
}
int utils::getThreadClientID() {
	intptr_t tmp = (intptr_t) (threadSpecificClientID.get() );
	return (int) tmp;
}
int utils::getThreadMessageID() {
	intptr_t tmp = (intptr_t) (threadSpecificMessageID.get() );
	return (int) tmp;
}

// c++11 magick
std::string utils::trim(const std::string &s)
{
// BOOST_NO_CXX11_AUTO_DECLARATIONS
#if __cplusplus > 199711L
	auto wsfront=std::find_if_not(s.begin(),s.end(),[](int c){return std::isspace(c);});
	auto wsback=std::find_if_not(s.rbegin(),s.rend(),[](int c){return std::isspace(c);}).base();
	return (wsback<=wsfront ? std::string() : std::string(wsfront,wsback));
#else
	size_t endpos = s.find_last_not_of(" \t");
	size_t startpos = s.find_first_not_of(" \t");
	std::string ret=s;
	if( std::string::npos != endpos ) {
		ret = s.substr( 0, endpos+1 );
		ret = ret.substr( startpos );
	}
	return ret;
#endif
}

void utils::Log::printf(const char *tag, int level, const char *file, int line, const char *fmt, ...) {
#ifdef ESP_PLATFORM
	Serial.printf("%d: ", utils::getThreadClientID());
	va_list args;
	va_start(args, fmt);
	char *data=NULL;
	vasprintf(&data, fmt, args);
	Serial.print(data);
	free(data);
	va_end(args);
	Serial.println();
#else
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
#endif
}

#if ! defined(ESP_PLATFORM) && ! defined(HAVE_RASPI_WIRINGPI)
unsigned int millis() {
	struct timespec ts;
	clock_gettime(CLOCK_BOOTTIME, &ts);
	return ts.tv_sec*1000 + ts.tv_nsec / 1000000;
}
#endif
