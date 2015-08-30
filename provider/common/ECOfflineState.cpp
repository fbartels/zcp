/*
 * Copyright 2005 - 2015  Zarafa B.V. and its licensors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation with the following
 * additional terms according to sec. 7:
 *
 * "Zarafa" is a registered trademark of Zarafa B.V.
 * The licensing of the Program under the AGPL does not imply a trademark
 * license. Therefore any rights, title and interest in our trademarks
 * remain entirely with us.
 *
 * Our trademark policy (see TRADEMARKS.txt) allows you to use our trademarks
 * in connection with Propagation and certain other acts regarding the Program.
 * In any case, if you propagate an unmodified version of the Program you are
 * allowed to use the term "Zarafa" to indicate that you distribute the Program.
 * Furthermore you may use our trademarks where it is necessary to indicate the
 * intended purpose of a product or service provided you use it in accordance
 * with honest business practices. For questions please contact Zarafa at
 * trademark@zarafa.com.
 *
 * The interactive user interface of the software displays an attribution
 * notice containing the term "Zarafa" and/or the logo of Zarafa.
 * Interactive user interfaces of unmodified and modified versions must
 * display Appropriate Legal Notices according to sec. 5 of the GNU Affero
 * General Public License, version 3, when you propagate unmodified or
 * modified versions of the Program. In accordance with sec. 7 b) of the GNU
 * Affero General Public License, version 3, these Appropriate Legal Notices
 * must retain the logo of Zarafa or display the words "Initial Development
 * by Zarafa" if the display of the logo is not reasonably feasible for
 * technical reasons.
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

#include "platform.h"
#ifdef HAVE_OFFLINE_SUPPORT

#include <mapidefs.h>
#include <ZarafaCode.h>
#include <edkguid.h>

#include "ECOfflineState.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// We normally store our offline state in the Outlook offline state API, which is represented by the
// 'work offline' button in outlook. However, if this is unavailable, we have to store it somewhere else,
// which should be accessible for all processes running the current profile, so we use the global profile
// for that information, which is shared between all processes working on this profile.

static ECOfflineState::OFFLINESTATE	g_ulOfflineState;


HRESULT ECOfflineState::SetOfflineState(const std::string &strProfname, ECOfflineState::OFFLINESTATE state)
{
	HRESULT hr = hrSuccess;
	IMAPIOffline *lpOffline = NULL;

	if(GetIMAPIOffline(strProfname, &lpOffline) == hrSuccess) {
		ULONG ulState = state == ECOfflineState::OFFLINESTATE_ONLINE ? MAPIOFFLINE_STATE_ONLINE : MAPIOFFLINE_STATE_OFFLINE;
		lpOffline->SetCurrentState(MAPIOFFLINE_FLAG_DEFAULT, MAPIOFFLINE_STATE_OFFLINE_MASK, ulState, NULL);
	}

	// Remember the state ourselves too, just in case we need it later
	g_ulOfflineState = state;

	return hr;
}

HRESULT ECOfflineState::GetOfflineState(const std::string &strProfName, ECOfflineState::OFFLINESTATE *state)
{
	HRESULT hr = hrSuccess;
	IMAPIOffline *lpOffline = NULL;

	// Try to get the offline state from the IMAPIOffline object. If that fails, just return
	// our internal state value (g_ulOfflineState)

	if(GetIMAPIOffline(strProfName, &lpOffline) == hrSuccess) {
		ULONG ulState = 0;
		if(lpOffline->GetCurrentState(&ulState) == hrSuccess) {
			if(ulState == MAPIOFFLINE_STATE_OFFLINE)
				*state = ECOfflineState::OFFLINESTATE_OFFLINE;
			else if(ulState == MAPIOFFLINE_STATE_ONLINE)
				*state = ECOfflineState::OFFLINESTATE_ONLINE;
		} else {
			*state = g_ulOfflineState;
		}
	} else {
		*state = g_ulOfflineState;
	}

	return hr;
}

HRESULT ECOfflineState::GetIMAPIOffline(const std::string &strProfname, IMAPIOffline **lppOffline)
{
#ifdef WIN32
	HRESULT hr = hrSuccess;
	HMODULE hLib = 0;
	WCHAR wProfName[256];

	typedef HRESULT (STDMETHODCALLTYPE HROPENOFFLINEOBJ)(
		ULONG ulReserved,
		LPCWSTR pwszProfileNameIn,
		const GUID* pGUID,
		const GUID* pReserved,
		IMAPIOfflineMgr** ppOfflineObj);

	HROPENOFFLINEOBJ *HrOpenOfflineObj = NULL;
	IMAPIOfflineMgr *lpOfflineMgr = NULL;
	IMAPIOffline *lpOffline = NULL;

	hLib = LoadLibrary(_T("msmapi32.dll"));
	if(!hLib) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	HrOpenOfflineObj = (HROPENOFFLINEOBJ *)GetProcAddress(hLib, "HrOpenOfflineObj@20");
	if(!HrOpenOfflineObj) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	memset(wProfName, 0, sizeof(wProfName));
	mbstowcs(wProfName, strProfname.c_str(), ARRAY_SIZE(wProfName) - 1);

	hr = HrOpenOfflineObj(0, wProfName, &GUID_GlobalState, NULL, &lpOfflineMgr);
	if(hr != hrSuccess)
		goto exit;

	hr = lpOfflineMgr->QueryInterface(IID_IMAPIOffline, (void **)&lpOffline);
	if(hr != hrSuccess)
		goto exit;

	*lppOffline = lpOffline;
exit:
	if(lpOfflineMgr)
		lpOfflineMgr->Release();

	if(hLib)
		FreeLibrary(hLib);

	return hr;
#else
	return MAPI_E_NOT_FOUND;
#endif
}
#endif
