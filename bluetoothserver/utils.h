#ifndef UTILS_H

#include <string>
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


#endif
