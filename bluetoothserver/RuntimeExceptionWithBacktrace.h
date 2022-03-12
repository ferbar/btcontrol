#ifndef RUNTIMEEXCEPTIONWITHBACKTRACE_H
#define RUNTIMEEXCEPTIONWITHBACKTRACE_H

#include <string>
#include <stdexcept>

namespace std
{

class RuntimeExceptionWithBacktrace : public std::runtime_error {
public:
	RuntimeExceptionWithBacktrace(const std::string &what) throw();
	virtual ~RuntimeExceptionWithBacktrace() throw ();
private:
};

}

#define runtime_error RuntimeExceptionWithBacktrace

#endif
