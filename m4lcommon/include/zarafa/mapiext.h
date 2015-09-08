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

//////////////////////////////////////////////
// Extended mapi definitions

#ifndef ECMAPIEXT_H
#define ECMAPIEXT_H

#ifndef MAPI_E_STORE_FULL
#define MAPI_E_STORE_FULL (SCODE)0x8004060c
#endif

#ifndef PR_ATTACH_CONTENT_ID
#define PR_ATTACH_CONTENT_ID (PROP_TAG(PT_TSTRING,0x3712))
#endif

#ifndef PR_ATTACH_CONTENT_ID_A
#define PR_ATTACH_CONTENT_ID_A (PROP_TAG(PT_STRING8,0x3712))
#endif

#ifndef PR_ATTACH_CONTENT_ID_W
#define PR_ATTACH_CONTENT_ID_W (PROP_TAG(PT_UNICODE,0x3712))
#endif

#ifndef PR_ATTACH_CONTENT_LOCATION
#define PR_ATTACH_CONTENT_LOCATION (PROP_TAG(PT_TSTRING,0x3713))
#endif

#ifndef PR_ATTACH_CONTENT_LOCATION_A
#define PR_ATTACH_CONTENT_LOCATION_A (PROP_TAG(PT_STRING8,0x3713))
#endif

#ifndef PR_ATTACH_CONTENT_LOCATION_W
#define PR_ATTACH_CONTENT_LOCATION_W (PROP_TAG(PT_UNICODE,0x3713))
#endif

#ifndef PR_USER_X509_CERTIFICATE
#define PR_USER_X509_CERTIFICATE (PROP_TAG(PT_MV_BINARY,0x3a70))
#endif

// it seems this is not an named prop?!
#ifndef PR_EMS_AB_X509_CERT
#define PR_EMS_AB_X509_CERT PROP_TAG(PT_MV_BINARY, 0x8c6a)
#endif

#ifndef PR_NT_SECURITY_DESCRIPTOR
#define PR_NT_SECURITY_DESCRIPTOR (PROP_TAG(PT_BINARY,0x0E27))
#endif

#ifndef PR_BODY_HTML
#define PR_BODY_HTML (PROP_TAG(PT_TSTRING,0x1013))
#endif

#ifndef PR_INTERNET_MESSAGE_ID
#define PR_INTERNET_MESSAGE_ID PROP_TAG(PT_TSTRING, 0x1035)
#endif

#ifndef PR_INTERNET_MESSAGE_ID_A
#define PR_INTERNET_MESSAGE_ID_A PROP_TAG(PT_STRING8, 0x1035)
#endif

#ifndef PR_INTERNET_MESSAGE_ID_W
#define PR_INTERNET_MESSAGE_ID_W PROP_TAG(PT_UNICODE, 0x1035)
#endif

//Same as PR_INBOUND_NEWSFEED_DN
//#ifndef PR_RULE_VERSION
//#define PR_RULE_VERSION	PROP_TAG( PT_I2, 0x668D)
//#endif

//#ifndef	PR_STORE_SLOWLINK
//#define PR_STORE_SLOWLINK PROP_TAG( PT_BOOLEAN,	0x7c0a)
//#endif

#ifndef	PR_SMTP_ADDRESS
#define PR_SMTP_ADDRESS PROP_TAG(PT_TSTRING,0x39FE)
#endif

#ifndef PR_SMTP_ADDRESS_A
#define PR_SMTP_ADDRESS_A PROP_TAG(PT_STRING8,0x39FE)
#endif

#ifndef PR_SMTP_ADDRESS_W
#define PR_SMTP_ADDRESS_W PROP_TAG(PT_UNICODE,0x39FE)
#endif

#ifndef	PR_DEF_POST_MSGCLASS
#define PR_DEF_POST_MSGCLASS PROP_TAG(PT_TSTRING, 0x36E5)
#endif

#ifndef	PR_DEF_POST_MSGCLASS_A
#define PR_DEF_POST_MSGCLASS_A PROP_TAG(PT_STRING8, 0x36E5)
#endif

#ifndef	PR_DEF_POST_MSGCLASS_W
#define PR_DEF_POST_MSGCLASS_W PROP_TAG(PT_UNICODE, 0x36E5)
#endif

#ifndef	PR_DEF_POST_DISPLAYNAME
#define PR_DEF_POST_DISPLAYNAME	PROP_TAG(PT_TSTRING, 0x36E6)
#endif

#ifndef	PR_DEF_POST_DISPLAYNAME_A
#define PR_DEF_POST_DISPLAYNAME_A	PROP_TAG(PT_STRING8, 0x36E6)
#endif

#ifndef	PR_DEF_POST_DISPLAYNAME_W
#define PR_DEF_POST_DISPLAYNAME_W	PROP_TAG(PT_UNICODE, 0x36E6)
#endif

#ifndef	PR_INTERNET_ARTICLE_NUMBER
#define PR_INTERNET_ARTICLE_NUMBER PROP_TAG(PT_LONG, 0x0E23)
#endif

#ifndef	PR_FREEBUSY_ENTRYIDS
#define PR_FREEBUSY_ENTRYIDS PROP_TAG(PT_MV_BINARY, 0x36E4)
#endif

//in the msdn, but left out of mapitags.h
#ifndef	PR_SEND_INTERNET_ENCODING	
#define PR_SEND_INTERNET_ENCODING PROP_TAG(PT_LONG, 0x3A71)
#endif

