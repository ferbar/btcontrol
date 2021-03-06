#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <map>
#include "RuntimeExceptionWithBacktrace.h"

std::string readFile(std::string filename);

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

// returns const-array length()
// https://blogs.msdn.microsoft.com/the1/2004/05/07/how-would-you-get-the-count-of-an-array-in-c-2/
template <typename T, size_t N>
char ( &_ArraySizeHelper( T (&array)[N] ))[N];

#define countof( array ) (sizeof( _ArraySizeHelper( array ) ))

// #define read myRead
ssize_t myRead(int so, void *data, size_t size);


  #define ANSI_DEFAULT "\x1b[0m"
  #define ANSI_RED "\x1b[31m"
  #define ANSI_RED2 "\x1b[31;1m"

  #define ANSI_GREEN1 "\x1b[32m"
  #define ANSI_GREEN2 "\x1b[32;1m"

void strtrim(char *s);

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


namespace utils
{
	template < typename T > std::string to_string( const T& n )
	{
		std::ostringstream stm ;
		stm << n ;
		return stm.str() ;
	}
	// -std=c++11 already knows this
	int stoi(const std::string &in);
	bool startsWith(const std::string &str, const std::string &with);
	bool startsWith(const std::string &str, const char *with);

	bool endsWith(const std::string &str, const char *with);

	// std::string format(const char *fmt, ...);
	template< typename... argv >
	std::string format( const char* format, argv... args ) {
		// const size_t SIZE = std::snprintf( NULL, 0, format, args... );
		const size_t SIZE = snprintf( NULL, 0, format, args... );
		std::string output;
		output.reserve(SIZE+1);
		output.resize(SIZE);
		snprintf( &(output[0]), SIZE+1, format, args... );
		return std::move(output);
	}

	bool isDir(const char *filename);

	void setThreadClientID(int clientID);
	void setThreadMessageID(int messageID);
	int getThreadClientID();
	int getThreadMessageID();
	// ESP32 hat keine boost lib
	std::string trim(const std::string &s);

	class Log {
	public:
		void printf(const char *tag, int level, const char *file, int line, const char *fmt, ...)
			__attribute__ ((format (printf, 6, 7)));
		static const int LEVEL_DEBUG=1;
		static const int LEVEL_NOTICE=2;
		static const int LEVEL_ERROR=3;
	};
	extern utils::Log log;
	void dumpBacktrace();
};

// hint ## => ... kann auch leer sein! -> https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html
#ifdef NODEBUG
#define DEBUGF(fmt, ...)
#define NOTICEF(fmt, ...)
#define ERRORF(fmt, ...)
#else
#define DEBUGF(fmt, ...) utils::log.printf(TAG, utils::Log::LEVEL_DEBUG, __FILE__, __LINE__, fmt "\n", ##__VA_ARGS__ )
#define NOTICEF(fmt, ...) utils::log.printf(TAG, utils::Log::LEVEL_NOTICE, __FILE__, __LINE__, fmt "\n", ##__VA_ARGS__ )
#define ERRORF(fmt, ...) utils::log.printf(TAG, utils::Log::LEVEL_ERROR, __FILE__, __LINE__, fmt "\n", ##__VA_ARGS__ )
#endif

// arduino / wiring pi / utils.cpp
extern "C" {
extern unsigned int millis();
}

#endif // utils.h
