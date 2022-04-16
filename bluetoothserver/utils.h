#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <map>
#include "RuntimeExceptionWithBacktrace.h"

#ifdef ESP32
#include "utils_esp32.h"
#endif

std::string readFile(const std::string &filename);
void writeFile(const std::string &filename, const std::string &data);

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
  #define ANSI_YELLOW "\x1b[33;1m"
  #define ANSI_PINK   "\x1b[35;1m"

  #define ANSI_GREEN1 "\x1b[32m"
  #define ANSI_GREEN2 "\x1b[32;1m"
#ifdef ESP32
// ESP32 gibt alles übern com port aus, minicom erwartet \r\n
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif
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
		// kein std::move machen!
		return output;
	}

	bool isDir(const char *filename);

	void setThreadClientID(int clientID);
	void setThreadMessageID(int messageID);
	int getThreadClientID();
	int getThreadMessageID();
	// &s wird nicht verändert !!
	std::string trim(const std::string &s)  __attribute__ ((warn_unused_result));

	class Log {
	public:
		void printf(int level, const char *file, int line, const char *fmt, ...)
			__attribute__ ((format (printf, 5, 6)));
		static const int LEVEL_DEBUG=1;
		static const int LEVEL_NOTICE=2;
		static const int LEVEL_ERROR=3;
    void init(bool softAP);
	};
	extern utils::Log log;
	void dumpBacktrace();
};

// hint ## => ... kann auch leer sein! -> https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html
#define LOG_SEP ": "

#ifdef NODEBUG
#define DEBUGF(fmt, ...)
#endif

#ifdef USETAG
  #ifndef NODEBUG
    #define DEBUGF(fmt, ...) utils::log.printf(utils::Log::LEVEL_DEBUG, __FILE__, __LINE__, TAG LOG_SEP ANSI_GREEN1 fmt ANSI_DEFAULT NEWLINE, ##__VA_ARGS__ )
  #endif
  #define NOTICEF(fmt, ...) utils::log.printf(utils::Log::LEVEL_NOTICE, __FILE__, __LINE__, TAG LOG_SEP ANSI_YELLOW fmt ANSI_DEFAULT NEWLINE, ##__VA_ARGS__ )
  #define ERRORF(fmt, ...) utils::log.printf(utils::Log::LEVEL_ERROR, __FILE__, __LINE__, TAG LOG_SEP ANSI_RED2 fmt ANSI_DEFAULT NEWLINE, ##__VA_ARGS__ )
#else
  #ifndef NODEBUG
    #define DEBUGF(fmt, ...) utils::log.printf(utils::Log::LEVEL_DEBUG, __FILE__, __LINE__, ANSI_GREEN1 fmt ANSI_DEFAULT NEWLINE, ##__VA_ARGS__ )
  #endif
  #define NOTICEF(fmt, ...) utils::log.printf(utils::Log::LEVEL_NOTICE, __FILE__, __LINE__, ANSI_YELLOW fmt ANSI_DEFAULT NEWLINE, ##__VA_ARGS__ )
  #define ERRORF(fmt, ...) utils::log.printf(utils::Log::LEVEL_ERROR, __FILE__, __LINE__, ANSI_RED2 fmt ANSI_DEFAULT NEWLINE, ##__VA_ARGS__ )
#endif

#ifdef DEBUG_FREE_HEAP
#define PRINT_FREE_HEAP(_TEXT) debugPrintFreeHeap(__FILE__, __LINE__, _TEXT)
#else
#define PRINT_FREE_HEAP(_TEXT)
#endif
void debugPrintFreeHeap(const char *file, int line, const char *text);

// arduino / wiring pi -> int millis() / utils.cpp
#if not defined HAVE_RASPI_WIRINGPI
extern "C" {
extern unsigned long millis();
}
#endif

#endif // utils.h
