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

#ifndef __M4L_MAPIDEFS_H_
#define __M4L_MAPIDEFS_H_
#define MAPIDEFS_H

/*
 * MAPI for linux
 *
 * mapidefs.h - All interface definitions
 *
 * (C) Zarafa 2005
 *
 */

#include <zarafa/platform.h>
#include <cstring>		/* memcmp() */

#define MAPI_DIM	1

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif

/*
 *  This flag is used in many different MAPI calls to signify that
 *  the object opened by the call should be modifiable (MAPI_MODIFY).
 *  If the flag MAPI_MAX_ACCESS is set, the object returned should be
 *  returned at the maximum access level allowed.  An additional
 *  property available on the object (PR_ACCESS_LEVEL) uses the same
 *  MAPI_MODIFY flag to say just what this new access level is.
 */
#define MAPI_MODIFY			((ULONG) 0x00000001)

/*
 *  The following flags are used to indicate to the client what access
 *  level is permissible in the object. They appear in PR_ACCESS in
 *  message and folder objects as well as in contents and associated
 *  contents tables
 */
#define MAPI_ACCESS_MODIFY		((ULONG) 0x00000001)
#define MAPI_ACCESS_READ		((ULONG) 0x00000002)
#define MAPI_ACCESS_DELETE		((ULONG) 0x00000004)
#define MAPI_ACCESS_CREATE_HIERARCHY	((ULONG) 0x00000008)
#define MAPI_ACCESS_CREATE_CONTENTS	((ULONG) 0x00000010)
#define MAPI_ACCESS_CREATE_ASSOCIATED	((ULONG) 0x00000020)

/*
 *  The MAPI_UNICODE flag is used in many different MAPI calls to signify
 *  that strings passed through the interface are in Unicode (a 16-bit
 *  character set). The default is an 8-bit character set.
 *
 *  The value fMapiUnicode can be used as the 'normal' value for
 *  that bit, given the application's default character set.
 */
#define MAPI_UNICODE            ((ULONG) 0x80000000)

#ifdef UNICODE
#define fMapiUnicode            MAPI_UNICODE
#else
#define fMapiUnicode            0
#endif


#define hrSuccess		0


/* Bit definitions for abFlags[0] of ENTRYID */
#define MAPI_SHORTTERM		0x80
#define MAPI_NOTRECIP		0x40
#define MAPI_THISSESSION	0x20
#define MAPI_NOW		0x10
#define MAPI_NOTRESERVED	0x08

/* Bit definitions for abFlags[1] of ENTRYID */
#define MAPI_COMPOUND		0x80

/* ENTRYID */
typedef struct _s_ENTRYID {
    BYTE    abFlags[4];
    BYTE    ab[MAPI_DIM];
} ENTRYID, *LPENTRYID;

#define CbNewENTRYID(_cb)	(offsetof(ENTRYID,ab) + (_cb))
#define CbENTRYID(_cb)		(offsetof(ENTRYID,ab) + (_cb))
#define SizedENTRYID(_cb, _name) \
    struct _ENTRYID_ ## _name \
{ \
    BYTE    abFlags[4]; \
    BYTE    ab[_cb]; \
} _name

/* Byte-order-independent version of GUID (world-unique identifier) */
typedef struct _s_MAPIUID {
    BYTE ab[16];
} MAPIUID, *LPMAPIUID;

/* require string.h */
#define IsEqualMAPIUID(lpuid1, lpuid2)  (!memcmp(lpuid1, lpuid2, sizeof(MAPIUID)))

/*
 * Constants for one-off entry ID:
 * The MAPIUID that identifies the one-off provider;
 * the flag that defines whether the embedded strings are Unicode;
 * the flag that specifies whether the recipient gets TNEF or not.
 */
#define MAPI_ONE_OFF_UID { 0x81, 0x2b, 0x1f, 0xa4, 0xbe, 0xa3, 0x10, 0x19, \
                           0x9d, 0x6e, 0x00, 0xdd, 0x01, 0x0f, 0x54, 0x02 }
#define MAPI_ONE_OFF_UNICODE        0x8000
#define MAPI_ONE_OFF_NO_RICH_INFO   0x0001

/* object types */
#define MAPI_STORE      ((ULONG) 0x00000001)
#define MAPI_ADDRBOOK   ((ULONG) 0x00000002)
#define MAPI_FOLDER     ((ULONG) 0x00000003)
#define MAPI_ABCONT     ((ULONG) 0x00000004)
#define MAPI_MESSAGE    ((ULONG) 0x00000005)
#define MAPI_MAILUSER   ((ULONG) 0x00000006)
#define MAPI_ATTACH     ((ULONG) 0x00000007)
#define MAPI_DISTLIST   ((ULONG) 0x00000008)
#define MAPI_PROFSECT   ((ULONG) 0x00000009)
#define MAPI_STATUS     ((ULONG) 0x0000000A)
#define MAPI_SESSION    ((ULONG) 0x0000000B)
#define MAPI_FORMINFO   ((ULONG) 0x0000000C)


/*
 *  Maximum length of profile names and passwords, not including
 *  the null termination character.
 */
    /* unused? */
#define cchProfileNameMax   64
#define cchProfilePassMax   64



/* Property Types */
#define MV_FLAG         0x1000

#define PT_UNSPECIFIED  ((ULONG) 0x0000)
#define PT_NULL         ((ULONG) 0x0001)
#define PT_SHORT        ((ULONG) 0x0002)
#define PT_LONG         ((ULONG) 0x0003)
#define PT_FLOAT        ((ULONG) 0x0004)
#define PT_DOUBLE       ((ULONG) 0x0005)
#define PT_CURRENCY     ((ULONG) 0x0006)
#define PT_APPTIME      ((ULONG) 0x0007)
#define PT_ERROR        ((ULONG) 0x000A)
#define PT_BOOLEAN      ((ULONG) 0x000B)
#define PT_OBJECT       ((ULONG) 0x000D)
#define PT_LONGLONG     ((ULONG) 0x0014)
#define PT_STRING8      ((ULONG) 0x001E)
#define PT_UNICODE      ((ULONG) 0x001F)
#define PT_SYSTIME      ((ULONG) 0x0040)
#define PT_CLSID        ((ULONG) 0x0048)
#define PT_BINARY       ((ULONG) 0x0102)

/* Alternate property type names for ease of use */
#define PT_I2	PT_SHORT
#define PT_I4	PT_LONG
#define PT_R4	PT_FLOAT
#define PT_R8	PT_DOUBLE
#define PT_I8	PT_LONGLONG

/*
 *  The type of a MAPI-defined string property is indirected, so
 *  that it defaults to Unicode string on a Unicode platform and to
 *  String8 on an ANSI or DBCS platform.
 *
 *  Macros are defined here both for the property type, and for the
 *  field of the property value structure which should be
 *  dereferenced to obtain the string pointer.
 */

#ifdef  UNICODE
#define PT_TSTRING          PT_UNICODE
#define PT_MV_TSTRING       (MV_FLAG|PT_UNICODE)
#define LPSZ                lpszW
#define LPPSZ               lppszW
#define MVSZ                MVszW
#else
#define PT_TSTRING          PT_STRING8
#define PT_MV_TSTRING       (MV_FLAG|PT_STRING8)
#define LPSZ                lpszA
#define LPPSZ               lppszA
#define MVSZ                MVszA
#endif


    /* Property Tags */
#define PROP_TYPE_MASK			((ULONG)0x0000FFFF)
#define PROP_TYPE(ulPropTag)		(((ULONG)(ulPropTag))&PROP_TYPE_MASK)
#define PROP_ID(ulPropTag)		(((ULONG)(ulPropTag))>>16)
#define PROP_TAG(ulPropType,ulPropID)	((((ULONG)(ulPropID))<<16)|((ULONG)(ulPropType)))
#define PROP_ID_NULL            0
#define PROP_ID_INVALID         0xFFFF
#define PR_NULL                 PROP_TAG( PT_NULL, PROP_ID_NULL)
#define CHANGE_PROP_TYPE(ulPropTag, ulPropType) \
                        (((ULONG)0xFFFF0000 & ulPropTag) | ulPropType)


/* Multi-valued Property Types */
#define PT_MV_SHORT     (MV_FLAG|PT_SHORT)
#define PT_MV_LONG      (MV_FLAG|PT_LONG)
#define PT_MV_FLOAT     (MV_FLAG|PT_FLOAT)
#define PT_MV_DOUBLE    (MV_FLAG|PT_DOUBLE)
#define PT_MV_CURRENCY  (MV_FLAG|PT_CURRENCY)
#define PT_MV_APPTIME   (MV_FLAG|PT_APPTIME)
#define PT_MV_SYSTIME   (MV_FLAG|PT_SYSTIME)
#define PT_MV_STRING8   (MV_FLAG|PT_STRING8)
#define PT_MV_BINARY    (MV_FLAG|PT_BINARY)
#define PT_MV_UNICODE   (MV_FLAG|PT_UNICODE)
#define PT_MV_CLSID     (MV_FLAG|PT_CLSID)
#define PT_MV_LONGLONG  (MV_FLAG|PT_LONGLONG)

/* Alternate property type names for ease of use */
#define PT_MV_I2     PT_MV_SHORT
#define PT_MV_I4     PT_MV_LONG
#define PT_MV_R4     PT_MV_FLOAT
#define PT_MV_R8     PT_MV_DOUBLE
#define PT_MV_I8     PT_MV_LONGLONG

/* Property type reserved bits */
#define MV_INSTANCE     0x2000
#define MVI_FLAG        (MV_FLAG | MV_INSTANCE)
#define MVI_PROP(tag)   ((tag) | MVI_FLAG)


/* Data Structures */

/* Property Tag Array */
typedef struct _SPropTagArray {
    ULONG   cValues;
    ULONG   aulPropTag[MAPI_DIM];
} SPropTagArray, *LPSPropTagArray;

#define CbNewSPropTagArray(_ctag) \
    (offsetof(SPropTagArray,aulPropTag) + (_ctag)*sizeof(ULONG))
#define CbSPropTagArray(_lparray) \
    (offsetof(SPropTagArray,aulPropTag) + \
    (UINT)((_lparray)->cValues)*sizeof(ULONG))
/* SPropTagArray */
#define SizedSPropTagArray(_ctag, _name) \
struct _SPropTagArray_ ## _name \
{ \
    ULONG   cValues; \
    ULONG   aulPropTag[_ctag]; \
} _name


/* Property Value */
//typedef struct _SPropValue  SPropValue;

/* 32-bit version only, in little endian, Lo is unsigned because the sign bit is in Hi */
#ifndef _tagCY_DEFINED
#define _tagCY_DEFINED

