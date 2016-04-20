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

#ifndef ECWSUTIL_H
#define ECWSUTIL_H

#include <mapidefs.h>
#include <mapicode.h>
#include "Zarafa.h"
#include <zarafa/ZarafaCode.h>

#include "soapZarafaCmdProxy.h"

#include "ECMsgStore.h"

class convert_context;

HRESULT CopyMAPIPropValToSOAPPropVal(propVal *lpPropValDst, const SPropValue *lpPropValSrc, convert_context *lpConverter = NULL);
HRESULT CopySOAPPropValToMAPIPropVal(LPSPropValue lpPropValDst, const struct propVal *lpPropValSrc, void *lpBase, convert_context *lpConverter = NULL);
HRESULT CopySOAPRowToMAPIRow(void *lpProvider, const struct propValArray *lpsRowSrc, LPSPropValue lpsRowDst, void **lpBase, ULONG ulType, convert_context *lpConverter = NULL);
HRESULT CopySOAPRowSetToMAPIRowSet(void *lpProvider, const struct rowSet *lpsRowSetSrc, LPSRowSet *lppRowSetDst, ULONG ulType);
HRESULT CopySOAPRestrictionToMAPIRestriction(LPSRestriction lpDst, const struct restrictTable *lpSrc, void *lpBase, convert_context *lpConverter = NULL);

HRESULT CopyMAPIRestrictionToSOAPRestriction(struct restrictTable **lppDst, const SRestriction *lpSrc, convert_context *lpConverter = NULL);
HRESULT CopyMAPIRowSetToSOAPRowSet(const SRowSet *lpRowSetSrc, struct rowSet **lppsRowSetDst, convert_context *lpConverter = NULL);
HRESULT CopyMAPIRowToSOAPRow(const SRow *lpRowSrc, struct propValArray *lpsRowDst, convert_context *lpConverter = NULL);
HRESULT CopySOAPRowToMAPIRow(const struct propValArray *lpsRowSrc, LPSPropValue lpsRowDst, void *lpBase, convert_context *lpConverter = NULL);

HRESULT CopySOAPEntryId(const entryId *lpSrc, entryId *lpDest);
HRESULT CopyMAPIEntryIdToSOAPEntryId(ULONG cbEntryIdSrc, const ENTRYID *lpEntryIdSrc, entryId **lppDest);
HRESULT CopyMAPIEntryIdToSOAPEntryId(ULONG cbEntryIdSrc, const ENTRYID *lpEntryIdSrc, entryId *lpDest, bool bCheapCopy = false);
HRESULT CopySOAPEntryIdToMAPIEntryId(const entryId *lpSrc, ULONG *lpcbDest, LPENTRYID *lppEntryIdDest, void *lpBase = NULL);
HRESULT CopySOAPEntryIdToMAPIEntryId(const entryId *lpSrc, ULONG ulObjId, ULONG *lpcbDest, LPENTRYID *lppEntryIdDest, void *lpBase = NULL);
HRESULT CopySOAPEntryIdToMAPIEntryId(const entryId *lpSrc, ULONG ulObjId, ULONG ulType, ULONG *lpcbDest, LPENTRYID *lppEntryIdDest, void *lpBase = NULL);
HRESULT CopyMAPIEntryListToSOAPEntryList(const ENTRYLIST *lpMsgList, struct entryList *lpsEntryList);
HRESULT CopySOAPEntryListToMAPIEntryList(const struct entryList *lpsEntryList, LPENTRYLIST *lppMsgList);
HRESULT CopyUserClientUpdateStatusFromSOAP(struct userClientUpdateStatusResponse &sUCUS, ULONG ulFlags, ECUSERCLIENTUPDATESTATUS **lppECUCUS);

HRESULT CopySOAPPropTagArrayToMAPIPropTagArray(const struct propTagArray *lpsPropTagArray, LPSPropTagArray *lppPropTagArray, void *lpBase = NULL);

HRESULT FreeABProps(struct propmapPairArray *lpsoapPropmap, struct propmapMVPairArray *lpsoapMVPropmap);
HRESULT CopyABPropsToSoap(const SPROPMAP *lpPropmap, const MVPROPMAP *lpMVPropmap, ULONG ulFlags, 
						  struct propmapPairArray **lppsoapPropmap, struct propmapMVPairArray **lppsoapMVPropmap);
