#include <string>

class ParseExpr {
public:
	ParseExpr() {};
	int getResult(std::string expr, int dir, int speed, int F0);
	int speed=0;
	time_t lastAccel=0;
	time_t lastBrake=0;
};