typedef union tagCY {
#ifdef __GNUC__
    __extension__ struct {
#else
    struct {
#endif
#ifdef _USE_NETWORK_ORDER
        LONG Hi;
        LONG Lo;
#else
        ULONG Lo;
        LONG Hi;
#endif
    };
    LONGLONG int64;
} CURRENCY;
#endif

typedef struct _SBinary
{
    ULONG       cb;
    LPBYTE      lpb;
} SBinary, *LPSBinary;

typedef struct _SShortArray
{
    ULONG       cValues;
    short int    *lpi;
} SShortArray;

typedef struct _SGuidArray
{
    ULONG       cValues;
    GUID        *lpguid;
} SGuidArray;

typedef struct _SRealArray
{
    ULONG       cValues;
    float       *lpflt;
} SRealArray;

typedef struct _SLongArray
{
    ULONG       cValues;
    LONG        *lpl;
} SLongArray;

typedef struct _SLargeIntegerArray
{
    ULONG       cValues;
    LARGE_INTEGER   *lpli;
} SLargeIntegerArray;

typedef struct _SDateTimeArray
{
    ULONG       cValues;
    FILETIME    *lpft;
} SDateTimeArray;

typedef struct _SAppTimeArray
{
    ULONG       cValues;
    double      *lpat;
} SAppTimeArray;

typedef struct _SCurrencyArray
{
    ULONG       cValues;
    CURRENCY    *lpcur;
} SCurrencyArray;

typedef struct _SBinaryArray
{
    ULONG       cValues;
    SBinary     *lpbin;
} SBinaryArray;

typedef struct _SDoubleArray
{
    ULONG       cValues;
    double      *lpdbl;
} SDoubleArray;

typedef struct _SWStringArray
{
    ULONG       cValues;
    LPWSTR      *lppszW;
} SWStringArray;

typedef struct _SLPSTRArray
{
    ULONG       cValues;
    LPSTR       *lppszA;
} SLPSTRArray;

typedef union _PV
{
    short int           i;          /* case PT_I2 */
    LONG                l;          /* case PT_LONG */
    ULONG               ul;         /* alias for PT_LONG */
    float               flt;        /* case PT_R4 */
    double              dbl;        /* case PT_DOUBLE */
    unsigned short int  b;          /* case PT_BOOLEAN */
    CURRENCY            cur;        /* case PT_CURRENCY */
    double              at;         /* case PT_APPTIME */
    FILETIME            ft;         /* case PT_SYSTIME */
    LPSTR               lpszA;      /* case PT_STRING8 */
    SBinary             bin;        /* case PT_BINARY */
    LPWSTR              lpszW;      /* case PT_UNICODE */
    LPGUID              lpguid;     /* case PT_CLSID */
    LARGE_INTEGER       li;         /* case PT_I8 */
    SShortArray         MVi;        /* case PT_MV_I2 */
    SLongArray          MVl;        /* case PT_MV_LONG */
    SRealArray          MVflt;      /* case PT_MV_R4 */
    SDoubleArray        MVdbl;      /* case PT_MV_DOUBLE */
    SCurrencyArray      MVcur;      /* case PT_MV_CURRENCY */
    SAppTimeArray       MVat;       /* case PT_MV_APPTIME */
    SDateTimeArray      MVft;       /* case PT_MV_SYSTIME */
    SBinaryArray        MVbin;      /* case PT_MV_BINARY */
    SLPSTRArray         MVszA;      /* case PT_MV_STRING8 */
    SWStringArray       MVszW;      /* case PT_MV_UNICODE */
    SGuidArray          MVguid;     /* case PT_MV_CLSID */
    SLargeIntegerArray  MVli;       /* case PT_MV_I8 */
    SCODE               err;        /* case PT_ERROR */
    LONG                x;          /* case PT_NULL, PT_OBJECT (no usable value) */
} __UPV;

typedef struct _SPropValue
{
    ULONG       ulPropTag;
    ULONG       dwAlignPad;
    union _PV   Value;
} SPropValue, *LPSPropValue;



/* Property Problem and Property Problem Arrays */
typedef struct _SPropProblem
{
    ULONG   ulIndex;
    ULONG   ulPropTag;
    SCODE   scode;
} SPropProblem, *LPSPropProblem;

typedef struct _SPropProblemArray
{
    ULONG           cProblem;
    SPropProblem    aProblem[MAPI_DIM];
} SPropProblemArray, *LPSPropProblemArray;

#define CbNewSPropProblemArray(_cprob) \
    (offsetof(SPropProblemArray,aProblem) + (_cprob)*sizeof(SPropProblem))
#define CbSPropProblemArray(_lparray) \
    (offsetof(SPropProblemArray,aProblem) + \
    (UINT) ((_lparray)->cProblem*sizeof(SPropProblem)))
#define SizedSPropProblemArray(_cprob, _name) \
struct _SPropProblemArray_ ## _name \
{ \
    ULONG           cProblem; \
    SPropProblem    aProblem[_cprob]; \
} _name


/* Entry List */
typedef SBinaryArray ENTRYLIST, *LPENTRYLIST;

typedef struct _s_FLATENTRY {
    ULONG cb;
    BYTE abEntry[MAPI_DIM];
} FLATENTRY, *LPFLATENTRY;

typedef struct _s_FLATENTRYLIST {
    ULONG       cEntries;
    ULONG       cbEntries;
    BYTE        abEntries[MAPI_DIM];
} FLATENTRYLIST, *LPFLATENTRYLIST;

typedef struct _s_MTSID {
    ULONG       cb;
    BYTE        ab[MAPI_DIM];
} MTSID, *LPMTSID;

typedef struct _s_FLATMTSIDLIST {
    ULONG       cMTSIDs;
    ULONG       cbMTSIDs;
    BYTE        abMTSIDs[MAPI_DIM];
} FLATMTSIDLIST, *LPFLATMTSIDLIST;

#define CbNewFLATENTRY(_cb)     (offsetof(FLATENTRY,abEntry) + (_cb))
#define CbFLATENTRY(_lpentry)   (offsetof(FLATENTRY,abEntry) + (_lpentry)->cb)
#define CbNewFLATENTRYLIST(_cb) (offsetof(FLATENTRYLIST,abEntries) + (_cb))
#define CbFLATENTRYLIST(_lplist) (offsetof(FLATENTRYLIST,abEntries) + (_lplist)->cbEntries)
#define CbNewMTSID(_cb)         (offsetof(MTSID,ab) + (_cb))
#define CbMTSID(_lpentry)       (offsetof(MTSID,ab) + (_lpentry)->cb)
#define CbNewFLATMTSIDLIST(_cb) (offsetof(FLATMTSIDLIST,abMTSIDs) + (_cb))
#define CbFLATMTSIDLIST(_lplist) (offsetof(FLATMTSIDLIST,abMTSIDs) + (_lplist)->cbMTSIDs)
/* No SizedXXX macros for these types. */

/* ADRENTRY, ADRLIST */
typedef struct _ADRENTRY
{
    ULONG           ulReserved1;
    ULONG           cValues;
    LPSPropValue    rgPropVals;
} ADRENTRY, *LPADRENTRY;

typedef struct _ADRLIST
{
    ULONG           cEntries;
    ADRENTRY        aEntries[MAPI_DIM];
} ADRLIST, *LPADRLIST;

#define CbNewADRLIST(_centries) \
    (offsetof(ADRLIST,aEntries) + (_centries)*sizeof(ADRENTRY))
#define CbADRLIST(_lpadrlist) \
    (offsetof(ADRLIST,aEntries) + (UINT)(_lpadrlist)->cEntries*sizeof(ADRENTRY))
#define SizedADRLIST(_centries, _name) \
struct _ADRLIST_ ## _name \
{ \
    ULONG           cEntries; \
    ADRENTRY        aEntries[_centries]; \
} _name

/* SRow, SRowSet */
typedef struct _SRow
{
    ULONG           ulAdrEntryPad;
    ULONG           cValues;
    LPSPropValue    lpProps;
} SRow, * LPSRow;

typedef struct _SRowSet
{
    ULONG           cRows;
    SRow            aRow[MAPI_DIM];
} SRowSet, * LPSRowSet;

#define CbNewSRowSet(_crow)     (offsetof(SRowSet,aRow) + (_crow)*sizeof(SRow))
#define CbSRowSet(_lprowset)    (offsetof(SRowSet,aRow) + \
                                    (UINT)((_lprowset)->cRows*sizeof(SRow)))
#define SizedSRowSet(_crow, _name) \
struct _SRowSet_ ## _name \
{ \
    ULONG           cRows; \
    SRow            aRow[_crow]; \
} _name


extern "C" {
/* MAPI Allocation Routines */
typedef SCODE (ALLOCATEBUFFER)(
    ULONG      cbSize,
    LPVOID*    lppBuffer
);

typedef SCODE (ALLOCATEMORE)(
    ULONG      cbSize,
    LPVOID     lpObject,
    LPVOID*    lppBuffer
);

typedef ULONG (FREEBUFFER)(
    LPVOID     lpBuffer
);

typedef ALLOCATEBUFFER* LPALLOCATEBUFFER;
typedef ALLOCATEMORE* LPALLOCATEMORE;
typedef FREEBUFFER* LPFREEBUFFER;
}

/* MAPI Component Object Model Macros */
/* #if defined(MAPI_IF) && (!defined(__cplusplus) || defined(CINTERFACE)) */
/* #define DECLARE_MAPI_INTERFACE(iface)                                   \ */
/*         typedef struct iface##Vtbl iface##Vtbl, * iface;            \ */
/*         struct iface##Vtbl */
/* #define DECLARE_MAPI_INTERFACE_(iface, baseiface)                       \ */
/*         DECLARE_MAPI_INTERFACE(iface) */
/* #define DECLARE_MAPI_INTERFACE_PTR(iface, piface)                       \ */
/*         typedef struct iface##Vtbl iface##Vtbl, * iface, * * piface; */
/* #else */
/* #define DECLARE_MAPI_INTERFACE(iface)                                   \ */
/*         DECLARE_INTERFACE(iface) */
/* #define DECLARE_MAPI_INTERFACE_(iface, baseiface)                       \ */
/*         DECLARE_INTERFACE_(iface, baseiface) */
/* #ifdef __cplusplus */
/* #define DECLARE_MAPI_INTERFACE_PTR(iface, piface)                       \ */
/*         interface iface; typedef iface * piface */
/* #else */
/* #define DECLARE_MAPI_INTERFACE_PTR(iface, piface)                       \ */
/*         typedef interface iface iface, * piface */
/* #endif */
/* #endif */
/* /\*--*\/ */

/* #define MAPIMETHOD(method)              MAPIMETHOD_(HRESULT, method) */
/* #define MAPIMETHOD_(type, method)       STDMETHOD_(type, method) */
/* #define MAPIMETHOD_DECLARE(type, method, prefix) \ */
/*         STDMETHODIMP_(type) prefix##method */
/* #define MAPIMETHOD_TYPEDEF(type, method, prefix) \ */
/*         typedef type (STDMETHODCALLTYPE prefix##method##_METHOD) */

