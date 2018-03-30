#include <string>
#include <stdexcept>

#include "ParseExpr.h"

int ParseExpr::getResult(std::string expr, int dir, int speed, int F0) {
	if(this->speed != speed) {
		if(speed > this->speed)
			this->lastAccel=time(NULL);
		if(speed < this->speed)
			this->lastBrake=time(NULL);
		this->speed=speed;
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
		int ret=(this->lastAccel > time(NULL) - 5 );
		// printf("ret: %d\n", ret);
		return ret;
	}
	if(expr == "accel+10s")  return (this->lastAccel > time(NULL) - 10 );
	if(expr == "accel+15s")  return (this->lastAccel > time(NULL) - 15 );

	if(expr == "brake+5s")   return (this->lastBrake > time(NULL) - 5 );
	if(expr == "brake+10s")  return (this->lastBrake > time(NULL) - 10 );
	if(expr == "brake+15s")  return (this->lastBrake > time(NULL) - 15 );

	throw std::runtime_error("invalid expression (" + expr + ")");
}
