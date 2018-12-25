#ifndef PARSEEXPR_H
#define PARSEEXPR_H
#include <string>

class ParseExpr {
public:
	ParseExpr() :speed(0), lastAccel(0), lastBrake(0) {};
	int getResult(const std::string &expr, int dir, int speed, bool *currentFunc);
	int speed;
	time_t lastAccel;
	time_t lastBrake;
};
#endif
