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

// Standard Conversion Library (Currently for Python only)
#ifndef SCL_H
#define SCL_H

#if PY_MAJOR_VERSION >= 3
	#define PyString_AsString(val) \
		PyBytes_AsString(val)
	#define PyString_AsStringAndSize(val, lpstr, size) \
		PyBytes_AsStringAndSize(val, lpstr, size)
	#define PyString_FromStringAndSize(val, size) \
		PyBytes_FromStringAndSize(val, size)
	#define PyString_FromString(val) \
		PyBytes_FromString(val)
	#define PyInt_AsLong(id) \
		PyLong_AsLong(id)
#endif

#include <zarafa/platform.h>

// Get Py_ssize_t for older versions of python
#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
# define PY_SSIZE_T_MAX INT_MAX
# define PY_SSIZE_T_MIN INT_MIN
#endif

namespace priv {
	/**
	 * Default version of conv_out, which is intended to convert one script value
	 * to a native value.
	 * This version will always generate a compile error as the actual conversions
	 * should be performed by specializations for the specific native types.
	 *
	 * @tparam		_Type	The type of the resulting value.
	 * @param[in]	Value	The scripted value to convert.
	 * @param[in]	flags	Allowed values:
	 *						@remark @c MAPI_UNICODE If the data is a string, it's a wide character string
	 * @param[out]	result	The native value.
	 */
	template <typename _Type>
	void conv_out(PyObject* value, LPVOID /*lpBase*/, ULONG /*ulFlags*/, _Type* result) {
		// Just generate an error here
		value = result;
	}

	/**
	 * Specialization for extracting a string (TCHAR*) from a script value.
	 *
	 * @tparam		_Type	The type of the resulting value.
	 * @param[in]	Value	The scripted value to convert.
	 * @param[out]	result	The native value.
	 */
	template <>
	void conv_out<LPTSTR>(PyObject* value, LPVOID lpBase, ULONG ulFlags, LPTSTR *lppResult) {
		if(value == Py_None) {
			*lppResult = NULL;
		} else {
			// FIXME: General helper function as improvement
			if ((ulFlags & MAPI_UNICODE) == 0)
				*(LPSTR*)lppResult = PyString_AsString(value);
			else {
				int len = PyUnicode_GetSize(value);
				MAPIAllocateMore((len + 1) * sizeof(WCHAR), lpBase, (LPVOID*)lppResult);
				// FIXME: Required for the PyUnicodeObject cast
				#if PY_MAJOR_VERSION >= 3
					len = PyUnicode_AsWideChar(value, *(LPWSTR*)lppResult, len);
				#else
					len = PyUnicode_AsWideChar((PyUnicodeObject*)value, *(LPWSTR*)lppResult, len);
				#endif
				(*(LPWSTR*)lppResult)[len] = L'\0';
			}
		}
	}

	/**
	 * Specialization for extracting an unsigned int from a script value.
	 *
	 * @tparam		_Type	The type of the resulting value.
	 * @param[in]	Value	The scripted value to convert.
	 * @param[out]	result	The native value.
	 */
	template <>
	void conv_out<unsigned int>(PyObject* value, LPVOID /*lpBase*/, ULONG /*ulFlags*/, unsigned int *lpResult) {
		*lpResult = (unsigned int)PyLong_AsUnsignedLong(value);
	}

	/**
	 * Specialization for extracting an unsigned short from a script value.
	 *
	 * @tparam		_Type	The type of the resulting value.
	 * @param[in]	Value	The scripted value to convert.
	 * @param[out]	result	The native value.
	 */
	template <>
	void conv_out<unsigned short>(PyObject* value, LPVOID /*lpBase*/, ULONG /*ulFlags*/, unsigned short *lpResult) {
		*lpResult = (unsigned short)PyLong_AsUnsignedLong(value);
	}

	/**
	 * Specialization for extracting a boolean from a script value.
	 *
	 * @tparam		_Type	The type of the resulting value.
	 * @param[in]	Value	The scripted value to convert.
	 * @param[out]	result	The native value.
	 */
	template <>
	void conv_out<bool>(PyObject* value, LPVOID /*lpBase*/, ULONG /*ulFlags*/, bool *lpResult) {
		*lpResult = (bool)PyLong_AsUnsignedLong(value);
	}

