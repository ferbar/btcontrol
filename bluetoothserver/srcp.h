
class SRCP {
public:
	SRCP();
	~SRCP();
	// add:0... dir:0 zurück 1 vor 2 notstop, nFahrstufen: 14, ,nFunc:4 
	void send(int addr, int dir, int nFahrstufen, int speed, int nFunc, bool *func);
	void pwrOn();
	void pwrOff();
private:
	int so;
};
