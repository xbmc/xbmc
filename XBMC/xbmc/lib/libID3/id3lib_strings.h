// -*- C++ -*-
// $Id$

// id3lib: a software library for creating and manipulating id3v1/v2 tags
// Copyright 1999, 2000  Scott Thomas Haug
// Copyright 2002 Thijmen Klok (thijmen@id3lib.org)

// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
// License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// The id3lib authors encourage improvements and optimisations to be sent to
// the id3lib coordinator.  Please see the README file for details on where to
// send such submissions.  See the AUTHORS file for a list of people who have
// contributed to id3lib.  See the ChangeLog file for a list of changes to
// id3lib.  These files are distributed with id3lib at
// http://download.sourceforge.net/id3lib/

#ifndef _ID3LIB_STRINGS_H_
#define _ID3LIB_STRINGS_H_

#include <string>

#if (defined(__GNUC__) && (__GNUC__ >= 3) || (defined(_MSC_VER) && _MSC_VER > 1000))
namespace std
{
  template<>
    struct char_traits<unsigned char>
    {
      typedef unsigned char char_type;
      // Unsigned as wint_t in unsigned.
      typedef unsigned long  	int_type;
      typedef streampos 	pos_type;
      typedef streamoff 	off_type;
      typedef mbstate_t 	state_type;

      static void
      assign(char_type& __c1, const char_type& __c2)
      { __c1 = __c2; }

      static bool
      eq(const char_type& __c1, const char_type& __c2)
      { return __c1 == __c2; }

      static bool
      lt(const char_type& __c1, const char_type& __c2)
      { return __c1 < __c2; }

      static int
      compare(const char_type* __s1, const char_type* __s2, size_t __n)
      {
        for (size_t __i = 0; __i < __n; ++__i)
          if (!eq(__s1[__i], __s2[__i]))
            return lt(__s1[__i], __s2[__i]) ? -1 : 1;
        return 0;
      }

      static size_t
      length(const char_type* __s)
      {
        const char_type* __p = __s;
          while (__p)
            ++__p;
        return (__p - __s);
      }

      static const char_type*
      find(const char_type* __s, size_t __n, const char_type& __a)
      {
        for (const char_type* __p = __s; size_t(__p - __s) < __n; ++__p)
          if (*__p == __a) return __p;
        return 0;
      }

      static char_type*
      move(char_type* __s1, const char_type* __s2, size_t __n)
      { return (char_type*) memmove(__s1, __s2, __n * sizeof(char_type)); }

      static char_type*
      copy(char_type* __s1, const char_type* __s2, size_t __n)
      { return (char_type*) memcpy(__s1, __s2, __n * sizeof(char_type)); }

      static char_type*
      assign(char_type* __s, size_t __n, char_type __a)
      {
        for (char_type* __p = __s; __p < __s + __n; ++__p)
          assign(*__p, __a);
        return __s;
      }

      static char_type
      to_char_type(const int_type& __c)
      { return char_type(); }

      static int_type
      to_int_type(const char_type& __c) { return int_type(); }

      static bool
      eq_int_type(const int_type& __c1, const int_type& __c2)
      { return __c1 == __c2; }

      static int_type
      eof() { return static_cast<int_type>(-1); }

      static int_type
      not_eof(const int_type& __c)
      { return eq_int_type(__c, eof()) ? int_type(0) : __c; }
    };

#ifndef _GLIBCPP_USE_WCHAR_T
#if (defined(ID3_NEED_WCHAR_TEMPLATE))
   template<>
     struct char_traits<wchar_t>
     {
       typedef wchar_t 		char_type;
       typedef wint_t 		int_type;
       typedef streamoff 	off_type;
       typedef streampos 	pos_type;
       typedef mbstate_t 	state_type;
       
       static void 
       assign(char_type& __c1, const char_type& __c2)
       { __c1 = __c2; }
 
       static bool 
       eq(const char_type& __c1, const char_type& __c2)
       { return __c1 == __c2; }
 
       static bool 
       lt(const char_type& __c1, const char_type& __c2)
       { return __c1 < __c2; }
 
       static int 
       compare(const char_type* __s1, const char_type* __s2, size_t __n)
       { return wmemcmp(__s1, __s2, __n); }
 
       static size_t
       length(const char_type* __s)
       { return wcslen(__s); }
 
       static const char_type* 
       find(const char_type* __s, size_t __n, const char_type& __a)
       { return wmemchr(__s, __a, __n); }
 
       static char_type* 
       move(char_type* __s1, const char_type* __s2, int_type __n)
       { return wmemmove(__s1, __s2, __n); }
 
       static char_type* 
       copy(char_type* __s1, const char_type* __s2, size_t __n)
       { return wmemcpy(__s1, __s2, __n); }
 
       static char_type* 
       assign(char_type* __s, size_t __n, char_type __a)
       { return wmemset(__s, __a, __n); }
 
       static char_type 
       to_char_type(const int_type& __c) { return char_type(__c); }
 
       static int_type 
       to_int_type(const char_type& __c) { return int_type(__c); }
 
       static bool 
       eq_int_type(const int_type& __c1, const int_type& __c2)
       { return __c1 == __c2; }
 
       static state_type 
       _S_get_state(const pos_type& __pos) { return __pos.state(); }
 
       static int_type 
       eof() { return static_cast<int_type>(WEOF); }
 
       static int_type 
       _S_eos() { return char_type(); }
 
       static int_type 
       not_eof(const int_type& __c)
       { return eq_int_type(__c, eof()) ? 0 : __c; }
   };
#endif
#endif
} // namespace std
#endif

namespace dami
{
  typedef std::basic_string<char>           String;
  typedef std::basic_string<unsigned char> BString;
  typedef std::basic_string<wchar_t>       WString;
};

#endif /* _ID3LIB_STRINGS_H_ */