	/**
	 * Specialization for extracting a int64_t from a script value.
	 *
	 * @tparam		_Type	The type of the resulting value.
	 * @param[in]	Value	The scripted value to convert.
	 * @param[out]	result	The native value.
	 */
	template <>
	void conv_out<int64_t>(PyObject* value, LPVOID /*lpBase*/, ULONG /*ulFlags*/, int64_t *lpResult) {
		*lpResult = (int64_t)PyLong_AsUnsignedLong(value);
	}

	/**
	 * Specialization for extracting an ECENTRYID from a script value.
	 *
	 * @tparam		_Type	The type of the resulting value.
	 * @param[in]	Value	The scripted value to convert.
	 * @param[out]	result	The native value.
	 */
	template <>
	void conv_out<ECENTRYID>(PyObject* value, LPVOID lpBase, ULONG /*ulFlags*/, ECENTRYID *lpResult) {
		char *data;
		Py_ssize_t size;
		if(value == Py_None) {
			lpResult->cb = 0;
			lpResult->lpb = NULL;
		} else {
			PyString_AsStringAndSize(value, &data, &size);
			lpResult->cb = size;
			MAPIAllocateMore(size, lpBase, (LPVOID*)&lpResult->lpb);
			memcpy(lpResult->lpb, data, size);
		}
	}

	/**
	 * Specialization for extracting an objectclass_t from a script value.
	 * @note In the script language an objectclass_t will be an unsigned int value.
	 *
	 * @tparam		_Type	The type of the resulting value.
	 * @param[in]	Value	The scripted value to convert.
	 * @param[out]	result	The native value.
	 */
	template <>
	void conv_out<objectclass_t>(PyObject* value, LPVOID /*lpBase*/, ULONG /*ulFlags*/, objectclass_t *lpResult) {
		*lpResult = (objectclass_t)PyLong_AsUnsignedLong(value);
	}	
} // namspace priv

/**
 * This is the default convert function for converting a script value to
 * a native value that's part of a struct (on both sides). The actual conversion
 * is delegated to a specialization of the private::conv_out template.
 *
 * @tparam			_ObjType	The type of the structure containing the values that
 * 								are to be converted.
 * @tparam			_MemType	The type of the member for which this particular instantiation
 * 								is intended.
 * @tparam			_Member		The data member pointer that points to the actual field
 * 								for which this particula instantiation is intended.
 * @param[in,out]	lpObj		The native object whos members will be populated with
 * 								values converted from the scripted object.
 * @param[in]		elem		The scipted object, whose values will be converted to
 * 								a native representation.
 * @param[in]		lpszMember	The name of the member in the scripted object.
 * @param[in]		flags		Allowed values:
 *								@remark @c MAPI_UNICODE If the data is a string, it's a wide character string
 */
template <typename _ObjType, typename _MemType, _MemType(_ObjType::*_Member)>
void conv_out_default(_ObjType *lpObj, PyObject *elem, const char *lpszMember, LPVOID lpBase, ULONG ulFlags) {
	PyObject *value = PyObject_GetAttrString(elem, const_cast<char*>(lpszMember));	// Older versions of python might expect a non-const char pointer.
	if (PyErr_Occurred())
		return;

	priv::conv_out(value, lpBase, ulFlags, &(lpObj->*_Member));
	Py_DECREF(value);
}

/**
 * This structure is used to create a list of items that need to be converted from
 * their scripted representation to their native representation.
 */
template <typename _ObjType>
struct conv_out_info {
	typedef void(*conv_out_func_t)(_ObjType*, PyObject*, const char*, LPVOID lpBase, ULONG ulFlags);
	
	conv_out_func_t		conv_out_func;
	const char*			membername;
};

/**
 * This function processes an array of conv_out_info structures, effectively
 * performing the complete conversion.
 *
 * @tparam		_	ObjType		The type of the structure containing the values that
 * 								are to be converted. This is determined automatically.
 * @tparam			N			The size of the array. This is determined automatically.
 * @param[in]		array		The array containing the conv_out_info structures that
 * 								define the conversion operations.
 * @param[in]		flags		Allowed values:
 *								@remark @c MAPI_UNICODE If the data is a string, it's a wide character string
 */
template <typename _ObjType, size_t N>
void process_conv_out_array(_ObjType *lpObj, PyObject *elem, const conv_out_info<_ObjType> (&array)[N], LPVOID lpBase, ULONG ulFlags) {
	for (size_t n = 0; !PyErr_Occurred() && n < N; ++n)
		array[n].conv_out_func(lpObj, elem, array[n].membername, lpBase, ulFlags);
}

#endif // ndef SCL_H
