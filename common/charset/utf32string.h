/*
 * Copyright 2005 - 2015  Zarafa B.V. and its licensors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef utf32string_INCLUDED
#define utf32string_INCLUDED

#include <zarafa/zcdefs.h>
#include <zarafa/charset/traits.h>

#ifdef LINUX
// The utf32string type
typedef std::wstring utf32string;

#else
// The utf32string type
typedef std::basic_string<unsigned int> utf32string;

namespace std {

template<>
    struct char_traits<unsigned int>
    {
      typedef unsigned int		char_type;
      typedef wint_t            int_type;
      typedef streamoff         off_type;
      typedef wstreampos        pos_type;
      typedef mbstate_t         state_type;

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
      { while(*__s1 != 0 && *__s2 != 0 && __n != 0) { 
		if(*__s1 != *__s2) return *__s1 - *__s2;
		++__s1;
		++__s2;
		--__n;
	}
	if((*__s1 == 0 && *__s2 == 0) || __n == 0) return 0;
	if(*__s1 != 0) return -1;
	return 1;
      }

      static size_t
      length(const char_type* __s)
      { int l = 0;
	while (*__s != 0) { ++l; ++__s; }
	return l; 
      }

      static const char_type* 
      find(const char_type* __s, size_t __n, const char_type& __a)
      { while(*__s != 0 && __n > 0) { 
	  if(*__s == __a) return __s; 
	  ++__s; --__n;
	}
	return NULL;
      }

      static char_type* 
      move(char_type* __s1, const char_type* __s2, int_type __n)
      { return (unsigned int *)memmove((void *)__s1, (void *)__s2, __n*sizeof(unsigned int)); }

      static char_type* 
      copy(char_type* __s1, const char_type* __s2, size_t __n)
      { return (unsigned int *)memcpy((void *)__s1, (void *)__s2, __n*sizeof(unsigned int)); }

      static char_type* 
      assign(char_type* __s, size_t __n, char_type __a)
      { char_type *__o = __s;
	while (__n > 0) { *__s = __a; --__n; ++__s; }
	return  __o;
      }

      static char_type 
      to_char_type(const int_type& __c) { return char_type(__c); }

      static int_type 
      to_int_type(const char_type& __c) { return int_type(__c); }

      static bool 
      eq_int_type(const int_type& __c1, const int_type& __c2)
      { return __c1 == __c2; }

      static int_type 
      eof() { return static_cast<int_type>(-1); }

      static int_type 
      not_eof(const int_type& __c)
      { return eq_int_type(__c, eof()) ? 0 : __c; }
  };
}

// 32-bit character specializations
template <>
class iconv_charset<utf32string> _zcp_final {
public:
	static const char *name() {
		return "UTF-32LE";
	}
	static const char *rawptr(const utf32string &from) {
		return reinterpret_cast<const char*>(from.c_str());
	}
	static size_t rawsize(const utf32string &from) {
		return from.size() * sizeof(utf32string::value_type);
	}
};

template <>
class iconv_charset<unsigned int *> _zcp_final {
public:
	static const char *name() {
		return "UTF32LE//TRANSLIT";
	}
	static const char *rawptr(const unsigned int *from) {
		return reinterpret_cast<const char*>(from);
	}
	static size_t rawsize(const unsigned int *from);
};

template <>
class iconv_charset<const unsigned int *> _zcp_final {
public:
	static const char *name() {
		return "UTF32LE//TRANSLIT";
	}
	static const char *rawptr(const unsigned int *from) {
		return reinterpret_cast<const char*>(from);
	}
	static size_t rawsize(const unsigned int *from);
};

#endif

#endif // ndef utf32string_INCLUDED
