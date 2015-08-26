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

#ifndef __M4L_MAPICODE_H_
#define __M4L_MAPICODE_H_
#define MAPICODE_H

/*
 * MAPI for linux
 *
 * mapicode.h - return codes
 *
 * (C) Zarafa 2005
 *
 */

#include "platform.h"

#define MAKE_MAPI_SCODE(sev,fac,code) \
    ((SCODE) (((ULONG)(sev)<<31) | ((ULONG)(fac)<<16) | ((ULONG)(code))) )

#define MAKE_MAPI_E( err )  (MAKE_MAPI_SCODE( 1, FACILITY_ITF, err ))
#define MAKE_MAPI_S( warn ) (MAKE_MAPI_SCODE( 0, FACILITY_ITF, warn ))

/* General errors (used by more than one MAPI object) */

#define MAPI_E_CALL_FAILED                              E_FAIL
#define MAPI_E_NOT_ENOUGH_MEMORY                        E_OUTOFMEMORY
#define MAPI_E_INVALID_PARAMETER                        E_INVALIDARG
#define MAPI_E_INTERFACE_NOT_SUPPORTED                  E_NOINTERFACE
#define MAPI_E_NO_ACCESS                                E_ACCESSDENIED

#define MAPI_E_NO_SUPPORT                               MAKE_MAPI_E( 0x102 )
#define MAPI_E_BAD_CHARWIDTH                            MAKE_MAPI_E( 0x103 )
#define MAPI_E_STRING_TOO_LONG                          MAKE_MAPI_E( 0x105 )
#define MAPI_E_UNKNOWN_FLAGS                            MAKE_MAPI_E( 0x106 )
#define MAPI_E_INVALID_ENTRYID                          MAKE_MAPI_E( 0x107 )
#define MAPI_E_INVALID_OBJECT                           MAKE_MAPI_E( 0x108 )
#define MAPI_E_OBJECT_CHANGED                           MAKE_MAPI_E( 0x109 )
#define MAPI_E_OBJECT_DELETED                           MAKE_MAPI_E( 0x10A )
#define MAPI_E_BUSY                                     MAKE_MAPI_E( 0x10B )
#define MAPI_E_NOT_ENOUGH_DISK                          MAKE_MAPI_E( 0x10D )
#define MAPI_E_NOT_ENOUGH_RESOURCES                     MAKE_MAPI_E( 0x10E )
#define MAPI_E_NOT_FOUND                                MAKE_MAPI_E( 0x10F )
#define MAPI_E_VERSION                                  MAKE_MAPI_E( 0x110 )
#define MAPI_E_LOGON_FAILED                             MAKE_MAPI_E( 0x111 )
#define MAPI_E_SESSION_LIMIT                            MAKE_MAPI_E( 0x112 )
#define MAPI_E_USER_CANCEL                              MAKE_MAPI_E( 0x113 )
#define MAPI_E_UNABLE_TO_ABORT                          MAKE_MAPI_E( 0x114 )
#define MAPI_E_NETWORK_ERROR                            MAKE_MAPI_E( 0x115 )
#define MAPI_E_DISK_ERROR                               MAKE_MAPI_E( 0x116 )
#define MAPI_E_TOO_COMPLEX                              MAKE_MAPI_E( 0x117 )
#define MAPI_E_BAD_COLUMN                               MAKE_MAPI_E( 0x118 )
#define MAPI_E_EXTENDED_ERROR                           MAKE_MAPI_E( 0x119 )
#define MAPI_E_COMPUTED                                 MAKE_MAPI_E( 0x11A )
#define MAPI_E_CORRUPT_DATA                             MAKE_MAPI_E( 0x11B )
#define MAPI_E_UNCONFIGURED                             MAKE_MAPI_E( 0x11C )
#define MAPI_E_FAILONEPROVIDER                          MAKE_MAPI_E( 0x11D )
#define MAPI_E_UNKNOWN_CPID                             MAKE_MAPI_E( 0x11E )
#define MAPI_E_UNKNOWN_LCID                             MAKE_MAPI_E( 0x11F )

/* Flavors of E_ACCESSDENIED, used at logon */

#define MAPI_E_PASSWORD_CHANGE_REQUIRED                 MAKE_MAPI_E( 0x120 )
#define MAPI_E_PASSWORD_EXPIRED                         MAKE_MAPI_E( 0x121 )
#define MAPI_E_INVALID_WORKSTATION_ACCOUNT              MAKE_MAPI_E( 0x122 )
#define MAPI_E_INVALID_ACCESS_TIME                      MAKE_MAPI_E( 0x123 )
#define MAPI_E_ACCOUNT_DISABLED                         MAKE_MAPI_E( 0x124 )

