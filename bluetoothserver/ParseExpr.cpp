#include <string>
#include <stdexcept>

#include "ParseExpr.h"

int ParseExpr::getResult(std::string expr, int dir, int speed, int F0) {
	if(this->speed != speed) {
		this->lastSpeed=this->speed;
		this->speed=speed;
		this->lastSpeedChange=time(NULL);
	}
		
	if(expr == "dir")	return dir;
	if(expr == "!dir")	return !dir;
	if(expr == "dir && F0")	return dir && F0;
	if(expr == "!dir && F0") return !dir && F0;
	if(expr == "dir && F0 && speed")	return dir && F0 && speed;
	if(expr == "!dir && F0 && speed")	return !dir && F0 && speed;
	if(expr == "dir && F0 && !speed")	return dir && F0 && !speed;
	if(expr == "!dir && F0 && !speed")	return !dir && F0 && !speed;
	if(expr == "speed")	return speed;
	if(expr == "!speed")	return !speed;
	if(expr == "accel+5s")  {
		// printf("accel: speed: %d (last: %d) time: %ld (last: %ld)\n", speed, lastSpeed, time(NULL), lastSpeedChange) ;
		int ret=(lastSpeedChange > time(NULL) - 5 ) && (this->lastSpeed < speed);
		// printf("ret: %d\n", ret);
		return ret;
	}
	if(expr == "accel+10s")  return (this->lastSpeedChange > time(NULL) - 10 ) && (this->lastSpeed < speed);
	throw std::runtime_error("invalid expression (" + expr + ")");
}