#ifndef	PR_RECIPIENT_TRACKSTATUS	
#define PR_RECIPIENT_TRACKSTATUS PROP_TAG(PT_LONG, 0x5FFF)
#endif

#ifndef	PR_RECIPIENT_FLAGS	
#define PR_RECIPIENT_FLAGS PROP_TAG(PT_LONG, 0x5FFD)
#endif

#ifndef	PR_RECIPIENT_ENTRYID	
#define PR_RECIPIENT_ENTRYID PROP_TAG(PT_BINARY, 0x5FF7)
#endif

#ifndef	PR_RECIPIENT_DISPLAY_NAME
#define PR_RECIPIENT_DISPLAY_NAME PROP_TAG(PT_TSTRING, 0x5FF6)
#endif

#ifndef	PR_RECIPIENT_DISPLAY_NAME_A
#define PR_RECIPIENT_DISPLAY_NAME_A PROP_TAG(PT_STRING8, 0x5FF6)
#endif

#ifndef	PR_RECIPIENT_DISPLAY_NAME_W
#define PR_RECIPIENT_DISPLAY_NAME_W PROP_TAG(PT_UNICODE, 0x5FF6)
#endif

#ifndef	PR_ICON_INDEX	
#define PR_ICON_INDEX PROP_TAG(PT_LONG, 0x1080)
#endif

#ifndef	PR_OST_OSTID
#define PR_OST_OSTID PROP_TAG(PT_BINARY, 0x7c04)
#endif

#ifndef	PR_OFFLINE_FOLDER
#define PR_OFFLINE_FOLDER PROP_TAG(PT_BINARY, 0x7c05)
#endif

#ifndef	PR_FAV_DISPLAY_NAME
#define PR_FAV_DISPLAY_NAME PROP_TAG(PT_TSTRING, 0x7C00)
#endif

#ifndef	PR_FAV_DISPLAY_NAME_A
#define PR_FAV_DISPLAY_NAME_A PROP_TAG(PT_STRING8, 0x7C00)
#endif

#ifndef	PR_FAV_DISPLAY_NAME_W
#define PR_FAV_DISPLAY_NAME_W PROP_TAG(PT_UNICODE, 0x7C00)
#endif

#ifndef	PR_FAV_DISPLAY_ALIAS
#define PR_FAV_DISPLAY_ALIAS PROP_TAG(PT_TSTRING, 0x7C01)
#endif

#ifndef	PR_FAV_DISPLAY_ALIAS_A
#define PR_FAV_DISPLAY_ALIAS_A PROP_TAG(PT_STRING8, 0x7C01)
#endif

#ifndef	PR_FAV_DISPLAY_ALIAS_W
#define PR_FAV_DISPLAY_ALIAS_W PROP_TAG(PT_UNICODE, 0x7C01)
#endif

#ifndef	PR_FAV_PUBLIC_SOURCE_KEY
#define PR_FAV_PUBLIC_SOURCE_KEY PROP_TAG(PT_BINARY, 0x7C02)
#endif

#ifndef	PR_FAV_AUTOSUBFOLDERS
#define PR_FAV_AUTOSUBFOLDERS PROP_TAG(PT_LONG, 0x7d01)
#endif

#ifndef	PR_FAV_PARENT_SOURCE_KEY
#define PR_FAV_PARENT_SOURCE_KEY PROP_TAG(PT_BINARY, 0x7d02)
#endif

#ifndef	PR_FAV_LEVEL_MASK
#define PR_FAV_LEVEL_MASK PROP_TAG(PT_LONG, 0x7D03)
#endif

#ifndef	PR_FAV_KNOWN_SUBS
#define PR_FAV_KNOWN_SUBS PROP_TAG(PT_BINARY, 0x7D04)
#endif

#ifndef	PR_FAV_GUID_MAP
#define PR_FAV_GUID_MAP PROP_TAG(PT_BINARY, 0x7D05)
#endif

#ifndef	PR_FAV_KNOWN_DELS_OLD
#define PR_FAV_KNOWN_DELS_OLD PROP_TAG(PT_BINARY, 0x7D06)
#endif

#ifndef	PR_FAV_INHERIT_AUTO
#define PR_FAV_INHERIT_AUTO PROP_TAG(PT_LONG, 0x7d07)
#endif

#ifndef	PR_FAV_DEL_SUBS
#define PR_FAV_DEL_SUBS PROP_TAG(PT_BINARY, 0x7D08)
#endif

#ifndef	PR_FAV_CONTAINER_CLASS
#define PR_FAV_CONTAINER_CLASS PROP_TAG(PT_TSTRING, 0x7D09)
#endif

#ifndef	PR_FAV_CONTAINER_CLASS_A
#define PR_FAV_CONTAINER_CLASS_A PROP_TAG(PT_STRING8, 0x7D09)
#endif

#ifndef	PR_FAV_CONTAINER_CLASS_W
#define PR_FAV_CONTAINER_CLASS_W PROP_TAG(PT_UNICODE, 0x7D09)
#endif

#ifndef	PR_IN_REPLY_TO_ID
#define PR_IN_REPLY_TO_ID PROP_TAG(PT_TSTRING, 0x1042)
#endif

#ifndef	PR_IN_REPLY_TO_ID_A
#define PR_IN_REPLY_TO_ID_A PROP_TAG(PT_STRING8, 0x1042)
#endif

