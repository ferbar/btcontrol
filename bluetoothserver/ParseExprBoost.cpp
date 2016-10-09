// http://www.boost.org/doc/libs/1_48_0/libs/spirit/example/qi/compiler_tutorial/calc3.cpp
// http://www.boost.org/doc/libs/1_40_0/libs/spirit/example/qi/complex_number.cpp
// das in [] sind die actions: http://boost-spirit.com/home/2010/03/03/the-anatomy-of-semantic-actions-in-qi/
// http://www.wilkening-online.de/programmieren/c++-user-treffen-aachen/2014_11_13/BoostSpirit_GeorgHellack.pdf

// Spirit v2.5 allows you to suppress automatic generation
// of predefined terminals to speed up complation. With
// BOOST_SPIRIT_NO_PREDEFINED_TERMINALS defined, you are
// responsible in creating instances of the terminals that
// you need (e.g. see qi::uint_type uint_ below).
#define BOOST_SPIRIT_NO_PREDEFINED_TERMINALS


#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <iostream>
#include <string>

#include "ParseExpr.h"

/*
int  func(int attribute)
{
    std::cout << "Matched integer: " << attribute << "\n";
	return attribute;
}
*/


namespace client
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;


    ///////////////////////////////////////////////////////////////////////////
    //  Our calculator grammar
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    struct Parser : qi::grammar<Iterator, int(), ascii::space_type>
    {
		int dir;
		int F0;

        Parser() : Parser::base_type(expression)
        {
			printf("init\n");
			dir=0;
			F0=0;

			qi::lit_type lit;
            qi::_val_type _val;
            qi::_1_type _1;
            qi::uint_type uint_;

		// in [] sind die actions, wichtig: nachdem das was da ist schon beim initialisieren passiert mÃ¼ssen hier fÃr alles Lambda Funktionen verwendet werden!
            expression =
                term 							[_val = _1]
                >> *(   ("&&" >> term			[_val &= _1 ] )
                    |   ("||" >> term			[_val |= _1 ] )
                    )
                ;

            term =
                factor 							[_val = _1]
				| ('!' >> factor				[_val = ! _1])
                ;

            factor =
                uint_ 							[_val = _1]
                | ( lit("dir")					[_val = boost::phoenix::ref(dir)]  )
				| ( lit("F0") >> "=" >> uint_	[boost::phoenix::ref(F0) = _1] )
				// | ( lit("F0") >> "=" >> uint_	[func] )
                | ( lit("F0")					[_val = boost::phoenix::ref(F0)] )
                ;
        }

        qi::rule<Iterator, int(), ascii::space_type> expression, term, factor;
    };
}

typedef std::string::const_iterator iterator_type;
typedef client::Parser<iterator_type> Parser;

Parser *parser=NULL;

ParseExpr::ParseExpr() {
	parser = new Parser();
}

/*
int main()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Expression parser...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Type an expression...or [q or Q] to quit\n\n";

    typedef std::string::const_iterator iterator_type;
    typedef client::calculator<iterator_type> calculator;

    boost::spirit::ascii::space_type space; // Our skipper
    calculator calc; // Our grammar

    std::string str;
    int result;
    while (std::getline(std::cin, str))
    {
		calc.dir++;
		calc.F0++;
		printf("current F0:%d\n", calc.F0);
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        std::string::const_iterator iter = str.begin();
        std::string::const_iterator end = str.end();
        bool r = phrase_parse(iter, end, calc, space, result);

        if (r && iter == end)
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            std::cout << "result = " << result << std::endl;
            std::cout << "-------------------------\n";
        }
        else
        {
            std::string rest(iter, end);
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "stopped at: \" " << rest << "\"\n";
            std::cout << "-------------------------\n";
        }
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}
*/

