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

#include "rules.h"
#include <mapi.h>
#include <mapidefs.h>
#include <mapicode.h>
#include <mapiutil.h>
#include <zarafa/mapiext.h>
#include <edkmdb.h>
#include <edkguid.h>
#include <zarafa/stringutil.h>
#include <zarafa/Util.h>
#include <zarafa/CommonUtil.h>
#include <zarafa/ECLogger.h>
#include <zarafa/MAPIErrors.h>
#include <zarafa/mapi_ptr.h>
#include <zarafa/mapiguidext.h>

#include "IECExchangeModifyTable.h"
#include "PyMapiPlugin.h"
#include "spmain.h"

using namespace std;

static HRESULT GetRecipStrings(LPMESSAGE lpMessage, std::wstring &wstrTo,
    std::wstring &wstrCc, std::wstring &wstrBcc)
{
	HRESULT hr = hrSuccess;
	SRowSetPtr ptrRows;
	MAPITablePtr ptrRecips;
	SizedSPropTagArray(2, sptaDisplay)  = {2, { PR_DISPLAY_NAME_W, PR_RECIPIENT_TYPE } };
	
	wstrTo.clear();
	wstrCc.clear();
	wstrBcc.clear();
	
	hr = lpMessage->GetRecipientTable(MAPI_UNICODE, &ptrRecips);
	if(hr != hrSuccess)
		goto exit;
		
	hr = ptrRecips->SetColumns((LPSPropTagArray)&sptaDisplay, TBL_BATCH);
	if(hr != hrSuccess)
		goto exit;
		
	while(1) {
		hr = ptrRecips->QueryRows(1, 0, &ptrRows);
		if(hr != hrSuccess)
			goto exit;
			
		if(ptrRows.size() == 0)
			break;
			
		if(ptrRows[0].lpProps[0].ulPropTag != PR_DISPLAY_NAME_W || ptrRows[0].lpProps[1].ulPropTag != PR_RECIPIENT_TYPE)
			continue;
			
		switch(ptrRows[0].lpProps[1].Value.ul) {
			case MAPI_TO:
				if (!wstrTo.empty()) wstrTo += L"; ";
				wstrTo += ptrRows[0].lpProps[0].Value.lpszW;
				break;
			case MAPI_CC:
				if (!wstrCc.empty()) wstrCc += L"; ";
				wstrCc += ptrRows[0].lpProps[0].Value.lpszW;
				break;
			case MAPI_BCC:
				if (!wstrBcc.empty()) wstrBcc += L"; ";
				wstrBcc += ptrRows[0].lpProps[0].Value.lpszW;
				break;
		}
	}

exit:	
	return hr;
}

static HRESULT MungeForwardBody(LPMESSAGE lpMessage, LPMESSAGE lpOrigMessage)
{
	HRESULT hr = hrSuccess;
	SPropArrayPtr ptrBodies;
	SizedSPropTagArray (4, sBody) = { 4, {
			PR_BODY_W,
			PR_HTML,
			PR_RTF_IN_SYNC,
			PR_INTERNET_CPID
		} };
	SPropArrayPtr ptrInfo;
	SizedSPropTagArray (4, sInfo) = { 4, {
			PR_SENT_REPRESENTING_NAME_W,
			PR_SENT_REPRESENTING_EMAIL_ADDRESS_W,
			PR_MESSAGE_DELIVERY_TIME,
			PR_SUBJECT_W
		} };
	ULONG ulCharset;
	ULONG cValues;
	bool bPlain = false;
	SPropValue sNewBody;
	StreamPtr ptrStream;
	string strHTML;
	string strHTMLForwardText;
	wstring wstrBody;
	wstring strForwardText;
	wstring wstrTo, wstrCc, wstrBcc;

	hr = lpOrigMessage->GetProps((LPSPropTagArray)&sBody, 0, &cValues, &ptrBodies);
	if (FAILED(hr))
		goto exit;
		
	if (PROP_TYPE(ptrBodies[3].ulPropTag) != PT_ERROR)
		ulCharset = ptrBodies[3].Value.ul;
	else
		ulCharset = 20127;		// us-ascii

	if (PROP_TYPE(ptrBodies[0].ulPropTag) == PT_ERROR && PROP_TYPE(ptrBodies[1].ulPropTag) == PT_ERROR) {
		// plain and html not found, check sync flag
		bPlain = (ptrBodies[2].Value.b == FALSE);
	} else {
		bPlain = PROP_TYPE(ptrBodies[1].ulPropTag) == PT_ERROR && ptrBodies[1].Value.err == MAPI_E_NOT_FOUND;
	}
	sNewBody.ulPropTag = bPlain ? PR_BODY_W : PR_HTML;

	// From: <fullname>
	// Sent: <date>
	// To: <original To:>
	// Cc: <original Cc:>
	// Subject: <>
	// Auto forwarded by a rule
	
	hr = GetRecipStrings(lpOrigMessage, wstrTo, wstrCc, wstrBcc);
	if (FAILED(hr))
		goto exit;

	hr = lpOrigMessage->GetProps((LPSPropTagArray)&sInfo, 0, &cValues, &ptrInfo);
	if (FAILED(hr))
		goto exit;

	if (bPlain) {
		// Plain text body

		strForwardText = L"From: ";
		if (PROP_TYPE(ptrInfo[0].ulPropTag) != PT_ERROR)
			strForwardText += ptrInfo[0].Value.lpszW;
		else if (PROP_TYPE(ptrInfo[1].ulPropTag) != PT_ERROR)
			strForwardText += ptrInfo[1].Value.lpszW;
		
		if (PROP_TYPE(ptrInfo[1].ulPropTag) != PT_ERROR) {
			strForwardText += L" <";
			strForwardText += ptrInfo[1].Value.lpszW;
			strForwardText += L">";
		}

		strForwardText += L"\nSent: ";
		if (PROP_TYPE(ptrInfo[2].ulPropTag) != PT_ERROR) {
			WCHAR buffer[64];
			time_t t;
			struct tm date;
			FileTimeToUnixTime(ptrInfo[2].Value.ft, &t);
			localtime_r(&t, &date);
			wcsftime(buffer, arraySize(buffer), L"%c", &date);
			strForwardText += buffer;
		}

		strForwardText += L"\nTo: ";
		strForwardText += wstrTo;

		strForwardText += L"\nCc: ";
		strForwardText += wstrCc;

		strForwardText += L"\nSubject: ";
		if (PROP_TYPE(ptrInfo[3].ulPropTag) != PT_ERROR)
			strForwardText += ptrInfo[3].Value.lpszW;

		strForwardText += L"\nAuto forwarded by a rule\n\n";

		if (ptrBodies[0].ulPropTag == PT_ERROR) {
			hr = lpOrigMessage->OpenProperty(PR_BODY_W, &IID_IStream, 0, 0, &ptrStream);
			if (hr == hrSuccess) {
				hr = Util::HrStreamToString(ptrStream, wstrBody);
			}
			// stream
			strForwardText.append(wstrBody);
		} else {
			strForwardText += ptrBodies[0].Value.lpszW;
		}

		sNewBody.Value.lpszW = (WCHAR*)strForwardText.c_str();
	}
	else {
		// HTML body (or rtf, but nuts to editing that!)
		string strFind("<body");
		const char* pos;

		hr = lpOrigMessage->OpenProperty(PR_HTML, &IID_IStream, 0, 0, &ptrStream);
		if (hr == hrSuccess) {
			hr = Util::HrStreamToString(ptrStream, strHTML);
		}
		// icase <body> tag 
		pos = str_ifind(strHTML.c_str(), strFind.c_str());
		pos = pos ? pos + strFind.length() : strHTML.c_str();
		// if body tag was not found, this will make it be placed after the first tag, probably <html>
		if ((pos == strHTML.c_str() && *pos == '<') || pos != strHTML.c_str()) {
			// not all html bodies start actually using tags, so only seek if we find a <, or if we found a body tag starting point.
			while (*pos && *pos != '>')
				++pos;
			if (*pos == '>')
				++pos;
		}


		{
			strHTMLForwardText = "<b>From:</b> ";
			if (PROP_TYPE(ptrInfo[0].ulPropTag) != PT_ERROR)
				Util::HrTextToHtml(ptrInfo[0].Value.lpszW, strHTMLForwardText, ulCharset);
			else if (PROP_TYPE(ptrInfo[1].ulPropTag) != PT_ERROR)
				Util::HrTextToHtml(ptrInfo[1].Value.lpszW, strHTMLForwardText, ulCharset);

			if (PROP_TYPE(ptrInfo[1].ulPropTag) != PT_ERROR) {
				strHTMLForwardText += " &lt;<a href=\"mailto:";
				Util::HrTextToHtml(ptrInfo[1].Value.lpszW, strHTMLForwardText, ulCharset);
				strHTMLForwardText += "\">";
				Util::HrTextToHtml(ptrInfo[1].Value.lpszW, strHTMLForwardText, ulCharset);
				strHTMLForwardText += "</a>&gt;";
			}

			strHTMLForwardText += "<br><b>Sent:</b> ";
			if (PROP_TYPE(ptrInfo[2].ulPropTag) != PT_ERROR) {
				char buffer[32];
				time_t t;
				struct tm date;
				FileTimeToUnixTime(ptrInfo[2].Value.ft, &t);
				localtime_r(&t, &date);
				strftime(buffer, 32, "%c", &date);
				strHTMLForwardText += buffer;
			}

			strHTMLForwardText += "<br><b>To:</b> ";
			Util::HrTextToHtml(wstrTo.c_str(), strHTMLForwardText, ulCharset);

			strHTMLForwardText += "<br><b>Cc:</b> ";
			Util::HrTextToHtml(wstrCc.c_str(), strHTMLForwardText, ulCharset);

			strHTMLForwardText += "<br><b>Subject:</b> ";
			if (PROP_TYPE(ptrInfo[3].ulPropTag) != PT_ERROR)
				Util::HrTextToHtml(ptrInfo[3].Value.lpszW, strHTMLForwardText, ulCharset);

			strHTMLForwardText += "<br><b>Auto forwarded by a rule</b><br><hr><br>";
		}

		strHTML.insert((pos - strHTML.c_str()), strHTMLForwardText);

		sNewBody.Value.bin.cb = strHTML.size();
		sNewBody.Value.bin.lpb = (BYTE*)strHTML.c_str();
	}

	// set new body with forward markers
	hr = lpMessage->SetProps(1, &sNewBody, NULL);

exit:
	return hr;
}