/* #define MAPI_IUNKNOWN_METHODS(IPURE)                                    \ */
/*     MAPIMETHOD(QueryInterface)                                          \ */
/*         (THIS_ REFIID riid, LPVOID * ppvObj) IPURE;                 \ */
/*     MAPIMETHOD_(ULONG,AddRef)  (THIS) IPURE;                            \ */
/*     MAPIMETHOD_(ULONG,Release) (THIS) IPURE;                            \ */


/* Pointers to MAPI Interfaces */
typedef const IID* LPCIID;


/* Extended MAPI Error Information */
typedef struct _MAPIERROR
{
    ULONG   ulVersion;
    LPTSTR  lpszError;
    LPTSTR  lpszComponent;
    ULONG   ulLowLevelError;
    ULONG   ulContext;
} MAPIERROR, *LPMAPIERROR;


// pre definitions
class IMsgStore;
class IMAPIFolder;
class IMessage;
class IAttach;
class IAddrBook;
class IABContainer;
class IMailUser;
class IDistList;
class IMAPIStatus;
class IMAPITable;
class IProfSect;
class IMAPIProp;
class IMAPIContainer;
class IMAPIAdviseSink;
class IMAPIProgress;
class IProviderAdmin;

typedef IMsgStore* LPMDB;
typedef IMAPIFolder* LPMAPIFOLDER;
typedef IMessage* LPMESSAGE;
typedef IAttach* LPATTACH;
typedef IAddrBook* LPADRBOOK;
typedef IABContainer* LPABCONT;
typedef IMailUser* LPMAILUSER;
typedef IDistList* LPDISTLIST;
typedef IMAPIStatus* LPMAPISTATUS;
typedef IMAPITable* LPMAPITABLE;
typedef IProfSect* LPPROFSECT;
typedef IMAPIProp* LPMAPIPROP;
typedef IMAPIContainer* LPMAPICONTAINER;
typedef IMAPIAdviseSink* LPMAPIADVISESINK;
typedef IMAPIProgress* LPMAPIPROGRESS;
typedef IProviderAdmin* LPPROVIDERADMIN;

// restrictions needed in multiple places
typedef struct _SRestriction* LPSRestriction;

/* Full C++ classes */

/*
 * IMAPIProp Interface
 */
#define MAPI_ERROR_VERSION      0x00000000L

#define KEEP_OPEN_READONLY      ((ULONG) 0x00000001)
#define KEEP_OPEN_READWRITE     ((ULONG) 0x00000002)
#define FORCE_SAVE              ((ULONG) 0x00000004)
#define MAPI_CREATE             ((ULONG) 0x00000002)
#define STREAM_APPEND           ((ULONG) 0x00000004)
#define MAPI_MOVE               ((ULONG) 0x00000001)
#define MAPI_NOREPLACE          ((ULONG) 0x00000002)
#define MAPI_DECLINE_OK         ((ULONG) 0x00000004)

/* Flags used in GetNamesFromIDs  (bit fields) */
#define MAPI_NO_STRINGS         ((ULONG) 0x00000001)
#define MAPI_NO_IDS             ((ULONG) 0x00000002)

/*  Union discriminator  */
#define MNID_ID                 0
#define MNID_STRING             1
typedef struct _MAPINAMEID
{
    LPGUID lpguid;
    ULONG ulKind;
    union {
        LONG lID;
        LPWSTR lpwstrName;
    } Kind;
} MAPINAMEID, *LPMAPINAMEID;

class IMAPIProp : public IUnknown {
public:
    //    virtual ~IMAPIProp() = 0;

    virtual HRESULT GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR* lppMAPIError) = 0;
    virtual HRESULT SaveChanges(ULONG ulFlags) = 0;
    virtual HRESULT GetProps(LPSPropTagArray lpPropTagArray, ULONG ulFlags, ULONG* lpcValues, LPSPropValue* lppPropArray) = 0;
    virtual HRESULT GetPropList(ULONG ulFlags, LPSPropTagArray* lppPropTagArray) = 0;
    virtual HRESULT OpenProperty(ULONG ulPropTag, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulFlags, LPUNKNOWN* lppUnk) = 0;
    virtual HRESULT SetProps(ULONG cValues, LPSPropValue lpPropArray, LPSPropProblemArray* lppProblems) = 0;
    virtual HRESULT DeleteProps(LPSPropTagArray lpPropTagArray, LPSPropProblemArray* lppProblems) = 0;
    virtual HRESULT CopyTo(ULONG ciidExclude, LPCIID rgiidExclude, LPSPropTagArray lpExcludeProps, ULONG ulUIParam,
			   LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags,
			   LPSPropProblemArray* lppProblems) = 0;
    virtual HRESULT CopyProps(LPSPropTagArray lpIncludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface,
			      LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray* lppProblems) = 0;
    virtual HRESULT GetNamesFromIDs(LPSPropTagArray* lppPropTags, LPGUID lpPropSetGuid, ULONG ulFlags, ULONG* lpcPropNames,
				    LPMAPINAMEID** lpppPropNames) = 0;
    virtual HRESULT GetIDsFromNames(ULONG cPropNames, LPMAPINAMEID* lppPropNames, ULONG ulFlags, LPSPropTagArray* lppPropTags) = 0;
};


/*
 * IMAPIContainer Interface
 */
#define MAPI_BEST_ACCESS        ((ULONG) 0x00000010)
#define CONVENIENT_DEPTH        ((ULONG) 0x00000001)
#define SEARCH_RUNNING          ((ULONG) 0x00000001)
#define SEARCH_REBUILD          ((ULONG) 0x00000002)
#define SEARCH_RECURSIVE        ((ULONG) 0x00000004)
#define SEARCH_FOREGROUND       ((ULONG) 0x00000008)
#define STOP_SEARCH             ((ULONG) 0x00000001)
#define RESTART_SEARCH          ((ULONG) 0x00000002)
#define RECURSIVE_SEARCH        ((ULONG) 0x00000004)
#define SHALLOW_SEARCH          ((ULONG) 0x00000008)
#define FOREGROUND_SEARCH       ((ULONG) 0x00000010)
#define BACKGROUND_SEARCH       ((ULONG) 0x00000020)

class IMAPIContainer : public IMAPIProp {
public:
    //    virtual ~IMAPIContainer() = 0;

    virtual HRESULT GetContentsTable(ULONG ulFlags, LPMAPITABLE* lppTable) = 0;
    virtual HRESULT GetHierarchyTable(ULONG ulFlags, LPMAPITABLE* lppTable) = 0;
    virtual HRESULT OpenEntry(ULONG cbEntryID, LPENTRYID lpEntryID, LPCIID lpInterface, ULONG ulFlags, ULONG* lpulObjType,
			      LPUNKNOWN* lppUnk) = 0;
    virtual HRESULT SetSearchCriteria(LPSRestriction lpRestriction, LPENTRYLIST lpContainerList, ULONG ulSearchFlags) = 0;
    virtual HRESULT GetSearchCriteria(ULONG ulFlags, LPSRestriction* lppRestriction, LPENTRYLIST* lppContainerList,
				      ULONG* lpulSearchState) = 0;
};


/* 
 * IMAPIAdviseSink Interface
 */
/*
 *  Notification event types. The event types can be combined in a bitmask
 *  for filtering. Each one has a parameter structure associated with it:
 *
 *      fnevCriticalError       ERROR_NOTIFICATION
 *      fnevNewMail             NEWMAIL_NOTIFICATION
 *      fnevObjectCreated       OBJECT_NOTIFICATION
 *      fnevObjectDeleted       OBJECT_NOTIFICATION
 *      fnevObjectModified      OBJECT_NOTIFICATION
 *      fnevObjectCopied        OBJECT_NOTIFICATION
 *      fnevSearchComplete      OBJECT_NOTIFICATION
 *      fnevTableModified       TABLE_NOTIFICATION
 *      fnevStatusObjectModified ?
 *
 *      fnevExtended            EXTENDED_NOTIFICATION
 */
#define fnevCriticalError           ((ULONG) 0x00000001)
#define fnevNewMail                 ((ULONG) 0x00000002)
#define fnevObjectCreated           ((ULONG) 0x00000004)
#define fnevObjectDeleted           ((ULONG) 0x00000008)
#define fnevObjectModified          ((ULONG) 0x00000010)
#define fnevObjectMoved             ((ULONG) 0x00000020)
#define fnevObjectCopied            ((ULONG) 0x00000040)
#define fnevSearchComplete          ((ULONG) 0x00000080)
#define fnevTableModified           ((ULONG) 0x00000100)
#define fnevStatusObjectModified    ((ULONG) 0x00000200)
#define fnevReservedForMapi         ((ULONG) 0x40000000)
#define fnevExtended                ((ULONG) 0x80000000)

/* TABLE_NOTIFICATION event types passed in ulTableEvent */
#define TABLE_CHANGED       1
#define TABLE_ERROR         2
#define TABLE_ROW_ADDED     3
#define TABLE_ROW_DELETED   4
#define TABLE_ROW_MODIFIED  5
#define TABLE_SORT_DONE     6
#define TABLE_RESTRICT_DONE 7
#define TABLE_SETCOL_DONE   8
#define TABLE_RELOAD        9

/* Event Structures */
typedef struct _ERROR_NOTIFICATION
{
    ULONG       cbEntryID;
    LPENTRYID   lpEntryID;
    SCODE       scode;
    ULONG       ulFlags;
    LPMAPIERROR lpMAPIError;
} ERROR_NOTIFICATION;

typedef struct _NEWMAIL_NOTIFICATION
{
    ULONG       cbEntryID;
    LPENTRYID   lpEntryID;
    ULONG       cbParentID;
    LPENTRYID   lpParentID;
    ULONG       ulFlags;
    LPTSTR      lpszMessageClass;
    ULONG       ulMessageFlags;
} NEWMAIL_NOTIFICATION;

typedef struct _OBJECT_NOTIFICATION
{
    ULONG               cbEntryID;
    LPENTRYID           lpEntryID;
    ULONG               ulObjType;
    ULONG               cbParentID;
    LPENTRYID           lpParentID;
    ULONG               cbOldID;
    LPENTRYID           lpOldID;
    ULONG               cbOldParentID;
    LPENTRYID           lpOldParentID;
    LPSPropTagArray     lpPropTagArray;
} OBJECT_NOTIFICATION;

typedef struct _TABLE_NOTIFICATION
{
    ULONG               ulTableEvent;
    HRESULT             hResult;
    SPropValue          propIndex;
    SPropValue          propPrior;
    SRow                row;
    ULONG               ulPad;
} TABLE_NOTIFICATION;

typedef struct _EXTENDED_NOTIFICATION
{
    ULONG       ulEvent;
    ULONG       cb;
    LPBYTE      pbEventParameters;
} EXTENDED_NOTIFICATION;

typedef struct
{
    ULONG           cbEntryID;
    LPENTRYID       lpEntryID;
    ULONG           cValues;
    LPSPropValue    lpPropVals;
} STATUS_OBJECT_NOTIFICATION;