#ifndef	PR_IN_REPLY_TO_ID_W
#define PR_IN_REPLY_TO_ID_W PROP_TAG(PT_UNICODE, 0x1042)
#endif

#ifndef PR_ATTACH_FLAGS
#define PR_ATTACH_FLAGS PROP_TAG(PT_LONG, 0x3714)
#endif

#ifndef PR_ATTACHMENT_LINKID
#define PR_ATTACHMENT_LINKID PROP_TAG(PT_LONG, 0x7FFA)
#endif

#ifndef PR_EXCEPTION_STARTTIME
#define PR_EXCEPTION_STARTTIME PROP_TAG(PT_SYSTIME, 0x7FFB)
#endif

#ifndef PR_EXCEPTION_ENDTIME
#define PR_EXCEPTION_ENDTIME PROP_TAG(PT_SYSTIME, 0x7FFC)
#endif

#ifndef PR_EXCEPTION_REPLACETIME
#define PR_EXCEPTION_REPLACETIME PROP_TAG(PT_SYSTIME, 0x7FF9)
#endif

#ifndef PR_ATTACHMENT_FLAGS
#define PR_ATTACHMENT_FLAGS PROP_TAG(PT_LONG, 0x7FFD)
#endif

#ifndef PR_ATTACHMENT_HIDDEN
#define PR_ATTACHMENT_HIDDEN PROP_TAG(PT_BOOLEAN, 0x7FFE)
#endif

#ifndef PR_ATTACHMENT_CONTACTPHOTO
#define PR_ATTACHMENT_CONTACTPHOTO PROP_TAG(PT_BOOLEAN, 0x7FFF)
#endif

#ifndef PR_CONFLICT_ITEMS
#define PR_CONFLICT_ITEMS PROP_TAG(PT_MV_BINARY,0x1098)
#endif

#ifndef PR_INTERNET_APPROVED
#define PR_INTERNET_APPROVED PROP_TAG(PT_TSTRING,0x1030)
#endif

#ifndef PR_INTERNET_APPROVED_A
#define PR_INTERNET_APPROVED_A PROP_TAG(PT_STRING8,0x1030)
#endif

#ifndef PR_INTERNET_APPROVED_W
#define PR_INTERNET_APPROVED_W PROP_TAG(PT_UNICODE,0x1030)
#endif

#ifndef PR_INTERNET_CONTROL
#define PR_INTERNET_CONTROL PROP_TAG(PT_TSTRING,0x1031)
#endif

#ifndef PR_INTERNET_CONTROL_A
#define PR_INTERNET_CONTROL_A PROP_TAG(PT_STRING8,0x1031)
#endif

#ifndef PR_INTERNET_CONTROL_W
#define PR_INTERNET_CONTROL_W PROP_TAG(PT_UNICODE,0x1031)
#endif

#ifndef PR_INTERNET_DISTRIBUTION
#define PR_INTERNET_DISTRIBUTION PROP_TAG(PT_TSTRING,0x1032)
#endif

#ifndef PR_INTERNET_DISTRIBUTION_A
#define PR_INTERNET_DISTRIBUTION_A PROP_TAG(PT_STRING8,0x1032)
#endif

#ifndef PR_INTERNET_DISTRIBUTION_W
#define PR_INTERNET_DISTRIBUTION_W PROP_TAG(PT_UNICODE,0x1032)
#endif

#ifndef PR_INTERNET_FOLLOWUP_TO
#define PR_INTERNET_FOLLOWUP_TO PROP_TAG(PT_TSTRING,0x1033)
#endif

#ifndef PR_INTERNET_FOLLOWUP_TO_A
#define PR_INTERNET_FOLLOWUP_TO_A PROP_TAG(PT_STRING8,0x1033)
#endif

#ifndef PR_INTERNET_FOLLOWUP_TO_W
#define PR_INTERNET_FOLLOWUP_TO_W PROP_TAG(PT_UNICODE,0x1033)
#endif

#ifndef PR_INTERNET_LINES
#define PR_INTERNET_LINES PROP_TAG(PT_LONG,0x1034)
#endif

#ifndef PR_INTERNET_NEWSGROUPS
#define PR_INTERNET_NEWSGROUPS PROP_TAG(PT_TSTRING,0x1036)
#endif

#ifndef PR_INTERNET_NEWSGROUPS_A
#define PR_INTERNET_NEWSGROUPS_A PROP_TAG(PT_STRING8,0x1036)
#endif

#ifndef PR_INTERNET_NEWSGROUPS_W
#define PR_INTERNET_NEWSGROUPS_W PROP_TAG(PT_UNICODE,0x1036)
#endif

#ifndef PR_INTERNET_NNTP_PATH
#define PR_INTERNET_NNTP_PATH PROP_TAG(PT_TSTRING,0x1038)
#endif

#ifndef PR_INTERNET_NNTP_PATH_A
#define PR_INTERNET_NNTP_PATH_A PROP_TAG(PT_STRING8,0x1038)
#endif

#ifndef PR_INTERNET_NNTP_PATH_W
#define PR_INTERNET_NNTP_PATH_W PROP_TAG(PT_UNICODE,0x1038)
#endif

#ifndef PR_INTERNET_ORGANIZATION
#define PR_INTERNET_ORGANIZATION PROP_TAG(PT_TSTRING,0x1037)
#endif

