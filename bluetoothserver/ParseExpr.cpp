/**
 * expression parser
 * !! Whitespaces werden nicht rausgefiltert, das muss genau passen !!
 *
 * es funktioniert:
 *     F[0-9] => func ein/aus               z.b.  "F4"
 *     (expr) && (expr)                   z.b.  "(accel+10s) || (F3)"
 *     (expr) || (expr)    => wichtig auf die spaces achten!
 *     die strings unten                    z.b.  "!dir && F0"
 *     pwm                 => wird für Motor PWM verwendet, kommt nicht daher weil nicht funktions pin
 *
 *     wird ein Pin öfters definiert wird ein or gemacht - wie (expr) || (expr)
 */
#include <string>
#include <stdexcept>
#include <stdio.h>

#include "ParseExpr.h"

int ParseExpr::getResult(const std::string &expr, int dir, int speed, bool *currentFunc) {
	if(expr.length() > 0 && expr[0] == '(') {
		size_t exprA_end=expr.find(")",1);
		if(exprA_end == std::string::npos) {
			throw std::runtime_error("invalid expressionA [" + expr + "]");
		}
		const std::string exprA=expr.substr(1,exprA_end-1);
		// printf("parseExpr::getResult A: %s\n", exprA.c_str());
		bool is_and=false;
		if(expr.compare(exprA_end, 6, ") && (") == 0) {
			is_and=true;
		} else if(expr.compare(exprA_end, 6, ") || (") == 0) {
		} else {
			throw std::runtime_error("invalid and (" + expr + ")");
		}
		size_t exprB_start=exprA_end+6;
		size_t exprB_end=expr.find(")",exprB_start);
		if(exprB_end == std::string::npos) {
			throw std::runtime_error("invalid expressionB [" + expr + "]");
		}

		const std::string exprB=expr.substr(exprB_start,exprB_end - exprB_start);
		// printf("parseExpr::getResult B: %s\n", exprB.c_str());
		bool a=this->getResult(exprA, dir, speed, currentFunc);

		bool b=this->getResult(exprB, dir, speed, currentFunc);
		
		if(is_and) {
			return a && b;
		} else {
			return a || b;
		}
	}

	if(expr.length() == 2 && expr[0]=='F') {
		if(expr[1] >= '0' && expr[1] <= '9') {
			// printf("parseExpr::getResult %s:%d\n",expr.c_str(), currentFunc[expr[1] - '0']);
			return currentFunc[expr[1] - '0'];
		}
	}

	if(this->speed != speed) {
		if(speed > this->speed)
			this->lastAccel=time(NULL);
		if(speed < this->speed)
			this->lastBrake=time(NULL);
		this->speed=speed;
	}
		
	if(expr == "dir")	return dir;
	if(expr == "!dir")	return !dir;
	if(expr == "dir && F0")	return dir && currentFunc[0];
	if(expr == "!dir && F0") return !dir && currentFunc[0];
	if(expr == "dir && F0 && speed")	return dir && currentFunc[0] && speed;
	if(expr == "!dir && F0 && speed")	return !dir && currentFunc[0] && speed;
	if(expr == "dir && F0 && !speed")	return dir && currentFunc[0] && !speed;
	if(expr == "!dir && F0 && !speed")	return !dir && currentFunc[0] && !speed;
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
