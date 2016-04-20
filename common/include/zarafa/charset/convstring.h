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

#ifndef convstring_INCLUDED
#define convstring_INCLUDED

#include <zarafa/zcdefs.h>
#include <zarafa/charset/convert.h>
#include <zarafa/tstring.h>
#include <string>
#include <zarafa/charset/utf8string.h>

#include <mapidefs.h>

class convstring _zcp_final {
public:
	static convstring from_SPropValue(const SPropValue *lpsPropVal, bool bCheapCopy = true);
	static convstring from_SPropValue(const SPropValue &sPropVal, bool bCheapCopy = true);

	convstring();
	convstring(const convstring &other);
	convstring(const char *lpsz, bool bCheapCopy = true);
	convstring(const wchar_t *lpsz, bool bCheapCopy = true);
	convstring(const TCHAR *lpsz, ULONG ulFlags, bool bCheapCopy = true);
	
	bool null_or_empty() const;
	
	operator utf8string() const;
	operator std::string() const;
	operator std::wstring() const;
	const char *c_str() const;
	const wchar_t *wc_str() const;
	const char *u8_str() const;

#ifdef UNICODE
	#define t_str	wc_str
#else
	#define t_str	c_str
#endif
	
private:
	template<typename T>
	T convert_to() const;
	
	template<typename T>
	T convert_to(const char *tocode) const;

private:
	const TCHAR *m_lpsz;
	ULONG		m_ulFlags;
	tstring		m_str;

	mutable convert_context	m_converter;
};

#endif // ndef convstring_INCLUDED