typedef struct _NOTIFICATION
{
    ULONG   ulEventType;
    ULONG   ulAlignPad;
    union
    {
        ERROR_NOTIFICATION          err;
        NEWMAIL_NOTIFICATION        newmail;
        OBJECT_NOTIFICATION         obj;
        TABLE_NOTIFICATION          tab;
        EXTENDED_NOTIFICATION       ext;
        STATUS_OBJECT_NOTIFICATION  statobj;
    } info;
} NOTIFICATION, *LPNOTIFICATION;


/* Callback function type for MAPIAllocAdviseSink */
typedef LONG (NOTIFCALLBACK)(
    LPVOID          lpvContext,
    ULONG           cNotification,
    LPNOTIFICATION  lpNotifications
);
typedef NOTIFCALLBACK *LPNOTIFCALLBACK;


/* Interface used for registering and issuing notification callbacks. */
class IMAPIAdviseSink : public IUnknown {
public:
    //    virtual ~IMAPIAdviseSink() = 0;

    virtual ULONG OnNotify(ULONG cNotif, LPNOTIFICATION lpNotifications) = 0;
};


/*
 * IMsgStore Interface
 */
#define STORE_ENTRYID_UNIQUE    ((ULONG) 0x00000001)
#define STORE_READONLY          ((ULONG) 0x00000002)
#define STORE_SEARCH_OK         ((ULONG) 0x00000004)
#define STORE_MODIFY_OK         ((ULONG) 0x00000008)
#define STORE_CREATE_OK         ((ULONG) 0x00000010)
#define STORE_ATTACH_OK         ((ULONG) 0x00000020)
#define STORE_OLE_OK            ((ULONG) 0x00000040)
#define STORE_SUBMIT_OK         ((ULONG) 0x00000080)
#define STORE_NOTIFY_OK         ((ULONG) 0x00000100)
#define STORE_MV_PROPS_OK       ((ULONG) 0x00000200)
#define STORE_CATEGORIZE_OK     ((ULONG) 0x00000400)
#define STORE_RTF_OK            ((ULONG) 0x00000800)
#define STORE_RESTRICTION_OK    ((ULONG) 0x00001000)
#define STORE_SORT_OK           ((ULONG) 0x00002000)
#define STORE_PUBLIC_FOLDERS    ((ULONG) 0x00004000)
#define STORE_UNCOMPRESSED_RTF  ((ULONG) 0x00008000)
#define STORE_HAS_SEARCHES      ((ULONG) 0x01000000)
#define LOGOFF_NO_WAIT          ((ULONG) 0x00000001)
#define LOGOFF_ORDERLY          ((ULONG) 0x00000002)
#define LOGOFF_PURGE            ((ULONG) 0x00000004)
#define LOGOFF_ABORT            ((ULONG) 0x00000008)
#define LOGOFF_QUIET            ((ULONG) 0x00000010)

#define LOGOFF_COMPLETE         ((ULONG) 0x00010000)
#define LOGOFF_INBOUND          ((ULONG) 0x00020000)
#define LOGOFF_OUTBOUND         ((ULONG) 0x00040000)
#define LOGOFF_OUTBOUND_QUEUE   ((ULONG) 0x00080000)
#define MSG_LOCKED              ((ULONG) 0x00000001)
#define MSG_UNLOCKED            ((ULONG) 0x00000000)
#define FOLDER_IPM_SUBTREE_VALID        ((ULONG) 0x00000001)
#define FOLDER_IPM_INBOX_VALID          ((ULONG) 0x00000002)
#define FOLDER_IPM_OUTBOX_VALID         ((ULONG) 0x00000004)
#define FOLDER_IPM_WASTEBASKET_VALID    ((ULONG) 0x00000008)
#define FOLDER_IPM_SENTMAIL_VALID       ((ULONG) 0x00000010)
#define FOLDER_VIEWS_VALID              ((ULONG) 0x00000020)
#define FOLDER_COMMON_VIEWS_VALID       ((ULONG) 0x00000040)
#define FOLDER_FINDER_VALID             ((ULONG) 0x00000080)

class IMsgStore : public IMAPIProp {
public:
    //    virtual ~IMsgStore() = 0;

    virtual HRESULT Advise(ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulEventMask, LPMAPIADVISESINK lpAdviseSink,
			   ULONG *lpulConnection) = 0;
    virtual HRESULT Unadvise(ULONG ulConnection) = 0;
    virtual HRESULT CompareEntryIDs(ULONG cbEntryID1, LPENTRYID lpEntryID1, ULONG cbEntryID2, LPENTRYID lpEntryID2,
				    ULONG ulFlags, ULONG *lpulResult) = 0;
    virtual HRESULT OpenEntry(ULONG cbEntryID, LPENTRYID lpEntryID, LPCIID lpInterface, ULONG ulFlags, ULONG *lpulObjType,
			      LPUNKNOWN *lppUnk) = 0;
    virtual HRESULT SetReceiveFolder(LPTSTR lpszMessageClass, ULONG ulFlags, ULONG cbEntryID, LPENTRYID lpEntryID) = 0;
    virtual HRESULT GetReceiveFolder(LPTSTR lpszMessageClass, ULONG ulFlags, ULONG *lpcbEntryID, LPENTRYID *lppEntryID,
				     LPTSTR *lppszExplicitClass) = 0;
    virtual HRESULT GetReceiveFolderTable(ULONG ulFlags, LPMAPITABLE *lppTable) = 0;
    virtual HRESULT StoreLogoff(ULONG *lpulFlags) = 0;
    virtual HRESULT AbortSubmit(ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulFlags) = 0;
    virtual HRESULT GetOutgoingQueue(ULONG ulFlags, LPMAPITABLE *lppTable) = 0;
    virtual HRESULT SetLockState(LPMESSAGE lpMessage,ULONG ulLockState) = 0;
    virtual HRESULT FinishedMsg(ULONG ulFlags, ULONG cbEntryID, LPENTRYID lpEntryID) = 0;
    virtual HRESULT NotifyNewMail(LPNOTIFICATION lpNotification) = 0;
};


/*
 * IMAPIFolder Interface
 */

/* Data structures (Shared with IMAPITable) */

typedef struct _SSortOrder
{
    ULONG   ulPropTag;          /* Column to sort on */
    ULONG   ulOrder;            /* Ascending, descending, combine to left */
} SSortOrder, * LPSSortOrder;

typedef struct _SSortOrderSet
{
    ULONG           cSorts;     /* Number of sort columns in aSort below*/
    ULONG           cCategories;    /* 0 for non-categorized, up to cSorts */
    ULONG           cExpanded;      /* 0 if no categories start expanded, */
                                    /*      up to cExpanded */
    SSortOrder      aSort[MAPI_DIM];    /* The sort orders */
} SSortOrderSet, *LPSSortOrderSet;

#define CbNewSSortOrderSet(_csort) \
    (offsetof(SSortOrderSet,aSort) + (_csort)*sizeof(SSortOrder))
#define CbSSortOrderSet(_lpset) \
    (offsetof(SSortOrderSet,aSort) + \
    (UINT)((_lpset)->cSorts*sizeof(SSortOrder)))
#define SizedSSortOrderSet(_csort, _name) \
struct _SSortOrderSet_ ## _name \
{ \
    ULONG           cSorts;         \
    ULONG           cCategories;    \
    ULONG           cExpanded;      \
    SSortOrder      aSort[_csort];  \
} _name


#define FOLDER_ROOT             ((ULONG) 0x00000000)
#define FOLDER_GENERIC          ((ULONG) 0x00000001)
#define FOLDER_SEARCH           ((ULONG) 0x00000002)
#define MESSAGE_MOVE            ((ULONG) 0x00000001)
#define MESSAGE_DIALOG          ((ULONG) 0x00000002)
#define OPEN_IF_EXISTS          ((ULONG) 0x00000001)
#define DEL_MESSAGES            ((ULONG) 0x00000001)
#define FOLDER_DIALOG           ((ULONG) 0x00000002)
#define DEL_FOLDERS             ((ULONG) 0x00000004)
#define DEL_ASSOCIATED          ((ULONG) 0x00000008)
#define FOLDER_MOVE             ((ULONG) 0x00000001)
#define COPY_SUBFOLDERS         ((ULONG) 0x00000010)
#define MSGSTATUS_HIGHLIGHTED   ((ULONG) 0x00000001)
#define MSGSTATUS_TAGGED        ((ULONG) 0x00000002)
#define MSGSTATUS_HIDDEN        ((ULONG) 0x00000004)
#define MSGSTATUS_DELMARKED     ((ULONG) 0x00000008)
#define MSGSTATUS_REMOTE_DOWNLOAD   ((ULONG) 0x00001000)
#define MSGSTATUS_REMOTE_DELETE     ((ULONG) 0x00002000)
#define RECURSIVE_SORT          ((ULONG) 0x00000002)
#define FLDSTATUS_HIGHLIGHTED   ((ULONG) 0x00000001)
#define FLDSTATUS_TAGGED        ((ULONG) 0x00000002)
#define FLDSTATUS_HIDDEN        ((ULONG) 0x00000004)
#define FLDSTATUS_DELMARKED     ((ULONG) 0x00000008)

class IMAPIFolder : public IMAPIContainer {
public:
    //    virtual ~IMAPIFolder() = 0;

    virtual HRESULT CreateMessage(LPCIID lpInterface, ULONG ulFlags, LPMESSAGE* lppMessage) = 0;
    virtual HRESULT CopyMessages(LPENTRYLIST lpMsgList, LPCIID lpInterface, LPVOID lpDestFolder, ULONG ulUIParam,
				 LPMAPIPROGRESS lpProgress, ULONG ulFlags) = 0;
    virtual HRESULT DeleteMessages(LPENTRYLIST lpMsgList, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags) = 0;
    virtual HRESULT CreateFolder(ULONG ulFolderType, LPTSTR lpszFolderName, LPTSTR lpszFolderComment, LPCIID lpInterface,
				 ULONG ulFlags, LPMAPIFOLDER* lppFolder) = 0;
    virtual HRESULT CopyFolder(ULONG cbEntryID, LPENTRYID lpEntryID, LPCIID lpInterface, LPVOID lpDestFolder, LPTSTR lpszNewFolderName,
			       ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags) = 0;
    virtual HRESULT DeleteFolder(ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags) = 0;
    virtual HRESULT SetReadFlags(LPENTRYLIST lpMsgList, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags) = 0;
    virtual HRESULT GetMessageStatus(ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulFlags, ULONG* lpulMessageStatus) = 0;
    virtual HRESULT SetMessageStatus(ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulNewStatus, ULONG ulNewStatusMask,
				     ULONG* lpulOldStatus) = 0;
    virtual HRESULT SaveContentsSort(LPSSortOrderSet lpSortCriteria, ULONG ulFlags) = 0;
    virtual HRESULT EmptyFolder(ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags) = 0;
};


/*
 * IMessage Interface
 */
