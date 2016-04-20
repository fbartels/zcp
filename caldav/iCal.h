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

#ifndef _ICAL_H_
#define _ICAL_H_

#include "Http.h"
#include <mapi.h>
#include <zarafa/CommonUtil.h>
#include "MAPIToICal.h"
#include "ICalToMAPI.h"
#include "CalDavProto.h"

class iCal: public ProtocolBase
{
public:
	iCal(Http *lpRequest, IMAPISession *lpSession, ECLogger *lpLogger, std::string strSrvTz, std::string strCharset);
	~iCal();

	HRESULT HrHandleCommand(const std::string &strMethod);
	
private:
	HRESULT HrHandleIcalGet(const std::string &strMethod);
	HRESULT HrHandleIcalPost();
	HRESULT HrDelFolder();

	HRESULT HrGetContents(IMAPITable **lppTable);
	HRESULT HrGetIcal(LPMAPITABLE lpTable, bool blCensorPrivate, std::string *strIcal);
	HRESULT HrModify(ICalToMapi *lpIcal2Mapi, SBinary sbSrvEid, ULONG ulPos, bool blCensor);
	HRESULT HrAddMessage(ICalToMapi *lpIcal2Mapi, ULONG ulPos);
	HRESULT HrDelMessage(SBinary sbEid, bool blCensor);
};

#endif