#ifndef PR_INTERNET_ORGANIZATION_A
#define PR_INTERNET_ORGANIZATION_A PROP_TAG(PT_STRING8,0x1037)
#endif

#ifndef PR_INTERNET_ORGANIZATION_W
#define PR_INTERNET_ORGANIZATION_W PROP_TAG(PT_UNICODE,0x1037)
#endif

#ifndef PR_INTERNET_PRECEDENCE
#define PR_INTERNET_PRECEDENCE PROP_TAG(PT_TSTRING,0x1041)
#endif

#ifndef PR_INTERNET_PRECEDENCE_A
#define PR_INTERNET_PRECEDENCE_A PROP_TAG(PT_STRING8,0x1041)
#endif

#ifndef PR_INTERNET_PRECEDENCE_W
#define PR_INTERNET_PRECEDENCE_W PROP_TAG(PT_UNICODE,0x1041)
#endif

#ifndef PR_INTERNET_REFERENCES
#define PR_INTERNET_REFERENCES PROP_TAG(PT_TSTRING,0x1039)
#endif

#ifndef PR_INTERNET_REFERENCES_A
#define PR_INTERNET_REFERENCES_A PROP_TAG(PT_STRING8,0x1039)
#endif

#ifndef PR_INTERNET_REFERENCES_W
#define PR_INTERNET_REFERENCES_W PROP_TAG(PT_UNICODE,0x1039)
#endif

#ifndef UNICODE_NEWSGROUP_NAME
#define PR_NEWSGROUP_NAME PROP_TAG(PT_TSTRING,0x0E24)
#endif

#ifndef PR_NNTP_XREF
#define PR_NNTP_XREF PROP_TAG(PT_TSTRING,0x1040)
#endif

#ifndef PR_NNTP_XREF_A
#define PR_NNTP_XREF_A PROP_TAG(PT_STRING8,0x1040)
#endif

#ifndef PR_NNTP_XREF_W
#define PR_NNTP_XREF_W PROP_TAG(PT_UNICODE,0x1040)
#endif

#ifndef PR_POST_FOLDER_ENTRIES
#define PR_POST_FOLDER_ENTRIES PROP_TAG(PT_BINARY,0x103B)
#endif

#ifndef PR_POST_FOLDER_NAMES
#define PR_POST_FOLDER_NAMES PROP_TAG(PT_TSTRING,0x103C)
#endif

#ifndef PR_POST_FOLDER_NAMES_A
#define PR_POST_FOLDER_NAMES_A PROP_TAG(PT_STRING8,0x103C)
#endif

#ifndef PR_POST_FOLDER_NAMES_W
#define PR_POST_FOLDER_NAMES_W PROP_TAG(PT_UNICODE,0x103C)
#endif

#ifndef PR_POST_REPLY_FOLDER_ENTRIES
#define PR_POST_REPLY_DENIED PROP_TAG(PT_BINARY,0x103F)
#endif

#ifndef PR_POST_REPLY_FOLDER_ENTRIES
#define PR_POST_REPLY_FOLDER_ENTRIES PROP_TAG(PT_BINARY,0x103D)
#endif

#ifndef PR_POST_REPLY_FOLDER_NAMES
#define PR_POST_REPLY_FOLDER_NAMES PROP_TAG(PT_TSTRING,0x103E)
#endif

#ifndef PR_POST_REPLY_FOLDER_NAMES_A
#define PR_POST_REPLY_FOLDER_NAMES_A PROP_TAG(PT_STRING8,0x103E)
#endif

#ifndef PR_POST_REPLY_FOLDER_NAMES_W
#define PR_POST_REPLY_FOLDER_NAMES_W PROP_TAG(PT_UNICODE,0x103E)
#endif

#ifndef PR_SUPERSEDES
#define PR_SUPERSEDES PROP_TAG(PT_TSTRING,0x103A)
#endif

#ifndef PR_SUPERSEDES_A
#define PR_SUPERSEDES_A PROP_TAG(PT_STRING8,0x103A)
#endif

#ifndef PR_SUPERSEDES_W
#define PR_SUPERSEDES_W PROP_TAG(PT_UNICODE,0x103A)
#endif

#ifndef PR_7BIT_DISPLAY_NAME_W
#define PR_7BIT_DISPLAY_NAME_W PROP_TAG(PT_UNICODE, PROP_ID(PR_7BIT_DISPLAY_NAME))
#endif

#ifndef PR_ASSOCIATED
#define PR_ASSOCIATED					PROP_TAG(PT_BOOLEAN, 0x67AA)
#endif

#ifndef PR_PROCESSED
#define PR_PROCESSED					PROP_TAG(PT_BOOLEAN, 0x7D01)
#endif

#define PR_PACKED_NAME_PROPS		PROP_TAG(PT_BINARY, 0x361C)