#define FORCE_SUBMIT                ((ULONG) 0x00000001)
#define MSGFLAG_READ            ((ULONG) 0x00000001)
#define MSGFLAG_UNMODIFIED      ((ULONG) 0x00000002)
#define MSGFLAG_SUBMIT          ((ULONG) 0x00000004)
#define MSGFLAG_UNSENT          ((ULONG) 0x00000008)
#define MSGFLAG_HASATTACH       ((ULONG) 0x00000010)
#define MSGFLAG_FROMME          ((ULONG) 0x00000020)
#define MSGFLAG_ASSOCIATED      ((ULONG) 0x00000040)
#define MSGFLAG_RESEND          ((ULONG) 0x00000080)
#define MSGFLAG_RN_PENDING      ((ULONG) 0x00000100)
#define MSGFLAG_NRN_PENDING     ((ULONG) 0x00000200)
#define SUBMITFLAG_LOCKED       ((ULONG) 0x00000001)
#define SUBMITFLAG_PREPROCESS   ((ULONG) 0x00000002)
#define MODRECIP_ADD            ((ULONG) 0x00000002)
#define MODRECIP_MODIFY         ((ULONG) 0x00000004)
#define MODRECIP_REMOVE         ((ULONG) 0x00000008)
#define SUPPRESS_RECEIPT        ((ULONG) 0x00000001)
#define CLEAR_READ_FLAG         ((ULONG) 0x00000004)
#define GENERATE_RECEIPT_ONLY   ((ULONG) 0x00000010)
#define CLEAR_RN_PENDING        ((ULONG) 0x00000020)
#define CLEAR_NRN_PENDING       ((ULONG) 0x00000040)
#define ATTACH_DIALOG           ((ULONG) 0x00000001)
#define SECURITY_SIGNED         ((ULONG) 0x00000001)
#define SECURITY_ENCRYPTED      ((ULONG) 0x00000002)
#define PRIO_URGENT             ((long)  1)
#define PRIO_NORMAL             ((long)  0)
#define PRIO_NONURGENT          ((long) -1)
#define SENSITIVITY_NONE                    ((ULONG) 0x00000000)
#define SENSITIVITY_PERSONAL                ((ULONG) 0x00000001)
#define SENSITIVITY_PRIVATE                 ((ULONG) 0x00000002)
#define SENSITIVITY_COMPANY_CONFIDENTIAL    ((ULONG) 0x00000003)
#define IMPORTANCE_LOW          ((long) 0)
#define IMPORTANCE_NORMAL       ((long) 1)
#define IMPORTANCE_HIGH         ((long) 2)

class IMessage : public IMAPIProp {
public:
    //    virtual ~IMessage() = 0;

    virtual HRESULT GetAttachmentTable(ULONG ulFlags, LPMAPITABLE *lppTable) = 0;
    virtual HRESULT OpenAttach(ULONG ulAttachmentNum, LPCIID lpInterface, ULONG ulFlags, LPATTACH *lppAttach) = 0;
    virtual HRESULT CreateAttach(LPCIID lpInterface, ULONG ulFlags, ULONG *lpulAttachmentNum, LPATTACH *lppAttach) = 0;
    virtual HRESULT DeleteAttach(ULONG ulAttachmentNum, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags) = 0;
    virtual HRESULT GetRecipientTable(ULONG ulFlags, LPMAPITABLE *lppTable) = 0;
    virtual HRESULT ModifyRecipients(ULONG ulFlags, LPADRLIST lpMods) = 0;
    virtual HRESULT SubmitMessage(ULONG ulFlags) = 0;
    virtual HRESULT SetReadFlag(ULONG ulFlags) = 0;
};


/*
 * IAttach Interface
 */
#define NO_ATTACHMENT           ((ULONG) 0x00000000)
#define ATTACH_BY_VALUE         ((ULONG) 0x00000001)
#define ATTACH_BY_REFERENCE     ((ULONG) 0x00000002)
#define ATTACH_BY_REF_RESOLVE   ((ULONG) 0x00000003)
#define ATTACH_BY_REF_ONLY      ((ULONG) 0x00000004)
#define ATTACH_EMBEDDED_MSG     ((ULONG) 0x00000005)
#define ATTACH_OLE              ((ULONG) 0x00000006)

class IAttach : public IMAPIProp {
public:
    //    virtual ~IAttach() = 0;
};


/*
 * IABContainer Interface
 */

/*
 *  IABContainer PR_CONTAINER_FLAGS values
 *  If AB_UNMODIFIABLE and AB_MODIFIABLE are both set, it means the container
 *  doesn't know if it's modifiable or not, and the client should
 *  try to modify the contents but we won't expect it to work.
 *  If the AB_RECIPIENTS flag is set and neither AB_MODIFIABLE or AB_UNMODIFIABLE
 *  bits are set, it is an error.
 */

typedef struct _flaglist
{
    ULONG cFlags;
    ULONG ulFlag[MAPI_DIM];
} FlagList, *LPFlagList;

/* Zarafa added */
#define CbNewFlagList(_cflags) \
    (offsetof(FlagList,ulFlag) + (_cflags)*sizeof(ULONG))

#define AB_RECIPIENTS           ((ULONG) 0x00000001)
#define AB_SUBCONTAINERS        ((ULONG) 0x00000002)
#define AB_MODIFIABLE           ((ULONG) 0x00000004)
#define AB_UNMODIFIABLE         ((ULONG) 0x00000008)
#define AB_FIND_ON_OPEN         ((ULONG) 0x00000010)
#define AB_NOT_DEFAULT          ((ULONG) 0x00000020)
#define AB_UNICODE_OK           ((ULONG) 0x00000040)
#define CREATE_CHECK_DUP_STRICT ((ULONG) 0x00000001)
#define CREATE_CHECK_DUP_LOOSE  ((ULONG) 0x00000002)
#define CREATE_REPLACE          ((ULONG) 0x00000004)
#define MAPI_UNRESOLVED         ((ULONG) 0x00000000)
#define MAPI_AMBIGUOUS          ((ULONG) 0x00000001)
#define MAPI_RESOLVED           ((ULONG) 0x00000002)

class IABContainer : public IMAPIContainer {
public:
    //    virtual ~IABContainer() = 0;

    virtual HRESULT CreateEntry(ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulCreateFlags, LPMAPIPROP* lppMAPIPropEntry) = 0;
    virtual HRESULT CopyEntries(LPENTRYLIST lpEntries, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags) = 0;
    virtual HRESULT DeleteEntries(LPENTRYLIST lpEntries, ULONG ulFlags) = 0;
    virtual HRESULT ResolveNames(LPSPropTagArray lpPropTagArray, ULONG ulFlags, LPADRLIST lpAdrList, LPFlagList lpFlagList) = 0;
};


/*
 * IMailUser Interface ( == IMAPIProp)
 */

/*  Any call which can create a one-off entryID (i.e. MAPISupport::CreateOneOff
    or IAddrBook::CreateOneOff) can encode the value for PR_SEND_RICH_INFO by
    passing in the following flag in the ulFlags parameter.  Setting this flag
    indicates that PR_SEND_RICH_INFO will be FALSE.
*/
#define MAPI_SEND_NO_RICH_INFO      ((ULONG) 0x00010000)

#define MAPI_DIAG(_code)    ((LONG) _code)

#define MAPI_DIAG_NO_DIAGNOSTIC                     MAPI_DIAG( -1 )
#define MAPI_DIAG_OR_NAME_UNRECOGNIZED              MAPI_DIAG( 0 )
#define MAPI_DIAG_OR_NAME_AMBIGUOUS                 MAPI_DIAG( 1 )
#define MAPI_DIAG_MTS_CONGESTED                     MAPI_DIAG( 2 )
#define MAPI_DIAG_LOOP_DETECTED                     MAPI_DIAG( 3 )
#define MAPI_DIAG_RECIPIENT_UNAVAILABLE             MAPI_DIAG( 4 )
#define MAPI_DIAG_MAXIMUM_TIME_EXPIRED              MAPI_DIAG( 5 )
#define MAPI_DIAG_EITS_UNSUPPORTED                  MAPI_DIAG( 6 )
#define MAPI_DIAG_CONTENT_TOO_LONG                  MAPI_DIAG( 7 )
#define MAPI_DIAG_IMPRACTICAL_TO_CONVERT            MAPI_DIAG( 8 )
#define MAPI_DIAG_PROHIBITED_TO_CONVERT             MAPI_DIAG( 9 )
#define MAPI_DIAG_CONVERSION_UNSUBSCRIBED           MAPI_DIAG( 10 )
#define MAPI_DIAG_PARAMETERS_INVALID                MAPI_DIAG( 11 )
#define MAPI_DIAG_CONTENT_SYNTAX_IN_ERROR           MAPI_DIAG( 12 )
#define MAPI_DIAG_LENGTH_CONSTRAINT_VIOLATD         MAPI_DIAG( 13 )
#define MAPI_DIAG_NUMBER_CONSTRAINT_VIOLATD         MAPI_DIAG( 14 )
#define MAPI_DIAG_CONTENT_TYPE_UNSUPPORTED          MAPI_DIAG( 15 )
#define MAPI_DIAG_TOO_MANY_RECIPIENTS               MAPI_DIAG( 16 )
#define MAPI_DIAG_NO_BILATERAL_AGREEMENT            MAPI_DIAG( 17 )
#define MAPI_DIAG_CRITICAL_FUNC_UNSUPPORTED         MAPI_DIAG( 18 )
#define MAPI_DIAG_CONVERSION_LOSS_PROHIB            MAPI_DIAG( 19 )
#define MAPI_DIAG_LINE_TOO_LONG                     MAPI_DIAG( 20 )
#define MAPI_DIAG_PAGE_TOO_LONG                     MAPI_DIAG( 21 )
#define MAPI_DIAG_PICTORIAL_SYMBOL_LOST             MAPI_DIAG( 22 )
#define MAPI_DIAG_PUNCTUATION_SYMBOL_LOST           MAPI_DIAG( 23 )
#define MAPI_DIAG_ALPHABETIC_CHARACTER_LOST         MAPI_DIAG( 24 )
#define MAPI_DIAG_MULTIPLE_INFO_LOSSES              MAPI_DIAG( 25 )
#define MAPI_DIAG_REASSIGNMENT_PROHIBITED           MAPI_DIAG( 26 )
#define MAPI_DIAG_REDIRECTION_LOOP_DETECTED         MAPI_DIAG( 27 )
#define MAPI_DIAG_EXPANSION_PROHIBITED              MAPI_DIAG( 28 )
#define MAPI_DIAG_SUBMISSION_PROHIBITED             MAPI_DIAG( 29 )
#define MAPI_DIAG_EXPANSION_FAILED                  MAPI_DIAG( 30 )
#define MAPI_DIAG_RENDITION_UNSUPPORTED             MAPI_DIAG( 31 )
#define MAPI_DIAG_MAIL_ADDRESS_INCORRECT            MAPI_DIAG( 32 )
#define MAPI_DIAG_MAIL_OFFICE_INCOR_OR_INVD         MAPI_DIAG( 33 )
#define MAPI_DIAG_MAIL_ADDRESS_INCOMPLETE           MAPI_DIAG( 34 )
#define MAPI_DIAG_MAIL_RECIPIENT_UNKNOWN            MAPI_DIAG( 35 )
#define MAPI_DIAG_MAIL_RECIPIENT_DECEASED           MAPI_DIAG( 36 )
#define MAPI_DIAG_MAIL_ORGANIZATION_EXPIRED         MAPI_DIAG( 37 )
#define MAPI_DIAG_MAIL_REFUSED                      MAPI_DIAG( 38 )
#define MAPI_DIAG_MAIL_UNCLAIMED                    MAPI_DIAG( 39 )
#define MAPI_DIAG_MAIL_RECIPIENT_MOVED              MAPI_DIAG( 40 )
#define MAPI_DIAG_MAIL_RECIPIENT_TRAVELLING         MAPI_DIAG( 41 )
#define MAPI_DIAG_MAIL_RECIPIENT_DEPARTED           MAPI_DIAG( 42 )
#define MAPI_DIAG_MAIL_NEW_ADDRESS_UNKNOWN          MAPI_DIAG( 43 )
#define MAPI_DIAG_MAIL_FORWARDING_UNWANTED          MAPI_DIAG( 44 )
#define MAPI_DIAG_MAIL_FORWARDING_PROHIB            MAPI_DIAG( 45 )
#define MAPI_DIAG_SECURE_MESSAGING_ERROR            MAPI_DIAG( 46 )
#define MAPI_DIAG_DOWNGRADING_IMPOSSIBLE            MAPI_DIAG( 47 )
#define MAPI_MH_DP_PUBLIC_UA                        ((ULONG) 0)
#define MAPI_MH_DP_PRIVATE_UA                       ((ULONG) 1)
#define MAPI_MH_DP_MS                               ((ULONG) 2)
#define MAPI_MH_DP_ML                               ((ULONG) 3)
#define MAPI_MH_DP_PDAU                             ((ULONG) 4)
#define MAPI_MH_DP_PDS_PATRON                       ((ULONG) 5)
#define MAPI_MH_DP_OTHER_AU                         ((ULONG) 6)

