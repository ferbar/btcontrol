#ifndef UTILS_H

#include <string>
#include <map>
#include <stdlib.h>
std::string readFile(std::string filename);
#define ANSI_RED "\x1b[31m"
#define ANSI_DEFAULT "\x1b[0m"

#define STREQ(s1,s2) (strcmp(s1,s2)==0)

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 255
#endif

extern bool cfg_debug;
extern int cfg_tcpTimeout;

#ifdef INCL_X11
extern bool cfg_X11;
#endif

// macht aus blah "blah"
#define     _STR(x)   _VAL(x)
#define     _VAL(x)   #x

#define read myRead
int myRead(int so, void *data, size_t size);


  #define ANSI_DEFAULT "\x1b[0m"
  #define ANSI_RED1 "\x1b[31m"
  #define ANSI_RED2 "\x1b[31;1m"

  #define ANSI_GREEN1 "\x1b[32m"
  #define ANSI_GREEN2 "\x1b[32;1m"

extern const std::string NOT_SET;

class Config {
public:
	Config();
	void init(const std::string &confFilename);
	const std::string get(const std::string key);
	std::multimap<std::string, std::string>::const_iterator begin() { return data.begin(); }
	std::multimap<std::string, std::string>::const_iterator end() { return data.end(); }
private:
	std::multimap<std::string, std::string> data;
};

extern Config config;

#include <string>
#include <sstream>

namespace utils
{
	template < typename T > std::string to_string( const T& n )
	{
		std::ostringstream stm ;
		stm << n ;
		return stm.str() ;
	}
	int stoi(const std::string &in);
	bool startsWith(const std::string &str, const std::string &with);
	bool startsWith(const std::string &str, const char *with);
}

#include <stdexcept>

namespace std
{

class RuntimeExceptionWithBacktrace : public std::runtime_error {
public:
	RuntimeExceptionWithBacktrace(const std::string &what);
	virtual ~RuntimeExceptionWithBacktrace() throw ();
private:
};

}
#define runtime_error RuntimeExceptionWithBacktrace

#endif