HRESULT CopyABPropsFromSoap(const struct propmapPairArray *lpsoapPropmap, const struct propmapMVPairArray *lpsoapMVPropmap,
							SPROPMAP *lpPropmap, MVPROPMAP *lpMVPropmap, void *lpBase, ULONG ulFlags);

HRESULT SoapUserArrayToUserArray(const struct userArray *lpUserArray, ULONG ulFLags, ULONG *lpcUsers, ECUSER **lppsUsers);
HRESULT SoapUserToUser(const struct user *lpUser, ULONG ulFLags, ECUSER **lppsUser);

HRESULT SoapGroupArrayToGroupArray(const struct groupArray *lpGroupArray, ULONG ulFLags, ULONG *lpcGroups, ECGROUP **lppsGroups);
HRESULT SoapGroupToGroup(const struct group *lpGroup, ULONG ulFLags, ECGROUP **lppsGroup);

HRESULT SoapCompanyArrayToCompanyArray(const struct companyArray *lpCompanyArray, ULONG ulFLags, ULONG *lpcCompanies, ECCOMPANY **lppsCompanies);
HRESULT SoapCompanyToCompany(const struct company *lpCompany, ULONG ulFLags, ECCOMPANY **lppsCompany);

HRESULT SvrNameListToSoapMvString8(ECSVRNAMELIST *lpSvrNameList, ULONG ulFLags, struct mv_string8 **lppsSvrNameList);
HRESULT SoapServerListToServerList(const struct serverList *lpsServerList, ULONG ulFLags, ECSERVERLIST **lppServerList);

#ifdef LINUX
int gsoap_connect_unixsocket(struct soap *soap, const char *endpoint, const char *host, int port);
#else
int gsoap_win_fsend(struct soap *soap, const char *s, size_t n);
size_t gsoap_win_frecv(struct soap *soap, char *s, size_t n);
int gsoap_win_fclose(struct soap *soap);
int gsoap_connect_namedpipe(struct soap *soap, const char *endpoint, const char *host, int port);
#endif

HRESULT CreateSoapTransport(ULONG ulUIFlags, const sGlobalProfileProps &sProfileProps, ZarafaCmd **const lppCmd);

HRESULT WrapServerClientStoreEntry(const char* lpszServerName, entryId* lpsStoreId, ULONG* lpcbStoreID, LPENTRYID* lppStoreID);
HRESULT UnWrapServerClientStoreEntry(ULONG cbWrapStoreID, LPENTRYID lpWrapStoreID, ULONG* lpcbUnWrapStoreID, LPENTRYID* lppUnWrapStoreID);
HRESULT UnWrapServerClientABEntry(ULONG cbWrapABID, LPENTRYID lpWrapABID, ULONG* lpcbUnWrapABID, LPENTRYID* lppUnWrapABID);
HRESULT	CopySOAPNotificationToMAPINotification(void *lpProvider, struct notification *lpSrc, LPNOTIFICATION *lppDst, convert_context *lpConverter = NULL);
HRESULT CopySOAPChangeNotificationToSyncState(struct notification *lpSrc, LPSBinary *lppDst, void *lpBase);

HRESULT CopyMAPISourceKeyToSoapSourceKey(SBinary *lpsSourceKey, struct xsd__base64Binary **lppsSourceKey, void *lpBase);
HRESULT CopyICSChangeToSOAPSourceKeys(ULONG cbChanges, ICSCHANGE *lpsChanges, sourceKeyPairArray **lppsSKPA);

HRESULT Utf8ToTString(LPCSTR lpszUtf8, ULONG ulFlags, LPVOID lpBase, convert_context *lpConverter, LPTSTR *lppszTString);
HRESULT TStringToUtf8(LPCTSTR lpszTstring, ULONG ulFlags, LPVOID lpBase, convert_context *lpConverter, LPSTR *lppszUtf8);

HRESULT ConvertString8ToUnicode(LPSRowSet lpRowSet);
HRESULT ConvertString8ToUnicode(LPSRow lpRow, void *base, convert_context &converter);

#endif


