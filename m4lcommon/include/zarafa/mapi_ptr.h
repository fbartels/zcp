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

#ifndef mapi_ptr_INCLUDED
#define mapi_ptr_INCLUDED

#include <zarafa/mapi_ptr/mapi_object_ptr.h>
#include <zarafa/mapi_ptr/mapi_memory_ptr.h>
#include <zarafa/mapi_ptr/mapi_array_ptr.h>
#include <zarafa/mapi_ptr/mapi_rowset_ptr.h>

#include <mapix.h>
#include <mapispi.h>
#include <edkmdb.h>
#include <edkguid.h>

#include <zarafa/IECServiceAdmin.h>
#include <zarafa/IECSecurity.h>
#include <zarafa/IECSingleInstance.h>
#include <zarafa/ECGuid.h>
#include <zarafa/mapiguidext.h>

typedef mapi_object_ptr<IABContainer, IID_IABContainer> ABContainerPtr;
typedef mapi_object_ptr<IAddrBook, IID_IAddrBook> AddrBookPtr;
typedef mapi_object_ptr<IDistList, IID_IDistList> DistListPtr;
typedef mapi_object_ptr<IECSecurity, IID_IECSecurity> ECSecurityPtr;
typedef mapi_object_ptr<IECServiceAdmin, IID_IECServiceAdmin> ECServiceAdminPtr;
typedef mapi_object_ptr<IECSingleInstance, IID_IECSingleInstance> ECSingleInstancePtr;
typedef mapi_object_ptr<IExchangeManageStore, IID_IExchangeManageStore> ExchangeManageStorePtr;
typedef mapi_object_ptr<IExchangeModifyTable, IID_IExchangeModifyTable> ExchangeModifyTablePtr;
typedef mapi_object_ptr<IExchangeExportChanges, IID_IExchangeExportChanges> ExchangeExportChangesPtr;
typedef mapi_object_ptr<IMAPIAdviseSink, IID_IMAPIAdviseSink> MAPIAdviseSinkPtr;
typedef mapi_object_ptr<IMAPIContainer, IID_IMAPIContainer> MAPIContainerPtr;
typedef mapi_object_ptr<IMAPIFolder, IID_IMAPIFolder> MAPIFolderPtr;
typedef mapi_object_ptr<IMAPIProp, IID_IMAPIProp> MAPIPropPtr;
typedef mapi_object_ptr<IMAPISession, IID_IMAPISession> MAPISessionPtr;
typedef mapi_object_ptr<IMAPITable, IID_IMAPITable> MAPITablePtr;
typedef mapi_object_ptr<IMailUser, IID_IMailUser> MailUserPtr;
typedef mapi_object_ptr<IMessage, IID_IMessage> MessagePtr;
typedef mapi_object_ptr<IMsgServiceAdmin, IID_IMsgServiceAdmin> MsgServiceAdminPtr;
typedef mapi_object_ptr<IMsgStore, IID_IMsgStore> MsgStorePtr;
typedef mapi_object_ptr<IProfAdmin, IID_IProfAdmin> ProfAdminPtr;
typedef mapi_object_ptr<IProfSect, IID_IProfSect> ProfSectPtr;
typedef mapi_object_ptr<IProviderAdmin, IID_IProviderAdmin> ProviderAdminPtr;
typedef mapi_object_ptr<IUnknown, IID_IUnknown> UnknownPtr;
typedef mapi_object_ptr<IStream, IID_IStream> StreamPtr;
typedef mapi_object_ptr<IAttach, IID_IAttachment> AttachPtr;
typedef mapi_object_ptr<IMAPIGetSession, IID_IMAPIGetSession> MAPIGetSessionPtr;

typedef mapi_memory_ptr<ECPERMISSION> ECPermissionPtr;
typedef mapi_memory_ptr<ENTRYID> EntryIdPtr;
typedef mapi_memory_ptr<ENTRYLIST> EntryListPtr;
typedef mapi_memory_ptr<MAPIERROR> MAPIErrorPtr;
typedef mapi_memory_ptr<ROWLIST> RowListPtr;
typedef mapi_memory_ptr<SPropProblemArray> SPropProblemArrayPtr;
typedef mapi_memory_ptr<SPropValue> SPropValuePtr;
typedef mapi_memory_ptr<SPropTagArray> SPropTagArrayPtr;
typedef mapi_memory_ptr<SRestriction> SRestrictionPtr;
typedef mapi_memory_ptr<SRow> SRowPtr;
typedef mapi_memory_ptr<SSortOrderSet> SSortOrderSetPtr;
typedef mapi_memory_ptr<char> StringPtr;
typedef mapi_memory_ptr<WCHAR> WStringPtr;
typedef mapi_memory_ptr<FlagList> FlagListPtr;
typedef mapi_memory_ptr<SBinary> SBinaryPtr;
typedef mapi_memory_ptr<BYTE> BytePtr;
typedef mapi_memory_ptr<MAPINAMEID> MAPINameIdPtr;

typedef mapi_array_ptr<ECPERMISSION> ECPermissionArrayPtr;
typedef mapi_array_ptr<SPropValue> SPropArrayPtr;

typedef mapi_rowset_ptr<SRow> SRowSetPtr;
typedef mapi_rowset_ptr<ADRENTRY> AdrListPtr;

#ifdef WIN32
#include <mapiform.h>
typedef mapi_object_ptr<IMAPIFormContainer, IID_IMAPIFormContainer> MAPIFormContainerPtr;
#endif

#endif // ndef mapi_ptr_INCLUDED