static HRESULT CreateOutboxMessage(LPMDB lpOrigStore, LPMESSAGE *lppMessage)
{
	HRESULT hr = hrSuccess;
	LPMAPIFOLDER lpOutbox = NULL;
	LPSPropValue lpOutboxEntryID = NULL;
	ULONG ulObjType = 0;

	hr = HrGetOneProp(lpOrigStore, PR_IPM_OUTBOX_ENTRYID, &lpOutboxEntryID);
	if (hr != hrSuccess)
		goto exit;

	hr = lpOrigStore->OpenEntry(lpOutboxEntryID->Value.bin.cb, (LPENTRYID)lpOutboxEntryID->Value.bin.lpb, NULL, MAPI_MODIFY, &ulObjType, (LPUNKNOWN*)&lpOutbox);
	if (hr != hrSuccess)
		goto exit;

	hr = lpOutbox->CreateMessage(NULL, 0, lppMessage);
	if (hr != hrSuccess)
		goto exit;

exit:
	MAPIFreeBuffer(lpOutboxEntryID);
	if (lpOutbox)
		lpOutbox->Release();

	return hr;
}

static HRESULT CreateReplyCopy(LPMAPISESSION lpSession, LPMDB lpOrigStore,
    IMAPIProp *lpOrigMessage, LPMESSAGE lpTemplate, LPMESSAGE *lppMessage)
{
	HRESULT hr = hrSuccess;
	LPMESSAGE lpReplyMessage = NULL;
	LPSPropValue lpProp = NULL;
	std::wstring strwSubject;
	LPSPropValue lpSentMailEntryID = NULL;
	ULONG cValues = 0;
	LPSPropValue lpFrom = NULL;
	LPSPropValue lpReplyRecipient = NULL;
	ADRLIST sRecip = {0};
	ULONG ulCmp = 0;
	
	SizedSPropTagArray (5, sFrom) = { 5, {
		PR_RECEIVED_BY_ENTRYID,
		PR_RECEIVED_BY_NAME,
		PR_RECEIVED_BY_ADDRTYPE,
		PR_RECEIVED_BY_EMAIL_ADDRESS,
		PR_RECEIVED_BY_SEARCH_KEY,
	} };

	SizedSPropTagArray (6, sReplyRecipient) = { 6, {
		PR_SENDER_ENTRYID,
		PR_SENDER_NAME,
		PR_SENDER_ADDRTYPE,
		PR_SENDER_EMAIL_ADDRESS,
		PR_SENDER_SEARCH_KEY,
		PR_NULL,
	} };

	hr = CreateOutboxMessage(lpOrigStore, &lpReplyMessage);
	if (hr != hrSuccess)
		goto exit;

	hr = lpTemplate->CopyTo(0, NULL, NULL, 0, NULL, &IID_IMessage, lpReplyMessage, 0, NULL);
	if (hr != hrSuccess)
		goto exit;

	// set "sent mail" folder entryid for spooler
	hr = HrGetOneProp(lpOrigStore, PR_IPM_SENTMAIL_ENTRYID, &lpSentMailEntryID);
	if (hr != hrSuccess)
		goto exit;

	lpSentMailEntryID->ulPropTag = PR_SENTMAIL_ENTRYID;

	hr = HrSetOneProp(lpReplyMessage, lpSentMailEntryID);
	if (hr != hrSuccess)
		goto exit;

	// set a sensible subject
	hr = HrGetOneProp(lpReplyMessage, PR_SUBJECT_W, &lpProp);
	if (hr == hrSuccess && lpProp->Value.lpszW[0] == L'\0') {
		MAPIFreeBuffer(lpProp);
		// Exchange: uses "BT: orig subject" if empty, or only subject from template.
		hr = HrGetOneProp(lpOrigMessage, PR_SUBJECT_W, &lpProp);
		if (hr == hrSuccess) {
			strwSubject = wstring(L"BT: ") + lpProp->Value.lpszW;
			lpProp->Value.lpszW = (WCHAR*)strwSubject.c_str();
			hr = HrSetOneProp(lpReplyMessage, lpProp);
			if (hr != hrSuccess)
				goto exit;
		}
	}
	MAPIFreeBuffer(lpProp);
	lpProp = NULL;
	hr = hrSuccess;

	hr = HrGetOneProp(lpOrigMessage, PR_INTERNET_MESSAGE_ID, &lpProp);
	if (hr == hrSuccess) {
		lpProp->ulPropTag = PR_IN_REPLY_TO_ID;
		hr = HrSetOneProp(lpReplyMessage, lpProp);
		if (hr != hrSuccess)
			goto exit;
	}
	MAPIFreeBuffer(lpProp);
	lpProp = NULL;
	hr = hrSuccess;

	// set From to self
	hr = lpOrigMessage->GetProps((LPSPropTagArray)&sFrom, 0, &cValues, &lpFrom);
	if (FAILED(hr))
		goto exit;

	lpFrom[0].ulPropTag = CHANGE_PROP_TYPE(PR_SENT_REPRESENTING_ENTRYID, PROP_TYPE(lpFrom[0].ulPropTag));
	lpFrom[1].ulPropTag = CHANGE_PROP_TYPE(PR_SENT_REPRESENTING_NAME, PROP_TYPE(lpFrom[1].ulPropTag));
	lpFrom[2].ulPropTag = CHANGE_PROP_TYPE(PR_SENT_REPRESENTING_ADDRTYPE, PROP_TYPE(lpFrom[2].ulPropTag));
	lpFrom[3].ulPropTag = CHANGE_PROP_TYPE(PR_SENT_REPRESENTING_EMAIL_ADDRESS, PROP_TYPE(lpFrom[3].ulPropTag));
	lpFrom[4].ulPropTag = CHANGE_PROP_TYPE(PR_SENT_REPRESENTING_SEARCH_KEY, PROP_TYPE(lpFrom[4].ulPropTag));

	hr = lpReplyMessage->SetProps(5, lpFrom, NULL);
	if (FAILED(hr))
		goto exit;

	if (parseBool(g_lpConfig->GetSetting("set_rule_headers", NULL, "yes"))) {
		SPropValue sPropVal;

		PROPMAP_START
		PROPMAP_NAMED_ID(ZarafaRuleAction, PT_UNICODE, PS_INTERNET_HEADERS, "x-zarafa-rule-action")
		PROPMAP_INIT(lpReplyMessage);

		sPropVal.ulPropTag = PROP_ZarafaRuleAction;
		sPropVal.Value.lpszW = const_cast<wchar_t *>(L"reply");

		hr = HrSetOneProp(lpReplyMessage, &sPropVal);
		if (hr != hrSuccess)
			goto exit;
	}


	// append To with original sender
	// @todo get Reply-To ?
	hr = lpOrigMessage->GetProps((LPSPropTagArray)&sReplyRecipient, 0, &cValues, &lpReplyRecipient);
	if (FAILED(hr))
		goto exit;

	// obvious loop is being obvious
	if (PROP_TYPE(lpReplyRecipient[0].ulPropTag) != PT_ERROR && PROP_TYPE(lpFrom[0].ulPropTag ) != PT_ERROR) {
		hr = lpSession->CompareEntryIDs(lpReplyRecipient[0].Value.bin.cb, (LPENTRYID)lpReplyRecipient[0].Value.bin.lpb,
										lpFrom[0].Value.bin.cb, (LPENTRYID)lpFrom[0].Value.bin.lpb, 0, &ulCmp);
		if (hr == hrSuccess && ulCmp == TRUE) {
			hr = MAPI_E_UNABLE_TO_COMPLETE;
			goto exit;
		}
	}

	lpReplyRecipient[0].ulPropTag = CHANGE_PROP_TYPE(PR_ENTRYID, PROP_TYPE(lpReplyRecipient[0].ulPropTag));
	lpReplyRecipient[1].ulPropTag = CHANGE_PROP_TYPE(PR_DISPLAY_NAME, PROP_TYPE(lpReplyRecipient[1].ulPropTag));
	lpReplyRecipient[2].ulPropTag = CHANGE_PROP_TYPE(PR_ADDRTYPE, PROP_TYPE(lpReplyRecipient[2].ulPropTag));
	lpReplyRecipient[3].ulPropTag = CHANGE_PROP_TYPE(PR_EMAIL_ADDRESS, PROP_TYPE(lpReplyRecipient[3].ulPropTag));
	lpReplyRecipient[4].ulPropTag = CHANGE_PROP_TYPE(PR_SEARCH_KEY, PROP_TYPE(lpReplyRecipient[4].ulPropTag));

	lpReplyRecipient[5].ulPropTag = PR_RECIPIENT_TYPE;
	lpReplyRecipient[5].Value.ul = MAPI_TO;

	sRecip.cEntries = 1;
	sRecip.aEntries[0].cValues = cValues;
	sRecip.aEntries[0].rgPropVals = lpReplyRecipient;

	hr = lpReplyMessage->ModifyRecipients(MODRECIP_ADD, &sRecip);
	if (FAILED(hr))
		goto exit;

	// return message
	hr = lpReplyMessage->QueryInterface(IID_IMessage, (void**)lppMessage);

exit:
	if (lpReplyMessage)
		lpReplyMessage->Release();
	MAPIFreeBuffer(lpProp);
	MAPIFreeBuffer(lpSentMailEntryID);
	MAPIFreeBuffer(lpFrom);
	MAPIFreeBuffer(lpReplyRecipient);
	return hr;
}

