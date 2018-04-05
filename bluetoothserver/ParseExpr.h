#include <string>

class ParseExpr {
public:
	ParseExpr() :speed(0), lastAccel(0), lastBrake(0) {};
	int getResult(std::string expr, int dir, int speed, int F0);
	int speed;
	time_t lastAccel;
	time_t lastBrake;
};