#define PR_IPM_APPOINTMENT_ENTRYID	PROP_TAG(PT_BINARY, 0x36D0)
#define PR_IPM_CONTACT_ENTRYID		PROP_TAG(PT_BINARY, 0x36D1)
#define PR_IPM_JOURNAL_ENTRYID		PROP_TAG(PT_BINARY, 0x36D2)
#define PR_IPM_NOTE_ENTRYID			PROP_TAG(PT_BINARY, 0x36D3)
#define PR_IPM_TASK_ENTRYID			PROP_TAG(PT_BINARY, 0x36D4)
#define PR_REM_ONLINE_ENTRYID		PROP_TAG(PT_BINARY, 0x36D5)
#define PR_REM_OFFLINE_ENTRYID		PROP_TAG(PT_BINARY, 0x36D6)
#define PR_IPM_DRAFTS_ENTRYID		PROP_TAG(PT_BINARY, 0x36D7)
#define PR_IPM_OL2007_ENTRYIDS		PROP_TAG(PT_BINARY, 0x36D9)

/*
PR_IPM_OL2007_ENTRYIDS:
	This is a single binary property containing the entryids for:
	- 'Rss feeds' folder
	- The searchfolder 'Tracked mail processing'
	- The searchfolder 'To-do list'

	However, it is encoded something like the following:

	01803200 (type: rss feeds ?)
	0100
	2E00
	00000000B774162F0098C84182DE9E4358E4249D01000B41FF66083D464EA7E34D6026C9B143000000006DDA0000 (entryid)
	04803200 (type: tracked mail processing ?)
	0100
	2E00
	00000000B774162F0098C84182DE9E4358E4249D01000B41FF66083D464EA7E34D6026C9B143000000006DDB0000 (entryid)
	02803200 (type: todo list ?)
	0100
	2E00
	00000000B774162F0098C84182DE9E4358E4249D01000B41FF66083D464EA7E34D6026C9B143000000006DE40000 (entryid)
	00000000 (terminator?)

	It may also only contain the rss feeds entryid, and then have the 00000000 terminator directly after the entryid:

	01803200 (type: rss feeds ?)
	0100
	2E00
	00000000B774162F0098C84182DE9E4358E4249D01000B41FF66083D464EA7E34D6026C9B143000000006DDA0000 (entryid)
	00000000 (terminator?)

PR_ADDITIONAL_REN_ENTRYIDS:
	This is a multivalued property which contains entry IDs for certain special folders. 
	The first 5 (0-4) entries in this multivalued property are as follows: 
		0 - Conflicts folder
		1 - Sync Issues folder
		2 - Local Failures folder
		3 - Server Failures folder
		4 - Junk E-mail Folder
		5 - sfSpamTagDontUse (unknown what this is, disable olk spam stuff?)
*/
#define PR_ADDITIONAL_REN_ENTRYIDS	PROP_TAG(PT_MV_BINARY, 0x36D8)

// Extra MAPI defines
//GetHierarchyTable(..) and GetContentsTable(..)
#define SHOW_SOFT_DELETES		((ULONG) 0x00000002)
//DeleteFolder(...)
#define DELETE_HARD_DELETE		((ULONG) 0x00000010)

#define PR_MDN_DISPOSITION_TYPE			PROP_TAG(PT_STRING8, 0x0080)
#define PR_MDN_DISPOSITION_SENDINGMODE	PROP_TAG(PT_STRING8, 0x0081)

#define PR_LAST_VERB_EXECUTED			PROP_TAG(PT_LONG, 0x1081)
#define PR_LAST_VERB_EXECUTION_TIME		PROP_TAG(PT_SYSTIME, 0x1082)

/* defines from [MS-OXOMSG].pdf, except they're really decimal, and not hexadecimal. */
#define NOTEIVERB_OPEN				0	// Open
#define NOTEIVERB_REPLYTOSENDER		102	// ReplyToSender
#define NOTEIVERB_REPLYTOALL		103	// ReplyToAll
#define NOTEIVERB_FORWARD			104	// Forward
#define NOTEIVERB_PRINT				105	// Print
#define NOTEIVERB_SAVEAS			106	// Save as
#define NOTEIVERB_REPLYTOFOLDER		108	// ReplyToFolder
#define NOTEIVERB_SAVE				500	// Save
#define NOTEIVERB_PROPERTIES		510	// properties
#define NOTEIVERB_FOLLOWUP			511	// Followup
#define NOTEIVERB_ACCEPT			512	// Accept
#define NOTEIVERB_TENTATIVE			513	// Tentative
#define NOTEIVERB_REJECT			514	// Reject
#define NOTEIVERB_DECLINE			515	// Decline
#define NOTEIVERB_INVITE			516	// Invite
#define NOTEIVERB_UPDATE			517	// Update
#define NOTEIVERB_CANCEL			518	// Cancel
#define NOTEIVERB_SILENTINVITE		519	// SilentInvite
#define NOTEIVERB_SILENTCANCEL		520	// SilentCancel
#define NOTEIVERB_RECALL_MESSAGE	521	// RecallMessage
#define NOTEIVERB_FORWARD_RESPONSE	522	// ForwardResponse

#define ICON_FOLDER_DEFAULT			0xFFFFFFFF	// Different from documentation: -1 is dependant on folder the item is in