/** 
 * Checks the rule recipient list for a possible loop, and filters
 * that recipient. Returns an error when no recipients are left after
 * the filter.
 * 
 * @param[in] lpMessage The original delivered message performing the rule action
 * @param[in] lpRuleRecipients The recipient list from the rule
 * @param[in] bOpDelegate	If the action a delegate or forward action
 * @param[out] lppNewRecipients The actual recipient list to perform the action on
 * 
 * @return MAPI error code
 */
static HRESULT CheckRecipients(ECLogger *lpLogger, LPADRBOOK lpAdrBook,
    IMAPIProp *lpMessage, LPADRLIST lpRuleRecipients, bool bOpDelegate,
    bool bIncludeAsP1, LPADRLIST *lppNewRecipients)
{
	HRESULT hr = hrSuccess;
	LPADRLIST lpRecipients = NULL;
	LPSPropValue lpMsgClass = NULL;
	std::wstring strFromName, strFromType, strFromAddress;
	std::wstring strRuleName, strRuleType, strRuleAddress;

	hr = HrGetAddress(lpAdrBook, (IMessage*)lpMessage,
					  PR_SENT_REPRESENTING_ENTRYID, PR_SENT_REPRESENTING_NAME_W,
					  PR_SENT_REPRESENTING_ADDRTYPE_W, PR_SENT_REPRESENTING_EMAIL_ADDRESS_W,
					  strFromName, strFromType, strFromAddress);
	if (hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_ERROR, "Unable to get from address 0x%08X", hr);
		goto exit;
	}

	hr = MAPIAllocateBuffer(CbNewADRLIST(lpRuleRecipients->cEntries), (void**)&lpRecipients);
	if (hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_ERROR, "CheckRecipients(): MAPIAllocateBuffer failed %x", hr);
		goto exit;
	}

	HrGetOneProp(lpMessage, PR_MESSAGE_CLASS_A, &lpMsgClass); //ignore errors

	lpRecipients->cEntries = 0;

	for (ULONG i = 0; i < lpRuleRecipients->cEntries; ++i) {
		hr = HrGetAddress(lpAdrBook, lpRuleRecipients->aEntries[i].rgPropVals, lpRuleRecipients->aEntries[i].cValues, PR_ENTRYID,
						  CHANGE_PROP_TYPE(PR_DISPLAY_NAME, PT_UNSPECIFIED), CHANGE_PROP_TYPE(PR_ADDRTYPE, PT_UNSPECIFIED), CHANGE_PROP_TYPE(PR_SMTP_ADDRESS, PT_UNSPECIFIED), 
						  strRuleName, strRuleType, strRuleAddress);
		if (hr != hrSuccess) {
			lpLogger->Log(EC_LOGLEVEL_ERROR, "Unable to get rule address 0x%08X", hr);
			goto exit;
		}

		if (strFromAddress == strRuleAddress) {
			// Hack for Meeting requests
			if (!bOpDelegate || !lpMsgClass || strstr(lpMsgClass->Value.lpszA, "IPM.Schedule.Meeting.") == NULL) 
			{
				lpLogger->Log(EC_LOGLEVEL_INFO, "Same user found in From and rule, blocking for loop protection");
				continue;
			}
		}

		// copy recipient
		hr = Util::HrCopyPropertyArray(lpRuleRecipients->aEntries[i].rgPropVals, lpRuleRecipients->aEntries[i].cValues, &lpRecipients->aEntries[lpRecipients->cEntries].rgPropVals, &lpRecipients->aEntries[lpRecipients->cEntries].cValues, true);

		if (hr != hrSuccess) {
			lpLogger->Log(EC_LOGLEVEL_ERROR, "CheckRecipients(): Util::HrCopyPropertyArray failed %x", hr);
			goto exit;
		}

        if(bIncludeAsP1) {
            LPSPropValue lpRecipType = PpropFindProp(lpRecipients->aEntries[lpRecipients->cEntries].rgPropVals, lpRecipients->aEntries[lpRecipients->cEntries].cValues, PR_RECIPIENT_TYPE);
            
            if(!lpRecipType) {
                lpLogger->Log(EC_LOGLEVEL_FATAL, "Attempt to add recipient with no PR_RECIPIENT_TYPE");
                hr = MAPI_E_INVALID_PARAMETER;
                goto exit;
            }
            
            lpRecipType->Value.ul = MAPI_P1;
        }
		++lpRecipients->cEntries;
	}

	if (lpRecipients->cEntries == 0) {
		lpLogger->Log(EC_LOGLEVEL_WARNING, "Loop protection blocked all recipients, skipping rule");
		hr = MAPI_E_UNABLE_TO_COMPLETE;
		goto exit;
	}

	if (lpRecipients->cEntries != lpRuleRecipients->cEntries)
		lpLogger->Log(EC_LOGLEVEL_INFO, "Loop protection blocked some recipients");

	*lppNewRecipients = lpRecipients;
	lpRecipients = NULL;

