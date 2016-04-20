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

#include <zarafa/platform.h>
#include <zarafa/ECDebugPrint.h>
#include <zarafa/ECDebug.h>
#include <zarafa/charset/convert.h>
#include <zarafa/stringutil.h>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif


namespace details {
	string conversion_helpers<string>::convert_from(const wstring &s) {
		return convert_to<string>(s);
	}

	string conversion_helpers<string>::stringify(LPCVOID lpVoid) {
		if(!lpVoid) return "NULL";

		char szBuff[33];
		sprintf(szBuff, "0x%p", lpVoid);
		return szBuff;
	}

	const string conversion_helpers<string>::strNULL = "NULL";
	const string conversion_helpers<string>::strCOMMA= ",";


	wstring conversion_helpers<wstring>::convert_from(const string &s) {
		return convert_to<wstring>(s);
	}

	wstring conversion_helpers<wstring>::stringify(LPCVOID lpVoid) {
		if(!lpVoid) return L"NULL";

		wchar_t szBuff[33];
#if WIN32
		swprintf(szBuff, L"0x%p", lpVoid);
#else
		swprintf(szBuff, ARRAY_SIZE(szBuff), L"0x%p", lpVoid);
#endif
		return szBuff;
	}

	const wstring conversion_helpers<wstring>::strNULL = L"NULL";
	const wstring conversion_helpers<wstring>::strCOMMA= L",";
} // namespace details
