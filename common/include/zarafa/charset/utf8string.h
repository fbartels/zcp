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

#ifndef utf8string_INCLUDED
#define utf8string_INCLUDED

#include <zarafa/zcdefs.h>
#include <string>
#include <stdexcept>

#include <zarafa/charset/traits.h>

/**
 * @brief	This class represents an UTF-8 string.
 *
 * This class does not expose the same methods as STL's std::string as most of those don't make
 * much sense.
 */
class utf8string _zcp_final {
public:
	typedef std::string::value_type		value_type;
	typedef std::string::const_pointer	const_pointer;
	typedef std::string::size_type		size_type;
	
	static utf8string from_string(const std::string &str) {
		utf8string s;
		s.m_str.assign(str);
		return s;
	}

	static utf8string null_string() {
		utf8string s;
		s.set_null();
		return s;
	}

	utf8string(): m_bNull(false) {}
	utf8string(const utf8string &other): m_bNull(other.m_bNull), m_str(other.m_str) {}
	utf8string(size_t n, char c): m_bNull(false), m_str(n, c) {}
	
	utf8string &operator=(const utf8string &other) {
		if (this != &other) {
			m_bNull = other.m_bNull;
			m_str = other.m_str;
		}
			
		return *this;
	}

	const_pointer c_str() const {
		return m_bNull ? NULL : m_str.c_str();
	}

	const_pointer data() const {
		return m_bNull ? NULL : m_str.data();
	}
	
	const std::string &str() const {
		return m_str;
	}

	size_type size() const {
		return m_str.size();
	}
	
	bool empty() const {
		return m_str.empty();
	}

	size_type length() const {
		return m_str.length();
	}
	
	void append(const_pointer lpData, size_t cbData) {
		m_str.append(lpData, cbData);
		m_bNull = false;
	}

	void append(const utf8string &str) {
		m_str.append(str.m_str);
		m_bNull = false;
	}
	
	void clear() {
		m_str.clear();
	}

	void set_null() {
		clear();
		m_bNull = true;
	}
	
private:
	bool m_bNull;
	std::string	m_str;
};

template <>
class iconv_charset<utf8string> _zcp_final {
public:
	static const char *name() {
		return "UTF-8";
	}
	static const char *rawptr(const utf8string &from) {
		return reinterpret_cast<const char*>(from.c_str());
	}
	static size_t rawsize(const utf8string &from) {
		return from.size();
	}
};


#endif //ndef utf8string_INCLUDED