exit:
	if (lpRecipients)
		FreeProws((LPSRowSet)lpRecipients);
	MAPIFreeBuffer(lpMsgClass);
	return hr;
}

static HRESULT CreateForwardCopy(ECLogger *lpLogger, LPADRBOOK lpAdrBook,
    LPMDB lpOrigStore, LPMESSAGE lpOrigMessage, LPADRLIST lpRuleRecipients,
    bool bOpDelegate, bool bDoPreserveSender, bool bDoNotMunge,
    bool bForwardAsAttachment, LPMESSAGE *lppMessage)
{
	HRESULT hr = hrSuccess;
	LPMESSAGE lpFwdMsg = NULL;
	LPSPropValue lpSentMailEntryID = NULL;
	LPSPropTagArray lpExclude = NULL; // non-free
	LPADRLIST lpRecipients = NULL;
	ULONG ulANr = 0;
	LPATTACH lpAttach = NULL;
	LPMESSAGE lpAttachMsg = NULL;

	SizedSPropTagArray (10, sExcludeFromCopyForward) = { 10, {
		PR_TRANSPORT_MESSAGE_HEADERS,
		PR_SENT_REPRESENTING_ENTRYID,
		PR_SENT_REPRESENTING_NAME,
		PR_SENT_REPRESENTING_ADDRTYPE,
		PR_SENT_REPRESENTING_EMAIL_ADDRESS,
		PR_SENT_REPRESENTING_SEARCH_KEY,
		PR_READ_RECEIPT_REQUESTED,
		PR_ORIGINATOR_DELIVERY_REPORT_REQUESTED,
		PR_MESSAGE_FLAGS,
		PR_MESSAGE_RECIPIENTS, // This must be the last entry, see bDoNotMunge
	} };

	SizedSPropTagArray (3, sExcludeFromCopyRedirect) = { 3, {
		PR_TRANSPORT_MESSAGE_HEADERS,
		PR_MESSAGE_FLAGS,
		PR_MESSAGE_RECIPIENTS, // This must be the last entry, see bDoNotMunge
	} };

	SizedSPropTagArray(1, sExcludeFromAttachedForward) = { 1, {
		PR_TRANSPORT_MESSAGE_HEADERS,
	} };

	LPSPropValue lpOrigSubject = NULL;
	SPropValue sForwardProps[5];
	ULONG cfp = 0;
	wstring strSubject;

	if (lpRuleRecipients == NULL || lpRuleRecipients->cEntries == 0) {
	    lpLogger->Log(EC_LOGLEVEL_FATAL, "No rule recipient");
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = CheckRecipients(lpLogger, lpAdrBook, lpOrigMessage, lpRuleRecipients, bOpDelegate, bDoNotMunge, &lpRecipients);
	if (hr == MAPI_E_UNABLE_TO_COMPLETE)
		goto exit;
	if (hr != hrSuccess)
		// use rule recipients without filter
		lpRecipients = lpRuleRecipients;

	hr = HrGetOneProp(lpOrigStore, PR_IPM_SENTMAIL_ENTRYID, &lpSentMailEntryID);
	if (hr != hrSuccess)
		goto exit;

	hr = CreateOutboxMessage(lpOrigStore, &lpFwdMsg);
	if (hr != hrSuccess)
		goto exit;

	// If we're doing a redirect, copy over the original PR_SENT_REPRESENTING_*, otherwise don't
	if(bDoPreserveSender)
		lpExclude = (LPSPropTagArray)&sExcludeFromCopyRedirect;
	else
		lpExclude = (LPSPropTagArray)&sExcludeFromCopyForward;

    if(bDoNotMunge) {
        // The idea here is to enable 'resend' mode and to include the original recipient list. What will
        // happen is that the original recipient list will be used to generate the headers of the message, but
        // only the MAPI_P1 recipients will be used to send the message to. This is exactly what we want. So
        // with bDoNotMunge, we copy the original recipient from the original message, and set MSGFLAG_RESEND.
        
        // Later on, we set the actual recipient to MAPI_P1
        SPropValue sPropResend;
        sPropResend.ulPropTag = PR_MESSAGE_FLAGS;
        sPropResend.Value.ul = MSGFLAG_UNSENT | MSGFLAG_RESEND | MSGFLAG_READ;
        
		--lpExclude->cValues; // strip PR_MESSAGE_RECIPIENTS, since original recipients should be used
        hr = HrSetOneProp(lpFwdMsg, &sPropResend);
        if(hr != hrSuccess)
            goto exit;
    }

	if (bForwardAsAttachment) {
		hr = lpFwdMsg->CreateAttach(NULL, 0, &ulANr, &lpAttach);
		if (hr != hrSuccess)
			goto exit;

		SPropValue sAttachMethod;

		sAttachMethod.ulPropTag = PR_ATTACH_METHOD;
		sAttachMethod.Value.ul = ATTACH_EMBEDDED_MSG;

		hr = lpAttach->SetProps(1, &sAttachMethod, NULL);
		if (hr != hrSuccess)
			goto exit;

		hr = lpAttach->OpenProperty(PR_ATTACH_DATA_OBJ, &IID_IMessage, 0, MAPI_CREATE | MAPI_MODIFY, (LPUNKNOWN *)&lpAttachMsg);
		if (hr != hrSuccess)
			goto exit;

		hr = lpOrigMessage->CopyTo(0, NULL,  (LPSPropTagArray)&sExcludeFromAttachedForward, 0, NULL, &IID_IMessage, lpAttachMsg, 0, NULL);
		if (hr != hrSuccess)
			goto exit;

		hr = lpAttachMsg->SaveChanges(0);
		if (hr != hrSuccess)
			goto exit;

		hr = lpAttach->SaveChanges(0);
		if (hr != hrSuccess)
			goto exit;

		lpAttachMsg->Release();
		lpAttachMsg = NULL;

		lpAttach->Release();
		lpAttach = NULL;
	}
	else {	
		hr = lpOrigMessage->CopyTo(0, NULL, lpExclude, 0, NULL, &IID_IMessage, lpFwdMsg, 0, NULL);
		if (hr != hrSuccess)
			goto exit;
	}

	hr = lpFwdMsg->ModifyRecipients(MODRECIP_ADD, lpRecipients);
	if (hr != hrSuccess)
		goto exit;

	// set from email ??

	hr = HrGetOneProp(lpOrigMessage, PR_SUBJECT, &lpOrigSubject);
	if (hr == hrSuccess)
		strSubject = lpOrigSubject->Value.lpszW;

	if(!bDoNotMunge || bForwardAsAttachment)
		strSubject.insert(0, L"FW: ");

	cfp = 0;
	sForwardProps[cfp].ulPropTag = PR_AUTO_FORWARDED;
	sForwardProps[cfp++].Value.b = TRUE;

	sForwardProps[cfp].ulPropTag = PR_SUBJECT;
	sForwardProps[cfp++].Value.lpszW = (WCHAR*)strSubject.c_str();

	sForwardProps[cfp].ulPropTag = PR_SENTMAIL_ENTRYID;
	sForwardProps[cfp].Value.bin.cb = lpSentMailEntryID->Value.bin.cb;
	sForwardProps[cfp++].Value.bin.lpb = lpSentMailEntryID->Value.bin.lpb;

	if (bForwardAsAttachment) {
		sForwardProps[cfp].ulPropTag = PR_MESSAGE_CLASS;
		sForwardProps[cfp++].Value.lpszW = const_cast<wchar_t *>(L"IPM.Note");
	}

	if (parseBool(g_lpConfig->GetSetting("set_rule_headers", NULL, "yes"))) {
		PROPMAP_START
		PROPMAP_NAMED_ID(ZarafaRuleAction, PT_UNICODE, PS_INTERNET_HEADERS, "x-zarafa-rule-action")
		PROPMAP_INIT(lpFwdMsg);

		sForwardProps[cfp].ulPropTag = PROP_ZarafaRuleAction;
		sForwardProps[cfp++].Value.lpszW = LPWSTR(bDoPreserveSender ? L"redirect" : L"forward");
	}

	hr = lpFwdMsg->SetProps(cfp, (LPSPropValue)&sForwardProps, NULL);
	if (hr != hrSuccess)
		goto exit;

	if (!bDoNotMunge && !bForwardAsAttachment) {
		// because we're forwarding this as a new message, clear the old received message id
		SizedSPropTagArray(1, sptaDeleteProps) = { 1, { PR_INTERNET_MESSAGE_ID } };

		hr = lpFwdMsg->DeleteProps((LPSPropTagArray)&sptaDeleteProps, NULL);
		if(hr != hrSuccess)
			goto exit;

		MungeForwardBody(lpFwdMsg, lpOrigMessage);
	}

	*lppMessage = lpFwdMsg;

exit:
	MAPIFreeBuffer(lpSentMailEntryID);
	MAPIFreeBuffer(lpOrigSubject);
	if (lpRecipients && lpRecipients != lpRuleRecipients)
		FreeProws((LPSRowSet)lpRecipients);

	if (lpAttachMsg)
		lpAttachMsg->Release();

	if (lpAttach)
		lpAttach->Release();

	return hr;
}

// HRESULT HrDelegateMessage(LPMAPISESSION lpSession, LPEXCHANGEMANAGESTORE lpIEMS, IMAPIProp *lpMessage, LPADRENTRY lpAddress, ECLogger *lpLogger)
static HRESULT HrDelegateMessage(IMAPIProp *lpMessage)
{
	HRESULT hr = hrSuccess;
	SPropValue sNewProps[6] = {{0}};
	LPSPropValue lpProps = NULL;
	ULONG cValues = 0;
	SizedSPropTagArray(5, sptaRecipProps) = {5, 
		{ PR_RECEIVED_BY_ENTRYID, PR_RECEIVED_BY_ADDRTYPE, PR_RECEIVED_BY_EMAIL_ADDRESS, PR_RECEIVED_BY_NAME, PR_RECEIVED_BY_SEARCH_KEY }
	};
	SizedSPropTagArray(1, sptaSentMail) = { 1, { PR_SENTMAIL_ENTRYID } };

	// set PR_RCVD_REPRESENTING on original receiver
	hr = lpMessage->GetProps((LPSPropTagArray)&sptaRecipProps, 0, &cValues, &lpProps);
	if (hr != hrSuccess)
		goto exit;

	lpProps[0].ulPropTag = PR_RCVD_REPRESENTING_ENTRYID;
	lpProps[1].ulPropTag = PR_RCVD_REPRESENTING_ADDRTYPE;
	lpProps[2].ulPropTag = PR_RCVD_REPRESENTING_EMAIL_ADDRESS;
	lpProps[3].ulPropTag = PR_RCVD_REPRESENTING_NAME;
	lpProps[4].ulPropTag = PR_RCVD_REPRESENTING_SEARCH_KEY;

	hr = lpMessage->SetProps(cValues, lpProps, NULL);
	if (hr != hrSuccess)
		goto exit;

	// TODO: delete PR_RECEIVED_BY_ values?

	sNewProps[0].ulPropTag = PR_DELEGATED_BY_RULE;
	sNewProps[0].Value.b = TRUE;

	sNewProps[1].ulPropTag = PR_DELETE_AFTER_SUBMIT;
	sNewProps[1].Value.b = TRUE;

	hr = lpMessage->SetProps(2, (LPSPropValue)&sNewProps, NULL);
	if (hr != hrSuccess)
		goto exit;
		
	// Don't want to move to sent mail
	hr = lpMessage->DeleteProps((LPSPropTagArray)&sptaSentMail, NULL);
	if (hr != hrSuccess)
		goto exit;

	hr = lpMessage->SaveChanges(KEEP_OPEN_READWRITE);

exit:
	MAPIFreeBuffer(lpProps);
	return hr;
}

// lpMessage: gets EntryID, maybe pass this and close message in DAgent.cpp
HRESULT HrProcessRules(const std::string &recip, PyMapiPlugin *pyMapiPlugin,
    LPMAPISESSION lpSession, LPADRBOOK lpAdrBook, LPMDB lpOrigStore,
    LPMAPIFOLDER lpOrigInbox, IMessage **lppMessage, ECLogger *lpLogger,
    StatsClient *const sc)
{
	HRESULT hr = hrSuccess;
    IExchangeModifyTable *lpTable = NULL;
    IMAPITable *lpView = NULL;

	LPMDB lpDestStore = NULL;
	LPMAPIFOLDER lpDestFolder = NULL;
	LPMESSAGE lpTemplate = NULL;
	LPMESSAGE lpReplyMsg = NULL;
	LPMESSAGE lpFwdMsg = NULL;
	ULONG ulObjType;
	bool bAddFwdFlag = false;
	bool bMoved = false;
	IMessage *lpNewMessage = NULL;

    LPSRowSet lpRowSet = NULL;
    SizedSPropTagArray(11, sptaRules) = {11,
										 { PR_RULE_ID, PR_RULE_IDS, PR_RULE_SEQUENCE, PR_RULE_STATE,
										   PR_RULE_USER_FLAGS, PR_RULE_CONDITION, PR_RULE_ACTIONS,
										   PR_RULE_PROVIDER, CHANGE_PROP_TYPE(PR_RULE_NAME, PT_STRING8), PR_RULE_LEVEL, PR_RULE_PROVIDER_DATA } };
	SizedSSortOrderSet(1, sosRules) = {1, 0, 0, { {PR_RULE_SEQUENCE, TABLE_SORT_ASCEND} } };
    LPSPropValue lpRuleName = NULL;
    LPSPropValue lpRuleState = NULL;
	LPSPropValue lpPropRule = NULL;
	std::string strRule;
    LPSPropValue lpProp = NULL;
    LPSRestriction lpCondition = NULL;
    ACTIONS* lpActions = NULL;

	SPropValue sForwardProps[4];
	IECExchangeModifyTable *lpECModifyTable = NULL;
	ULONG ulResult= 0;

	sc -> countInc("rules", "invocations");

    hr = lpOrigInbox->OpenProperty(PR_RULES_TABLE, &IID_IExchangeModifyTable, 0, 0, (LPUNKNOWN *)&lpTable);
	if (hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_ERROR, "HrProcessRules(): OpenProperty failed %x", hr);
        goto exit;
	}

	hr = lpTable->QueryInterface(IID_IECExchangeModifyTable, (void**)&lpECModifyTable);
	if(hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_ERROR, "HrProcessRules(): QueryInterface failed %x", hr);
		goto exit;
	}

	hr = lpECModifyTable->DisablePushToServer();
	if(hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_ERROR, "HrProcessRules(): DisablePushToServer failed %x", hr);
		goto exit;
	}

	hr = pyMapiPlugin->RulesProcessing("PreRuleProcess", lpSession, lpAdrBook, lpOrigStore, lpTable, &ulResult);
	if(hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_ERROR, "HrProcessRules(): RulesProcessing failed %x", hr);
		goto exit;
	}

	//TODO do something with ulResults
    hr = lpTable->GetTable(0, &lpView);
	if(hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_ERROR, "HrProcessRules(): GetTable failed %x", hr);
        goto exit;
	}
        
    hr = lpView->SetColumns((LPSPropTagArray)&sptaRules, 0);
	if (hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_ERROR, "HrProcessRules(): SetColumns failed %x", hr);
        goto exit;
	}

	hr = lpView->SortTable((LPSSortOrderSet)&sosRules, 0);
	if (hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_ERROR, "HrProcessRules(): SortTable failed %x", hr);
        goto exit;
	}

	while (TRUE) {
        hr = lpView->QueryRows(1, 0, &lpRowSet);
	if (hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_ERROR, "HrProcessRules(): QueryRows failed %x", hr);
			goto exit;
	}

        if (lpRowSet->cRows == 0)
            break;

		sc -> countAdd("rules", "n_rules", int64_t(lpRowSet->cRows));

        lpRuleName = PpropFindProp(lpRowSet->aRow[0].lpProps, lpRowSet->aRow[0].cValues, CHANGE_PROP_TYPE(PR_RULE_NAME, PT_STRING8));
		if (lpRuleName)
			strRule = lpRuleName->Value.lpszA;
		else
			strRule = "(no name)";

		lpLogger->Log(EC_LOGLEVEL_DEBUG, "Processing rule %s for %s", strRule.c_str(), recip.c_str());
		lpRuleState = PpropFindProp(lpRowSet->aRow[0].lpProps, lpRowSet->aRow[0].cValues, PR_RULE_STATE);
		if (lpRuleState) {
			if (!(lpRuleState->Value.i & ST_ENABLED)) {
				lpLogger->Log(EC_LOGLEVEL_DEBUG, "Rule '%s' is disabled, skipping...", strRule.c_str());
				goto nextrule;		// rule is disabled
			}
		}

		lpCondition = NULL;
		lpActions = NULL;

		lpProp = PpropFindProp(lpRowSet->aRow[0].lpProps, lpRowSet->aRow[0].cValues, PR_RULE_CONDITION);
		if (lpProp)
			// NOTE: object is placed in Value.lpszA, not Value.x
			lpCondition = (LPSRestriction)lpProp->Value.lpszA;
		if (!lpCondition) {
			lpLogger->Log(EC_LOGLEVEL_DEBUG, "Rule '%s' has no contition, skipping...", strRule.c_str());
			goto nextrule;
		}

		lpProp = PpropFindProp(lpRowSet->aRow[0].lpProps, lpRowSet->aRow[0].cValues, PR_RULE_ACTIONS);
		if (lpProp)
			// NOTE: object is placed in Value.lpszA, not Value.x
			lpActions = (ACTIONS*)lpProp->Value.lpszA;
		if (!lpActions) {
			lpLogger->Log(EC_LOGLEVEL_DEBUG, "Rule '%s' has no action, skipping...", strRule.c_str());
			goto nextrule;
		}
		
		// test if action should be done...
		// @todo: Create the correct locale for the current store.
		if (TestRestriction(lpCondition, *lppMessage, createLocaleFromName("")) != hrSuccess) {
			lpLogger->Log(EC_LOGLEVEL_INFO, (std::string)"Rule " + strRule + " doesn't match");
			goto nextrule;
		}	

		lpLogger->Log(EC_LOGLEVEL_INFO, (std::string)"Rule " + strRule + " matches");

		sc -> countAdd("rules", "n_actions", int64_t(lpActions->cActions));

		for (ULONG n = 0; n < lpActions->cActions; ++n) {
			// do action
			switch(lpActions->lpAction[n].acttype) {
			case OP_MOVE:
			case OP_COPY:
					sc -> countInc("rules", "copy_move");

				if (lpActions->lpAction[n].acttype == OP_COPY)
					lpLogger->Log(EC_LOGLEVEL_DEBUG, "Rule action: copying e-mail");
				else
					lpLogger->Log(EC_LOGLEVEL_DEBUG, "Rule action: moving e-mail");

				// First try to open the folder on the session as that will just work if we have the store open
				hr = lpSession->OpenEntry(lpActions->lpAction[n].actMoveCopy.cbFldEntryId,
										  lpActions->lpAction[n].actMoveCopy.lpFldEntryId, &IID_IMAPIFolder, MAPI_MODIFY, &ulObjType,
										  (IUnknown**)&lpDestFolder);
				if (hr != hrSuccess) {
					std::string msg = std::string("Rule ") + strRule + ": Unable to open folder through session, trying through store: %s (%x)";
					lpLogger->Log(EC_LOGLEVEL_INFO, msg.c_str(), GetMAPIErrorMessage(hr), hr);

					hr = lpSession->OpenMsgStore(0, lpActions->lpAction[n].actMoveCopy.cbStoreEntryId,
													lpActions->lpAction[n].actMoveCopy.lpStoreEntryId, NULL, MAPI_BEST_ACCESS, &lpDestStore);
					if (hr != hrSuccess) {
						std::string msg = std::string("Rule ") + strRule + ": Unable to open destination store: %s (%x)";
						lpLogger->Log(EC_LOGLEVEL_ERROR, msg.c_str(), GetMAPIErrorMessage(hr), hr);
						goto nextact;
					}

					hr = lpDestStore->OpenEntry(lpActions->lpAction[n].actMoveCopy.cbFldEntryId,
												lpActions->lpAction[n].actMoveCopy.lpFldEntryId, &IID_IMAPIFolder, MAPI_MODIFY, &ulObjType,
												(IUnknown**)&lpDestFolder);
					if (hr != hrSuccess || ulObjType != MAPI_FOLDER) {
						std::string msg = std::string("Rule ") + strRule + ": Unable to open destination folder: %s (%x)";
						lpLogger->Log(EC_LOGLEVEL_ERROR, msg.c_str(), GetMAPIErrorMessage(hr), hr);
						goto nextact;
					}
				}

				hr = lpDestFolder->CreateMessage(NULL, 0, &lpNewMessage);
				if(hr != hrSuccess) {
					std::string msg = "Unable to create e-mail for rule " + strRule + ": %s (%x)";
					lpLogger->Log(EC_LOGLEVEL_ERROR, msg.c_str(), GetMAPIErrorMessage(hr), hr);
					goto exit;
				}
					
				hr = (*lppMessage)->CopyTo(0, NULL, NULL, 0, NULL, &IID_IMessage, lpNewMessage, 0, NULL);
				if(hr != hrSuccess) {
					std::string msg = "Unable to copy e-mail for rule " + strRule + ": %s (%x)";
					lpLogger->Log(EC_LOGLEVEL_ERROR, msg.c_str(), GetMAPIErrorMessage(hr), hr);
					goto exit;
				}

				hr = Util::HrCopyIMAPData((*lppMessage), lpNewMessage);
				// the function only returns errors on get/setprops, not when the data is just missing
				if (hr != hrSuccess) {
					std::string msg = "Unable to copy IMAP data e-mail for rule " + strRule + ", continuing: %s (%x)";
					lpLogger->Log(EC_LOGLEVEL_ERROR, msg.c_str(), GetMAPIErrorMessage(hr), hr);
					hr = hrSuccess;
					goto exit;
				}

				// Save the copy in its new location
				hr = lpNewMessage->SaveChanges(0);
				if (hr != hrSuccess) {
					std::string msg = std::string("Rule ") + strRule + ": Unable to copy/move message: %s (%x)";
					lpLogger->Log(EC_LOGLEVEL_ERROR, msg.c_str(), GetMAPIErrorMessage(hr), hr);
					goto nextact;
				}
				if (lpActions->lpAction[n].acttype == OP_MOVE)
					bMoved = true;
				break;

			// may become dam's, may become normal rules (ol2k3)

			case OP_REPLY:
			case OP_OOF_REPLY:
					sc -> countInc("rules", "reply_and_oof");
				if (lpActions->lpAction[n].acttype == OP_REPLY)
					lpLogger->Log(EC_LOGLEVEL_DEBUG, "Rule action: replying e-mail");
				else
					lpLogger->Log(EC_LOGLEVEL_DEBUG, "Rule action: OOF replying e-mail");

				hr = lpOrigInbox->OpenEntry(lpActions->lpAction[n].actReply.cbEntryId,
											lpActions->lpAction[n].actReply.lpEntryId, &IID_IMessage, 0, &ulObjType,
											(IUnknown**)&lpTemplate);
				if (hr != hrSuccess) {
					std::string msg = std::string("Rule ") + strRule + ": Unable to open reply message: %s (%x)";
					lpLogger->Log(EC_LOGLEVEL_ERROR, msg.c_str(), GetMAPIErrorMessage(hr), hr);
					goto nextact;
				}

				hr = CreateReplyCopy(lpSession, lpOrigStore, *lppMessage, lpTemplate, &lpReplyMsg);
				if (hr != hrSuccess) {
					std::string msg = std::string("Rule ") + strRule + ": Unable to create reply message: %s (%x)";
					lpLogger->Log(EC_LOGLEVEL_ERROR, msg.c_str(), GetMAPIErrorMessage(hr), hr);
					goto nextact;
				}

				hr = lpReplyMsg->SubmitMessage(0);
				if (hr != hrSuccess) {
					std::string msg = std::string("Rule ") + strRule + ": Unable to send reply message: %s (%x)";
					lpLogger->Log(EC_LOGLEVEL_ERROR, msg.c_str(), GetMAPIErrorMessage(hr), hr);
					goto nextact;
				}
				break;

			case OP_FORWARD:
					sc -> countInc("rules", "forward");
				// TODO: test lpActions->lpAction[n].ulActionFlavor
				// FWD_PRESERVE_SENDER			1
				// FWD_DO_NOT_MUNGE_MSG			2
				// FWD_AS_ATTACHMENT			4

				// redirect == 3

				if (lpActions->lpAction[n].lpadrlist->cEntries == 0) {
					lpLogger->Log(EC_LOGLEVEL_DEBUG, "Forwarding rule doesn't have recipients");
					continue; // Nothing todo
				}

				if(parseBool(g_lpConfig->GetSetting("no_double_forward"))) {
					// Loop protection, when header 'x-zarafa-rule-action' is added to the message it will stop to forward or redirect the message.
					PROPMAP_START
					PROPMAP_NAMED_ID(ZarafaRuleAction, PT_UNICODE, PS_INTERNET_HEADERS, "x-zarafa-rule-action")
					PROPMAP_INIT( (*lppMessage) );

					if (HrGetOneProp(*lppMessage, PROP_ZarafaRuleAction, &lpPropRule) == hrSuccess) {
						MAPIFreeBuffer(lpPropRule);
						lpLogger->Log(EC_LOGLEVEL_WARNING, (std::string)"Rule "+strRule+": FORWARD loop protection. Message will not be forwarded or redirected because it includes header 'x-zarafa-rule-action'");
						continue;
					}
				}

				lpLogger->Log(EC_LOGLEVEL_DEBUG, "Rule action: %s e-mail", (lpActions->lpAction[n].ulActionFlavor & FWD_PRESERVE_SENDER) ? "redirecting" : "forwarding");

				hr = CreateForwardCopy(lpLogger, lpAdrBook, lpOrigStore, *lppMessage, lpActions->lpAction[n].lpadrlist,
									   false,
									   lpActions->lpAction[n].ulActionFlavor & FWD_PRESERVE_SENDER,
									   lpActions->lpAction[n].ulActionFlavor & FWD_DO_NOT_MUNGE_MSG, 
									   lpActions->lpAction[n].ulActionFlavor & FWD_AS_ATTACHMENT,
									   &lpFwdMsg);
				if (hr != hrSuccess) {
					std::string msg = std::string("Rule ") + strRule + ": FORWARD Unable to create forward message: %s (%x)";
					lpLogger->Log(EC_LOGLEVEL_ERROR, msg.c_str(), GetMAPIErrorMessage(hr), hr);
					goto nextact;
				}

				hr = lpFwdMsg->SubmitMessage(0);
				if (hr != hrSuccess) {
					std::string msg = std::string("Rule ") + strRule + ": FORWARD Unable to send forward message: %s (%x)";
					lpLogger->Log(EC_LOGLEVEL_ERROR, msg.c_str(), GetMAPIErrorMessage(hr), hr);
					goto nextact;
				}

				// update original message, set as forwarded
				bAddFwdFlag = true;
				break;

			case OP_BOUNCE:
					sc -> countInc("rules", "bounce");
				// scBounceCode?
				// TODO:
				// 1. make copy of lpMessage, needs CopyTo() function
				// 2. copy From: to To:
				// 3. SubmitMessage()
				lpLogger->Log(EC_LOGLEVEL_WARNING, (std::string)"Rule "+strRule+": BOUNCE actions are currently unsupported");
				break;

			case OP_DELEGATE:
					sc -> countInc("rules", "delegate");
				
				if (lpActions->lpAction[n].lpadrlist->cEntries == 0) {
					lpLogger->Log(EC_LOGLEVEL_DEBUG, "Delegating rule doesn't have recipients");
					continue; // Nothing todo
				}
				lpLogger->Log(EC_LOGLEVEL_DEBUG, "Rule action: delegating e-mail");

				hr = CreateForwardCopy(lpLogger, lpAdrBook, lpOrigStore, *lppMessage, lpActions->lpAction[n].lpadrlist, true, true, true, false, &lpFwdMsg);
				if (hr != hrSuccess) {
					std::string msg = std::string("Rule ") + strRule + ": DELEGATE Unable to create delegate message: %s (%x)";
					lpLogger->Log(EC_LOGLEVEL_ERROR, msg.c_str(), GetMAPIErrorMessage(hr), hr);
					goto nextact;
				}

				// set delegate properties
				hr = HrDelegateMessage(lpFwdMsg);
				if (hr != hrSuccess) {
					std::string msg = std::string("Rule ") + strRule + ": DELEGATE Unable to modify delegate message: %s (%x)";
					lpLogger->Log(EC_LOGLEVEL_ERROR, msg.c_str(), GetMAPIErrorMessage(hr), hr);
					goto nextact;
				}

				hr = lpFwdMsg->SubmitMessage(0);
				if (hr != hrSuccess) {
					std::string msg = std::string("Rule ") + strRule + ": DELEGATE Unable to send delegate message: %s (%x)";
					lpLogger->Log(EC_LOGLEVEL_ERROR, msg.c_str(), GetMAPIErrorMessage(hr), hr);
					goto nextact;
				}

				// don't set forwarded flag
				break;

			// will become a DAM atm, so I won't even bother implementing these ...

			case OP_DEFER_ACTION:
				sc -> countInc("rules", "defer");
					// DAM crud, but outlook doesn't check these messages... yet
				lpLogger->Log(EC_LOGLEVEL_WARNING, (std::string)"Rule "+strRule+": DEFER client actions are currently unsupported");
				break;
			case OP_TAG:
				sc -> countInc("rules", "tag");
				// sure. WHEN YOU STOP WITH THE FRIGGIN' DEFER ACTION MESSAGES!!
				lpLogger->Log(EC_LOGLEVEL_WARNING, (std::string)"Rule "+strRule+": TAG actions are currently unsupported");
				break;
			case OP_DELETE:
					sc -> countInc("rules", "delete");
				// since *lppMessage wasn't yet saved in the server, we can just return a special MAPI Error code here,
				// this will trigger the out-of-office mail (according to microsoft), but not save the message and drop it.
				// The error code will become hrSuccess automatically after returning from the post processing function.
				lpLogger->Log(EC_LOGLEVEL_DEBUG, "Rule action: deleting e-mail");
				hr = MAPI_E_CANCEL;
				goto exit;
				break;
			case OP_MARK_AS_READ:
					sc -> countInc("rules", "mark_read");
				// add prop read
				lpLogger->Log(EC_LOGLEVEL_WARNING, (std::string)"Rule "+strRule+": MARK AS READ actions are currently unsupported");
				break;
			};

			// next action
nextact:
			if (lpDestStore) {
				lpDestStore->Release();
				lpDestStore = NULL;
			}
			if (lpDestFolder) {
				lpDestFolder->Release();
				lpDestFolder = NULL;
			}
			if (lpReplyMsg) {
				lpReplyMsg->Release();
				lpReplyMsg = NULL;
			}
			if (lpFwdMsg) {
				lpFwdMsg->Release();
				lpFwdMsg = NULL;
			}
			if (lpNewMessage) {
				lpNewMessage->Release();
				lpNewMessage = NULL;
			}
		} // end action loop

		if (lpRuleState && (lpRuleState->Value.i & ST_EXIT_LEVEL))
			break;

