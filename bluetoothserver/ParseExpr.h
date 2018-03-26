#include <string>

class ParseExpr {
public:
	ParseExpr() {};
	int getResult(std::string expr, int dir, int speed, int F0);
	int lastSpeed=0;
	int speed=0;
	time_t lastSpeedChange=0;
};
