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

#include "licenseclient_conv.h"

// Get Py_ssize_t for older versions of python
#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
# define PY_SSIZE_T_MAX INT_MAX
# define PY_SSIZE_T_MIN INT_MIN
#endif

using namespace std;

PyObject* List_from_StringVector(const vector<string> &v)
{
    PyObject *list = PyList_New(0);
    PyObject *item = NULL;

    vector<string>::const_iterator i;

    for (i = v.begin(); i != v.end(); ++i) {
		item = Py_BuildValue("s", i->c_str());
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

int List_to_StringVector(PyObject *object, vector<string> &v)
{
    Py_ssize_t size = 0;
    vector<string> vTmp;
    PyObject *iter = NULL;
    PyObject *elem = NULL;
    int retval = -1;
    
    if(object == Py_None) {
		v.clear();
		return 0;
	}
    
    iter = PyObject_GetIter(object);
    if(!iter)
        goto exit;
            
    size = PyObject_Size(object);
    vTmp.reserve(size);
    
    while ((elem = PyIter_Next(iter)) != NULL) {
		char *ptr;
		Py_ssize_t strlen;

		#if PY_MAJOR_VERSION >= 3
			PyBytes_AsStringAndSize(elem, &ptr, &strlen);
		#else
			PyString_AsStringAndSize(elem, &ptr, &strlen);
		#endif
        if (PyErr_Occurred())
            goto exit;

		vTmp.push_back(string(ptr, strlen));
            
        Py_DECREF(elem);
        elem = NULL;
    }

	v.swap(vTmp);
	retval = 0;
    
exit:
    if(elem) {
        Py_DECREF(elem);
    }
        
    if(iter) {
        Py_DECREF(iter);
    }
        
    return retval;
}