nextrule:
		if (lpRowSet) {
			FreeProws(lpRowSet);
			lpRowSet = NULL;
		}
	}

	if (bAddFwdFlag) {
		sForwardProps[0].ulPropTag = PR_ICON_INDEX;
		sForwardProps[0].Value.ul = ICON_MAIL_FORWARDED;

		sForwardProps[1].ulPropTag = PR_LAST_VERB_EXECUTED;
		sForwardProps[1].Value.ul = NOTEIVERB_FORWARD;

		sForwardProps[2].ulPropTag = PR_LAST_VERB_EXECUTION_TIME;
		GetSystemTimeAsFileTime(&sForwardProps[2].Value.ft);

		// set forward in msg flag
		hr = (*lppMessage)->SetProps(3, (LPSPropValue)&sForwardProps, NULL);
	}

exit:
	if (hr != hrSuccess && hr != MAPI_E_CANCEL)
		lpLogger->Log(EC_LOGLEVEL_INFO, "Error while processing rules: 0x%08X", hr);

	// The message was moved to another folder(s), do not save it in the inbox anymore, so cancel it.
	if (hr == hrSuccess && bMoved)
		hr = MAPI_E_CANCEL;
	
	if (lpECModifyTable)
		lpECModifyTable->Release();

	if (lpNewMessage)
		lpNewMessage->Release();
		
	if (lpTable)
		lpTable->Release();

	if (lpView)
		lpView->Release();

	if (lpRowSet)
		FreeProws(lpRowSet);

	if (hr != hrSuccess)
		sc -> countInc("rules", "invocations_fail");

	return hr;
}
