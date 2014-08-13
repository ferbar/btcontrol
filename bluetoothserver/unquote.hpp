#ifndef _included_boost_range_algorithm_unquote_hpp_
#define _included_boost_range_algorithm_unquote_hpp_

// Boost string_algo library unquote.hpp header file

// Copyright Robert Stewart 2010.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/ for updates, documentation, and revision history.

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator.hpp>
#include <boost/range/value_type.hpp>
//#include <boost/range/algorithm/copy.hpp> // only in 1.43

namespace boost
{
   namespace algorithm
   {
      template <class OutIt, class Range, class Char>
      OutIt
      unquote(OutIt _out, Range & _input, Char _delimiter = '"',
         Char _escape = '\\');

      template <class Sequence>
      inline Sequence
      unquote(Sequence const & _input);

      template <class Sequence, class Char>
      inline Sequence
      unquote(Sequence const & _input, Char _delimiter = '"', 
         Char _escape = '\\');
   }
}

template <class OutIt, class Range, class Char>
OutIt
boost::algorithm::unquote(OutIt _out, Range & _input, Char _delimiter, 
   Char _escape)
{
   typedef typename boost::range_iterator<Range>::type iterator_type;
   typedef typename boost::range_value<Range>::type value_type;
   iterator_type it(boost::begin(_input));
   iterator_type end(boost::end(_input));
   unsigned nesting(0);
   while (it != end)
   {
      value_type ch(*it);
      if (ch == _delimiter)
      {
         ++nesting;
         ++it;
         while (it != end)
         {
            ch = *it;
            if (ch == _escape)
            {
               ++it;
               ch = *it;
               if (it == end)
               {
                  break;
               }
            }
            else if (ch == _delimiter)
            {
               --nesting;
               break;
            }
            *_out++ = ch;
            ++it;
         }
      }
      else
      {
         *_out++ = ch;
      }
      ++it;
   }
   if (nesting)
   {
      throw std::logic_error("Missing end delimiter");
   }
}

template <class Sequence, class Char>
inline Sequence
boost::algorithm::unquote(Sequence const & _input, Char _delimiter, 
   Char _escape)
{
   Sequence result;
   unquote(std::back_inserter(result), _input, _delimiter, _escape);
   return result;
}

template <class Sequence>
inline Sequence
boost::algorithm::unquote(Sequence const & _input)
{
   return unquote(_input, '"', '\\');
}

#endif


