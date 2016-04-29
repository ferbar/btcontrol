#include <stdio.h>
#include <boost/algorithm/string.hpp>
#include "utils.h"
#include <stdexcept>
// fürs backtrace:
#include <execinfo.h>
// fürs demangle:
#include <cxxabi.h>



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
	try {
		return this->data.at(key);
	} catch(std::out_of_range &e) {
		throw std::out_of_range("key " + key + " not found");
	}
}

int utils::stoi(const std::string &in)	{
	char *endptr;
	int ret=strtol(in.c_str(), &endptr, 0);
	if(endptr != in.c_str() + in.length()) {
		throw std::runtime_error("error converting number");
	}
	return ret;
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

