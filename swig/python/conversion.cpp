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
#include <mapix.h>
#include <mapidefs.h>
#include <mapicode.h>
#include <mapiutil.h>
#include <edkmdb.h>
#include <Python.h>
#include <zarafa/charset/convert.h>
#include <zarafa/conversion.h>
#include "scl.h"

// mapi4linux does defines this in edkmdb.h
#ifndef CbNewROWLIST
	#define CbNewROWLIST(_centries) \
		(offsetof(ROWLIST,aEntries) + (_centries)*sizeof(ROWENTRY))
#endif

// From Structs.py
static PyObject *PyTypeSPropValue;
static PyObject *PyTypeSSort;
static PyObject *PyTypeSSortOrderSet;
static PyObject *PyTypeSPropProblem;
static PyObject *PyTypeMAPINAMEID;
static PyObject *PyTypeMAPIError;
static PyObject *PyTypeREADSTATE;
static PyObject *PyTypeSTATSTG;
static PyObject *PyTypeSYSTEMTIME;

static PyObject *PyTypeNEWMAIL_NOTIFICATION;
static PyObject *PyTypeOBJECT_NOTIFICATION;
static PyObject *PyTypeTABLE_NOTIFICATION;

static PyObject *PyTypeMVPROPMAP;
static PyObject *PyTypeECUser;
static PyObject *PyTypeECGroup;
static PyObject *PyTypeECCompany;
static PyObject *PyTypeECQuota;
static PyObject *PyTypeECUserClientUpdateStatus;
static PyObject *PyTypeECServer;
static PyObject *PyTypeECQuotaStatus;

static PyObject *PyTypeSAndRestriction;
static PyObject *PyTypeSOrRestriction;
static PyObject *PyTypeSNotRestriction;
static PyObject *PyTypeSContentRestriction;
static PyObject *PyTypeSBitMaskRestriction;
static PyObject *PyTypeSPropertyRestriction;
static PyObject *PyTypeSComparePropsRestriction;
static PyObject *PyTypeSSizeRestriction;
static PyObject *PyTypeSExistRestriction;
static PyObject *PyTypeSSubRestriction;
static PyObject *PyTypeSCommentRestriction;

static PyObject *PyTypeActMoveCopy;
static PyObject *PyTypeActReply;
static PyObject *PyTypeActDeferAction;
static PyObject *PyTypeActBounce;
static PyObject *PyTypeActFwdDelegate;
static PyObject *PyTypeActTag;
static PyObject *PyTypeAction;
static PyObject *PyTypeACTIONS;

// From Time.py
static PyObject *PyTypeFiletime;

// Work around "bad argument to internal function"
#if defined(_M_X64) || defined(__amd64__)
#define PyLong_AsUINT64 PyLong_AsUnsignedLong
#define PyLong_AsINT64 PyLong_AsLong
#else
#define PyLong_AsUINT64 PyLong_AsUnsignedLongLong
#define PyLong_AsINT64 PyLong_AsLongLong
#endif

// Get Py_ssize_t for older versions of python
#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
# define PY_SSIZE_T_MAX INT_MAX
# define PY_SSIZE_T_MIN INT_MIN
#endif

// Depending on native vs python unicode representation, set NATIVE_UNICODE
  #define WCHAR_T_SIZE __SIZEOF_WCHAR_T__

#if (Py_UNICODE_SIZE == WCHAR_T_SIZE)
  #define NATIVE_UNICODE 1
#else
  #define NATIVE_UNICODE 0
#endif


void Init()
{
	PyObject *lpMAPIStruct = PyImport_ImportModule("MAPI.Struct");
	PyObject *lpMAPITime = PyImport_ImportModule("MAPI.Time");

	if(!lpMAPIStruct) {
		PyErr_SetString(PyExc_RuntimeError, "Unable to import MAPI.Struct");
		goto exit;
	}

	if(!lpMAPITime) {
		PyErr_SetString(PyExc_RuntimeError, "Unable to import MAPI.Time");
		goto exit;
	}

	PyTypeSPropValue = PyObject_GetAttrString(lpMAPIStruct, "SPropValue");
	PyTypeSPropProblem = PyObject_GetAttrString(lpMAPIStruct, "SPropProblem");
	PyTypeSSort = PyObject_GetAttrString(lpMAPIStruct, "SSort");
	PyTypeSSortOrderSet = PyObject_GetAttrString(lpMAPIStruct, "SSortOrderSet");
	PyTypeMAPINAMEID = PyObject_GetAttrString(lpMAPIStruct, "MAPINAMEID");
	PyTypeMAPIError = PyObject_GetAttrString(lpMAPIStruct, "MAPIError");
	PyTypeREADSTATE = PyObject_GetAttrString(lpMAPIStruct, "READSTATE");
	PyTypeSTATSTG = PyObject_GetAttrString(lpMAPIStruct, "STATSTG");
	PyTypeSYSTEMTIME = PyObject_GetAttrString(lpMAPIStruct, "SYSTEMTIME");

        PyTypeMVPROPMAP = PyObject_GetAttrString(lpMAPIStruct, "MVPROPMAP");
	PyTypeECUser = PyObject_GetAttrString(lpMAPIStruct, "ECUSER");
	PyTypeECGroup = PyObject_GetAttrString(lpMAPIStruct, "ECGROUP");
	PyTypeECCompany = PyObject_GetAttrString(lpMAPIStruct, "ECCOMPANY");
	PyTypeECQuota = PyObject_GetAttrString(lpMAPIStruct, "ECQUOTA");
	PyTypeECUserClientUpdateStatus = PyObject_GetAttrString(lpMAPIStruct, "ECUSERCLIENTUPDATESTATUS");
	PyTypeECServer = PyObject_GetAttrString(lpMAPIStruct, "ECSERVER");
	PyTypeECQuotaStatus = PyObject_GetAttrString(lpMAPIStruct, "ECQUOTASTATUS");

	PyTypeNEWMAIL_NOTIFICATION = PyObject_GetAttrString(lpMAPIStruct, "NEWMAIL_NOTIFICATION");
	PyTypeOBJECT_NOTIFICATION = PyObject_GetAttrString(lpMAPIStruct, "OBJECT_NOTIFICATION");
	PyTypeTABLE_NOTIFICATION = PyObject_GetAttrString(lpMAPIStruct, "TABLE_NOTIFICATION");

	PyTypeSAndRestriction = PyObject_GetAttrString(lpMAPIStruct, "SAndRestriction");
	PyTypeSOrRestriction = PyObject_GetAttrString(lpMAPIStruct, "SOrRestriction");
	PyTypeSNotRestriction = PyObject_GetAttrString(lpMAPIStruct, "SNotRestriction");
	PyTypeSContentRestriction = PyObject_GetAttrString(lpMAPIStruct, "SContentRestriction");
	PyTypeSBitMaskRestriction = PyObject_GetAttrString(lpMAPIStruct, "SBitMaskRestriction");
	PyTypeSPropertyRestriction = PyObject_GetAttrString(lpMAPIStruct, "SPropertyRestriction");
	PyTypeSComparePropsRestriction = PyObject_GetAttrString(lpMAPIStruct, "SComparePropsRestriction");
	PyTypeSSizeRestriction = PyObject_GetAttrString(lpMAPIStruct, "SSizeRestriction");
	PyTypeSExistRestriction = PyObject_GetAttrString(lpMAPIStruct, "SExistRestriction");
	PyTypeSSubRestriction = PyObject_GetAttrString(lpMAPIStruct, "SSubRestriction");
	PyTypeSCommentRestriction = PyObject_GetAttrString(lpMAPIStruct, "SCommentRestriction");

	PyTypeActMoveCopy = PyObject_GetAttrString(lpMAPIStruct, "actMoveCopy");
	PyTypeActReply = PyObject_GetAttrString(lpMAPIStruct, "actReply");
	PyTypeActDeferAction = PyObject_GetAttrString(lpMAPIStruct, "actDeferAction");
	PyTypeActBounce = PyObject_GetAttrString(lpMAPIStruct, "actBounce");
	PyTypeActFwdDelegate = PyObject_GetAttrString(lpMAPIStruct, "actFwdDelegate");
	PyTypeActTag = PyObject_GetAttrString(lpMAPIStruct, "actTag");
	PyTypeAction = PyObject_GetAttrString(lpMAPIStruct, "ACTION");
	PyTypeACTIONS = PyObject_GetAttrString(lpMAPIStruct, "ACTIONS");

	PyTypeFiletime = PyObject_GetAttrString(lpMAPITime, "FileTime");

exit:
	;
}

// Coerce PyObject into PyUnicodeObject, copy and zero-terminate
wchar_t * CopyPyUnicode(wchar_t **lpWide, PyObject *o, void *lpBase)
{
    PyObject *unicode = NULL;
    int size;
    
    unicode = PyUnicode_FromObject(o);
    if(!unicode) {
        *lpWide = NULL;
        return NULL;
    }
        
    size = PyUnicode_GetSize(unicode);
    
    if (MAPIAllocateMore((size + 1) * sizeof(wchar_t), lpBase, (void **)lpWide) == hrSuccess) {
	    #if PY_MAJOR_VERSION >= 3
		PyUnicode_AsWideChar(unicode, *lpWide, size);
	    #else
		PyUnicode_AsWideChar((PyUnicodeObject *)unicode, *lpWide, size);
	    #endif
	    
	    (*lpWide)[size] = '\0';
	 
	    Py_DECREF(unicode);
	    
	    return *lpWide;
    }

    Py_DECREF(unicode);

    return NULL;
}

int Object_is_list_of(PyObject *object, TypeCheckFunc fnTypeCheck)
{
	int rc = 1;
	PyObject *iter = NULL;
	PyObject *elem = NULL;

	if (object == Py_None)
		return 0;

	iter = PyObject_GetIter(object);
	if (!iter)
		return 0;

	while (rc == 1 && (elem = PyIter_Next(iter))) {
		rc = fnTypeCheck(elem);
		Py_DECREF(elem);
		elem = NULL;
	}

	Py_DECREF(iter);
	return rc;
}

FILETIME Object_to_FILETIME(PyObject *object)
{
	FILETIME ft = {0, 0};
	unsigned long long periods = 0;

	PyObject *filetime = PyObject_GetAttrString(object, "filetime");
	if (!filetime) {
		PyErr_Format(PyExc_TypeError, "PT_SYSTIME object does not have 'filetime' attribute");
		goto exit;
	}

	#if PY_MAJOR_VERSION >= 3
		periods = PyLong_AsUnsignedLongLongMask(filetime);
	#else
		periods = PyInt_AsUnsignedLongLongMask(filetime);
	#endif
	ft.dwHighDateTime = periods >> 32;
	ft.dwLowDateTime = periods & 0xffffffff;

exit:
	return ft;
}

PyObject *Object_from_FILETIME(FILETIME ft)
{
	PyObject *object = NULL;
	PyObject *filetime = NULL;

	filetime = PyLong_FromUnsignedLongLong(((unsigned long long)ft.dwHighDateTime << 32) + ft.dwLowDateTime);
	if (PyErr_Occurred()) {
		goto exit;
	}

	object = PyObject_CallFunction(PyTypeFiletime, "(O)", filetime);

exit:
	if (filetime != NULL)
		Py_DECREF(filetime);

	return object;
}

int Object_is_FILETIME(PyObject *object)
{
	return PyObject_IsInstance(object, PyTypeFiletime);
}

