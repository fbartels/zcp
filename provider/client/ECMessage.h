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

#ifndef ECMESSAGE_H
#define ECMESSAGE_H

#include <zarafa/zcdefs.h>
#include <zarafa/ECMemTable.h>

#include <mapidefs.h>
#include "WSTransport.h"
#include "ECMsgStore.h"
#include "ECMAPIProp.h"

class ECAttach;
class IAttachFactory {
public:
	virtual HRESULT Create(ECMsgStore *lpMsgStore, ULONG ulObjType, BOOL fModify, ULONG ulAttachNum, ECMAPIProp *lpRoot, ECAttach **lppAttach) const = 0;
};

/**
 * \brief Represents a MAPI message.
 *
 * This class represents any kind of MAPI message and exposes it through
 * the IMessage interface.
 */
class ECMessage : public ECMAPIProp {
protected:
	/**
	 * \brief Constructor
	 *
	 * \param lpMsgStore	The store owning this message.
	 * \param fNew			Specifies whether the message is a new message.
	 * \param fModify		Specifies whether the message is writable.
	 * \param ulFlags		Flags.
	 * \param bEmbedded		Specifies whether the message is embedded.
	 * \param lpRoot		The parent object when the message is embedded.
	 */
	ECMessage(ECMsgStore *lpMsgStore, BOOL fNew, BOOL fModify, ULONG ulFlags, BOOL bEmbedded, ECMAPIProp *lpRoot);
	virtual ~ECMessage();

public:
	/**
	 * \brief Creates a new ECMessage object.
	 *
	 * Use this static method to create a new ECMessage object.
	 *
	 * \param lpMsgStore	The store owning this message.
	 * \param fNew			Specifies whether the message is a new message.
	 * \param fModify		Specifies whether the message is writable.
	 * \param ulFlags		Flags.
	 * \param bEmbedded		Specifies whether the message is embedded.
	 * \param lpRoot		The parent object when the message is embedded.
	 * \param lpMessage		Pointer to a pointer in which the create object will be returned.
	 *
	 * \return hrSuccess on success.
	 */
	static HRESULT	Create(ECMsgStore *lpMsgStore, BOOL fNew, BOOL fModify, ULONG ulFlags, BOOL bEmbedded, ECMAPIProp *lpRoot, ECMessage **lpMessage);

	/**
	 * \brief Handles GetProp requests for previously registered properties.
	 *
	 * Properties can be registered through ECGenericProp::HrAddPropHandlers to be obtained through 
	 * this function when special processing is needed. 
	 *
	 * \param[in] ulPropTag		The proptag of the requested property.
	 * \param[in] lpProvider	The provider for the requested property (Probably an ECMsgStore pointer).
	 * \param[out] lpsPropValue	Pointer to an SPropValue structure in which the result will be stored.
	 * \param[in] lpParam		Pointer passed to ECGenericProp::HrAddPropHandlers (usually an ECMessage pointer).
	 * \param[in] lpBase		Base pointer used for allocating more memory
	 *
	 * \return hrSuccess on success.
	 */
	static HRESULT	GetPropHandler(ULONG ulPropTag, void* lpProvider, ULONG ulFlags, LPSPropValue lpsPropValue, void *lpParam, void *lpBase);
	
	/**
	 * \brief Handles SetProp requests for previously registered properties.
	 *
	 * Properties can be registered through ECGenericProp::HrAddPropHandlers to be set through
	 * this function when special processing is needed.
	 *
	 * \param ulPropTag		The proptag of the requested property.
	 * \param lpProvider	The provider for the requested property (Probably an ECMsgStore pointer).
	 * \param lpsPropValue	Pointer to an SPropValue structure which holds the data to be set.
	 * \param lpParam		Pointer passed to ECGenericProp::HrAddPropHandlers (usually an ECMessage pointer).
	 *
	 * \return hrSuccess on success.
	 */
	static HRESULT	SetPropHandler(ULONG ulPropTag, void* lpProvider, LPSPropValue lpsPropValue, void *lpParam);

