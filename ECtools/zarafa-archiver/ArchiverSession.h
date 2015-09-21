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

#ifndef ARCHIVERSESSION_H_INCLUDED
#define ARCHIVERSESSION_H_INCLUDED

#include "ArchiverSessionPtr.h"
#include <zarafa/mapi_ptr.h>
#include <zarafa/tstring.h>
#include "archiver-common.h"

// Forward declarations
class ECConfig;
class ECLogger;

/**
 * The ArchiverSession class wraps the MAPISession an provides commonly used operations. It also
 * checks the license. This way the license doesn't need to be checked all over the place.
 */
class ArchiverSession
{
public:
	static HRESULT Create(ECConfig *lpConfig, ECLogger *lpLogger, ArchiverSessionPtr *lpptrSession);
	static HRESULT Create(const MAPISessionPtr &ptrSession, ECLogger *lpLogger, ArchiverSessionPtr *lpptrSession);
	static HRESULT Create(const MAPISessionPtr &ptrSession, ECConfig *lpConfig, ECLogger *lpLogger, ArchiverSessionPtr *lpptrSession);
	~ArchiverSession();
	
	HRESULT OpenStoreByName(const tstring &strUser, LPMDB *lppMsgStore);
	HRESULT OpenStore(const entryid_t &sEntryId, ULONG ulFlags, LPMDB *lppMsgStore);
	HRESULT OpenStore(const entryid_t &sEntryId, LPMDB *lppMsgStore);
	HRESULT OpenReadOnlyStore(const entryid_t &sEntryId, LPMDB *lppMsgStore);
	HRESULT GetUserInfo(const tstring &strUser, abentryid_t *lpsEntryId, tstring *lpstrFullname, bool *bAclCapable);
	HRESULT GetUserInfo(const abentryid_t &sEntryId, tstring *lpstrUser, tstring *lpstrFullname);
	HRESULT GetGAL(LPABCONT *lppAbContainer);
	HRESULT CompareStoreIds(LPMDB lpUserStore, LPMDB lpArchiveStore, bool *lpbResult);
	HRESULT CompareStoreIds(const entryid_t &sEntryId1, const entryid_t &sEntryId2, bool *lpbResult);
	HRESULT ServerIsLocal(const std::string &strServername, bool *lpbResult);
	
	HRESULT CreateRemote(const char *lpszServerPath, ECLogger *lpLogger, ArchiverSessionPtr *lpptrSession);

	HRESULT OpenMAPIProp(ULONG cbEntryID, LPENTRYID lpEntryID, LPMAPIPROP *lppProp);

	HRESULT OpenOrCreateArchiveStore(const tstring& strUserName, const tstring& strServerName, LPMDB *lppArchiveStore);
	HRESULT GetArchiveStoreEntryId(const tstring& strUserName, const tstring& strServerName, entryid_t *lpArchiveId);

	IMAPISession *GetMAPISession() const;
	const char *GetSSLPath() const;
	const char *GetSSLPass() const;

	HRESULT ValidateArchiverLicense() const;

private:
	ArchiverSession(ECLogger *lpLogger);
	HRESULT Init(ECConfig *lpConfig);
	HRESULT Init(const char *lpszServerPath, const char *lpszSslPath, const char *lpszSslPass);
	HRESULT Init(const MAPISessionPtr &ptrSession, const char *lpszSslPath, const char *lpszSslPass);

	HRESULT CreateArchiveStore(const tstring& strUserName, const tstring& strServerName, LPMDB *lppArchiveStore);

private:
	MAPISessionPtr	m_ptrSession;
	MsgStorePtr		m_ptrAdminStore;
	ECLogger		*m_lpLogger;
	
	std::string		m_strSslPath;
	std::string		m_strSslPass;
};

#endif // !defined ARCHIVERSESSION_H_INCLUDED