/* MAPI base function and status object specific errors and warnings */

#define MAPI_E_END_OF_SESSION                           MAKE_MAPI_E( 0x200 )
#define MAPI_E_UNKNOWN_ENTRYID                          MAKE_MAPI_E( 0x201 )
#define MAPI_E_MISSING_REQUIRED_COLUMN                  MAKE_MAPI_E( 0x202 )
#define MAPI_W_NO_SERVICE                               MAKE_MAPI_S( 0x203 )

/* Property specific errors and warnings */

#define MAPI_E_BAD_VALUE                                MAKE_MAPI_E( 0x301 )
#define MAPI_E_INVALID_TYPE                             MAKE_MAPI_E( 0x302 )
#define MAPI_E_TYPE_NO_SUPPORT                          MAKE_MAPI_E( 0x303 )
#define MAPI_E_UNEXPECTED_TYPE                          MAKE_MAPI_E( 0x304 )
#define MAPI_E_TOO_BIG                                  MAKE_MAPI_E( 0x305 )
#define MAPI_E_DECLINE_COPY                             MAKE_MAPI_E( 0x306 )
#define MAPI_E_UNEXPECTED_ID                            MAKE_MAPI_E( 0x307 )

#define MAPI_W_ERRORS_RETURNED                          MAKE_MAPI_S( 0x380 )

/* Table specific errors and warnings */

#define MAPI_E_UNABLE_TO_COMPLETE                       MAKE_MAPI_E( 0x400 )
#define MAPI_E_TIMEOUT                                  MAKE_MAPI_E( 0x401 )
#define MAPI_E_TABLE_EMPTY                              MAKE_MAPI_E( 0x402 )
#define MAPI_E_TABLE_TOO_BIG                            MAKE_MAPI_E( 0x403 )

#define MAPI_E_INVALID_BOOKMARK                         MAKE_MAPI_E( 0x405 )

#define MAPI_W_POSITION_CHANGED                         MAKE_MAPI_S( 0x481 )
#define MAPI_W_APPROX_COUNT                             MAKE_MAPI_S( 0x482 )

/* Transport specific errors and warnings */

#define MAPI_E_WAIT                                     MAKE_MAPI_E( 0x500 )
#define MAPI_E_CANCEL                                   MAKE_MAPI_E( 0x501 )
#define MAPI_E_NOT_ME                                   MAKE_MAPI_E( 0x502 )

#define MAPI_W_CANCEL_MESSAGE                           MAKE_MAPI_S( 0x580 )

/* Message Store, Folder, and Message specific errors and warnings */

#define MAPI_E_CORRUPT_STORE                            MAKE_MAPI_E( 0x600 )
#define MAPI_E_NOT_IN_QUEUE                             MAKE_MAPI_E( 0x601 )
#define MAPI_E_NO_SUPPRESS                              MAKE_MAPI_E( 0x602 )
#define MAPI_E_COLLISION                                MAKE_MAPI_E( 0x604 )
#define MAPI_E_NOT_INITIALIZED                          MAKE_MAPI_E( 0x605 )
#define MAPI_E_NON_STANDARD                             MAKE_MAPI_E( 0x606 )
#define MAPI_E_NO_RECIPIENTS                            MAKE_MAPI_E( 0x607 )
#define MAPI_E_SUBMITTED                                MAKE_MAPI_E( 0x608 )
#define MAPI_E_HAS_FOLDERS                              MAKE_MAPI_E( 0x609 )
#define MAPI_E_HAS_MESSAGES                             MAKE_MAPI_E( 0x60A )
#define MAPI_E_FOLDER_CYCLE                             MAKE_MAPI_E( 0x60B )

#define MAPI_W_PARTIAL_COMPLETION                       MAKE_MAPI_S( 0x680 )

/* Address Book specific errors and warnings */

#define MAPI_E_AMBIGUOUS_RECIP                          MAKE_MAPI_E( 0x700 )

/* The range 0x0800 to 0x08FF is reserved */


/* We expect these to eventually be defined by OLE, but for now,
 * here they are.  When OLE defines them they can be much more
 * efficient than these, but these are "proper" and don't make
 * use of any hidden tricks.
 */
#ifndef HR_SUCCEEDED
#define HR_SUCCEEDED(_hr) SUCCEEDED((SCODE)(_hr))
#define HR_FAILED(_hr) FAILED((SCODE)(_hr))
#endif

#endif  /* MAPICODE_H */
