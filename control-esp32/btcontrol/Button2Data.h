#include <Button2.h>

template<typename DataType>
class Button2Data : public Button2 {
	public:
		Button2Data(byte gpio, DataType data) : Button2(gpio), data(data) { };
		DataType data;
};