class IMailUser : public IMAPIProp {
public:
    //    virtual ~IMailUser() = 0;
};


/*
 * IDistList Interface
 */
class IDistList : public IMAPIContainer {
public:
    //    virtual ~IDistList() = 0;

    virtual HRESULT CreateEntry(ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulCreateFlags, LPMAPIPROP* lppMAPIPropEntry) = 0;
    virtual HRESULT CopyEntries(LPENTRYLIST lpEntries, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags) = 0;
    virtual HRESULT DeleteEntries(LPENTRYLIST lpEntries, ULONG ulFlags) = 0;
    virtual HRESULT ResolveNames(LPSPropTagArray lpPropTagArray, ULONG ulFlags, LPADRLIST lpAdrList, LPFlagList lpFlagList) = 0;
};


class IMAPIStatus : public IMAPIProp {
public:
    //    virtual ~IMAPIStatus() = 0;
	virtual HRESULT ValidateState(ULONG ulUIParam, ULONG ulFlags) = 0;
    virtual HRESULT SettingsDialog(ULONG ulUIParam, ULONG ulFlags) = 0;
    virtual HRESULT ChangePassword(LPTSTR lpOldPass, LPTSTR lpNewPass, ULONG ulFlags) = 0;
    virtual HRESULT FlushQueues(ULONG ulUIParam, ULONG cbTargetTransport, LPENTRYID lpTargetTransport, ULONG ulFlags) = 0;
};


/*
 * IMAPITable Interface
 */

/* Table status */

#define TBLSTAT_COMPLETE            ((ULONG) 0)
#define TBLSTAT_QCHANGED            ((ULONG) 7)
#define TBLSTAT_SORTING             ((ULONG) 9)
#define TBLSTAT_SORT_ERROR          ((ULONG) 10)
#define TBLSTAT_SETTING_COLS        ((ULONG) 11)
#define TBLSTAT_SETCOL_ERROR        ((ULONG) 13)
#define TBLSTAT_RESTRICTING         ((ULONG) 14)
#define TBLSTAT_RESTRICT_ERROR      ((ULONG) 15)


/* Table Type */

#define TBLTYPE_SNAPSHOT            ((ULONG) 0)
#define TBLTYPE_KEYSET              ((ULONG) 1)
#define TBLTYPE_DYNAMIC             ((ULONG) 2)


/* Sort order */

/* bit 0: set if descending, clear if ascending */

#define TABLE_SORT_ASCEND       ((ULONG) 0x00000000)
#define TABLE_SORT_DESCEND      ((ULONG) 0x00000001)
#define TABLE_SORT_COMBINE      ((ULONG) 0x00000002)


typedef ULONG       BOOKMARK;

#define BOOKMARK_BEGINNING  ((BOOKMARK) 0)      /* Before first row */
#define BOOKMARK_CURRENT    ((BOOKMARK) 1)      /* Before current row */
#define BOOKMARK_END        ((BOOKMARK) 2)      /* After last row */

/* Fuzzy Level */

#define FL_FULLSTRING       ((ULONG) 0x00000000)
#define FL_SUBSTRING        ((ULONG) 0x00000001)
#define FL_PREFIX           ((ULONG) 0x00000002)

#define FL_IGNORECASE       ((ULONG) 0x00010000)
#define FL_IGNORENONSPACE   ((ULONG) 0x00020000)
#define FL_LOOSE            ((ULONG) 0x00040000)

/* Restrictions */

/* Restriction types */

#define RES_AND             ((ULONG) 0x00000000)
#define RES_OR              ((ULONG) 0x00000001)
#define RES_NOT             ((ULONG) 0x00000002)
#define RES_CONTENT         ((ULONG) 0x00000003)
#define RES_PROPERTY        ((ULONG) 0x00000004)
#define RES_COMPAREPROPS    ((ULONG) 0x00000005)
#define RES_BITMASK         ((ULONG) 0x00000006)
#define RES_SIZE            ((ULONG) 0x00000007)
#define RES_EXIST           ((ULONG) 0x00000008)
#define RES_SUBRESTRICTION  ((ULONG) 0x00000009)
#define RES_COMMENT         ((ULONG) 0x0000000A)

/* Relational operators. These apply to all property comparison restrictions. */

#define RELOP_LT        ((ULONG) 0)     /* <  */
#define RELOP_LE        ((ULONG) 1)     /* <= */
#define RELOP_GT        ((ULONG) 2)     /* >  */
#define RELOP_GE        ((ULONG) 3)     /* >= */
#define RELOP_EQ        ((ULONG) 4)     /* == */
#define RELOP_NE        ((ULONG) 5)     /* != */
#define RELOP_RE        ((ULONG) 6)     /* LIKE (Regular expression) */

/* Bitmask operators, for RES_BITMASK only. */

#define BMR_EQZ     ((ULONG) 0)     /* ==0 */
#define BMR_NEZ     ((ULONG) 1)     /* !=0 */

/* Subobject identifiers for RES_SUBRESTRICTION only. See MAPITAGS.H. */

/* #define PR_MESSAGE_RECIPIENTS  PROP_TAG(PT_OBJECT,0x0E12) */
/* #define PR_MESSAGE_ATTACHMENTS PROP_TAG(PT_OBJECT,0x0E13) */

typedef struct _SAndRestriction
{
    ULONG           cRes;
    LPSRestriction  lpRes;
} SAndRestriction;

typedef struct _SOrRestriction
{
    ULONG           cRes;
    LPSRestriction  lpRes;
} SOrRestriction;

typedef struct _SNotRestriction
{
    ULONG           ulReserved;
    LPSRestriction  lpRes;
} SNotRestriction;

typedef struct _SContentRestriction
{
    ULONG           ulFuzzyLevel;
    ULONG           ulPropTag;
    LPSPropValue    lpProp;
} SContentRestriction;

typedef struct _SBitMaskRestriction
{
    ULONG           relBMR;
    ULONG           ulPropTag;
    ULONG           ulMask;
} SBitMaskRestriction;

typedef struct _SPropertyRestriction
{
    ULONG           relop;
    ULONG           ulPropTag;
    LPSPropValue    lpProp;
} SPropertyRestriction;

typedef struct _SComparePropsRestriction
{
    ULONG           relop;
    ULONG           ulPropTag1;
    ULONG           ulPropTag2;
} SComparePropsRestriction;

typedef struct _SSizeRestriction
{
    ULONG           relop;
    ULONG           ulPropTag;
    ULONG           cb;
} SSizeRestriction;

typedef struct _SExistRestriction
{
    ULONG           ulReserved1;
    ULONG           ulPropTag;
    ULONG           ulReserved2;
} SExistRestriction;

typedef struct _SSubRestriction
{
    ULONG           ulSubObject;
    LPSRestriction  lpRes;
} SSubRestriction;

typedef struct _SCommentRestriction
{
    ULONG           cValues; /* # of properties in lpProp */
    LPSRestriction  lpRes;
    LPSPropValue    lpProp;
} SCommentRestriction;

typedef struct _SRestriction
{
    ULONG   rt;         /* Restriction type */
    union
    {
        SComparePropsRestriction    resCompareProps;    /* first */
        SAndRestriction             resAnd;
        SOrRestriction              resOr;
        SNotRestriction             resNot;
        SContentRestriction         resContent;
        SPropertyRestriction        resProperty;
        SBitMaskRestriction         resBitMask;
        SSizeRestriction            resSize;
        SExistRestriction           resExist;
        SSubRestriction             resSub;
        SCommentRestriction         resComment;
    } res;
} SRestriction;

/* SComparePropsRestriction is first in the union so that */
/* static initializations of 3-value restriction work.    */

/* Flags of the methods of IMAPITable */

/* QueryColumn */

#define TBL_ALL_COLUMNS     ((ULONG) 0x00000001)

/* QueryRows */
/* Possible values for PR_ROW_TYPE (for categorization) */

#define TBL_LEAF_ROW            ((ULONG) 1)
#define TBL_EMPTY_CATEGORY      ((ULONG) 2)
#define TBL_EXPANDED_CATEGORY   ((ULONG) 3)
#define TBL_COLLAPSED_CATEGORY  ((ULONG) 4)

/* Table wait flag */

#define TBL_NOWAIT          ((ULONG) 0x00000001)
/* alternative name for TBL_NOWAIT */
#define TBL_ASYNC           ((ULONG) 0x00000001)
#define TBL_BATCH           ((ULONG) 0x00000002)

/* FindRow */

#define DIR_BACKWARD        ((ULONG) 0x00000001)

/* Table cursor states */

#define TBL_NOADVANCE       ((ULONG) 0x00000001)