/* defines from [MS-OXOMSG].pdf for email icons */
#define ICON_MAIL_READ				0x00000100	// Read mail
#define ICON_MAIL_UNREAD			0x00000101	// Unread mail
#define ICON_MAIL_SUBMITTED			0x00000102	// Submitted mail
#define ICON_MAIL_UNSENT			0x00000103	// Unsent mail
#define ICON_MAIL_RECEIPT			0x00000104	// Receipt mail
#define ICON_MAIL_REPLIED			0x00000105	// Replied mail
#define ICON_MAIL_FORWARDED			0x00000106	// Forwarded mail
#define ICON_MAIL_REMOTE			0x00000107	// Remote mail
#define ICON_MAIL_DELIVERY_RECEIPT	0x00000108	// Delivery receipt 
#define ICON_MAIL_READ_RECEIPT		0x00000109	// Read receipt 
#define ICON_MAIL_NONDELIVERY_RECEIPT 0x0000010A	// Non-delivery Report
#define ICON_MAIL_NONREAD_RECEIPT	0x0000010B	// Non-read receipt
#define ICON_MAIL_RECALL_S			0x0000010C	// Recall_S mails
#define ICON_MAIL_RECALL_F			0x0000010D	// Recall_F mail
#define ICON_MAIL_TRACKING			0x0000010E	// Tracking mail
#define ICON_MAIL_OOF				0x0000011B	// Out of Office mail
#define ICON_MAIL_RECALL			0x0000011C	// Recall mail
#define ICON_MAIL_TRACKED			0x00000130	// Tracked mail

/* defines from [MS-OXOCNTC].pdf for contact icons, but nothing useful was there */
#define ICON_CONTACT_USER			0x00000200		// Contact
#define ICON_CONTACT_ADDRESSBOOK	0x00000201		// Contact that was imported from GAB
#define ICON_CONTACT_DISTLIST		0x00000202		// Distribution list

/* defines from [MS-OXONOTE].pdf for note icons*/
#define ICON_NOTE					0x00000300		// Note, only documented in pdf
#define ICON_NOTE_BLUE				0x00000300
#define ICON_NOTE_GREEN				0x00000301
#define ICON_NOTE_PINK				0x00000302
#define ICON_NOTE_YELLOW			0x00000303
#define ICON_NOTE_WHITE				0x00000304

/* defines from [MS-OXOCAL].pdf for calendar */
#define ICON_APPT_APPOINTMENT		0x00000400		// Single-instance appointment on Appointment object  
#define ICON_APPT_RECURRING			0x00000401		// Recurring appointment on Appointment object  
#define ICON_APPT_MEETING_SINGLE	0x00000402		// Single-instance meeting on Meeting object  
#define ICON_APPT_MEETING_RECURRING	0x00000403		// Recurring meeting on Meeting object  
#define ICON_APPT_MEETING_REQUEST	0x00000404		// meeting request/full update on Meeting Request object, Meeting Update object 
#define ICON_APPT_MEETING_ACCEPT	0x00000405		// Accept on Meeting Response object  
#define ICON_APPT_MEETING_DECLINE	0x00000406		// Decline on Meeting Response object  
#define ICON_APPT_MEETING_TENTATIVE	0x00000407		// Tentatively accept on Meeting Response object  
#define ICON_APPT_MEETING_CANCEL	0x00000408		// Cancellation on Meeting Cancellation object  
#define ICON_APPT_MEETING_UPDATE	0x00000409		// informational update  on Meeting Update object  
#define ICON_APPT_MEETING_FORWARD	0x0000040b		// Forward notification on Meeting Forward Notification object  

/* defines from [MS-OXOTASK].pdf for task icons */
#define ICON_TASK_NORMAL			0x00000500		// None of the other conditions apply
#define ICON_TASK_RECURRING			0x00000501		// The Task object has not been assigned and is a recurring task
#define ICON_TASK_ASSIGNEE			0x00000502		// The Task object is the task assignee's copy of the Task object
#define ICON_TASK_ASSIGNER			0x00000503		// The Task object is the task assigner's copy of the Task object

/* defines from [MS-OXOJRNL].pdf for journal icons */
#define ICON_JOURNAL_CONVERSATION	0x00000601		// Conversation
#define ICON_JOURNAL_DOCUMENT		0x00000612		// Document
#define ICON_JOURNAL_EMAIL			0x00000602		// E-mail Message
#define ICON_JOURNAL_FAX			0x00000609		// Fax
#define ICON_JOURNAL_LETTER			0x0000060C		// Letter
#define ICON_JOURNAL_MEETING		0x00000613		// Meeting
#define ICON_JOURNAL_MEETING_CANCEL	0x00000614		// Meeting cancellation
#define ICON_JOURNAL_MEETING_REQUEST	0x00000603		// Meeting request
#define ICON_JOURNAL_MEETING_RESPONSE	0x00000604		// Meeting response
#define ICON_JOURNAL_ACCESS			0x00000610		// Microsoft Office Access
#define ICON_JOURNAL_EXCEL			0x0000060E		// Microsoft Office Excel
#define ICON_JOURNAL_POWERPOINT		0x0000060F		// Microsoft Office PowerPoint
#define ICON_JOURNAL_WORD			0x0000060D		// Microsoft Office Word
#define ICON_JOURNAL_NOTE			0x00000608		// Note
#define ICON_JOURNAL_PHONE_CALL		0x0000060A		// Phone call
#define ICON_JOURNAL_REMOTE_SESSION	0x00000615		// Remote session
#define ICON_JOURNAL_TASK			0x0000060B		// Task
#define ICON_JOURNAL_TASK_REQUEST	0x00000606		// Task request
#define ICON_JOURNAL_TASK_RESPONSE	0x00000607		// Task response
#define ICON_JOURNAL_OTHER			0x00000003		// Other

