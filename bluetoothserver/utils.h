#ifndef UTILS_H

#include <string>
std::string readFile(std::string filename);
#define ANSI_RED "\x1b[31m"
#define ANSI_DEFAULT "\x1b[0m"

#define STREQ(s1,s2) (strcmp(s1,s2)==0)

extern bool cfg_debug;

#ifdef INCL_X11
extern bool cfg_X11;
#endif

// macht aus blah "blah"
#define     _STR(x)   _VAL(x)
#define     _VAL(x)   #x

#endif