PyObject *Object_from_LPSPropValue(LPSPropValue lpProp)
{
	PyObject *Value = NULL;
	PyObject *ulPropTag = NULL;
	PyObject *object = NULL;

	ulPropTag = PyLong_FromUnsignedLong(lpProp->ulPropTag);
	switch(PROP_TYPE(lpProp->ulPropTag)) {

		case PT_STRING8:
			Value = PyString_FromString(lpProp->Value.lpszA);
			break;
		case PT_UNICODE:
			Value = PyUnicode_FromWideChar(lpProp->Value.lpszW, wcslen(lpProp->Value.lpszW));
			break;
		case PT_BINARY:
			Value = PyString_FromStringAndSize((const char*)lpProp->Value.bin.lpb, lpProp->Value.bin.cb);
			break;
		case PT_SHORT:
			Value = PyLong_FromLong(lpProp->Value.i);
			break;
		case PT_ERROR:
			Value = PyLong_FromUnsignedLong((unsigned int)lpProp->Value.err);
			break;
		case PT_LONG:
			Value = PyLong_FromLongLong(lpProp->Value.l); // We have to use LongLong since it could be either PT_LONG or PT_ULONG. 'Long' doesn't cover the range
			break;
		case PT_FLOAT:
			Value = PyFloat_FromDouble(lpProp->Value.flt);
			break;
		case PT_APPTIME:
		case PT_DOUBLE:
			Value = PyFloat_FromDouble(lpProp->Value.dbl);
			break;
		case PT_LONGLONG:
		case PT_CURRENCY:
			Value = PyLong_FromLongLong(lpProp->Value.cur.int64);
			break;
		case PT_BOOLEAN:
			Value = PyBool_FromLong(lpProp->Value.b);
			break;
		case PT_SYSTIME: {
			Value = Object_from_FILETIME(lpProp->Value.ft);
			break;
		}
		case PT_CLSID:
			Value = PyString_FromStringAndSize((const char*)lpProp->Value.lpguid, sizeof(GUID));
			break;
		case PT_OBJECT:
			Py_INCREF(Py_None);
			Value = Py_None;
			break;

		case PT_SRESTRICTION:
			Value = Object_from_LPSRestriction((LPSRestriction)lpProp->Value.lpszA);
			break;

		case PT_ACTIONS:
			Value = Object_from_LPACTIONS((ACTIONS*)lpProp->Value.lpszA);
			break;

#define BASE(x) x
#define INT64(x) x.int64
#define QUADPART(x) x.QuadPart
#define PT_MV_CASE(MVname,MVelem,From,Sub) \
			Value = PyList_New(0); \
			for (unsigned int i = 0; i < lpProp->Value.MV##MVname.cValues; ++i) { \
				PyObject *elem = From(Sub(lpProp->Value.MV##MVname.lp##MVelem[i])); \
				PyList_Append(Value, elem); \
				Py_DECREF(elem); \
			} \
			break;

		case PT_MV_SHORT:
			PT_MV_CASE(i,i,PyLong_FromLong,BASE)
		case PT_MV_LONG:
			PT_MV_CASE(l,l,PyLong_FromLong,BASE)
		case PT_MV_FLOAT:
			PT_MV_CASE(flt,flt,PyFloat_FromDouble,BASE)
		case PT_MV_DOUBLE:
			PT_MV_CASE(dbl,dbl,PyFloat_FromDouble,BASE)
		case PT_MV_CURRENCY:
			PT_MV_CASE(cur,cur,PyLong_FromLongLong,INT64)
		case PT_MV_APPTIME:
			PT_MV_CASE(at,at,PyFloat_FromDouble,BASE)
		case PT_MV_LONGLONG:
			PT_MV_CASE(li,li,PyLong_FromLongLong,QUADPART)
		case PT_MV_SYSTIME:
			Value = PyList_New(0);
			for (unsigned int i = 0; i < lpProp->Value.MVft.cValues; ++i) {
				PyObject *elem = Object_from_FILETIME(lpProp->Value.MVft.lpft[i]);
				PyList_Append(Value, elem);
				Py_DECREF(elem);
			}
			break;
		case PT_MV_STRING8:
			PT_MV_CASE(szA,pszA,PyString_FromString,BASE)
		case PT_MV_BINARY:
			Value = PyList_New(0);
			for (unsigned int i = 0; i < lpProp->Value.MVbin.cValues; ++i) {
				PyObject *elem = PyString_FromStringAndSize((const char *)lpProp->Value.MVbin.lpbin[i].lpb, lpProp->Value.MVbin.lpbin[i].cb);
				PyList_Append(Value, elem);
				Py_DECREF(elem);
			}
			break;
		case PT_MV_UNICODE:
			Value = PyList_New(0);
			for (unsigned int i = 0; i < lpProp->Value.MVszW.cValues; ++i) {
				int len = wcslen(lpProp->Value.MVszW.lppszW[i]);
				PyObject *elem = PyUnicode_FromWideChar(lpProp->Value.MVszW.lppszW[i], len);
				PyList_Append(Value, elem);
				Py_DECREF(elem);
			}
			break;
		case PT_MV_CLSID:
			Value = PyList_New(0);
			for (unsigned int i = 0; i < lpProp->Value.MVguid.cValues; ++i) {
				PyObject *elem = PyString_FromStringAndSize((const char *)&lpProp->Value.MVguid.lpguid[i], sizeof(GUID));
				PyList_Append(Value, elem);
				Py_DECREF(elem);
			}
			break;
		case PT_NULL:
			Py_INCREF(Py_None);
			Value = Py_None;
			break;

		default:
			PyErr_Format(PyExc_RuntimeError, "Bad property type %x", PROP_TYPE(lpProp->ulPropTag));
			break;
	}

	if(PyErr_Occurred()) {
		goto exit;
	}

	object = PyObject_CallFunction(PyTypeSPropValue, "(OO)", ulPropTag, Value);

exit:
	if(Value) {
		Py_DECREF(Value);
	}

	if(ulPropTag) {
		Py_DECREF(ulPropTag);
	}

	return object;
}

int Object_is_LPSPropValue(PyObject *object)
{
	return PyObject_IsInstance(object, PyTypeSPropValue);
}

PyObject *		List_from_LPSPropValue(LPSPropValue lpProps, ULONG cValues)
{
	PyObject *list = PyList_New(0);
	PyObject *item = NULL;

	for (unsigned int i = 0; i < cValues; ++i) {
		item = Object_from_LPSPropValue(&lpProps[i]);
		if(PyErr_Occurred())
			goto exit;

		PyList_Append(list, item);

		Py_DECREF(item);
		item = NULL;
	}

exit:
	if(PyErr_Occurred()) {
		if(list) {
			Py_DECREF(list);
		}
		list = NULL;
	}

	if(item) {
		Py_DECREF(item);
	}
	return list;
}

PyObject *	  List_from_wchar_t(wchar_t **lpStrings, ULONG cElements)
{
	PyObject *list = PyList_New(0);
	PyObject *item = NULL;

	for (unsigned int i = 0; i < cElements; ++i) {
		item = PyUnicode_FromWideChar(lpStrings[i], wcslen(lpStrings[i]));
		if(PyErr_Occurred())
			goto exit;

		PyList_Append(list, item);

		Py_DECREF(item);
		item = NULL;
	}

exit:
	if(PyErr_Occurred()) {
		if(list) {
			Py_DECREF(list);
		}
		list = NULL;
	}

	if(item) {
		Py_DECREF(item);
	}
	return list;
}


void Object_to_LPSPropValue(PyObject *object, LPSPropValue lpProp, ULONG ulFlags, void *lpBase)
{
	PyObject *ulPropTag = NULL;
	PyObject *Value = NULL;
	char *lpstr = NULL;
	Py_ssize_t size = 0;

	ulPropTag = PyObject_GetAttrString(object, "ulPropTag");
	Value = PyObject_GetAttrString(object, "Value");

	if(!ulPropTag || !Value) {
		PyErr_SetString(PyExc_RuntimeError, "ulPropTag or Value missing from SPropValue");
		goto exit;
	}

	lpProp->dwAlignPad = 0;
	lpProp->ulPropTag = (ULONG)PyLong_AsUnsignedLong(ulPropTag);
	switch(PROP_TYPE(lpProp->ulPropTag)) {
		case PT_NULL:
			lpProp->Value.x = 0;
			break;
		case PT_STRING8:
			if (ulFlags == CONV_COPY_SHALLOW)
				lpProp->Value.lpszA = PyString_AsString(Value);
			else {
				PyString_AsStringAndSize(Value, &lpstr, &size);
				if (MAPIAllocateMore(size + 1, lpBase, (LPVOID*)&lpProp->Value.lpszA) != hrSuccess)
					goto exit;
				memcpy(lpProp->Value.lpszA, lpstr, size + 1);
			}
			break;
		case PT_UNICODE:
			// @todo add PyUnicode_Check call?
			if (ulFlags == CONV_COPY_SHALLOW && NATIVE_UNICODE)
				lpProp->Value.lpszW = (WCHAR*)PyUnicode_AsUnicode(Value);
			else {
			    CopyPyUnicode(&lpProp->Value.lpszW, Value, lpBase);
			}
			break;
		case PT_ERROR:
			lpProp->Value.ul = (ULONG)PyLong_AsUnsignedLong(Value);
			break;
		case PT_SHORT:
			lpProp->Value.i = (short int)PyLong_AsLong(Value);
			break;
		case PT_LONG:
			lpProp->Value.ul = (ULONG)PyLong_AsLongLong(Value); // We have to use LongLong since it could be either PT_LONG or PT_ULONG. 'Long' doesn't cover the range
			break;
		case PT_FLOAT:
			lpProp->Value.flt = (float)PyFloat_AsDouble(Value);
			break;
		case PT_APPTIME:
		case PT_DOUBLE:
			lpProp->Value.dbl = PyFloat_AsDouble(Value);
			break;
		case PT_LONGLONG:
		case PT_CURRENCY:
			lpProp->Value.cur.int64 = PyLong_AsINT64(Value);
			break;
		case PT_BOOLEAN:
			lpProp->Value.b = (Value == Py_True);
			break;
		case PT_OBJECT:
			lpProp->Value.lpszA = NULL;
			break;
		case PT_SYSTIME: {
			lpProp->Value.ft = Object_to_FILETIME(Value);
			break;
		}
		case PT_CLSID:
			PyString_AsStringAndSize(Value, &lpstr, &size);
			if(size == sizeof(GUID)) {
				if (ulFlags == CONV_COPY_SHALLOW)
					lpProp->Value.lpguid = (LPGUID)lpstr;
				else {
					if (MAPIAllocateMore(sizeof(GUID), lpBase, (LPVOID*)&lpProp->Value.lpguid) != hrSuccess)
						goto exit;
					memcpy(lpProp->Value.lpguid, lpstr, sizeof(GUID));
				}
			}
			else
				PyErr_Format(PyExc_TypeError, "PT_CLSID Value must be exactly %d bytes", (int)sizeof(GUID));
			break;
		case PT_BINARY: {
			PyString_AsStringAndSize(Value, &lpstr, &size);
			if (ulFlags == CONV_COPY_SHALLOW)
				lpProp->Value.bin.lpb = (LPBYTE)lpstr;
			else {
				if (MAPIAllocateMore(size, lpBase, (LPVOID*)&lpProp->Value.bin.lpb) != hrSuccess)
					goto exit;
				memcpy(lpProp->Value.bin.lpb, lpstr, size);
			}
			lpProp->Value.bin.cb = size;
			break;
		}

		case PT_SRESTRICTION: {
			if (MAPIAllocateMore(sizeof(SRestriction), lpBase, (void**)&lpProp->Value.lpszA) != hrSuccess)
				goto exit;
			Object_to_LPSRestriction(Value, (LPSRestriction)lpProp->Value.lpszA, lpBase);
			break;
		}
		case PT_ACTIONS: {
			if (MAPIAllocateMore(sizeof(ACTIONS), lpBase, (void**)&lpProp->Value.lpszA) != hrSuccess)
				goto exit;
			Object_to_LPACTIONS(Value, (ACTIONS*)lpProp->Value.lpszA, lpBase);
			break;
		}

#undef PT_MV_CASE
#define PT_MV_CASE(MVname,MVelem,As,Sub) \
		{ \
			Py_ssize_t len = PyObject_Size(Value); \
			PyObject *iter = PyObject_GetIter(Value); \
			PyObject *elem = NULL; \
			int n = 0; \
			\
			if (len) { \
				if (MAPIAllocateMore(sizeof(*lpProp->Value.MV##MVname.lp##MVelem) * len, lpBase, (void **)&lpProp->Value.MV##MVname.lp##MVelem) != hrSuccess) \
					goto exit; \
				while((elem = PyIter_Next(iter))) {						\
					Sub(lpProp->Value.MV##MVname.lp##MVelem[n]) = As(elem); \
					Py_DECREF(elem);									\
					++n;												\
				}														\
			}															\
			lpProp->Value.MV##MVname.cValues = n; \
			Py_DECREF(iter); \
			break; \
		}

		case PT_MV_SHORT:
			PT_MV_CASE(i,i,PyLong_AsLong,BASE)
		case PT_MV_LONG:
			PT_MV_CASE(l,l,PyLong_AsLong,BASE)
		case PT_MV_FLOAT:
			PT_MV_CASE(flt,flt,PyFloat_AsDouble,BASE)
		case PT_MV_DOUBLE:
			PT_MV_CASE(dbl,dbl,PyFloat_AsDouble,BASE)
		case PT_MV_CURRENCY:
			PT_MV_CASE(cur,cur,PyLong_AsINT64,INT64)
		case PT_MV_APPTIME:
			PT_MV_CASE(at,at,PyFloat_AsDouble,BASE)
		case PT_MV_LONGLONG:
			PT_MV_CASE(li,li,PyLong_AsINT64,QUADPART)
		case PT_MV_SYSTIME:
		{
			Py_ssize_t len = PyObject_Size(Value);
			PyObject *iter = PyObject_GetIter(Value);
			PyObject *elem = NULL;
			int n = 0;

			if (MAPIAllocateMore(sizeof(SDateTimeArray) * len, lpBase, (void **)&lpProp->Value.MVft.lpft) != hrSuccess)
				goto exit;
			while((elem = PyIter_Next(iter))) {
				lpProp->Value.MVft.lpft[n] = Object_to_FILETIME(elem);
				Py_DECREF(elem);
				++n;
			}

			lpProp->Value.MVft.cValues = n;
			Py_DECREF(iter);
			break;
		}

		case PT_MV_STRING8:
		{
			Py_ssize_t len = PyObject_Size(Value);
			PyObject *iter = PyObject_GetIter(Value);
			PyObject *elem = NULL;
			int n = 0;

			if (MAPIAllocateMore(sizeof(*lpProp->Value.MVszA.lppszA) * len, lpBase, (LPVOID*)&lpProp->Value.MVszA.lppszA) != hrSuccess)
				goto exit;
			while((elem = PyIter_Next(iter))) {
				if (ulFlags == CONV_COPY_SHALLOW)
					lpProp->Value.MVszA.lppszA[n] = PyString_AsString(elem);
				else {
					PyString_AsStringAndSize(elem, &lpstr, &size);
					if (MAPIAllocateMore(size+1, lpBase, (LPVOID*)&lpProp->Value.MVszA.lppszA[n]) != hrSuccess)
						goto exit;
					memcpy(lpProp->Value.MVszA.lppszA[n], lpstr, size+1);
				}

				Py_DECREF(elem);
				++n;
			}

			lpProp->Value.MVszA.cValues = n;
			Py_DECREF(iter);
			break;
		}

		case PT_MV_BINARY:
		{
			Py_ssize_t len = PyObject_Size(Value);
			PyObject *iter = PyObject_GetIter(Value);
			PyObject *elem = NULL;
			int n = 0;

			if (MAPIAllocateMore(sizeof(SBinaryArray) * len, lpBase, (void **)&lpProp->Value.MVbin.lpbin) != hrSuccess)
				goto exit;
			while((elem = PyIter_Next(iter))) {
				PyString_AsStringAndSize(elem, &lpstr, &size);
				if (ulFlags == CONV_COPY_SHALLOW)
					lpProp->Value.MVbin.lpbin[n].lpb = (LPBYTE)lpstr;
				else {
					if (MAPIAllocateMore(size, lpBase, (LPVOID*)&lpProp->Value.MVbin.lpbin[n].lpb) != hrSuccess)
						goto exit;
					memcpy(lpProp->Value.MVbin.lpbin[n].lpb, lpstr, size);
				}
				lpProp->Value.MVbin.lpbin[n].cb = size;

				Py_DECREF(elem);
				++n;
			}

			lpProp->Value.MVbin.cValues = n;
			Py_DECREF(iter);
			break;
		}

		case PT_MV_UNICODE:
		{
			Py_ssize_t len = PyObject_Size(Value);
			PyObject *iter = PyObject_GetIter(Value);
			PyObject *elem = NULL;
			int n = 0;

			if (MAPIAllocateMore(sizeof(*lpProp->Value.MVszW.lppszW) * len, lpBase, (LPVOID*)&lpProp->Value.MVszW.lppszW) != hrSuccess)
				goto exit;
			while((elem = PyIter_Next(iter))) {
				if (ulFlags == CONV_COPY_SHALLOW && NATIVE_UNICODE)
					lpProp->Value.MVszW.lppszW[n] = (WCHAR*)PyUnicode_AsUnicode(elem);
				else {
				    CopyPyUnicode(&lpProp->Value.MVszW.lppszW[n], Value, lpBase);
				}


				Py_DECREF(elem);
				++n;
			}

			lpProp->Value.MVszW.cValues = n;
			Py_DECREF(iter);
			break;
		}

		case PT_MV_CLSID:
		{
			Py_ssize_t len = PyObject_Size(Value);
			PyObject *iter = PyObject_GetIter(Value);
			PyObject *elem = NULL;
			int n = 0;
			char *guid;

			if (MAPIAllocateMore(sizeof(GUID) * len, lpBase, (void **)&lpProp->Value.MVguid.lpguid) != hrSuccess)
				goto exit;
			while((elem = PyIter_Next(iter))) {
				PyString_AsStringAndSize(elem, &guid, &size);
				if(size != sizeof(GUID)) {
					PyErr_Format(PyExc_TypeError, "PT_CLSID Value must be exactly %d bytes", (int)sizeof(GUID));
					break;
				}
				memcpy(&lpProp->Value.MVguid.lpguid[n], guid, size);

				Py_DECREF(elem);
				++n;
			}

			lpProp->Value.MVguid.cValues = n;
			Py_DECREF(iter);
			break;
		}

		default:
			PyErr_Format(PyExc_TypeError, "ulPropTag has unknown type %x", PROP_TYPE(lpProp->ulPropTag));
			break;
	}

exit:
	if (ulPropTag)
		Py_DECREF(ulPropTag);
	if (Value)
		Py_DECREF(Value);
	;
}

LPSPropValue Object_to_LPSPropValue(PyObject *object, ULONG ulFlags, void *lpBase)
{
	LPSPropValue lpProp = NULL;

	if(lpBase) {
		if (MAPIAllocateMore(sizeof(SPropValue), lpBase, (void **)&lpProp) != hrSuccess)
			return NULL;
	}
	else {
		if (MAPIAllocateBuffer(sizeof(SPropValue), (void **)&lpProp) != hrSuccess)
			return NULL;

		lpBase = lpProp;
	}

	Object_to_LPSPropValue(object, lpProp, ulFlags, lpBase);

	if(PyErr_Occurred()) {
		if(!lpBase)
			MAPIFreeBuffer(lpProp);
		return NULL;
	} else {
		return lpProp;
	}
}


LPSPropValue	List_to_LPSPropValue(PyObject *object, ULONG *cValues, ULONG ulFlags, void *lpBase)
{
	Py_ssize_t size = 0;
	LPSPropValue lpProps = NULL;
	LPSPropValue lpResult = NULL;
	PyObject *iter = NULL;
	PyObject *elem = NULL;
	int i = 0;

	if(object == Py_None) {
		*cValues = 0;
		return NULL;
	}

	iter = PyObject_GetIter(object);
	if(!iter)
		goto exit;

	size = PyObject_Size(object);

	if (MAPIAllocateBuffer(sizeof(SPropValue)*size, (void**)&lpProps) != hrSuccess)
		goto exit;

	memset(lpProps, 0, sizeof(SPropValue)*size);

	while((elem = PyIter_Next(iter))) {
		Object_to_LPSPropValue(elem, &lpProps[i], ulFlags, lpProps);
		if(PyErr_Occurred())
			goto exit;

		Py_DECREF(elem);
		elem = NULL;
		++i;
	}

	lpResult = lpProps;
	*cValues = size;

exit:
	if (PyErr_Occurred() && lpBase != NULL)
		MAPIFreeBuffer(lpProps);
	if(elem) {
		Py_DECREF(elem);
	}

	if(iter) {
		Py_DECREF(iter);
	}

	return lpResult;
}

PyObject *		List_from_LPTSTRPtr(LPTSTR *lpStrings, ULONG cValues)
{
	PyErr_SetString(PyExc_RuntimeError, "LPSSTRPtr unsupported");

	return NULL;
}

LPSPropTagArray	List_to_LPSPropTagArray(PyObject *object, ULONG /*ulFlags*/)
{
	PyObject *elem = NULL;
	PyObject *iter = NULL;
	Py_ssize_t len = 0;
	LPSPropTagArray lpPropTagArray = NULL;
	int n = 0;

	if(object == Py_None)
		return NULL;

	len = PyObject_Length(object);
	if(len < 0) {
		PyErr_Format(PyExc_TypeError, "Invalid list passed as property list");
		goto exit;
	}

	if (MAPIAllocateBuffer(CbNewSPropTagArray(len), (void **)&lpPropTagArray) != hrSuccess)
		goto exit;

	iter = PyObject_GetIter(object);
	if(iter == NULL)
		goto exit;

	while((elem = PyIter_Next(iter))) {
		lpPropTagArray->aulPropTag[n] = (ULONG)PyLong_AsUnsignedLong(elem);
		Py_DECREF(elem);
		++n;
	}

	lpPropTagArray->cValues = n;

exit:
	if(PyErr_Occurred()) {
		MAPIFreeBuffer(lpPropTagArray);
		lpPropTagArray = NULL;
	}
	if(iter) {
		Py_DECREF(iter);
	}

	return lpPropTagArray;
}

PyObject *		List_from_LPSPropTagArray(LPSPropTagArray lpPropTagArray)
{
	PyObject *list = NULL;
	PyObject *elem = NULL;

	if(lpPropTagArray == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	list = PyList_New(0);

	for (unsigned int i = 0; i < lpPropTagArray->cValues; ++i) {
		elem = PyLong_FromUnsignedLong(lpPropTagArray->aulPropTag[i]);
		PyList_Append(list, elem);
		if(PyErr_Occurred())
			goto exit;

		Py_DECREF(elem);
		elem = NULL;
	}

exit:
	if(elem) {
		Py_DECREF(elem);
	}

	if(PyErr_Occurred()) {
		Py_DECREF(list);
		list = NULL;
	}
	return list;
}

void Object_to_LPSRestriction(PyObject *object, LPSRestriction lpsRestriction, void *lpBase)
{
	PyObject *rt = NULL;
	PyObject *iter = NULL;
	PyObject *sub = NULL;
	PyObject *elem = NULL;
	PyObject *ulPropTag = NULL, *ulPropTag2 = NULL, *ulMask = NULL, *cb = NULL, *ulFuzzyLevel = NULL, *relop = NULL, *lpProp = NULL;
	Py_ssize_t len;
	int n = 0;

	if(lpBase == NULL)
		lpBase = lpsRestriction;

	rt = PyObject_GetAttrString(object, "rt");
	if(!rt) {
		PyErr_SetString(PyExc_RuntimeError, "rt (type) missing for restriction");
		goto exit;
	}

	lpsRestriction->rt = (ULONG)PyLong_AsUnsignedLong(rt);

	switch(lpsRestriction->rt) {
	case RES_AND:
	case RES_OR:
		sub = PyObject_GetAttrString(object, "lpRes");
		if(!sub) {
			PyErr_SetString(PyExc_RuntimeError, "lpRes missing for restriction");
			goto exit;
		}
		len = PyObject_Length(sub);

		// Handle RES_AND and RES_OR the same since they are binary-compatible
		if (MAPIAllocateMore(sizeof(SRestriction) * len, lpBase, (void **)&lpsRestriction->res.resAnd.lpRes) != hrSuccess) {
			PyErr_SetString(PyExc_RuntimeError, "Out of memory");
			goto exit;
		}

		iter = PyObject_GetIter(sub);
		if(iter == NULL)
			goto exit;

		while((elem = PyIter_Next(iter))) {
			Object_to_LPSRestriction(elem, &lpsRestriction->res.resAnd.lpRes[n], lpBase);

			if(PyErr_Occurred())
				goto exit;
			++n;
			Py_DECREF(elem);
			elem = NULL;
		}

		lpsRestriction->res.resAnd.cRes = n;
		break;

	case RES_NOT:
		sub = PyObject_GetAttrString(object, "lpRes");
		if(!sub) {
			PyErr_SetString(PyExc_RuntimeError, "lpRes missing for restriction");
			goto exit;
		}

		if (MAPIAllocateMore(sizeof(SRestriction), lpBase, (void **)&lpsRestriction->res.resNot.lpRes) != hrSuccess) {
			PyErr_SetString(PyExc_RuntimeError, "Out of memory");
			goto exit;
		}

		Object_to_LPSRestriction(sub, lpsRestriction->res.resNot.lpRes, lpBase);

		if(PyErr_Occurred())
			goto exit;
		break;

	case RES_CONTENT:
		ulFuzzyLevel = PyObject_GetAttrString(object, "ulFuzzyLevel");
		ulPropTag = PyObject_GetAttrString(object, "ulPropTag");
		sub = PyObject_GetAttrString(object, "lpProp");

		if(!ulFuzzyLevel || ! ulPropTag || !sub) {
			PyErr_SetString(PyExc_RuntimeError, "ulFuzzyLevel, ulPropTag or lpProp missing for RES_CONTENT restriction");
			goto exit;
		}

		lpsRestriction->res.resContent.ulFuzzyLevel = PyLong_AsUnsignedLong(ulFuzzyLevel);
		lpsRestriction->res.resContent.ulPropTag = PyLong_AsUnsignedLong(ulPropTag);
		lpsRestriction->res.resContent.lpProp = Object_to_LPSPropValue(sub, CONV_COPY_SHALLOW, lpBase);
		break;

	case RES_PROPERTY:
		relop = PyObject_GetAttrString(object, "relop");
		ulPropTag = PyObject_GetAttrString(object, "ulPropTag");
		sub = PyObject_GetAttrString(object, "lpProp");

		if(!relop || !ulPropTag || !sub) {
			PyErr_SetString(PyExc_RuntimeError, "relop, ulPropTag or lpProp missing for RES_PROPERTY restriction");
			goto exit;
		}

		lpsRestriction->res.resProperty.relop = PyLong_AsUnsignedLong(relop);
		lpsRestriction->res.resProperty.ulPropTag = PyLong_AsUnsignedLong(ulPropTag);
		lpsRestriction->res.resProperty.lpProp = Object_to_LPSPropValue(sub, CONV_COPY_SHALLOW, lpBase);
		break;

	case RES_COMPAREPROPS:
		relop = PyObject_GetAttrString(object, "relop");
		ulPropTag = PyObject_GetAttrString(object, "ulPropTag1");
		ulPropTag2 = PyObject_GetAttrString(object, "ulPropTag2");

		if(!relop || !ulPropTag || !ulPropTag2) {
			PyErr_SetString(PyExc_RuntimeError, "relop, ulPropTag1 or ulPropTag2 missing for RES_COMPAREPROPS restriction");
			goto exit;
		}

		lpsRestriction->res.resCompareProps.relop = PyLong_AsUnsignedLong(relop);
		lpsRestriction->res.resCompareProps.ulPropTag1 = PyLong_AsUnsignedLong(ulPropTag);
		lpsRestriction->res.resCompareProps.ulPropTag2 = PyLong_AsUnsignedLong(ulPropTag2);
		break;

	case RES_BITMASK:
		relop = PyObject_GetAttrString(object, "relBMR");
		ulPropTag = PyObject_GetAttrString(object, "ulPropTag");
		ulMask = PyObject_GetAttrString(object, "ulMask");

		if(!relop || !ulPropTag || !ulMask) {
			PyErr_SetString(PyExc_RuntimeError, "relBMR, ulPropTag or ulMask missing for RES_BITMASK restriction");
			goto exit;
		}

		lpsRestriction->res.resBitMask.relBMR = PyLong_AsUnsignedLong(relop);
		lpsRestriction->res.resBitMask.ulPropTag = PyLong_AsUnsignedLong(ulPropTag);
		lpsRestriction->res.resBitMask.ulMask = PyLong_AsUnsignedLong(ulMask);
		break;

	case RES_SIZE:
		relop = PyObject_GetAttrString(object, "relop");
		ulPropTag = PyObject_GetAttrString(object, "ulPropTag");
		cb = PyObject_GetAttrString(object, "cb");

		if(!relop || !ulPropTag || !cb) {
			PyErr_SetString(PyExc_RuntimeError, "relop, ulPropTag or cb missing from RES_SIZE restriction");
			goto exit;
		}

		lpsRestriction->res.resSize.relop = PyLong_AsUnsignedLong(relop);
		lpsRestriction->res.resSize.ulPropTag = PyLong_AsUnsignedLong(ulPropTag);
		lpsRestriction->res.resSize.cb = PyLong_AsUnsignedLong(cb);
		break;

	case RES_EXIST:
		ulPropTag = PyObject_GetAttrString(object, "ulPropTag");

		if(!ulPropTag) {
			PyErr_SetString(PyExc_RuntimeError, "ulPropTag missing from RES_EXIST restriction");
			goto exit;
		}

		lpsRestriction->res.resExist.ulPropTag = PyLong_AsUnsignedLong(ulPropTag);
		break;

	case RES_SUBRESTRICTION:
		ulPropTag = PyObject_GetAttrString(object, "ulSubObject");
		sub = PyObject_GetAttrString(object, "lpRes");

		if(!ulPropTag || !sub) {
			PyErr_SetString(PyExc_RuntimeError, "ulSubObject or lpRes missing from RES_SUBRESTRICTION restriction");
			goto exit;
		}

		lpsRestriction->res.resSub.ulSubObject = PyLong_AsUnsignedLong(ulPropTag);
		if (MAPIAllocateMore(sizeof(SRestriction), lpBase, (void **)&lpsRestriction->res.resSub.lpRes) != hrSuccess) {
			PyErr_SetString(PyExc_RuntimeError, "Out of memory");
			goto exit;
		}
		Object_to_LPSRestriction(sub, lpsRestriction->res.resSub.lpRes, lpBase);

		if(PyErr_Occurred())
			goto exit;
		break;

	case RES_COMMENT:
		lpProp = PyObject_GetAttrString(object, "lpProp");
		sub = PyObject_GetAttrString(object, "lpRes");

		if(!lpProp || !sub) {
			PyErr_SetString(PyExc_RuntimeError, "lpProp or sub missing from RES_COMMENT restriction");
			goto exit;
		}

		if (MAPIAllocateMore(sizeof(SRestriction), lpBase, (void **)&lpsRestriction->res.resComment.lpRes) != hrSuccess) {
			PyErr_SetString(PyExc_RuntimeError, "Out of memory");
			goto exit;
		}
		
		Object_to_LPSRestriction(sub, lpsRestriction->res.resComment.lpRes, lpBase);

		if(PyErr_Occurred())
			goto exit;

		lpsRestriction->res.resComment.lpProp = List_to_LPSPropValue(lpProp, &lpsRestriction->res.resComment.cValues, CONV_COPY_SHALLOW, lpBase);
		break;

	default:
		PyErr_Format(PyExc_RuntimeError, "Bad restriction type %d", lpsRestriction->rt);
		goto exit;
	}

exit:
	if(lpProp) {
		Py_DECREF(lpProp);
	}

	if(ulPropTag2) {
		Py_DECREF(ulPropTag2);
	}

	if(ulPropTag) {
		Py_DECREF(ulPropTag);
	}

	if(relop) {
		Py_DECREF(relop);
	}

	if(cb) {
		Py_DECREF(cb);
	}

	if(ulMask) {
		Py_DECREF(ulMask);
	}

	if(ulFuzzyLevel) {
		Py_DECREF(ulFuzzyLevel);
	}

	if(rt) {
		Py_DECREF(rt);
	}

	if(iter) {
		Py_DECREF(iter);
	}

	if(elem) {
		Py_DECREF(elem);
	}

	if(sub) {
		Py_DECREF(sub);
	}
}

LPSRestriction	Object_to_LPSRestriction(PyObject *object, void *lpBase)
{
	LPSRestriction lpRestriction = NULL;

	if(object == Py_None)
		return NULL;

	if (MAPIAllocateBuffer(sizeof(SRestriction), (void **)&lpRestriction) != hrSuccess)
		goto exit;

	Object_to_LPSRestriction(object, lpRestriction);

	if(PyErr_Occurred()) {
		MAPIFreeBuffer(lpRestriction);
		return NULL;
	}

exit:
	return lpRestriction;
}

PyObject *		Object_from_LPSRestriction(LPSRestriction lpsRestriction)
{
	PyObject *sub = NULL;
	PyObject *subs = NULL;
	PyObject *result = NULL;
	PyObject *propval = NULL;
	PyObject *proplist = NULL;

	if (lpsRestriction == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	switch(lpsRestriction->rt) {
	case RES_AND:
	case RES_OR:
		subs = PyList_New(0);

		for (ULONG i = 0; i < lpsRestriction->res.resAnd.cRes; ++i) {
			sub = Object_from_LPSRestriction(lpsRestriction->res.resAnd.lpRes + i);
			if (!sub)
				goto exit;

			PyList_Append(subs, sub);

			Py_DECREF(sub);
			sub = NULL;
		}

		if (lpsRestriction->rt == RES_AND)
			result = PyObject_CallFunction(PyTypeSAndRestriction, "O", subs);
		else
			result = PyObject_CallFunction(PyTypeSOrRestriction, "O", subs);
		break;

	case RES_NOT:
		sub = Object_from_LPSRestriction(lpsRestriction->res.resNot.lpRes);
		if(!sub)
			goto exit;

		result = PyObject_CallFunction(PyTypeSNotRestriction, "O", sub);
		break;

	case RES_CONTENT:
		propval = Object_from_LPSPropValue(lpsRestriction->res.resContent.lpProp);
		if (!propval)
			goto exit;

		result = PyObject_CallFunction(PyTypeSContentRestriction, "kkO", lpsRestriction->res.resContent.ulFuzzyLevel, lpsRestriction->res.resContent.ulPropTag, propval);
		break;

	case RES_PROPERTY:
		propval = Object_from_LPSPropValue(lpsRestriction->res.resProperty.lpProp);
		if (!propval)
			goto exit;

		result = PyObject_CallFunction(PyTypeSPropertyRestriction, "kkO", lpsRestriction->res.resProperty.relop, lpsRestriction->res.resProperty.ulPropTag, propval);
		break;

	case RES_COMPAREPROPS:
		result = PyObject_CallFunction(PyTypeSComparePropsRestriction, "kkk", lpsRestriction->res.resCompareProps.relop, lpsRestriction->res.resCompareProps.ulPropTag1, lpsRestriction->res.resCompareProps.ulPropTag2);
		break;

	case RES_BITMASK:
		result = PyObject_CallFunction(PyTypeSBitMaskRestriction, "kkk", lpsRestriction->res.resBitMask.relBMR, lpsRestriction->res.resBitMask.ulPropTag, lpsRestriction->res.resBitMask.ulMask);
		break;

	case RES_SIZE:
		result = PyObject_CallFunction(PyTypeSSizeRestriction, "kkk", lpsRestriction->res.resSize.relop, lpsRestriction->res.resSize.ulPropTag, lpsRestriction->res.resSize.cb);
		break;

	case RES_EXIST:
		result = PyObject_CallFunction(PyTypeSExistRestriction, "k", lpsRestriction->res.resExist.ulPropTag);
		break;

	case RES_SUBRESTRICTION:
		sub = Object_from_LPSRestriction(lpsRestriction->res.resSub.lpRes);
		if (!sub)
			goto exit;

		result = PyObject_CallFunction(PyTypeSSubRestriction, "kO", lpsRestriction->res.resSub.ulSubObject, sub);
		break;

	case RES_COMMENT:
		sub = Object_from_LPSRestriction(lpsRestriction->res.resComment.lpRes);
		if (!sub)
			goto exit;

		proplist = List_from_LPSPropValue(lpsRestriction->res.resComment.lpProp, lpsRestriction->res.resComment.cValues);
		if (!proplist)
			goto exit;

		result = PyObject_CallFunction(PyTypeSCommentRestriction, "OO", sub, proplist);
		break;

	default:
		PyErr_Format(PyExc_RuntimeError, "Bad restriction type %d", lpsRestriction->rt);
		goto exit;
	}

exit:
	if(sub) {
		Py_DECREF(sub);
	}

	if(subs) {
		Py_DECREF(subs);
	}

	if(propval) {
		Py_DECREF(propval);
	}

	if(proplist) {
		Py_DECREF(proplist);
	}

	if(PyErr_Occurred()) {
		if(result) {
			Py_DECREF(result);
		}
		result = NULL;
	}

	return result;
}

PyObject *		Object_from_LPACTION(LPACTION lpAction)
{
	PyObject *result = NULL;
	PyObject *act = NULL;

	if (lpAction == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	switch(lpAction->acttype) {
	case OP_MOVE:
	case OP_COPY:
		act = PyObject_CallFunction(PyTypeActMoveCopy, "s#s#",
									lpAction->actMoveCopy.lpStoreEntryId, lpAction->actMoveCopy.cbStoreEntryId,
									lpAction->actMoveCopy.lpFldEntryId, lpAction->actMoveCopy.cbFldEntryId);
		break;
	case OP_REPLY:
	case OP_OOF_REPLY:
		act = PyObject_CallFunction(PyTypeActReply, "s#s#",
									lpAction->actReply.lpEntryId, lpAction->actReply.cbEntryId,
									&lpAction->actReply.guidReplyTemplate, sizeof(GUID));
		break;
	case OP_DEFER_ACTION:
		act = PyObject_CallFunction(PyTypeActDeferAction, "s#",
									lpAction->actDeferAction.pbData, lpAction->actDeferAction.cbData);
		break;
	case OP_BOUNCE:
		act = PyObject_CallFunction(PyTypeActBounce, "l", lpAction->scBounceCode);
		break;
	case OP_FORWARD:
	case OP_DELEGATE:
		act = PyObject_CallFunction(PyTypeActFwdDelegate, "O", List_from_LPADRLIST(lpAction->lpadrlist));
		break;
	case OP_TAG:
		act = PyObject_CallFunction(PyTypeActTag, "O", Object_from_LPSPropValue(&lpAction->propTag));
		break;
	case OP_DELETE:
	case OP_MARK_AS_READ:
		act = Py_None;
		Py_INCREF(Py_None);
		break;
	};

	// restriction and proptype are always NULL
	Py_INCREF(Py_None);
	Py_INCREF(Py_None);
	result = PyObject_CallFunction(PyTypeAction, "llOOlO", lpAction->acttype, lpAction->ulActionFlavor, Py_None, Py_None, lpAction->ulFlags, act);

	return result;
}

PyObject *		Object_from_LPACTIONS(ACTIONS *lpsActions)
{
	PyObject *sub = NULL;
	PyObject *subs = NULL;
	PyObject *result = NULL;

	if (lpsActions == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	subs = PyList_New(0);
	for (UINT i = 0; i < lpsActions->cActions; ++i) {
		sub = Object_from_LPACTION(&lpsActions->lpAction[i]);
		if (!sub)
			goto exit;

		PyList_Append(subs, sub);

		Py_DECREF(sub);
		sub = NULL;
	}

	result = PyObject_CallFunction(PyTypeACTIONS, "lO", lpsActions->ulVersion, subs);

exit:
	if(sub) {
		Py_DECREF(sub);
	}

	if(subs) {
		Py_DECREF(subs);
	}

	if(PyErr_Occurred()) {
		if(result) {
			Py_DECREF(result);
		}
		result = NULL;
	}

	return result;
}

void Object_to_LPACTION(PyObject *object, ACTION *lpAction, void *lpBase)
{
	PyObject *poActType = PyObject_GetAttrString(object, "acttype");
	PyObject *poActionFlavor = PyObject_GetAttrString(object, "ulActionFlavor");
	PyObject *poRes = PyObject_GetAttrString(object, "lpRes");
	PyObject *poPropTagArray = PyObject_GetAttrString(object, "lpPropTagArray");
	PyObject *poFlags = PyObject_GetAttrString(object, "ulFlags");
	PyObject *poActObject = PyObject_GetAttrString(object, "actobj");

	lpAction->acttype = (ACTTYPE)PyLong_AsUnsignedLong(poActType);
	lpAction->ulActionFlavor = PyLong_AsUnsignedLong(poActionFlavor);
	// @todo convert (unused) restriction and proptagarray
	lpAction->lpRes = NULL;
	lpAction->lpPropTagArray = NULL;
	lpAction->ulFlags = PyLong_AsUnsignedLong(poFlags);
	lpAction->dwAlignPad = 0;
	switch (lpAction->acttype) {
	case OP_MOVE:
	case OP_COPY:
	{
		PyObject *poStore = PyObject_GetAttrString(poActObject, "StoreEntryId");
		PyObject *poFolder = PyObject_GetAttrString(poActObject, "FldEntryId");
		Py_ssize_t size;
		PyString_AsStringAndSize(poStore, (char**)&lpAction->actMoveCopy.lpStoreEntryId, &size);
		lpAction->actMoveCopy.cbStoreEntryId = size;
		PyString_AsStringAndSize(poFolder, (char**)&lpAction->actMoveCopy.lpFldEntryId, &size);
		lpAction->actMoveCopy.cbFldEntryId = size;
		Py_DECREF(poFolder);
		Py_DECREF(poStore);
		break;
	}
	case OP_REPLY:
	case OP_OOF_REPLY:
	{
		PyObject *poEntryId = PyObject_GetAttrString(poActObject, "EntryId");
		PyObject *poGuid = PyObject_GetAttrString(poActObject, "guidReplyTemplate");
		char *ptr;
		Py_ssize_t size;
		PyString_AsStringAndSize(poEntryId, (char**)&lpAction->actReply.lpEntryId, &size);
		lpAction->actReply.cbEntryId = size;
		PyString_AsStringAndSize(poGuid, &ptr, &size);
		if (size == sizeof(GUID))
			memcpy(&lpAction->actReply.guidReplyTemplate, ptr, size);
		else
			memset(&lpAction->actReply.guidReplyTemplate, 0, sizeof(GUID));
		Py_DECREF(poEntryId);
		Py_DECREF(poGuid);
		break;
	}
	case OP_DEFER_ACTION:
	{
		PyObject *poData = PyObject_GetAttrString(poActObject, "data");
		Py_ssize_t size;
		PyString_AsStringAndSize(poData, (char**)&lpAction->actDeferAction.pbData, &size);
		lpAction->actDeferAction.cbData = size;
		Py_DECREF(poData);
		break;
	}
	case OP_BOUNCE:
	{
		PyObject *poBounce = PyObject_GetAttrString(poActObject, "scBounceCode");
		lpAction->scBounceCode = PyLong_AsUnsignedLong(poBounce);
		Py_DECREF(poBounce);
		break;
	}
	case OP_FORWARD:
	case OP_DELEGATE:
	{
		PyObject *poAdrList = PyObject_GetAttrString(poActObject, "lpadrlist");
		// @todo fix memleak
		lpAction->lpadrlist = List_to_LPADRLIST(poAdrList);
		Py_DECREF(poAdrList);
		break;
	}
	case OP_TAG:
	{
		PyObject *poPropTag = PyObject_GetAttrString(poActObject, "propTag");
		Object_to_LPSPropValue(poPropTag, &lpAction->propTag, CONV_COPY_SHALLOW, lpBase);
		Py_DECREF(poPropTag);
		break;
	}
	case OP_DELETE:
	case OP_MARK_AS_READ:
		break;
	}

	if (poActType) {
		Py_DECREF(poActType);
	}
	if (poActionFlavor) {
		Py_DECREF(poActionFlavor);
	}
	if (poRes) {
		Py_DECREF(poRes);
	}
	if (poPropTagArray) {
		Py_DECREF(poPropTagArray);
	}
	if (poFlags) {
		Py_DECREF(poFlags);
	}
	if (poActObject) {
		Py_DECREF(poActObject);
	}
}

void Object_to_LPACTIONS(PyObject *object, ACTIONS *lpActions, void *lpBase)
{
	HRESULT hr = hrSuccess;
	PyObject *poVersion = NULL;
	PyObject *poAction = NULL;
	PyObject *iter = NULL;
	PyObject *elem = NULL;
	Py_ssize_t len = 0;
	unsigned int i = 0;

	if(object == Py_None)
		goto exit;

	if (lpBase == NULL)
		lpBase = lpActions;

	poVersion = PyObject_GetAttrString(object, "ulVersion");
	poAction = PyObject_GetAttrString(object, "lpAction");

	if(!poVersion || !poAction) {
		PyErr_SetString(PyExc_RuntimeError, "Missing ulVersion or lpAction for ACTIONS struct");
		goto exit;
	}

	len = PyObject_Length(poAction);
	if (len == 0) {
		PyErr_SetString(PyExc_RuntimeError, "No actions found in ACTIONS struct");
		goto exit;
	} else if (len == -1) {
		PyErr_SetString(PyExc_RuntimeError, "No action array found in ACTIONS struct");
		goto exit;
	}

	hr = MAPIAllocateMore(sizeof(ACTION)*len, lpBase, (void**)&lpActions->lpAction);
	if (hr != hrSuccess) {
		PyErr_SetString(PyExc_RuntimeError, "Out of memory");
		goto exit;
	}

	lpActions->ulVersion = PyLong_AsUnsignedLong(poVersion); // EDK_RULES_VERSION
	lpActions->cActions = len;

	iter = PyObject_GetIter(poAction);
	if(iter == NULL)
		goto exit;

	i = 0;
	while ((elem = PyIter_Next(iter))) {
		Object_to_LPACTION(elem, &lpActions->lpAction[i++], lpActions);
		Py_DECREF(elem);
	}

exit:
	if (poVersion) { Py_DECREF(poVersion); }
	if (poAction) { Py_DECREF(poAction); }
	if(iter) { Py_DECREF(iter); }
	if(elem) { Py_DECREF(elem); }
}

LPSSortOrderSet	Object_to_LPSSortOrderSet(PyObject *object)
{
	PyObject *aSort = NULL;
	PyObject *cCategories = NULL;
	PyObject *cExpanded = NULL;
	PyObject *iter = NULL;
	PyObject *elem = NULL;
	PyObject *ulPropTag = NULL;
	PyObject *ulOrder = NULL;
	LPSSortOrderSet lpsSortOrderSet = NULL;
	Py_ssize_t len = 0;
	unsigned int i = 0;

	if(object == Py_None)
		goto exit;

	aSort = PyObject_GetAttrString(object, "aSort");
	cCategories = PyObject_GetAttrString(object, "cCategories");
	cExpanded = PyObject_GetAttrString(object, "cExpanded");

	if(!aSort || !cCategories || !cExpanded) {
		PyErr_SetString(PyExc_RuntimeError, "Missing aSort, cCategories or cExpanded for sort order");
		goto exit;
	}

	len = PyObject_Length(aSort);
	if(len < 0) {
		PyErr_SetString(PyExc_RuntimeError, "aSort is not a sequence");
		goto exit;
	}

	if (MAPIAllocateBuffer(CbNewSSortOrderSet(len), (void **)&lpsSortOrderSet) != hrSuccess)
		goto exit;

	iter = PyObject_GetIter(aSort);
	if(iter == NULL)
		goto exit;

	while((elem = PyIter_Next(iter))) {
		ulOrder = PyObject_GetAttrString(elem, "ulOrder");
		ulPropTag = PyObject_GetAttrString(elem, "ulPropTag");

		if(!ulOrder || !ulPropTag) {
			PyErr_SetString(PyExc_RuntimeError, "ulOrder or ulPropTag missing for sort order");
			goto exit;
		}

		lpsSortOrderSet->aSort[i].ulOrder = PyLong_AsUnsignedLong(ulOrder);
		lpsSortOrderSet->aSort[i].ulPropTag = PyLong_AsUnsignedLong(ulPropTag);
		++i;
		Py_DECREF(elem);
	}

	lpsSortOrderSet->cSorts = i;
	lpsSortOrderSet->cCategories = PyLong_AsUnsignedLong(cCategories);
	lpsSortOrderSet->cExpanded = PyLong_AsUnsignedLong(cExpanded);

exit:
	if(PyErr_Occurred()) {
		MAPIFreeBuffer(lpsSortOrderSet);
		lpsSortOrderSet = NULL;
	}

	if(ulOrder) { Py_DECREF(ulOrder); }
	if(ulPropTag) { Py_DECREF(ulPropTag); }
	if(iter) { Py_DECREF(iter); }
	if(elem) { Py_DECREF(elem); }
	if(aSort) { Py_DECREF(aSort); }
	if(cCategories) { Py_DECREF(cCategories); }
	if(cExpanded) { Py_DECREF(cExpanded); }

	return lpsSortOrderSet;
}

PyObject *		Object_from_LPSSortOrderSet(LPSSortOrderSet lpSortOrderSet)
{
	PyObject *sort = NULL;
	PyObject *sorts = NULL;
	PyObject *result = NULL;

	if(lpSortOrderSet == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	sorts = PyList_New(0);

	for (unsigned int i = 0; i < lpSortOrderSet->cSorts; ++i) {
		sort = PyObject_CallFunction(PyTypeSSort, "(ll)", lpSortOrderSet->aSort[i].ulPropTag, lpSortOrderSet->aSort[i].ulOrder);

		if(PyErr_Occurred())
			goto exit;

		PyList_Append(sorts,sort);

		Py_DECREF(sort);
		sort = NULL;
	}

	result = PyObject_CallFunction(PyTypeSSortOrderSet, "(Oll)", sorts, lpSortOrderSet->cCategories, lpSortOrderSet->cExpanded);

exit:
	if(sorts) {
		Py_DECREF(sorts);
	}

	if(sort) {
		Py_DECREF(sort);
	}

	if(PyErr_Occurred()) {
		if(result) {
			Py_DECREF(result);
		}
		result = NULL;
	}

	return result;
}

PyObject *		List_from_LPSRowSet(LPSRowSet lpRowSet)
{
	PyObject *list = PyList_New(0);
	PyObject *item = NULL;

	for (unsigned int i = 0; i < lpRowSet->cRows; ++i) {
		item = List_from_LPSPropValue(lpRowSet->aRow[i].lpProps, lpRowSet->aRow[i].cValues);

		if(PyErr_Occurred())
			goto exit;

		PyList_Append(list, item);

		Py_DECREF(item);
		item = NULL;
	}

exit:
	if(item) {
		Py_DECREF(item);
	}
	if(PyErr_Occurred()) {
		if(list) {
			Py_DECREF(list);
		}
		list = NULL;
	}

	return list;
}

LPSRowSet		List_to_LPSRowSet(PyObject *list, ULONG ulFlags)
{
	PyObject *iter = NULL;
	PyObject *elem = NULL;
	Py_ssize_t len = 0;
	LPSRowSet lpsRowSet = NULL;
	int i = 0;

	if (list == Py_None)
		goto exit;

	len = PyObject_Length(list);

	iter = PyObject_GetIter(list);
	if(!iter)
		goto exit;

	// Zero out the whole struct so that failures halfway don't leave the struct
	// in an uninitialized state for FreeProws()
	if (MAPIAllocateBuffer(CbNewSRowSet(len), (void **)&lpsRowSet) != hrSuccess)
		goto exit;

	memset(lpsRowSet, 0, CbNewSRowSet(len));

	while((elem = PyIter_Next(iter))) {
		lpsRowSet->aRow[i].lpProps = List_to_LPSPropValue(elem, &lpsRowSet->aRow[i].cValues, ulFlags);

		if(PyErr_Occurred())
			goto exit;

		Py_DECREF(elem);
		elem = NULL;
		++i;
	}

	lpsRowSet->cRows = i;

exit:
	if(elem) {
		Py_DECREF(elem);
	}
	if(iter) {
		Py_DECREF(iter);
	}

	if(PyErr_Occurred()) {
		if(lpsRowSet)
			FreeProws(lpsRowSet);
		lpsRowSet = NULL;
	}

	return lpsRowSet;
}

LPADRLIST		List_to_LPADRLIST(PyObject *av, ULONG ulFlags)
{
	// Binary compatible
	return (LPADRLIST) List_to_LPSRowSet(av, ulFlags);
}

PyObject *		List_from_LPADRLIST(LPADRLIST lpAdrList)
{
	// Binary compatible
	return List_from_LPSRowSet((LPSRowSet)lpAdrList);
}

LPADRPARM		Object_to_LPADRPARM(PyObject *av)
{
	// Unsupported for now
	PyErr_SetString(PyExc_RuntimeError, "LPADRPARM is not yet supported");
	return NULL;
}

LPADRENTRY		Object_to_LPADRENTRY(PyObject *av)
{
	// Unsupported for now
	PyErr_SetString(PyExc_RuntimeError, "LPADRENTRY is not yet supported");
	return NULL;
}

PyObject *		Object_from_LPSPropProblem(LPSPropProblem lpProblem)
{
	PyObject *problem = NULL;

	problem = PyObject_CallFunction(PyTypeSPropProblem, "(lII)", lpProblem->ulIndex, lpProblem->ulPropTag, lpProblem->scode);

	return problem;
}

void	Object_to_LPSPropProblem(PyObject *object, LPSPropProblem lpProblem)
{
	PyObject *scode = NULL;
	PyObject *ulIndex = NULL;
	PyObject *ulPropTag = NULL;

	scode = PyObject_GetAttrString(object, "scode");
	ulIndex = PyObject_GetAttrString(object, "ulIndex");
	ulPropTag = PyObject_GetAttrString(object, "ulPropTag");

	lpProblem->scode = PyLong_AsUnsignedLong(scode);
	lpProblem->ulIndex = PyLong_AsUnsignedLong(ulIndex);
	lpProblem->ulPropTag = PyLong_AsUnsignedLong(ulPropTag);

	if(scode)
		Py_DECREF(scode);
	if(ulIndex)
		Py_DECREF(ulIndex);
	if(ulPropTag)
		Py_DECREF(ulPropTag);
}

PyObject *		List_from_LPSPropProblemArray(LPSPropProblemArray lpProblemArray)
{
	PyObject *list = NULL;
	PyObject *elem = NULL;

	if(lpProblemArray == NULL) {
		Py_INCREF(Py_None);
		list = Py_None;
		goto exit;
	}

	list = PyList_New(0);

	for (unsigned int i = 0; i < lpProblemArray->cProblem; ++i) {
		elem = Object_from_LPSPropProblem(&lpProblemArray->aProblem[i]);

		if(PyErr_Occurred())
			goto exit;

		PyList_Append(list, elem);
		Py_DECREF(elem);
		elem = NULL;
	}

exit:
	if(PyErr_Occurred()) {
		if(list) {
			Py_DECREF(list);
		}
		list = NULL;
	}
	if(elem) {
		Py_DECREF(elem);
	}

	return list;
}

LPSPropProblemArray List_to_LPSPropProblemArray(PyObject *list, ULONG /*ulFlags*/)
{
	PyObject *iter = NULL;
	PyObject *elem = NULL;
	Py_ssize_t len = 0;
	LPSPropProblemArray lpsProblems = NULL;
	int i = 0;

	if (list == Py_None)
		goto exit;

	len = PyObject_Length(list);

	iter = PyObject_GetIter(list);
	if(!iter)
		goto exit;

	if (MAPIAllocateBuffer(CbNewSPropProblemArray(len), (void **)&lpsProblems) != hrSuccess)
		goto exit;

	memset(lpsProblems, 0, CbNewSPropProblemArray(len));

	while((elem = PyIter_Next(iter))) {
		Object_to_LPSPropProblem(elem, &lpsProblems->aProblem[i]);

		if(PyErr_Occurred())
			goto exit;

		Py_DECREF(elem);
		elem = NULL;
		++i;
	}

	lpsProblems->cProblem = i;

exit:
	if(elem) {
		Py_DECREF(elem);
	}
	if(iter) {
		Py_DECREF(iter);
	}

	if(PyErr_Occurred()) {
		MAPIFreeBuffer(lpsProblems);
		lpsProblems = NULL;
	}

	return lpsProblems;
}

PyObject * Object_from_LPMAPINAMEID(LPMAPINAMEID lpMAPINameId)
{
	PyObject *elem = NULL;
	PyObject *guid = NULL;

	if(lpMAPINameId == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	guid = PyString_FromStringAndSize((char *)lpMAPINameId->lpguid, sizeof(GUID));

	if(lpMAPINameId->ulKind == MNID_ID)
		elem = PyObject_CallFunction(PyTypeMAPINAMEID, "(Oll)", guid, MNID_ID, lpMAPINameId->Kind.lID);
	else {
		elem = PyObject_CallFunction(PyTypeMAPINAMEID, "(Olu)", guid, MNID_STRING, lpMAPINameId->Kind.lpwstrName);
	}

	if(guid) {
		Py_DECREF(guid);
	}

	return elem;
}

PyObject * List_from_LPMAPINAMEID(LPMAPINAMEID *lppMAPINameId, ULONG cNames)
{
	PyObject *list = NULL;
	PyObject *elem = NULL;

	list = PyList_New(0);

	for (unsigned int i = 0; i < cNames; ++i) {
		elem = Object_from_LPMAPINAMEID(lppMAPINameId[i]);

		if(PyErr_Occurred())
			goto exit;

		PyList_Append(list, elem);

		Py_DECREF(elem);
		elem = NULL;
	}

exit:
	if(PyErr_Occurred()) {
		Py_DECREF(list);
		list = NULL;
	}
	if(elem) {
		Py_DECREF(elem);
	}

	return list;
}

void Object_to_LPMAPINAMEID(PyObject *elem, LPMAPINAMEID *lppName, void *lpBase)
{
	LPMAPINAMEID lpName = NULL;
	PyObject *kind = NULL;
	PyObject *id = NULL;
	PyObject *guid = NULL;
	ULONG ulKind = 0;
	Py_ssize_t len = 0;

	if (MAPIAllocateMore(sizeof(MAPINAMEID), lpBase, (void **)&lpName) != hrSuccess) {
		PyErr_SetString(PyExc_RuntimeError, "Out of memory");
		goto exit;
	}
	memset(lpName, 0, sizeof(MAPINAMEID));

	kind = PyObject_GetAttrString(elem, "kind");
	id = PyObject_GetAttrString(elem, "id");
	guid = PyObject_GetAttrString(elem, "guid");

	if(!guid || !id) {
		PyErr_SetString(PyExc_RuntimeError, "Missing id or guid on MAPINAMEID object");
		goto exit;
	}

	if(!kind) {
		// Detect kind from type of 'id' parameter by first trying to use it as an int, then as string
		PyInt_AsLong(id);
		if(PyErr_Occurred()) {
			// Clear error
			PyErr_Clear();
			ulKind = MNID_STRING;
		} else {
			ulKind = MNID_ID;
		}
	} else {
		ulKind = PyInt_AsLong(kind);
	}

	lpName->ulKind = ulKind;
	if(ulKind == MNID_ID) {
		lpName->Kind.lID = PyInt_AsLong(id);
	} else {
		if(!PyUnicode_Check(id)) {
			PyErr_SetString(PyExc_RuntimeError, "Must pass unicode string for MNID_STRING ID part of MAPINAMEID");
			goto exit;
		}
		
		CopyPyUnicode(&lpName->Kind.lpwstrName, id, lpBase);
	}

	PyString_AsStringAndSize(guid, (char **)&lpName->lpguid, &len);
	if(len != sizeof(GUID)) {
		PyErr_Format(PyExc_RuntimeError, "GUID parameter of MAPINAMEID must be exactly %d bytes", (int)sizeof(GUID));
		goto exit;
	}

	*lppName = lpName;

exit:
	if(PyErr_Occurred()) {
		if(!lpBase)
			MAPIFreeBuffer(lpName);
	}

	if(guid) {
		Py_DECREF(guid);
	}
	if(id) {
		Py_DECREF(id);
	}
	if(kind) {
		Py_DECREF(kind);
	}
}

LPMAPINAMEID *	List_to_p_LPMAPINAMEID(PyObject *list, ULONG *lpcNames, ULONG /*ulFlags*/)
{
	LPMAPINAMEID *lpNames = NULL;
	Py_ssize_t len = 0;
	PyObject *iter = NULL;
	PyObject *elem = NULL;
	unsigned int i = 0;

	iter = PyObject_GetIter(list);
	if(!iter)
		goto exit;

	len = PyObject_Length(list);

	if (MAPIAllocateBuffer(sizeof(LPMAPINAMEID) * len, (void **)&lpNames) != hrSuccess)
		goto exit;

	memset(lpNames, 0, sizeof(LPMAPINAMEID) * len);

	while((elem = PyIter_Next(iter))) {
		Object_to_LPMAPINAMEID(elem, &lpNames[i], lpNames);

		if(PyErr_Occurred())
			goto exit;
		++i;
		Py_DECREF(elem);
		elem = NULL;
	}

	*lpcNames = i;

exit:
	if(PyErr_Occurred()) {
		MAPIFreeBuffer(lpNames);
		lpNames = NULL;
	}

	if(elem) {
		Py_DECREF(elem);
	}
	if(iter) {
		Py_DECREF(iter);
	}

	return lpNames;
}

LPENTRYLIST		List_to_LPENTRYLIST(PyObject *list)
{
	LPENTRYLIST lpEntryList = NULL;
	Py_ssize_t len = 0;
	PyObject *iter = NULL;
	PyObject *elem = NULL;
	unsigned int i = 0;

	if(list == Py_None)
		return NULL;

	iter = PyObject_GetIter(list);
	if(!iter)
		goto exit;

	len = PyObject_Length(list);

	if (MAPIAllocateBuffer(sizeof(*lpEntryList), (void **)&lpEntryList) != hrSuccess)
		goto exit;

	if (MAPIAllocateMore(len * sizeof *lpEntryList->lpbin, lpEntryList, (void**)&lpEntryList->lpbin) != hrSuccess)
		goto exit;

	lpEntryList->cValues = len;

	while((elem = PyIter_Next(iter))) {
		char *ptr;
		Py_ssize_t strlen;

		PyString_AsStringAndSize(elem, &ptr, &strlen);
		if (PyErr_Occurred())
			goto exit;

		lpEntryList->lpbin[i].cb = strlen;
		if (MAPIAllocateMore(strlen, lpEntryList, (void**)&lpEntryList->lpbin[i].lpb) != hrSuccess)
			goto exit;
		memcpy(lpEntryList->lpbin[i].lpb, ptr, strlen);
		++i;
		Py_DECREF(elem);
		elem = NULL;
	}

exit:
	if(PyErr_Occurred()) {
		MAPIFreeBuffer(lpEntryList);
		lpEntryList = NULL;
	}

	if(elem) {
		Py_DECREF(elem);
	}
	if(iter) {
		Py_DECREF(iter);
	}

	return lpEntryList;
}

PyObject *		List_from_LPENTRYLIST(LPENTRYLIST lpEntryList)
{
	PyObject *list = NULL;
	PyObject *elem = NULL;

	list = PyList_New(0);

	if (lpEntryList) {
		for (unsigned int i = 0; i < lpEntryList->cValues; ++i) {
			elem = PyString_FromStringAndSize((const char*)lpEntryList->lpbin[i].lpb, lpEntryList->lpbin[i].cb);
			if(PyErr_Occurred())
				goto exit;

			PyList_Append(list, elem);

			Py_DECREF(elem);
			elem = NULL;
		}
	}

exit:
	if(PyErr_Occurred()) {
		Py_DECREF(list);
		list = NULL;
	}
	if(elem) {
		Py_DECREF(elem);
	}

	return list;
}

LPNOTIFICATION	List_to_LPNOTIFICATION(PyObject *, ULONG *lpcNotifs)
{

	return NULL;
}

PyObject *		List_from_LPNOTIFICATION(LPNOTIFICATION lpNotif, ULONG cNotifs)
{
	PyObject *list = PyList_New(0);
	PyObject *item = NULL;

	for (unsigned int i = 0; i < cNotifs; ++i) {
		item = Object_from_LPNOTIFICATION(&lpNotif[i]);
		if(PyErr_Occurred())
			goto exit;

		PyList_Append(list, item);

		Py_DECREF(item);
		item = NULL;
	}

exit:
	if(PyErr_Occurred()) {
		if(list) {
			Py_DECREF(list);
		}
		list = NULL;
	}

	if(item) {
		Py_DECREF(item);
	}
	return list;
}

PyObject *		Object_from_LPNOTIFICATION(NOTIFICATION *lpNotif)
{
	PyObject *elem = NULL;
	PyObject *proptags = NULL;
	PyObject *index = NULL;
	PyObject *prior = NULL;
	PyObject *row = NULL;

	if(lpNotif == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	switch(lpNotif->ulEventType) {
		case fnevObjectCopied:
		case fnevObjectCreated:
		case fnevObjectDeleted:
		case fnevObjectModified:
		case fnevObjectMoved:
		case fnevSearchComplete:
			proptags = List_from_LPSPropTagArray(lpNotif->info.obj.lpPropTagArray);

			if(!proptags)
				goto exit;

			elem = PyObject_CallFunction(PyTypeOBJECT_NOTIFICATION, "(ls#ls#s#s#O)",
												lpNotif->ulEventType,
												lpNotif->info.obj.lpEntryID, lpNotif->info.obj.cbEntryID,
												lpNotif->info.obj.ulObjType,
												lpNotif->info.obj.lpParentID, lpNotif->info.obj.cbParentID,
												lpNotif->info.obj.lpOldID, lpNotif->info.obj.cbOldID,
												lpNotif->info.obj.lpOldParentID, lpNotif->info.obj.cbOldParentID,
												proptags);

			Py_DECREF(proptags);
			break;
		case fnevTableModified:
			index = Object_from_LPSPropValue(&lpNotif->info.tab.propIndex);
			if(!index)
				goto exit;

			prior = Object_from_LPSPropValue(&lpNotif->info.tab.propPrior);
			if(!prior)
				goto exit;

			row = List_from_LPSPropValue(lpNotif->info.tab.row.lpProps, lpNotif->info.tab.row.cValues);
			if(!row)
				goto exit;

			elem = PyObject_CallFunction(PyTypeTABLE_NOTIFICATION, "(lIOOO)", lpNotif->info.tab.ulTableEvent, lpNotif->info.tab.hResult, index, prior, row);

			Py_DECREF(index);
			Py_DECREF(prior);
			Py_DECREF(row);

			break;
		case fnevNewMail:
			elem = PyObject_CallFunction(PyTypeNEWMAIL_NOTIFICATION, "(s#s#lsl)", lpNotif->info.newmail.lpEntryID, lpNotif->info.newmail.cbEntryID,
																					lpNotif->info.newmail.lpParentID, lpNotif->info.newmail.cbParentID,
																					lpNotif->info.newmail.ulFlags,
																					lpNotif->info.newmail.lpszMessageClass,
																					lpNotif->info.newmail.ulMessageFlags);

			break;
		default:
			PyErr_Format(PyExc_RuntimeError, "Bad notification type %x", lpNotif->ulEventType);
			break;
	}

exit:
	return elem;
}

NOTIFICATION *	Object_to_LPNOTIFICATION(PyObject *obj)
{
	PyObject *oTmp = NULL;
	LPNOTIFICATION lpNotif = NULL;

	if(obj == Py_None)
		return NULL;

	if (MAPIAllocateBuffer(sizeof(NOTIFICATION), (void**)&lpNotif) != hrSuccess)
		goto exit;

	memset(lpNotif, 0, sizeof(NOTIFICATION));

	if(PyObject_IsInstance(obj, PyTypeNEWMAIL_NOTIFICATION))
	{
		lpNotif->ulEventType = fnevNewMail;

		Py_ssize_t size;
		oTmp = PyObject_GetAttrString(obj, "lpEntryID");
		if(!oTmp) {
			PyErr_SetString(PyExc_RuntimeError, "lpEntryID missing for newmail notification");
	   		goto exit;
		}

		if (oTmp != Py_None) {
			PyString_AsStringAndSize(oTmp, (char**)&lpNotif->info.newmail.lpEntryID, &size);
			lpNotif->info.newmail.cbEntryID = size;
		}

		Py_DECREF(oTmp);

		oTmp = PyObject_GetAttrString(obj, "lpParentID");
			if(!oTmp) {
				PyErr_SetString(PyExc_RuntimeError, "lpParentID missing for newmail notification");
				goto exit;
			}

		 if (oTmp != Py_None) {
			PyString_AsStringAndSize(oTmp, (char**)&lpNotif->info.newmail.lpParentID, &size);
			lpNotif->info.newmail.cbParentID = size;
		 }

			Py_DECREF(oTmp);

			oTmp = PyObject_GetAttrString(obj, "ulFlags");
			if(!oTmp) {
				PyErr_SetString(PyExc_RuntimeError, "ulFlags missing for newmail notification");
				goto exit;
			}

			if (oTmp != Py_None) {
				lpNotif->info.newmail.ulFlags = (ULONG)PyLong_AsUnsignedLong(oTmp);
			}

			Py_DECREF(oTmp);

			oTmp = PyObject_GetAttrString(obj, "ulMessageFlags");
			if(!oTmp) {
				PyErr_SetString(PyExc_RuntimeError, "ulMessageFlags missing for newmail notification");
				goto exit;
			}

			if (oTmp != Py_None) {
				lpNotif->info.newmail.ulMessageFlags = (ULONG)PyLong_AsUnsignedLong(oTmp);
			}
			Py_DECREF(oTmp);

			// MessageClass
			oTmp= PyObject_GetAttrString(obj, "lpszMessageClass");
			if(!oTmp) {
				PyErr_SetString(PyExc_RuntimeError, "lpszMessageClass missing for newmail notification");
				goto exit;
			}

			if (oTmp != Py_None) {
				if(lpNotif->info.newmail.ulFlags & MAPI_UNICODE) {
				    CopyPyUnicode(&lpNotif->info.newmail.lpszMessageClass, oTmp, lpNotif);
                }
				else
					PyString_AsStringAndSize(oTmp, (char**)&lpNotif->info.newmail.lpszMessageClass, NULL);
			}

			Py_DECREF(oTmp);
			oTmp = NULL;

	} else {
		PyErr_Format(PyExc_RuntimeError, "Bad object type %p", obj->ob_type);
	}

exit:
	if(PyErr_Occurred()) {
		MAPIFreeBuffer(lpNotif);
		lpNotif = NULL;
	}

	if(oTmp)
		Py_DECREF(oTmp);

	return lpNotif;
}

LPFlagList		List_to_LPFlagList(PyObject *list)
{
	PyObject *iter = NULL;
	PyObject *elem = NULL;
	Py_ssize_t len = 0;
	LPFlagList lpList = NULL;
	int i = 0;

	iter = PyObject_GetIter(list);
	if(!iter)
		goto exit;

	len = PyObject_Length(list);

	if (MAPIAllocateBuffer(CbNewFlagList(len), (void **)&lpList) != hrSuccess)
		goto exit;

	while((elem = PyIter_Next(iter))) {
		lpList->ulFlag[i] = PyLong_AsUnsignedLong(elem);

		if(PyErr_Occurred())
			goto exit;
		++i;
		Py_DECREF(elem);
		elem = NULL;
	}

	lpList->cFlags = i;

exit:
	if(PyErr_Occurred()) {
		MAPIFreeBuffer(lpList);
		lpList = NULL;
	}
	if(elem) {
		Py_DECREF(elem);
	}
	if(iter) {
		Py_DECREF(iter);
	}

	return lpList;
}

PyObject *		List_from_LPFlagList(LPFlagList lpFlags)
{
	PyObject *list = PyList_New(0);
	PyObject *elem = NULL;

	for (unsigned int i = 0; i < lpFlags->cFlags; ++i) {
		elem = PyLong_FromUnsignedLong(lpFlags->ulFlag[i]);
		PyList_Append(list, elem);

		Py_DECREF(elem);
		elem = NULL;
	}

	return list;
}

PyObject *		Object_from_LPMAPIERROR(LPMAPIERROR lpMAPIError)
{
	Py_INCREF(Py_None);
	return Py_None;
}

LPMAPIERROR		Object_to_LPMAPIERROR(PyObject *)
{
	LPMAPIERROR	lpError = NULL;
	if (MAPIAllocateBuffer(sizeof(LPMAPIERROR), (LPVOID*)&lpError) == hrSuccess)
		memset(lpError, 0, sizeof(*lpError));
	return lpError;
}

LPREADSTATE		List_to_LPREADSTATE(PyObject *list, ULONG *lpcElements)
{
	PyObject *iter = NULL;
	PyObject *elem = NULL;
	PyObject *sourcekey = NULL;
	PyObject *flags = NULL;
	Py_ssize_t len = 0;
	LPREADSTATE lpList = NULL;
	int i = 0;

	iter = PyObject_GetIter(list);
	if(!iter)
		goto exit;

	len = PyObject_Length(list);

	if (MAPIAllocateBuffer(len * sizeof *lpList, (void **)&lpList) != hrSuccess)
		goto exit;

	while((elem = PyIter_Next(iter))) {
		HRESULT hr;

		sourcekey = PyObject_GetAttrString(elem, "SourceKey");
		flags = PyObject_GetAttrString(elem, "ulFlags");

		if (!sourcekey || !flags)
			continue;

		char *ptr = NULL;
		Py_ssize_t len = 0;

		lpList[i].ulFlags = PyLong_AsUnsignedLong(flags);
		if (PyErr_Occurred())
			goto exit;

		PyString_AsStringAndSize(sourcekey, &ptr, &len);
		if (PyErr_Occurred())
			goto exit;

		hr = MAPIAllocateMore(len, lpList, (LPVOID*)&lpList[i].pbSourceKey);
		if (hr != hrSuccess) {
			PyErr_SetString(PyExc_RuntimeError, "Out of memory");
			goto exit;
		}

		memcpy(lpList[i].pbSourceKey, ptr, len);
		lpList[i].cbSourceKey = len;
		++i;
		Py_DECREF(flags);
		flags = NULL;

		Py_DECREF(sourcekey);
		sourcekey = NULL;

		Py_DECREF(elem);
		elem = NULL;
	}

	*lpcElements = len;

exit:
	if(PyErr_Occurred()) {
		MAPIFreeBuffer(lpList);
		lpList = NULL;
	}

	if(flags) {
		Py_DECREF(flags);
	}

	if(sourcekey) {
		Py_DECREF(sourcekey);
	}

	if(elem) {
		Py_DECREF(elem);
	}

	if(iter) {
		Py_DECREF(iter);
	}

	return lpList;
}

PyObject *		List_from_LPREADSTATE(LPREADSTATE lpReadState, ULONG cElements)
{
	PyObject *list = PyList_New(0);
	PyObject *elem = NULL;
	PyObject *sourcekey = NULL;

	for (unsigned int i = 0; i < cElements; ++i) {
		sourcekey = PyString_FromStringAndSize((char*)lpReadState[i].pbSourceKey, lpReadState[i].cbSourceKey);
		if (PyErr_Occurred())
			goto exit;

		elem = PyObject_CallFunction(PyTypeREADSTATE, "(Ol)", sourcekey, lpReadState[i].ulFlags);
		if (PyErr_Occurred())
			goto exit;

		PyList_Append(list, elem);

		Py_DECREF(sourcekey);
		sourcekey = NULL;

		Py_DECREF(elem);
		elem = NULL;
	}

exit:
	if (PyErr_Occurred()) {
		Py_DECREF(list);
		list = NULL;
	}

	return list;
}

LPCIID			List_to_LPCIID(PyObject *list, ULONG *cInterfaces)
{
	PyObject *iter = NULL;
	PyObject *elem = NULL;
	Py_ssize_t len = 0;
	LPIID lpList = NULL;
	int i = 0;

	if(list == Py_None) {
		cInterfaces = 0;
		return NULL;
	}

	iter = PyObject_GetIter(list);
	if(!iter)
		goto exit;

	len = PyObject_Length(list);

	if (MAPIAllocateBuffer(len * sizeof *lpList, (void **)&lpList) != hrSuccess)
		goto exit;

	while((elem = PyIter_Next(iter))) {
		char *ptr = NULL;
		Py_ssize_t strlen = 0;

		PyString_AsStringAndSize(elem, &ptr, &strlen);
		if (PyErr_Occurred())
			goto exit;

		if (strlen != sizeof(*lpList)) {
			PyErr_Format(PyExc_RuntimeError, "IID parameter must be exactly %d bytes", (int)sizeof(IID));
			goto exit;
		}

		memcpy(&lpList[i], ptr, sizeof(*lpList));
		++i;
		Py_DECREF(elem);
		elem = NULL;
	}

	*cInterfaces = len;

exit:
	if(PyErr_Occurred()) {
		MAPIFreeBuffer(lpList);
		lpList = NULL;
	}
	if(elem) {
		Py_DECREF(elem);
	}

	if(iter) {
		Py_DECREF(iter);
	}

	return lpList;
}

PyObject *List_from_LPCIID(LPCIID iids, ULONG cElements)
{
	if (iids == NULL) {
		Py_INCREF(Py_None);
		return(Py_None);
	}

	PyObject *list = PyList_New(0);
	PyObject *iid = NULL;

	for (unsigned int i = 0; i < cElements; ++i) {
		iid = PyString_FromStringAndSize((char*)&iids[i], sizeof(IID));
		if (PyErr_Occurred())
			goto exit;

		PyList_Append(list, iid);

		Py_DECREF(iid);
		iid = NULL;
	}

exit:
	if (PyErr_Occurred()) {
		Py_DECREF(list);
		list = NULL;
	}

	return list;
}

void Object_to_MVPROPMAP(PyObject *elem, ECUSER *&lpUser, ULONG ulFlags)
{
	HRESULT hr = hrSuccess;
	PyObject *MVPropMaps, *Item, *PropID, *ListItem, *Values;
	int ValuesLength, MVPropMapsSize = 0;

	/* Multi-Value PropMap support. */
	MVPropMaps = PyObject_GetAttrString(elem, "MVPropMap");

	if (MVPropMaps == NULL || !PyList_Check(MVPropMaps)) {
		Py_XDECREF(MVPropMaps);
		return;
	}

	MVPropMapsSize = PyList_Size(MVPropMaps);
	/* No PropMaps - bail out */
	if (MVPropMapsSize != 2) {
		PyErr_SetString(PyExc_TypeError, "MVPropMap should contain two entries");
		Py_DECREF(MVPropMaps);
		return;
	}

	/* If we have more mv props than the feature lists, adjust this value! */
	lpUser->sMVPropmap.cEntries = 2;
	hr = MAPIAllocateMore(sizeof(MVPROPMAPENTRY) * lpUser->sMVPropmap.cEntries, lpUser, reinterpret_cast<void **>(&lpUser->sMVPropmap.lpEntries));

	for (int i = 0; i < MVPropMapsSize; ++i) {
		Item = PyList_GetItem(MVPropMaps, i);
		PropID = PyObject_GetAttrString(Item, "ulPropId");
		Values = PyObject_GetAttrString(Item, "Values");

		if (PropID == NULL || Values == NULL || !PyList_Check(Values)) {
			PyErr_SetString(PyExc_TypeError, "ulPropId or Values is empty or values is not a list");

			Py_XDECREF(PropID);
			Py_XDECREF(Values);
			Py_DECREF(MVPropMaps);
			return;
		}

		/* Set default struct entry to empty stub values */
		lpUser->sMVPropmap.lpEntries[i].ulPropId = PyLong_AsUnsignedLong(PropID);
		lpUser->sMVPropmap.lpEntries[i].cValues = 0;
		lpUser->sMVPropmap.lpEntries[i].lpszValues = NULL;

		//if ((PropID != NULL && PropID != Py_None) && (Values != NULL && Values != Py_None && PyList_Check(Values)))
		ValuesLength = PyList_Size(Values);
		lpUser->sMVPropmap.lpEntries[i].cValues = ValuesLength;

		if (ValuesLength > 0) {
			hr = MAPIAllocateMore(sizeof(LPTSTR) * lpUser->sMVPropmap.lpEntries[i].cValues, lpUser, reinterpret_cast<void **>(&lpUser->sMVPropmap.lpEntries[i].lpszValues));
			if (hr != hrSuccess) {
				PyErr_SetString(PyExc_RuntimeError, "Out of memory");
				Py_DECREF(PropID);
				Py_DECREF(Values);
				Py_DECREF(MVPropMaps);
				return;
			}
		}

		for (int j = 0; j < ValuesLength; ++j) {
			ListItem = PyList_GetItem(Values, j);

			if (ListItem != Py_None) {
				if ((ulFlags & MAPI_UNICODE) == 0)
					// XXX: meh, not sure what todo here. Maybe use process_conv_out??
					lpUser->sMVPropmap.lpEntries[i].lpszValues[j] = reinterpret_cast<TCHAR *>(PyString_AsString(ListItem));
				else
					CopyPyUnicode(&lpUser->sMVPropmap.lpEntries[i].lpszValues[j], ListItem, lpUser);
			}
		}

		Py_DECREF(PropID);
		Py_DECREF(Values);
	}

	Py_DECREF(MVPropMaps);
}

PyObject *Object_from_MVPROPMAP(MVPROPMAP propmap, ULONG ulFlags)
{
	/*
	 * Multi-Value PropMap support.
	 *
	 * This holds the enabled/disabled features of a user. It is
	 * represented as a list of PropMaps which contains multiple values. A
	 * Normal Propmap only contains one value, so we use a list to display
	 * these values.
	 *
	 * Note that the enabled/disabled MVPropMap is special since, for
	 * example, setting both PR_EC_ENABLED_FEATUES and
	 * PR_EC_DISALBED_FEATURES to an empty list will still set the features
	 * to either disabled or enabled according to the default set in the
	 * server configuration.
	 */
	PyObject *MVProps = PyList_New(0);
	PyObject *MVPropValues, *MVPropMap, *MVPropValue = NULL;

	MVPROPMAP *lpMVPropmap = &propmap;
	for (unsigned int i = 0; i < lpMVPropmap->cEntries; ++i) {
		PyObject *MVPropValues = PyList_New(0);

		for (unsigned int j = 0; j < lpMVPropmap->lpEntries[i].cValues; ++j) {
			LPTSTR strval = lpMVPropmap->lpEntries[i].lpszValues[j];
			std::string str = reinterpret_cast<LPSTR>(strval);

			if (str.empty())
				continue;
			if (ulFlags & MAPI_UNICODE)
				MVPropValue = PyUnicode_FromWideChar(strval, wcslen(strval));
			else
				MVPropValue = PyString_FromStringAndSize(str.c_str(), str.length());

			PyList_Append(MVPropValues, MVPropValue);
			Py_DECREF(MVPropValue);
			MVPropValue = NULL;
		}

		MVPropMap = PyObject_CallFunction(PyTypeMVPROPMAP, "(lO)", lpMVPropmap->lpEntries[i].ulPropId, MVPropValues);
		Py_DECREF(MVPropValues);
		PyList_Append(MVProps, MVPropMap);
		Py_DECREF(MVPropMap);
		MVPropMap = NULL;
		MVPropValues = NULL;
	}

	return MVProps;
}

ECUSER *Object_to_LPECUSER(PyObject *elem, ULONG ulFlags)
{
	static conv_out_info<ECUSER> conv_info[] = {
		{conv_out_default<ECUSER, LPTSTR, &ECUSER::lpszUsername>, "Username"},
		{conv_out_default<ECUSER, LPTSTR, &ECUSER::lpszPassword>, "Password"},
		{conv_out_default<ECUSER, LPTSTR, &ECUSER::lpszMailAddress>, "Email"},
		{conv_out_default<ECUSER, LPTSTR, &ECUSER::lpszFullName>, "FullName"},
		{conv_out_default<ECUSER, LPTSTR, &ECUSER::lpszServername>, "Servername"},
		{conv_out_default<ECUSER, objectclass_t, &ECUSER::ulObjClass>, "Class"},
		{conv_out_default<ECUSER, unsigned int, &ECUSER::ulIsAdmin>, "IsAdmin"},
		{conv_out_default<ECUSER, unsigned int, &ECUSER::ulIsABHidden>, "IsHidden"},
		{conv_out_default<ECUSER, unsigned int, &ECUSER::ulCapacity>, "Capacity"},
		{conv_out_default<ECUSER, ECENTRYID, &ECUSER::sUserId>, "UserID"},
	};

	HRESULT hr = hrSuccess;
	ECUSER *lpUser = NULL;

	if (elem == Py_None)
		goto exit;

	hr = MAPIAllocateBuffer(sizeof *lpUser, (LPVOID*)&lpUser);
	if (hr != hrSuccess) {
		PyErr_SetString(PyExc_RuntimeError, "Out of memory");
		goto exit;
	}
	memset(lpUser, 0, sizeof *lpUser);
	process_conv_out_array(lpUser, elem, conv_info, lpUser, ulFlags);
	Object_to_MVPROPMAP(elem, lpUser, ulFlags);
exit:
	if (PyErr_Occurred()) {
		MAPIFreeBuffer(lpUser);
		lpUser = NULL;
	}

	return lpUser;
}

PyObject *Object_from_LPECUSER(ECUSER *lpUser, ULONG ulFlags)
{
	PyObject *MVProps = Object_from_MVPROPMAP(lpUser->sMVPropmap, ulFlags);
	PyObject *result = NULL;

	// XXX: Caller should DECREF List?
	if (ulFlags & MAPI_UNICODE)
		result = PyObject_CallFunction(PyTypeECUser, "(uuuuulllls#O)", lpUser->lpszUsername, lpUser->lpszPassword, lpUser->lpszMailAddress, lpUser->lpszFullName, lpUser->lpszServername, lpUser->ulObjClass, lpUser->ulIsAdmin, lpUser->ulIsABHidden, lpUser->ulCapacity, lpUser->sUserId.lpb, lpUser->sUserId.cb, MVProps);
	else
		result = PyObject_CallFunction(PyTypeECUser, "(ssssslllls#O)", lpUser->lpszUsername, lpUser->lpszPassword, lpUser->lpszMailAddress, lpUser->lpszFullName, lpUser->lpszServername, lpUser->ulObjClass, lpUser->ulIsAdmin, lpUser->ulIsABHidden, lpUser->ulCapacity, lpUser->sUserId.lpb, lpUser->sUserId.cb, MVProps);

	Py_DECREF(MVProps);
	return result;
}


PyObject *List_from_LPECUSER(ECUSER *lpUser, ULONG cElements, ULONG ulFlags)
{
	PyObject *list = PyList_New(0);
	PyObject *item = NULL;

	for (unsigned int i = 0; i < cElements; ++i) {
		item = Object_from_LPECUSER(&lpUser[i], ulFlags);
		if (PyErr_Occurred())
			goto exit;

		PyList_Append(list, item);

		Py_DECREF(item);
		item = NULL;
	}

exit:
	if(PyErr_Occurred()) {
		if(list) {
			Py_DECREF(list);
		}
		list = NULL;
	}

	if(item) {
		Py_DECREF(item);
	}

	return list;
}

ECGROUP *Object_to_LPECGROUP(PyObject *elem, ULONG ulFlags)
{
	static conv_out_info<ECGROUP> conv_info[] = {
		{conv_out_default<ECGROUP, LPTSTR, &ECGROUP::lpszGroupname>, "Groupname"},
		{conv_out_default<ECGROUP, LPTSTR, &ECGROUP::lpszFullname>, "Fullname"},
		{conv_out_default<ECGROUP, LPTSTR, &ECGROUP::lpszFullEmail>, "Email"},
		{conv_out_default<ECGROUP, unsigned int, &ECGROUP::ulIsABHidden>, "IsHidden"},
		{conv_out_default<ECGROUP, ECENTRYID, &ECGROUP::sGroupId>, "GroupID"},
	};

	HRESULT hr = hrSuccess;
	ECGROUP *lpGroup = NULL;

	if (elem == Py_None)
		goto exit;

	hr = MAPIAllocateBuffer(sizeof *lpGroup, (LPVOID*)&lpGroup);
	if (hr != hrSuccess) {
		PyErr_SetString(PyExc_RuntimeError, "Out of memory");
		goto exit;
	}
	memset(lpGroup, 0, sizeof *lpGroup);

	process_conv_out_array(lpGroup, elem, conv_info, lpGroup, ulFlags);

exit:
	if (PyErr_Occurred()) {
		MAPIFreeBuffer(lpGroup);
		lpGroup = NULL;
	}

	return lpGroup;
}

PyObject *Object_from_LPECGROUP(ECGROUP *lpGroup, ULONG ulFlags)
{
	if(ulFlags & MAPI_UNICODE)
		return PyObject_CallFunction(PyTypeECGroup, "(uuuls#)", lpGroup->lpszGroupname, lpGroup->lpszFullname, lpGroup->lpszFullEmail, lpGroup->ulIsABHidden, lpGroup->sGroupId.lpb, lpGroup->sGroupId.cb);
	else
		return PyObject_CallFunction(PyTypeECGroup, "(sssls#)", lpGroup->lpszGroupname, lpGroup->lpszFullname, lpGroup->lpszFullEmail, lpGroup->ulIsABHidden, lpGroup->sGroupId.lpb, lpGroup->sGroupId.cb);
}

PyObject *List_from_LPECGROUP(ECGROUP *lpGroup, ULONG cElements, ULONG ulFlags)
{
	PyObject *list = PyList_New(0);
	PyObject *item = NULL;

	for (unsigned int i = 0; i < cElements; ++i) {
		item = Object_from_LPECGROUP(&lpGroup[i], ulFlags);
		if (PyErr_Occurred())
			goto exit;

		PyList_Append(list, item);

		Py_DECREF(item);
		item = NULL;
	}

exit:
	if(PyErr_Occurred()) {
		if(list) {
			Py_DECREF(list);
		}
		list = NULL;
	}

	if(item) {
		Py_DECREF(item);
	}

	return list;
}

ECCOMPANY *Object_to_LPECCOMPANY(PyObject *elem, ULONG ulFlags)
{
	static conv_out_info<ECCOMPANY> conv_info[] = {
		{conv_out_default<ECCOMPANY, LPTSTR, &ECCOMPANY::lpszCompanyname>, "Companyname"},
		{conv_out_default<ECCOMPANY, LPTSTR, &ECCOMPANY::lpszServername>, "Servername"},
		{conv_out_default<ECCOMPANY, unsigned int, &ECCOMPANY::ulIsABHidden>, "IsHidden"},
		{conv_out_default<ECCOMPANY, ECENTRYID, &ECCOMPANY::sCompanyId>, "CompanyID"},
	};

	HRESULT hr = hrSuccess;
	ECCOMPANY *lpCompany = NULL;

	if (elem == Py_None)
		goto exit;

	hr = MAPIAllocateBuffer(sizeof *lpCompany, (LPVOID*)&lpCompany);
	if (hr != hrSuccess) {
		PyErr_SetString(PyExc_RuntimeError, "Out of memory");
		goto exit;
	}
	memset(lpCompany, 0, sizeof *lpCompany);

	process_conv_out_array(lpCompany, elem, conv_info, lpCompany, ulFlags);

exit:
	if (PyErr_Occurred()) {
		MAPIFreeBuffer(lpCompany);
		lpCompany = NULL;
	}

	return lpCompany;
}

PyObject *Object_from_LPECCOMPANY(ECCOMPANY *lpCompany, ULONG ulFlags)
{
        if(ulFlags & MAPI_UNICODE)
		return PyObject_CallFunction(PyTypeECCompany, "(uuls#)", lpCompany->lpszCompanyname, lpCompany->lpszServername, lpCompany->ulIsABHidden, lpCompany->sCompanyId.lpb, lpCompany->sCompanyId.cb);
	else
		return PyObject_CallFunction(PyTypeECCompany, "(ssls#)", lpCompany->lpszCompanyname, lpCompany->lpszServername, lpCompany->ulIsABHidden, lpCompany->sCompanyId.lpb, lpCompany->sCompanyId.cb);
}

PyObject *List_from_LPECCOMPANY(ECCOMPANY *lpCompany, ULONG cElements,
    ULONG ulFlags)
{
	PyObject *list = PyList_New(0);
	PyObject *item = NULL;

	for (unsigned int i = 0; i < cElements; ++i) {
		item = Object_from_LPECCOMPANY(&lpCompany[i], ulFlags);
		if (PyErr_Occurred())
			goto exit;

		PyList_Append(list, item);

		Py_DECREF(item);
		item = NULL;
	}

exit:
	if(PyErr_Occurred()) {
		if(list) {
			Py_DECREF(list);
		}
		list = NULL;
	}

	if(item) {
		Py_DECREF(item);
	}

	return list;
}

PyObject *Object_from_LPECUSERCLIENTUPDATESTATUS(ECUSERCLIENTUPDATESTATUS *lpECUCUS)
{
	// @todo charset conversion ?
	return PyObject_CallFunction(PyTypeECUserClientUpdateStatus, "(llsssl)", lpECUCUS->ulTrackId, lpECUCUS->tUpdatetime, lpECUCUS->lpszCurrentversion, lpECUCUS->lpszLatestversion, lpECUCUS->lpszComputername, lpECUCUS->ulStatus);
}


LPROWLIST List_to_LPROWLIST(PyObject *object, ULONG ulFlags)
{
	PyObject *elem = NULL;
	PyObject *iter = NULL;
	PyObject *rowflags = NULL;
	PyObject *props = NULL;
	Py_ssize_t len = 0;
	LPROWLIST lpRowList = NULL;
	int n = 0;

	if (object == Py_None)
		return NULL;

	len = PyObject_Length(object);
	if (len < 0) {
		PyErr_Format(PyExc_TypeError, "Invalid list passed as row list");
		goto exit;
	}

	if (MAPIAllocateBuffer(CbNewROWLIST(len), (void **)&lpRowList) != hrSuccess)
		goto exit;

	iter = PyObject_GetIter(object);
	if (iter == NULL)
		goto exit;

	while ((elem = PyIter_Next(iter))) {
		rowflags = PyObject_GetAttrString(elem, "ulRowFlags");
		if (rowflags == NULL)
			goto exit;

		props = PyObject_GetAttrString(elem, "rgPropVals");
		if (props == NULL)
			goto exit;

		lpRowList->aEntries[n].ulRowFlags = (ULONG)PyLong_AsUnsignedLong(rowflags);
		lpRowList->aEntries[n].rgPropVals = List_to_LPSPropValue(props, &lpRowList->aEntries[n].cValues, ulFlags);

		Py_DECREF(props);
		props = NULL;

		Py_DECREF(rowflags);
		rowflags = NULL;

		Py_DECREF(elem);
		elem = NULL;
		++n;
	}

	lpRowList->cEntries = n;

exit:
	if (PyErr_Occurred()) {
		MAPIFreeBuffer(lpRowList);
		lpRowList = NULL;
	}
	if (props)
		Py_DECREF(props);
	if (rowflags)
		Py_DECREF(rowflags);
	if (elem)
		Py_DECREF(elem);
	if (iter)
		Py_DECREF(iter);

	return lpRowList;
}

void DoException(HRESULT hr)
{
#if PY_VERSION_HEX >= 0x02040300	// 2.4.3
	PyObject *hrObj = Py_BuildValue("I", (unsigned int)hr);
#else
	// Python 2.4.2 and earlier don't support the "I" format so create a
	// PyLong object instead.
	PyObject *hrObj = PyLong_FromUnsignedLong((unsigned int)hr);
#endif

	#if PY_MAJOR_VERSION >= 3
		PyObject *attr_name = PyUnicode_FromString("_errormap");
	#else
		PyObject *attr_name = PyString_FromString("_errormap");
	#endif
	PyObject *errormap = PyObject_GetAttr(PyTypeMAPIError, attr_name);
	PyObject *errortype = NULL;
	PyObject *ex = NULL;
	if (errormap != NULL) {
		errortype = PyDict_GetItem(errormap, hrObj);
		if (errortype)
			ex = PyObject_CallFunction(errortype, NULL);
	}
	if (!errortype) {
		errortype = PyTypeMAPIError;
		ex = PyObject_CallFunction(PyTypeMAPIError, "O", hrObj);
	}

	PyErr_SetObject(errortype, ex);
	if (ex)
		Py_DECREF(ex);
	if (errormap)
		Py_DECREF(errormap);
	if (attr_name)
		Py_DECREF(attr_name);
	if (hrObj)
		Py_DECREF(hrObj);
}

int GetExceptionError(PyObject *object, HRESULT *lphr)
{
	if (!PyErr_GivenExceptionMatches(object, PyTypeMAPIError))
		return 0;

	PyObject *type = NULL, *value = NULL, *traceback = NULL;
	PyErr_Fetch(&type, &value, &traceback);

	PyObject *hr = PyObject_GetAttrString(value, "hr");
	if (!hr) {
		PyErr_SetString(PyExc_RuntimeError, "hr or Value missing from MAPIError");
		return -1;
	}

	*lphr = (HRESULT)PyLong_AsUnsignedLong(hr);
	Py_DECREF(hr);

	if (type) {
		Py_DECREF(type);
	}

	if (value) {
		Py_DECREF(value);
	}

	if (traceback) {
		Py_DECREF(traceback);
	}

	return 1;
}

ECQUOTA *Object_to_LPECQUOTA(PyObject *elem)
{
	static conv_out_info<ECQUOTA> conv_info[] = {
		{conv_out_default<ECQUOTA, bool, &ECQUOTA::bUseDefaultQuota>, "bUseDefaultQuota"},
		{conv_out_default<ECQUOTA, bool, &ECQUOTA::bIsUserDefaultQuota>, "bIsUserDefaultQuota"},
		{conv_out_default<ECQUOTA, int64_t, &ECQUOTA::llWarnSize>, "llWarnSize"},
		{conv_out_default<ECQUOTA, int64_t, &ECQUOTA::llSoftSize>, "llSoftSize"},
		{conv_out_default<ECQUOTA, int64_t, &ECQUOTA::llHardSize>, "llHardSize"},
	};

	HRESULT hr = hrSuccess;
	ECQUOTA *lpQuota = NULL;

	if (elem == Py_None)
		goto exit;

	hr = MAPIAllocateBuffer(sizeof *lpQuota, (LPVOID*)&lpQuota);
	if (hr != hrSuccess) {
		PyErr_SetString(PyExc_RuntimeError, "Out of memory");
		goto exit;
	}
	memset(lpQuota, 0, sizeof *lpQuota);

	process_conv_out_array(lpQuota, elem, conv_info, lpQuota, 0);

exit:
	if (PyErr_Occurred()) {
		MAPIFreeBuffer(lpQuota);
		lpQuota = NULL;
	}

	return lpQuota;
}

PyObject *Object_from_LPECQUOTA(ECQUOTA *lpQuota)
{
	return PyObject_CallFunction(PyTypeECQuota, "(llLLL)", lpQuota->bUseDefaultQuota, lpQuota->bIsUserDefaultQuota, lpQuota->llWarnSize, lpQuota->llSoftSize, lpQuota->llHardSize);
}

PyObject *Object_from_LPECQUOTASTATUS(ECQUOTASTATUS *lpQuotaStatus)
{
	return PyObject_CallFunction(PyTypeECQuotaStatus, "Ll", lpQuotaStatus->llStoreSize, lpQuotaStatus->quotaStatus);
}

ECSVRNAMELIST *List_to_LPECSVRNAMELIST(PyObject *object)
{
	HRESULT hr = hrSuccess;
	Py_ssize_t len = 0;
	PyObject *iter = NULL;
	PyObject *elem = NULL;
	ECSVRNAMELIST *lpSvrNameList = NULL;

	if (object == Py_None)
		goto exit;

	len = PyObject_Length(object);
	if (len < 0) {
		PyErr_Format(PyExc_TypeError, "Invalid list passed as servername list");
		goto exit;
	}

	if (MAPIAllocateBuffer(sizeof(ECSVRNAMELIST) + (sizeof(ECSERVER *) * len), reinterpret_cast<void **>(&lpSvrNameList)) != hrSuccess)
		goto exit;

	memset(lpSvrNameList, 0, sizeof(ECSVRNAMELIST) + (sizeof(ECSERVER *) * len) );

	iter = PyObject_GetIter(object);
	if (iter == NULL)
		goto exit;

	while ((elem = PyIter_Next(iter))) {
		char *ptr = NULL;
		Py_ssize_t strlen = 0;

		PyString_AsStringAndSize(elem, &ptr, &strlen);
		if (PyErr_Occurred())
			goto exit;

		hr = MAPIAllocateMore(strlen,  lpSvrNameList, (void**)&lpSvrNameList->lpszaServer[lpSvrNameList->cServers]);
		if (hr != hrSuccess) {
			PyErr_SetString(PyExc_RuntimeError, "Out of memory");
			goto exit;
		}

		memcpy(lpSvrNameList->lpszaServer[lpSvrNameList->cServers], ptr, strlen);

		Py_DECREF(elem);
		elem = NULL;
		++lpSvrNameList->cServers;
	}


exit:
	if(PyErr_Occurred()) {
		MAPIFreeBuffer(lpSvrNameList);
		lpSvrNameList = NULL;
	}
	if(elem) {
		Py_DECREF(elem);
	}

	if(iter) {
		Py_DECREF(iter);
	}

	return lpSvrNameList;
}

PyObject *Object_from_LPECSERVER(ECSERVER *lpServer)
{
	return PyObject_CallFunction(PyTypeECServer, "(sssssl)", lpServer->lpszName, lpServer->lpszFilePath, lpServer->lpszHttpPath, lpServer->lpszSslPath, lpServer->lpszPreferedPath, lpServer->ulFlags);
}

PyObject *List_from_LPECSERVERLIST(ECSERVERLIST *lpServerList)
{
	PyObject *list = PyList_New(0);
	PyObject *item = NULL;

	for (unsigned int i = 0; i < lpServerList->cServers; ++i) {
		item = Object_from_LPECSERVER(&lpServerList->lpsaServer[i]);
		if (PyErr_Occurred())
			goto exit;

		PyList_Append(list, item);

		Py_DECREF(item);
		item = NULL;
	}

exit:
	if(PyErr_Occurred()) {
		if(list) {
			Py_DECREF(list);
		}
		list = NULL;
	}

	if(item) {
		Py_DECREF(item);
	}

	return list;

}

void Object_to_STATSTG(PyObject *object, STATSTG *stg)
{
	PyObject *cbSize = NULL;

	if(object == Py_None) {
		PyErr_Format(PyExc_TypeError, "Invalid None passed for STATSTG");
		goto exit;
	}

	cbSize = PyObject_GetAttrString(object, "cbSize");

	if(!cbSize) {
		PyErr_Format(PyExc_TypeError, "STATSTG does not contain cbSize");
		goto exit;
	}

	stg->cbSize.QuadPart = PyLong_AsINT64(cbSize);

exit:
	if(cbSize) {
		Py_DECREF(cbSize);
	}
}

PyObject *Object_from_STATSTG(STATSTG *lpStatStg)
{
	PyObject *result = NULL;
	PyObject *cbSize = NULL;

	if(lpStatStg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	cbSize = PyLong_FromLongLong(lpStatStg->cbSize.QuadPart);

	result = PyObject_CallFunction(PyTypeSTATSTG, "(O)", cbSize);
	if(cbSize) {
		Py_DECREF(cbSize);
	}

	if(PyErr_Occurred()) {
		if(result) {
			Py_DECREF(result);
		}
		result = NULL;
	}

	return result;
}

PyObject *Object_from_SYSTEMTIME(const SYSTEMTIME &time)
{
	PyObject *object = NULL;

	object = PyObject_CallFunction(PyTypeSYSTEMTIME, "(iiiiiiii)",
								   time.wYear, time.wMonth, time.wDayOfWeek, time.wDay,
								   time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

	return object;
}

SYSTEMTIME Object_to_SYSTEMTIME(PyObject *object)
{
	static conv_out_info<SYSTEMTIME> conv_info[] = {
		{conv_out_default<SYSTEMTIME, WORD, &SYSTEMTIME::wYear>, "wYear"},
		{conv_out_default<SYSTEMTIME, WORD, &SYSTEMTIME::wMonth>, "wMonth"},
		{conv_out_default<SYSTEMTIME, WORD, &SYSTEMTIME::wDayOfWeek>, "wDayOfWeek"},
		{conv_out_default<SYSTEMTIME, WORD, &SYSTEMTIME::wDay>, "wDay"},
		{conv_out_default<SYSTEMTIME, WORD, &SYSTEMTIME::wHour>, "wHour"},
		{conv_out_default<SYSTEMTIME, WORD, &SYSTEMTIME::wMinute>, "wMinute"},
		{conv_out_default<SYSTEMTIME, WORD, &SYSTEMTIME::wSecond>, "wSecond"},
		{conv_out_default<SYSTEMTIME, WORD, &SYSTEMTIME::wMilliseconds>, "wMilliseconds"},
	};

	HRESULT hr = hrSuccess;
	SYSTEMTIME st = {0};

	if (object == Py_None)
		goto exit;

	process_conv_out_array(&st, object, conv_info, NULL, 0);

exit:

	return st;
}
