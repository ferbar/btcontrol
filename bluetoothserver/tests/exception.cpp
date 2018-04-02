// exception example
#include <iostream>       // std::cerr
#include <typeinfo>       // operator typeid
#include <exception>      // std::exception
#include <stdexcept>

class Polymorphic {virtual void member(){}};

int main () {
	try
	{
		/*
		Polymorphic * pb = 0;
		typeid(*pb);  // throws a bad_typeid exception
		*/
		try {
			throw std::runtime_error("blaaaaah");
		} catch (std::exception& e)
		{
			std::cerr << "exception caught: " << e.what() << '\n';
			throw;
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "exception caught: " << e.what() << '\n';
	}
	return 0;
}
