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

#ifndef ICALMAPI_VTODO_H
#define ICALMAPI_VTODO_H

#include "vconverter.h"

class VTodoConverter : public VConverter {
public:
	/* lpNamedProps must be the GetIDsFromNames() of the array in nameids.h */
	VTodoConverter(LPADRBOOK lpAdrBook, timezone_map *mapTimeZones, LPSPropTagArray lpNamedProps, const std::string& strCharset, bool blCensor, bool bNoRecipients, IMailUser *lpImailUser);
	virtual ~VTodoConverter();

	HRESULT HrICal2MAPI(icalcomponent *lpEventRoot /* in */, icalcomponent *lpEvent /* in */, icalitem *lpPrevItem /* in */, icalitem **lppRet /* out */);
	//	HRESULT HrMAPI2ICal(LPMESSAGE lpMessage /* in */, icalproperty_method *lpicMethod /* out */, std::list<icalcomponent*> *lpEventList /* out */);

private:
	/* overrides, ical -> mapi */
	HRESULT HrAddBaseProperties(icalproperty_method icMethod, icalcomponent *lpicEvent, void *base, bool bIsException, std::list<SPropValue> *lstMsgProps);
	HRESULT HrAddTimes(icalproperty_method icMethod, icalcomponent *lpicEventRoot, icalcomponent *lpicEvent, bool bIsAllday, icalitem *lpIcalItem);

	/* overrides, mapi -> ical */
	HRESULT HrMAPI2ICal(LPMESSAGE lpMessage, icalproperty_method *lpicMethod, icaltimezone **lppicTZinfo, std::string *lpstrTZid, icalcomponent **lppEvent);
	HRESULT HrSetTimeProperties(LPSPropValue lpMsgProps, ULONG ulMsgProps, icaltimezone *lpicTZinfo, const std::string &strTZid, icalcomponent *lpEvent);
	HRESULT HrSetItemSpecifics(ULONG ulProps, LPSPropValue lpProps, icalcomponent *lpicEvent);
};

#endif