	virtual HRESULT	QueryInterface(REFIID refiid, void **lppInterface);

	virtual HRESULT OpenProperty(ULONG ulPropTag, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulFlags, LPUNKNOWN FAR * lppUnk);
	virtual HRESULT GetAttachmentTable(ULONG ulFlags, LPMAPITABLE *lppTable);
	virtual HRESULT OpenAttach(ULONG ulAttachmentNum, LPCIID lpInterface, ULONG ulFlags, LPATTACH *lppAttach);
	virtual HRESULT CreateAttach(LPCIID lpInterface, ULONG ulFlags, ULONG *lpulAttachmentNum, LPATTACH *lppAttach);
	virtual HRESULT DeleteAttach(ULONG ulAttachmentNum, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags);
	virtual HRESULT GetRecipientTable(ULONG ulFlags, LPMAPITABLE *lppTable);
	virtual HRESULT ModifyRecipients(ULONG ulFlags, LPADRLIST lpMods);
	virtual HRESULT SubmitMessage(ULONG ulFlags);
	virtual HRESULT SetReadFlag(ULONG ulFlags);

	// override for IMAPIProp::SaveChanges
	virtual HRESULT SaveChanges(ULONG ulFlags);
	virtual HRESULT HrSaveChild(ULONG ulFlags, MAPIOBJECT *lpsMapiObject);
	// override for IMAPIProp::CopyTo
	virtual HRESULT CopyTo(ULONG ciidExclude, LPCIID rgiidExclude, LPSPropTagArray lpExcludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems);
	virtual HRESULT CopyProps(LPSPropTagArray lpIncludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems);

	// RTF/Subject overrides
	virtual HRESULT SetProps(ULONG cValues, LPSPropValue lpPropArray, LPSPropProblemArray FAR * lppProblems);
	virtual HRESULT DeleteProps(LPSPropTagArray lpPropTagArray, LPSPropProblemArray FAR * lppProblems);
	virtual HRESULT HrLoadProps();

	// Our table-row getprop handler (handles client-side generation of table columns)
	static HRESULT TableRowGetProp(void* lpProvider, struct propVal *lpsPropValSrc, LPSPropValue lpsPropValDst, void **lpBase, ULONG ulType);

	// RTF overrides
	virtual HRESULT		HrSetRealProp(SPropValue *lpsPropValue);

	class xMessage _zcp_final : public IMessage {
		// From IUnknown
		virtual HRESULT __stdcall QueryInterface(REFIID refiid, void **lppInterface) _zcp_override;
		virtual ULONG __stdcall AddRef(void) _zcp_override;
		virtual ULONG __stdcall Release(void) _zcp_override;
		
		// From IMAPIProp
		virtual HRESULT __stdcall GetLastError(HRESULT hError, ULONG ulFlags, LPMAPIERROR * lppMapiError);
		virtual HRESULT __stdcall SaveChanges(ULONG ulFlags);
		virtual HRESULT __stdcall GetProps(LPSPropTagArray lpPropTagArray, ULONG ulFlags, ULONG FAR * lpcValues, LPSPropValue FAR * lppPropArray);
		virtual HRESULT __stdcall GetPropList(ULONG ulFlags, LPSPropTagArray FAR * lppPropTagArray);
		virtual HRESULT __stdcall OpenProperty(ULONG ulPropTag, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulFlags, LPUNKNOWN FAR * lppUnk);
		virtual HRESULT __stdcall SetProps(ULONG cValues, LPSPropValue lpPropArray, LPSPropProblemArray FAR * lppProblems);
		virtual HRESULT __stdcall DeleteProps(LPSPropTagArray lpPropTagArray, LPSPropProblemArray FAR * lppProblems);
		virtual HRESULT __stdcall CopyTo(ULONG ciidExclude, LPCIID rgiidExclude, LPSPropTagArray lpExcludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems);
		virtual HRESULT __stdcall CopyProps(LPSPropTagArray lpIncludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems);
		virtual HRESULT __stdcall GetNamesFromIDs(LPSPropTagArray * pptaga, LPGUID lpguid, ULONG ulFlags, ULONG * pcNames, LPMAPINAMEID ** pppNames);
		virtual HRESULT __stdcall GetIDsFromNames(ULONG cNames, LPMAPINAMEID * ppNames, ULONG ulFlags, LPSPropTagArray * pptaga);