//The following properties are used in MAPI restrictions: 
// Used when searching for attachment contents. 
#define PR_SEARCH_ATTACHMENTS			PROP_TAG(PT_TSTRING, 0x0EA5)
#define PR_SEARCH_ATTACHMENTS_A			PROP_TAG(PT_STRING8, 0x0EA5)
#define PR_SEARCH_ATTACHMENTS_W			PROP_TAG(PT_UNICODE, 0x0EA5)

// Used when searching for email address or display names the message was sent to.
#define PR_SEARCH_RECIP_EMAIL_TO		PROP_TAG(PT_TSTRING, 0x0EA6)
#define PR_SEARCH_RECIP_EMAIL_TO_A		PROP_TAG(PT_STRING8, 0x0EA6)
#define PR_SEARCH_RECIP_EMAIL_TO_W		PROP_TAG(PT_UNICODE, 0x0EA6)

// Used when searching for email address or display names the message was Cc'ed.
#define PR_SEARCH_RECIP_EMAIL_CC		PROP_TAG(PT_TSTRING, 0x0EA7)
#define PR_SEARCH_RECIP_EMAIL_CC_A		PROP_TAG(PT_STRING8, 0x0EA7)
#define PR_SEARCH_RECIP_EMAIL_CC_W		PROP_TAG(PT_UNICODE, 0x0EA7)

// Used when searching for email address or display names the message was Bcc'ed. 
// This is only interesting for messages not yet sent otherwise the BCC information will not be there.
#define PR_SEARCH_RECIP_EMAIL_BCC		PROP_TAG(PT_TSTRING, 0x0EA8)
#define PR_SEARCH_RECIP_EMAIL_BCC_A		PROP_TAG(PT_STRING8, 0x0EA8)
#define PR_SEARCH_RECIP_EMAIL_BCC_W		PROP_TAG(PT_UNICODE, 0x0EA8)

#define PR_FOLDER_XVIEWINFO_E			PROP_TAG(PT_BINARY, 0x36E0)
#define PR_FOLDER_DISPLAY_FLAGS			PROP_TAG(PT_BINARY, 0x36DA)
#define PR_NET_FOLDER_FLAGS				PROP_TAG(PT_LONG, 0x36DE)
#define PR_FOLDER_WEBVIEWINFO			PROP_TAG(PT_BINARY, 0x36DF) 
#define PR_FOLDER_VIEWS_ONLY			PROP_TAG(PT_LONG, 0x36E1)

#define MDB_ONLINE ((ULONG) 0x00000100)
#define MAPI_NO_CACHE ((ULONG) 0x00000200)


#define PR_MANAGED_FOLDER_INFORMATION	PROP_TAG(PT_LONG, 0x672D)
#define PR_MANAGED_FOLDER_STORAGE_QUOTA	PROP_TAG(PT_LONG, 0x6731)

/* delegate properties */
/* from exchange private range? */
#define PR_SCHDINFO_DELEGATE_NAMES			PROP_TAG(PT_MV_TSTRING, 0x6844)
#define PR_SCHDINFO_DELEGATE_ENTRYIDS		PROP_TAG(PT_MV_BINARY, 0x6845)
#define PR_DELEGATE_FLAGS					PROP_TAG(PT_MV_LONG, 0x686B)

#define DELEGATE_FLAG_SEE_PRIVATE	1

#define PR_TODO_ITEM_FLAGS				PROP_TAG(PT_LONG, 0x0E2B)
#define PR_FOLLOWUP_ICON				PROP_TAG(PT_LONG, 0x1095)
#define PR_FLAG_STATUS					PROP_TAG(PT_LONG, 0x1090)
#define PR_FLAG_COMPLETE_TIME			PROP_TAG(PT_SYSTIME, 0x1091)
#define PR_INETMAIL_OVERRIDE_FORMAT		PROP_TAG(PT_LONG, 0x5902)

/* GetMessageStatus */
#define MSGSTATUS_DRAFT 0x20000
#define MSGSTATUS_ANSWERED 0x10000


#define FL_PREFIX_ON_ANY_WORD 			0x00000010
#define FL_PHRASE_MATCH					0x00000020


/* from edkmapi.h .. or so they say on internet */
/* Values of PR_NDR_REASON_CODE */
#ifndef MAPI_REASON
#define MAPI_REASON(_code)	((LONG) _code)
#define MAPI_REASON_TRANSFER_FAILED           MAPI_REASON( 0 )
#define MAPI_REASON_TRANSFER_IMPOSSIBLE       MAPI_REASON( 1 )
#define MAPI_REASON_CONVERSION_NOT_PERFORMED  MAPI_REASON( 2 )
#define MAPI_REASON_PHYSICAL_RENDITN_NOT_DONE MAPI_REASON( 3 )
#define MAPI_REASON_PHYSICAL_DELIV_NOT_DONE   MAPI_REASON( 4 )
#define MAPI_REASON_RESTRICTED_DELIVERY       MAPI_REASON( 5 )
#define MAPI_REASON_DIRECTORY_OPERATN_FAILED  MAPI_REASON( 6 )
#endif

/* new addressbook properties */
#define PR_DISPLAY_TYPE_EX				PROP_TAG(PT_LONG, 0x3905)

/* EMSAbTag.h */
/* 6: MV DESCRIPTION? */
#define PR_EMS_AB_ROOM_CAPACITY			PROP_TAG(PT_LONG, 0x0807)
/* 8: MV resource type:<>? */
#define PR_EMS_AB_ROOM_DESCRIPTION		PROP_TAG(PT_STRING8, 0x0809)