class IMAPITable : public IUnknown {
public:
    //    virtual ~IMAPITable() = 0;

    virtual HRESULT GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR *lppMAPIError) = 0;
    virtual HRESULT Advise(ULONG ulEventMask, LPMAPIADVISESINK lpAdviseSink, ULONG * lpulConnection) = 0;
    virtual HRESULT Unadvise(ULONG ulConnection) = 0;
    virtual HRESULT GetStatus(ULONG *lpulTableStatus, ULONG *lpulTableType) = 0;
    virtual HRESULT SetColumns(LPSPropTagArray lpPropTagArray, ULONG ulFlags) = 0;
    virtual HRESULT QueryColumns(ULONG ulFlags, LPSPropTagArray *lpPropTagArray) = 0;
    virtual HRESULT GetRowCount(ULONG ulFlags, ULONG *lpulCount) = 0;
    virtual HRESULT SeekRow(BOOKMARK bkOrigin, LONG lRowCount, LONG *lplRowsSought) = 0;
    virtual HRESULT SeekRowApprox(ULONG ulNumerator, ULONG ulDenominator) = 0;
    virtual HRESULT QueryPosition(ULONG *lpulRow, ULONG *lpulNumerator, ULONG *lpulDenominator) = 0;
    virtual HRESULT FindRow(LPSRestriction lpRestriction, BOOKMARK bkOrigin, ULONG ulFlags) = 0;
    virtual HRESULT Restrict(LPSRestriction lpRestriction, ULONG ulFlags) = 0;
    virtual HRESULT CreateBookmark(BOOKMARK* lpbkPosition) = 0;
    virtual HRESULT FreeBookmark(BOOKMARK bkPosition) = 0;
    virtual HRESULT SortTable(LPSSortOrderSet lpSortCriteria, ULONG ulFlags) = 0;
    virtual HRESULT QuerySortOrder(LPSSortOrderSet *lppSortCriteria) = 0;
    virtual HRESULT QueryRows(LONG lRowCount, ULONG ulFlags, LPSRowSet *lppRows) = 0;
    virtual HRESULT Abort() = 0;
    virtual HRESULT ExpandRow(ULONG cbInstanceKey, LPBYTE pbInstanceKey, ULONG ulRowCount,
			      ULONG ulFlags, LPSRowSet * lppRows, ULONG *lpulMoreRows) = 0;
    virtual HRESULT CollapseRow(ULONG cbInstanceKey, LPBYTE pbInstanceKey, ULONG ulFlags, ULONG *lpulRowCount) = 0;
    virtual HRESULT WaitForCompletion(ULONG ulFlags, ULONG ulTimeout, ULONG *lpulTableStatus) = 0;
    virtual HRESULT GetCollapseState(ULONG ulFlags, ULONG cbInstanceKey, LPBYTE lpbInstanceKey, ULONG *lpcbCollapseState,
				     LPBYTE *lppbCollapseState) = 0;
    virtual HRESULT SetCollapseState(ULONG ulFlags, ULONG cbCollapseState, LPBYTE pbCollapseState, BOOKMARK *lpbkLocation) = 0;
};


/*
 * IProfSect Interface
 */

/* Standard section for public profile properties */

#define PS_PROFILE_PROPERTIES_INIT \
{   0x98, 0x15, 0xAC, 0x08, 0xAA, 0xB0, 0x10, 0x1A, \
    0x8C, 0x93, 0x08, 0x00, 0x2B, 0x2A, 0x56, 0xC2  }

/*
 * IMAPIStatus Interface
 */
#define MAPI_STORE_PROVIDER     ((ULONG) 33)
#define MAPI_AB                 ((ULONG) 34)
#define MAPI_AB_PROVIDER        ((ULONG) 35)
#define MAPI_TRANSPORT_PROVIDER ((ULONG) 36)
#define MAPI_SPOOLER            ((ULONG) 37)
#define MAPI_PROFILE_PROVIDER   ((ULONG) 38)
#define MAPI_SUBSYSTEM          ((ULONG) 39)
#define MAPI_HOOK_PROVIDER      ((ULONG) 40)

#define STATUS_VALIDATE_STATE   ((ULONG) 0x00000001)
#define STATUS_SETTINGS_DIALOG  ((ULONG) 0x00000002)
#define STATUS_CHANGE_PASSWORD  ((ULONG) 0x00000004)
#define STATUS_FLUSH_QUEUES     ((ULONG) 0x00000008)

#define STATUS_DEFAULT_OUTBOUND ((ULONG) 0x00000001)
#define STATUS_DEFAULT_STORE    ((ULONG) 0x00000002)
#define STATUS_PRIMARY_IDENTITY ((ULONG) 0x00000004)
#define STATUS_SIMPLE_STORE     ((ULONG) 0x00000008)
#define STATUS_XP_PREFER_LAST   ((ULONG) 0x00000010)
#define STATUS_NO_PRIMARY_IDENTITY ((ULONG) 0x00000020)
#define STATUS_NO_DEFAULT_STORE ((ULONG) 0x00000040)
#define STATUS_TEMP_SECTION     ((ULONG) 0x00000080)
#define STATUS_OWN_STORE        ((ULONG) 0x00000100)
/****** HOOK_INBOUND            ((ULONG) 0x00000200) Defined in MAPIHOOK.H */
/****** HOOK_OUTBOUND           ((ULONG) 0x00000400) Defined in MAPIHOOK.H */
#define STATUS_NEED_IPM_TREE    ((ULONG) 0x00000800)
#define STATUS_PRIMARY_STORE    ((ULONG) 0x00001000)
#define STATUS_SECONDARY_STORE  ((ULONG) 0x00002000)

#define STATUS_AVAILABLE        ((ULONG) 0x00000001)
#define STATUS_OFFLINE          ((ULONG) 0x00000002)
#define STATUS_FAILURE          ((ULONG) 0x00000004)

#define STATUS_INBOUND_ENABLED  ((ULONG) 0x00010000)
#define STATUS_INBOUND_ACTIVE   ((ULONG) 0x00020000)
#define STATUS_INBOUND_FLUSH    ((ULONG) 0x00040000)
#define STATUS_OUTBOUND_ENABLED ((ULONG) 0x00100000)
#define STATUS_OUTBOUND_ACTIVE  ((ULONG) 0x00200000)
#define STATUS_OUTBOUND_FLUSH   ((ULONG) 0x00400000)
#define STATUS_REMOTE_ACCESS    ((ULONG) 0x00800000)

#define SUPPRESS_UI                 ((ULONG) 0x00000001)
#define REFRESH_XP_HEADER_CACHE     ((ULONG) 0x00010000)
#define PROCESS_XP_HEADER_CACHE     ((ULONG) 0x00020000)
#define FORCE_XP_CONNECT            ((ULONG) 0x00040000)
#define FORCE_XP_DISCONNECT         ((ULONG) 0x00080000)
#define CONFIG_CHANGED              ((ULONG) 0x00100000)
#define ABORT_XP_HEADER_OPERATION   ((ULONG) 0x00200000)
#define SHOW_XP_SESSION_UI          ((ULONG) 0x00400000)

#define UI_READONLY     ((ULONG) 0x00000001)

#define FLUSH_UPLOAD        ((ULONG) 0x00000002)
#define FLUSH_DOWNLOAD      ((ULONG) 0x00000004)
#define FLUSH_FORCE         ((ULONG) 0x00000008)
#define FLUSH_NO_UI         ((ULONG) 0x00000010)
#define FLUSH_ASYNC_OK      ((ULONG) 0x00000020)

class IProfSect : public IMAPIProp {
public:
    //    virtual ~IProfSect() = 0;

};


/*
 * IMAPIProgress Interface
 */
#define MAPI_TOP_LEVEL      ((ULONG) 0x00000001)

class IMAPIProgress : public IUnknown {
public:
    //    virtual ~IMAPIProgress() = 0;

    virtual HRESULT Progress(ULONG ulValue, ULONG ulCount, ULONG ulTotal) = 0;
    virtual HRESULT GetFlags(ULONG* lpulFlags) = 0;
    virtual HRESULT GetMax(ULONG* lpulMax) = 0;
    virtual HRESULT GetMin(ULONG* lpulMin) = 0;
    virtual HRESULT SetLimits(ULONG* lpulMin, ULONG* lpulMax, ULONG* lpulFlags) = 0;
};


/*
 * IProviderAdmin Interface
 */
#define UI_SERVICE                  0x00000002
#define SERVICE_UI_ALWAYS           0x00000002
#define SERVICE_UI_ALLOWED          0x00000010
#define UI_CURRENT_PROVIDER_FIRST   0x00000004

class IProviderAdmin : public IUnknown {
public:
    //    virtual ~IProviderAdmin() = 0;

    virtual HRESULT GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR* lppMAPIError) = 0;
    virtual HRESULT GetProviderTable(ULONG ulFlags, LPMAPITABLE* lppTable) = 0;
    virtual HRESULT CreateProvider(LPTSTR lpszProvider, ULONG cValues, LPSPropValue lpProps, ULONG ulUIParam,
				   ULONG ulFlags, MAPIUID* lpUID) = 0;
    virtual HRESULT DeleteProvider(LPMAPIUID lpUID) = 0;
    virtual HRESULT OpenProfileSection(LPMAPIUID lpUID, LPCIID lpInterface, ULONG ulFlags, LPPROFSECT* lppProfSect) = 0;
};


/*

 *

 */


/* Address Book interface definition */
#define GET_ADRPARM_VERSION(ulFlags)  (((ULONG)ulFlags) & 0xF0000000)
#define SET_ADRPARM_VERSION(ulFlags, ulVersion)  (((ULONG)ulVersion) | (((ULONG)ulFlags) & 0x0FFFFFFF))
#define ADRPARM_HELP_CTX        ((ULONG) 0x00000000)

#define DIALOG_MODAL            ((ULONG) 0x00000001)
#define DIALOG_SDI              ((ULONG) 0x00000002)
#define DIALOG_OPTIONS          ((ULONG) 0x00000004)
#define ADDRESS_ONE             ((ULONG) 0x00000008)
#define AB_SELECTONLY           ((ULONG) 0x00000010)
#define AB_RESOLVE              ((ULONG) 0x00000020)

/*  PR_DISPLAY_TYPEs                 */

/*  For address book contents tables */
#define DT_MAILUSER         ((ULONG) 0x00000000)
#define DT_DISTLIST         ((ULONG) 0x00000001)
#define DT_FORUM            ((ULONG) 0x00000002)
#define DT_AGENT            ((ULONG) 0x00000003)
#define DT_ORGANIZATION     ((ULONG) 0x00000004)
#define DT_PRIVATE_DISTLIST ((ULONG) 0x00000005)
#define DT_REMOTE_MAILUSER  ((ULONG) 0x00000006)
#define DT_ROOM	            ((ULONG) 0x00000007)
#define DT_EQUIPMENT        ((ULONG) 0x00000008)
#define DT_SEC_DISTLIST     ((ULONG) 0x00000009)

