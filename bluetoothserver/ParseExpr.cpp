#include <string>
#include <stdexcept>

#include "ParseExpr.h"

int ParseExpr::getResult(std::string expr, int dir, int speed, int F0) {
	if(expr == "dir")	return dir;
	if(expr == "!dir")	return !dir;
	if(expr == "dir && F0")	return dir && F0;
	if(expr == "!dir && F0") return !dir && F0;
	if(expr == "speed")	return speed;
	if(expr == "!speed")	return !speed;
	throw std::runtime_error("invalid expression (" + expr + ")");
}