/* PR_DISPLAY_TYPE_EX values */
/*  PR_DISPLAY_TYPEs (mapidefs.h)
#define DT_MAILUSER         ((ULONG) 0x00000000)
#define DT_DISTLIST         ((ULONG) 0x00000001)
#define DT_FORUM            ((ULONG) 0x00000002)
#define DT_AGENT            ((ULONG) 0x00000003)
#define DT_ORGANIZATION     ((ULONG) 0x00000004)
#define DT_PRIVATE_DISTLIST ((ULONG) 0x00000005)
#define DT_REMOTE_MAILUSER  ((ULONG) 0x00000006)
*/
#define DT_ROOM	            ((ULONG) 0x00000007)
#define DT_EQUIPMENT        ((ULONG) 0x00000008)
#define DT_SEC_DISTLIST     ((ULONG) 0x00000009)

/* PR_DISPLAY_TYPE_EX flags */
#define DTE_FLAG_REMOTE_VALID 0x80000000 /* multiserver? */
#define DTE_FLAG_ACL_CAPABLE  0x40000000 /* on for DT_MAILUSER and DT_SEQ_DISTLIST */
#define DTE_MASK_REMOTE       0x0000ff00
#define DTE_MASK_LOCAL        0x000000ff
 
#define DTE_IS_REMOTE_VALID(v)	(!!((v) & DTE_FLAG_REMOTE_VALID))
#define DTE_IS_ACL_CAPABLE(v)	(!!((v) & DTE_FLAG_ACL_CAPABLE))
#define DTE_REMOTE(v)		(((v) & DTE_MASK_REMOTE) >> 8)
#define DTE_LOCAL(v)		((v) & DTE_MASK_LOCAL)

/* Extra ADRPARM ulFlags values */
#define AB_UNICODEUI	0x00000040
/* these are custom names! */
#define AB_RESERVED1	0x00000100
#define AB_LOCK_NON_ACL	0x00000200

#define PR_ASSOCIATED_SHARING_PROVIDER			PROP_TAG(PT_CLSID, 0x0ea0)
#define PR_EMSMDB_SECTION_UID					PROP_TAG(PT_BINARY, 0x3d15)
#define PR_EMSMDB_LEGACY						PROP_TAG(PT_BOOLEAN, 0x3D18)
#define PR_EMSABP_USER_UID						PROP_TAG(PT_BINARY, 0x3D1A)

#define PR_ARCHIVE_TAG			PROP_TAG(PT_BINARY, 0x3018)
#define PR_ARCHIVE_PERIOD		PROP_TAG(PT_LONG, 0x301e)
#define PR_ARCHIVE_DATE			PROP_TAG(PT_SYSTIME, 0x301f)
#define PR_RETENTION_FLAGS		PROP_TAG(PT_LONG, 0x301d)
#define PR_RETENTION_DATE		PROP_TAG(PT_SYSTIME, 0x301c)
#define PR_POLICY_TAG			PROP_TAG(PT_BINARY, 0x3019)
#define PR_ROAMING_DATATYPES	PROP_TAG(PT_LONG, 0x7c06)

#define PR_ITEM_TMPFLAGS		PROP_TAG(PT_LONG, 0x1097)
#define PR_SECURE_SUBMIT_FLAGS	PROP_TAG(PT_LONG, 0x65C6)
#define PR_SECURITY_FLAGS		PROP_TAG(PT_LONG, 0x6E01)

#define PR_CONVERSATION_ID		PROP_TAG(PT_BINARY, 0x3013)

#define PR_AB_CHOOSE_DIRECTORY_AUTOMATICALLY PROP_TAG( PT_BOOLEAN, 0x3D1C)

#define PR_STORE_UNICODE_MASK	PROP_TAG(PT_LONG, 0x340f)

#define PR_PROCESS_MEETING_REQUESTS				PROP_TAG(PT_BOOLEAN, 0x686d)
#define PR_DECLINE_CONFLICTING_MEETING_REQUESTS	PROP_TAG(PT_BOOLEAN, 0x686f)
#define PR_DECLINE_RECURRING_MEETING_REQUESTS	PROP_TAG(PT_BOOLEAN, 0x686e)

#define PR_SCHDINFO_RESOURCE_TYPE	PROP_TAG(PT_LONG, 0x6841)

// Notify long term entryid, do not use this!
#define fnevLongTermEntryIDs ((ULONG) 0x20000000)

#define PERSIST_SENTINEL 			0x0000
#define RSF_PID_RSS_SUBSCRIPTION	0x8001
#define RSF_PID_SEND_AND_TRACK		0x8002
#define RSF_PID_TODO_SEARCH			0x8004
#define RSF_PID_CONV_ACTIONS		0x8006
#define RSF_PID_COMBINED_ACTIONS	0x8007
#define RSF_PID_SUGGESTED_CONTACTS	0x8008

#define RSF_ELID_HEADER				0x0002
#define RSF_ELID_ENTRYID			0x0001
#define ELEMENT_SENTINEL			0x0000

#define PR_PROFILE_MDB_DN			PROP_TAG(PT_STRING8, 0x7CFF)
#define PR_FORCE_USE_ENTRYID_SERVER PROP_TAG(PT_BOOLEAN, 0x7CFE)

#endif