/*  For address book hierarchy tables */
#define DT_MODIFIABLE       ((ULONG) 0x00010000)
#define DT_GLOBAL           ((ULONG) 0x00020000)
#define DT_LOCAL            ((ULONG) 0x00030000)
#define DT_WAN              ((ULONG) 0x00040000)
#define DT_NOT_SPECIFIC     ((ULONG) 0x00050000)

/*  For folder hierarchy tables */
#define DT_FOLDER           ((ULONG) 0x01000000)
#define DT_FOLDER_LINK      ((ULONG) 0x02000000)
#define DT_FOLDER_SPECIAL   ((ULONG) 0x04000000)

/* PR_DISPLAY_TYPE_EX flags */
#define DTE_FLAG_REMOTE_VALID 0x80000000
#define DTE_FLAG_ACL_CAPABLE  0x40000000
#define DTE_MASK_REMOTE       0x0000ff00
#define DTE_MASK_LOCAL        0x000000ff
 
#define DTE_IS_REMOTE_VALID(v)	(!!((v) & DTE_FLAG_REMOTE_VALID))
#define DTE_IS_ACL_CAPABLE(v)	(!!((v) & DTE_FLAG_ACL_CAPABLE))
#define DTE_REMOTE(v)		(((v) & DTE_MASK_REMOTE) >> 8)
#define DTE_LOCAL(v)		((v) & DTE_MASK_LOCAL)


extern "C" {

/*  Accelerator callback for DIALOG_SDI form of AB UI */
typedef BOOL (ACCELERATEABSDI)(ULONG ulUIParam, LPVOID lpvmsg);
typedef ACCELERATEABSDI* LPFNABSDI;

/*  Callback to application telling it that the DIALOG_SDI form of the */
/*  AB UI has been dismissed.  This is so that the above LPFNABSDI     */
/*  function doesn't keep being called.                                */
typedef void (DISMISSMODELESS)(ULONG ulUIParam, LPVOID lpvContext);
typedef DISMISSMODELESS* LPFNDISMISS;

/*
 * Prototype for the client function hooked to an optional button on
 * the address book dialog
 */
typedef SCODE (*LPFNBUTTON)(
    ULONG               ulUIParam,
    LPVOID              lpvContext,
    ULONG               cbEntryID,
    LPENTRYID           lpSelection,
    ULONG               ulFlags
);


/* Parameters for the address book dialog */
typedef struct _ADRPARM {
    ULONG           cbABContEntryID;
    LPENTRYID       lpABContEntryID;
    ULONG           ulFlags;

    LPVOID          lpReserved;
    ULONG           ulHelpContext;
    LPTSTR          lpszHelpFileName;

    LPFNABSDI       lpfnABSDI;
    LPFNDISMISS     lpfnDismiss;
    LPVOID          lpvDismissContext;
    LPTSTR          lpszCaption;
    LPTSTR          lpszNewEntryTitle;
    LPTSTR          lpszDestWellsTitle;
    ULONG           cDestFields;
    ULONG           nDestFieldFocus;
    LPTSTR *    lppszDestTitles;
    ULONG *     lpulDestComps;
    LPSRestriction  lpContRestriction;
    LPSRestriction  lpHierRestriction;
} ADRPARM, *LPADRPARM;

} // EXTERN "C"

/* Random flags */

/* Flag for deferred error */
#define MAPI_DEFERRED_ERRORS    ((ULONG) 0x00000008)

/* Flag for creating and using Folder Associated Information Messages */
#define MAPI_ASSOCIATED         ((ULONG) 0x00000040)

/* Flags for OpenMessageStore() */
#define MDB_NO_DIALOG           ((ULONG) 0x00000001)
#define MDB_WRITE               ((ULONG) 0x00000004)
#define MDB_TEMPORARY           ((ULONG) 0x00000020)
#define MDB_NO_MAIL             ((ULONG) 0x00000080)

/* Flags for OpenAddressBook */
#define AB_NO_DIALOG            ((ULONG) 0x00000001)

/*-----------*/


/*
 * IMAPIControl Interface
 */
#define  MAPI_ENABLED       ((ULONG) 0x00000000)
#define  MAPI_DISABLED      ((ULONG) 0x00000001)
class IMAPIControl : public IUnknown {
public:
    //    virtual ~IMAPIControl() = 0;

    virtual HRESULT GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR* lppMAPIError) = 0;
    virtual HRESULT Activate(ULONG ulFlags, ULONG ulUIParam) = 0;
    virtual HRESULT GetState(ULONG ulFlags, ULONG* lpulState) = 0;
};


/* 
 * Display Tables
 */
#define DT_MULTILINE        ((ULONG) 0x00000001)
#define DT_EDITABLE         ((ULONG) 0x00000002)
#define DT_REQUIRED         ((ULONG) 0x00000004)
#define DT_SET_IMMEDIATE    ((ULONG) 0x00000008)
#define DT_PASSWORD_EDIT    ((ULONG) 0x00000010)
#define DT_ACCEPT_DBCS      ((ULONG) 0x00000020)
#define DT_SET_SELECTION    ((ULONG) 0x00000040)

#define DTCT_LABEL          ((ULONG) 0x00000000)
#define DTCT_EDIT           ((ULONG) 0x00000001)
#define DTCT_LBX            ((ULONG) 0x00000002)
#define DTCT_COMBOBOX       ((ULONG) 0x00000003)
#define DTCT_DDLBX          ((ULONG) 0x00000004)
#define DTCT_CHECKBOX       ((ULONG) 0x00000005)
#define DTCT_GROUPBOX       ((ULONG) 0x00000006)
#define DTCT_BUTTON         ((ULONG) 0x00000007)
#define DTCT_PAGE           ((ULONG) 0x00000008)
#define DTCT_RADIOBUTTON    ((ULONG) 0x00000009)
#define DTCT_MVLISTBOX      ((ULONG) 0x0000000B)
#define DTCT_MVDDLBX        ((ULONG) 0x0000000C)

/* Labels */
typedef struct _DTBLLABEL
{
    ULONG ulbLpszLabelName;
    ULONG ulFlags;
} DTBLLABEL, *LPDTBLLABEL;
#define SizedDtblLabel(n,u) \
struct _DTBLLABEL_ ## u \
{ \
    DTBLLABEL   dtbllabel; \
    TCHAR       lpszLabelName[n]; \
} u

/*  Simple Text Edits  */
typedef struct _DTBLEDIT
{
    ULONG ulbLpszCharsAllowed;
    ULONG ulFlags;
    ULONG ulNumCharsAllowed;
    ULONG ulPropTag;
} DTBLEDIT, *LPDTBLEDIT;
#define SizedDtblEdit(n,u) \
struct _DTBLEDIT_ ## u \
{ \
    DTBLEDIT    dtbledit; \
    TCHAR       lpszCharsAllowed[n]; \
} u

/*  List Box  */
#define MAPI_NO_HBAR        ((ULONG) 0x00000001)
#define MAPI_NO_VBAR        ((ULONG) 0x00000002)

typedef struct _DTBLLBX
{
    ULONG ulFlags;
    ULONG ulPRSetProperty;
    ULONG ulPRTableName;
} DTBLLBX, *LPDTBLLBX;


/*  Combo Box   */
typedef struct _DTBLCOMBOBOX
{
    ULONG ulbLpszCharsAllowed;
    ULONG ulFlags;
    ULONG ulNumCharsAllowed;
    ULONG ulPRPropertyName;
    ULONG ulPRTableName;
} DTBLCOMBOBOX, *LPDTBLCOMBOBOX;
#define SizedDtblComboBox(n,u) \
struct _DTBLCOMBOBOX_ ## u \
{ \
    DTBLCOMBOBOX    dtblcombobox; \
    TCHAR           lpszCharsAllowed[n]; \
} u


/*  Drop Down   */
typedef struct _DTBLDDLBX
{
    ULONG ulFlags;
    ULONG ulPRDisplayProperty;
    ULONG ulPRSetProperty;
    ULONG ulPRTableName;
} DTBLDDLBX, *LPDTBLDDLBX;


/*  Check Box   */
typedef struct _DTBLCHECKBOX
{
    ULONG ulbLpszLabel;
    ULONG ulFlags;
    ULONG ulPRPropertyName;
} DTBLCHECKBOX, *LPDTBLCHECKBOX;
#define SizedDtblCheckBox(n,u) \
struct _DTBLCHECKBOX_ ## u \
{ \
    DTBLCHECKBOX    dtblcheckbox; \
    TCHAR       lpszLabel[n]; \
} u


/*  Group Box   */
typedef struct _DTBLGROUPBOX
{
    ULONG ulbLpszLabel;
    ULONG ulFlags;
} DTBLGROUPBOX, *LPDTBLGROUPBOX;
#define SizedDtblGroupBox(n,u) \
struct _DTBLGROUPBOX_ ## u \
{ \
    DTBLGROUPBOX    dtblgroupbox; \
    TCHAR           lpszLabel[n]; \
} u


/*  Button control   */
typedef struct _DTBLBUTTON
{
    ULONG ulbLpszLabel;
    ULONG ulFlags;
    ULONG ulPRControl;
} DTBLBUTTON, *LPDTBLBUTTON;
#define SizedDtblButton(n,u) \
struct _DTBLBUTTON_ ## u \
{ \
    DTBLBUTTON  dtblbutton; \
    TCHAR       lpszLabel[n]; \
} u


/*  Pages   */
typedef struct _DTBLPAGE
{
    ULONG ulbLpszLabel;
    ULONG ulFlags;
    ULONG ulbLpszComponent;
    ULONG ulContext;
} DTBLPAGE, *LPDTBLPAGE;
#define SizedDtblPage(n,n1,u) \
struct _DTBLPAGE_ ## u \
{ \
    DTBLPAGE    dtblpage; \
    TCHAR       lpszLabel[n]; \
    TCHAR       lpszComponent[n1]; \
} u


/*  Radio button   */
typedef struct _DTBLRADIOBUTTON
{
    ULONG ulbLpszLabel;
    ULONG ulFlags;
    ULONG ulcButtons;
    ULONG ulPropTag;
    long lReturnValue;
} DTBLRADIOBUTTON, *LPDTBLRADIOBUTTON;
#define SizedDtblRadioButton(n,u) \
struct _DTBLRADIOBUTTON_ ## u \
{ \
    DTBLRADIOBUTTON dtblradiobutton; \
    TCHAR           lpszLabel[n]; \
} u


/*  MultiValued listbox */
typedef struct _DTBLMVLISTBOX
{
    ULONG ulFlags;
    ULONG ulMVPropTag;
} DTBLMVLISTBOX, *LPDTBLMVLISTBOX;


/*  MultiValued dropdown */
typedef struct _DTBLMVDDLBX
{
    ULONG ulFlags;
    ULONG ulMVPropTag;
} DTBLMVDDLBX, *LPDTBLMVDDLBX;


#endif /* MAPIDEFS_H */