		// From IMessage
		virtual HRESULT __stdcall GetAttachmentTable(ULONG ulFlags, LPMAPITABLE *lppTable);
		virtual HRESULT __stdcall OpenAttach(ULONG ulAttachmentNum, LPCIID lpInterface, ULONG ulFlags, LPATTACH *lppAttach);
		virtual HRESULT __stdcall CreateAttach(LPCIID lpInterface, ULONG ulFlags, ULONG *lpulAttachmentNum, LPATTACH *lppAttach);
		virtual HRESULT __stdcall DeleteAttach(ULONG ulAttachmentNum, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags);
		virtual HRESULT __stdcall GetRecipientTable(ULONG ulFlags, LPMAPITABLE *lppTable);
		virtual HRESULT __stdcall ModifyRecipients(ULONG ulFlags, LPADRLIST lpMods);
		virtual HRESULT __stdcall SubmitMessage(ULONG ulFlags);
		virtual HRESULT __stdcall SetReadFlag(ULONG ulFlags);
	} m_xMessage;

protected:
	void RecursiveMarkDelete(MAPIOBJECT *lpObj);

	HRESULT CreateAttach(LPCIID lpInterface, ULONG ulFlags, const IAttachFactory &refFactory, ULONG *lpulAttachmentNum, LPATTACH *lppAttach);

	HRESULT GetRtfData(std::string *lpstrRtfData);
	HRESULT GetCodePage(unsigned int *lpulCodePage);

private:
	enum eSyncChange {syncChangeNone, syncChangeBody, syncChangeRTF, syncChangeHTML};
	enum eBodyType { bodyTypeUnknown, bodyTypePlain, bodyTypeRTF, bodyTypeHTML };

	HRESULT UpdateTable(ECMemTable *lpTable, ULONG ulObjType, ULONG ulObjKeyProp);
	HRESULT SaveRecips();
	HRESULT SyncAttachments();
	BOOL HasAttachment();

	HRESULT SyncRecips();
	HRESULT SyncSubject();
	HRESULT GetBodyType(eBodyType *lpulBodyType);
	
	// Override GetProps/GetPropList so we can sync RTF before calling GetProps
	virtual HRESULT GetProps(LPSPropTagArray lpPropTagArray, ULONG ulFlags, ULONG FAR * lpcValues, LPSPropValue FAR * lppPropArray);
	virtual HRESULT GetPropList(ULONG ulFlags, LPSPropTagArray FAR * lppPropTagArray);

	HRESULT GetSyncedBodyProp(ULONG ulPropTag, ULONG ulFlags, void *lpBase, LPSPropValue lpsPropValue);
	HRESULT SyncBody(ULONG ulPropTag);
	HRESULT SyncPlainToRtf();
	HRESULT SyncPlainToHtml();
	HRESULT SyncRtf();
	HRESULT SyncHtmlToPlain();
	HRESULT SyncHtmlToRtf();
	
	BOOL			fNew;
	BOOL			m_bEmbedded;
	BOOL			m_bExplicitSubjectPrefix;
	BOOL			m_bRecipsDirty;

	eBodyType		m_ulBodyType;
	BOOL			m_bInhibitSync;

public:
	ULONG			m_cbParentID;
	LPENTRYID		m_lpParentID; // For new messages

	ECMemTable		*lpRecips;
	ECMemTable		*lpAttachments;

	ULONG			ulNextAttUniqueId;
	ULONG			ulNextRecipUniqueId;
};

class ECMessageFactory : public IMessageFactory{
public:
	HRESULT Create(ECMsgStore *lpMsgStore, BOOL fNew, BOOL fModify, ULONG ulFlags, BOOL bEmbedded, ECMAPIProp *lpRoot, ECMessage **lpMessage) const;
};

#endif // ECMESSAGE_H
