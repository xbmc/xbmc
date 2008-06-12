/* 
   Python wrappers for TDB module

   Copyright (C) Tim Potter, 2002-2003
   
     ** NOTE! The following LGPL license applies to the tdb python
     ** scripting library. This does NOT imply that all of Samba is 
     ** released under the LGPL
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "includes.h"

/* This symbol is used in both includes.h and Python.h which causes an
   annoying compiler warning. */

#ifdef HAVE_FSTAT
#undef HAVE_FSTAT
#endif

#include "Python.h"

/* Tdb exception */

PyObject *py_tdb_error;

/* tdb handle object */

typedef struct {
	PyObject_HEAD
	TDB_CONTEXT *tdb;
} tdb_hnd_object;

PyTypeObject tdb_hnd_type;
     
PyObject *new_tdb_hnd_object(TDB_CONTEXT *tdb)
{
	tdb_hnd_object *obj;

	obj = PyObject_New(tdb_hnd_object, &tdb_hnd_type);
	obj->tdb = tdb;

	return (PyObject *)obj;
}

PyObject *py_tdb_close(PyObject *self, PyObject *args)
{
	tdb_hnd_object *obj;

	if (!PyArg_ParseTuple(args, "O!", &tdb_hnd_type, &obj))
		return NULL;

	if (tdb_close(obj->tdb) == -1) {
		obj->tdb = NULL;
		PyErr_SetString(py_tdb_error, strerror(errno));
		return NULL;
	}

	obj->tdb = NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *py_tdb_open(PyObject *self, PyObject *args, PyObject *kw)
{
	static char *kwlist[] = { "name", "hash_size", "tdb_flags",
				  "open_flags", "mode", NULL };
	char *name;
	int hash_size = 0, flags = TDB_DEFAULT, open_flags = -1, open_mode = 0600;	
	TDB_CONTEXT *tdb;

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "s|iiii", kwlist, &name, &hash_size, &flags,
		    &open_flags, &open_mode))
		return NULL;

	/* Default open_flags to read/write */

	if (open_flags == -1) {
		if (access(name, W_OK) == -1)
			open_flags = O_RDONLY;
		else
			open_flags = O_RDWR;
	}

	if (!(tdb = tdb_open(name, hash_size, flags, open_flags, open_mode))) {
		PyErr_SetString(py_tdb_error, strerror(errno));
		return NULL;
	}

	return new_tdb_hnd_object(tdb);
}

/*
 * Allow a tdb to act as a python mapping (dictionary)
 */

static int tdb_traverse_count(TDB_CONTEXT *tdb, TDB_DATA key, TDB_DATA value,
			      void *state)
{
	/* Do nothing - tdb_traverse will return the number of records
           traversed. */

	return 0;
}

static int tdb_hnd_length(tdb_hnd_object *obj)
{
	int result;

	result = tdb_traverse(obj->tdb, tdb_traverse_count, NULL);

	return result;
}

static PyObject *tdb_hnd_subscript(tdb_hnd_object *obj, PyObject *key)
{
	TDB_DATA drec, krec;
	PyObject *result;

	if (!PyArg_Parse(key, "s#", &krec.dptr, &krec.dsize))
		return NULL;

	drec = tdb_fetch(obj->tdb, krec);

	if (!drec.dptr) {
		PyErr_SetString(PyExc_KeyError,
				PyString_AsString(key));
		return NULL;
	}

	result = PyString_FromStringAndSize(drec.dptr, drec.dsize);
	free(drec.dptr);

	return result;
}
	
static int tdb_ass_subscript(tdb_hnd_object *obj, PyObject *key, PyObject *value)
{
	TDB_DATA krec, drec;

        if (!PyArg_Parse(key, "s#", &krec.dptr, &krec.dsize)) {
		PyErr_SetString(PyExc_TypeError,
				"tdb mappings have string indices only");
		return -1;
	}

        if (!obj->tdb) {
		PyErr_SetString(
			py_tdb_error, "tdb object has been closed"); 
		return -1; 
        }

	if (!value) {

		/* Delete value */

		if (tdb_delete(obj->tdb, krec) == -1) {
			PyErr_SetString(PyExc_KeyError,
					PyString_AsString(value));
			return -1;
		}

	} else {

		/* Set value */

		if (!PyArg_Parse(value, "s#", &drec.dptr, &drec.dsize)) {
			PyErr_SetString(PyExc_TypeError,
				    "tdb mappings have string elements only");
			return -1;
		}

		errno = 0;

		if (tdb_store(obj->tdb, krec, drec, 0) < 0 ) {
			if (errno != 0)
				PyErr_SetFromErrno(py_tdb_error);
			else
				PyErr_SetString(
					py_tdb_error, 
					(char *)tdb_errorstr(obj->tdb));

			return -1;
		}
	}

	return 0;
} 

static PyMappingMethods tdb_mapping = {
	(inquiry) tdb_hnd_length,
	(binaryfunc) tdb_hnd_subscript,
	(objobjargproc) tdb_ass_subscript
};

/*
 * Utility methods
 */

/* Return non-zero if a given key exists in the tdb */

PyObject *py_tdb_hnd_has_key(PyObject *self, PyObject *args)
{
	tdb_hnd_object *obj = (tdb_hnd_object *)self;
	TDB_DATA key;

	if (!PyArg_ParseTuple(args, "s#", &key.dptr, &key.dsize))
		return NULL;

        if (!obj->tdb) {
		PyErr_SetString(
			py_tdb_error, "tdb object has been closed"); 
		return NULL;
        }	

	return PyInt_FromLong(tdb_exists(obj->tdb, key));
}

/* Return a list of keys in the tdb */

static int tdb_traverse_keys(TDB_CONTEXT *tdb, TDB_DATA key, TDB_DATA value,
			     void *state)
{
	PyObject *key_list = (PyObject *)state;

	PyList_Append(key_list, 
		      PyString_FromStringAndSize(key.dptr, key.dsize));

	return 0;
}

PyObject *py_tdb_hnd_keys(PyObject *self, PyObject *args)
{
	tdb_hnd_object *obj = (tdb_hnd_object *)self;
	PyObject *key_list = PyList_New(0);

        if (!obj->tdb) {
		PyErr_SetString(py_tdb_error, "tdb object has been closed"); 
		return NULL;
        }	

	if (tdb_traverse(obj->tdb, tdb_traverse_keys, key_list) == -1) {
		PyErr_SetString(py_tdb_error, "error traversing tdb");
		Py_DECREF(key_list);
		return NULL;
	}

	return key_list;	
}

PyObject *py_tdb_hnd_first_key(PyObject *self, PyObject *args)
{
	tdb_hnd_object *obj = (tdb_hnd_object *)self;
	TDB_DATA key;

        if (!obj->tdb) {
		PyErr_SetString(py_tdb_error, "tdb object has been closed"); 
		return NULL;
        }	

	key = tdb_firstkey(obj->tdb);

	return Py_BuildValue("s#", key.dptr, key.dsize);
}

PyObject *py_tdb_hnd_next_key(PyObject *self, PyObject *py_oldkey)
{
	tdb_hnd_object *obj = (tdb_hnd_object *)self;
	TDB_DATA key, oldkey;

        if (!obj->tdb) {
		PyErr_SetString(py_tdb_error, "tdb object has been closed"); 
		return NULL;
        }	

	if (!PyArg_Parse(py_oldkey, "s#", &oldkey.dptr, &oldkey.dsize))
		return NULL;

	key = tdb_nextkey(obj->tdb, oldkey);

	return Py_BuildValue("s#", key.dptr, key.dsize);
}

/*
 * Locking routines
 */

PyObject *py_tdb_hnd_lock_all(PyObject *self, PyObject *args)
{
	tdb_hnd_object *obj = (tdb_hnd_object *)self;
	int result;

        if (!obj->tdb) {
		PyErr_SetString(py_tdb_error, "tdb object has been closed"); 
		return NULL;
        }	

	result = tdb_lockall(obj->tdb);

	return PyInt_FromLong(result != -1);
}

PyObject *py_tdb_hnd_unlock_all(PyObject *self, PyObject *args)
{
	tdb_hnd_object *obj = (tdb_hnd_object *)self;

        if (!obj->tdb) {
		PyErr_SetString(py_tdb_error, "tdb object has been closed"); 
		return NULL;
        }	

	tdb_unlockall(obj->tdb);

	Py_INCREF(Py_None);
	return Py_None;
}

/* Return an array of keys from a python object which must be a string or a
   list of strings. */

static BOOL make_lock_list(PyObject *py_keys, TDB_DATA **keys, int *num_keys)
{
	/* Are we a list or a string? */

	if (!PyList_Check(py_keys) && !PyString_Check(py_keys)) {
		PyErr_SetString(PyExc_TypeError, "arg must be list of string");
		return False;
	}

	if (PyList_Check(py_keys)) {
		int i;

		/* Turn python list into array of keys */
		
		*num_keys = PyList_Size(py_keys);
		*keys = (TDB_DATA *)SMB_XMALLOC_ARRAY(TDB_DATA, (*num_keys));
		
		for (i = 0; i < *num_keys; i++) {
			PyObject *key = PyList_GetItem(py_keys, i);
			
			if (!PyString_Check(key)) {
				PyErr_SetString(
					PyExc_TypeError,
					"list elements must be strings");
				return False;
			}

			PyArg_Parse(key, "s#", &(*keys)[i].dptr, 
				    &(*keys)[i].dsize);
		}

	} else {

		/* Turn python string into a single key */

		*keys = (TDB_DATA *)SMB_XMALLOC_P(TDB_DATA);
		*num_keys = 1;
		PyArg_Parse(py_keys, "s#", &(*keys)->dptr, &(*keys)->dsize);
	}

	return True;
}

/*
 * tdb traversal
 */

struct traverse_info {
	PyObject *callback;
	PyObject *state;
};

static int tdb_traverse_traverse(TDB_CONTEXT *tdb, TDB_DATA key, TDB_DATA value,
				 void *state)
{
	struct traverse_info *info = state;
	PyObject *arglist, *py_result;
	int result;

	arglist = Py_BuildValue("(s#s#O)", key.dptr, key.dsize, value.dptr,
				value.dsize, info->state);

	py_result = PyEval_CallObject(info->callback, arglist);

	Py_DECREF(arglist);
	
	if (!PyInt_Check(py_result)) {
		result = 1;	/* Hmm - non-integer object returned by callback */
		goto done;
	}

	result = PyInt_AsLong(py_result);

done:
	Py_DECREF(py_result);
	return result;
}

PyObject *py_tdb_hnd_traverse(PyObject *self, PyObject *args, PyObject *kw)
{
	tdb_hnd_object *obj = (tdb_hnd_object *)self;
	static char *kwlist[] = { "traverse_fn", "state", NULL };
	PyObject *state = Py_None, *callback;
	struct traverse_info info;
	int result;

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "O|O", kwlist, &callback, &state))
		return NULL;

	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "parameter must be callable");
		return NULL;
        }

	Py_INCREF(callback);
	Py_INCREF(state);

	info.callback = callback;
	info.state = state;

	result = tdb_traverse(obj->tdb, tdb_traverse_traverse, &info);

	Py_DECREF(callback);
	Py_DECREF(state);

	return PyInt_FromLong(result);
}

PyObject *py_tdb_hnd_chainlock(PyObject *self, PyObject *args)
{
	tdb_hnd_object *obj = (tdb_hnd_object *)self;
	TDB_DATA key;
	int result;

        if (!obj->tdb) {
		PyErr_SetString(py_tdb_error, "tdb object has been closed"); 
		return NULL;
        }	

	if (!PyArg_ParseTuple(args, "s#", &key.dptr, &key.dsize))
		return NULL;

	result = tdb_chainlock(obj->tdb, key);

	return PyInt_FromLong(result != -1);
}

PyObject *py_tdb_hnd_chainunlock(PyObject *self, PyObject *args)
{
	tdb_hnd_object *obj = (tdb_hnd_object *)self;
	TDB_DATA key;
	int result;

        if (!obj->tdb) {
		PyErr_SetString(py_tdb_error, "tdb object has been closed"); 
		return NULL;
        }	

	if (!PyArg_ParseTuple(args, "s#", &key.dptr, &key.dsize))
		return NULL;

	result = tdb_chainunlock(obj->tdb, key);

	return PyInt_FromLong(result != -1);
}

PyObject *py_tdb_hnd_lock_bystring(PyObject *self, PyObject *args)
{
	tdb_hnd_object *obj = (tdb_hnd_object *)self;
	int result, timeout = 30;
	char *s;

        if (!obj->tdb) {
		PyErr_SetString(py_tdb_error, "tdb object has been closed"); 
		return NULL;
        }	

	if (!PyArg_ParseTuple(args, "s|i", &s, &timeout))
		return NULL;

	result = tdb_lock_bystring_with_timeout(obj->tdb, s, timeout);

	return PyInt_FromLong(result != -1);
}

PyObject *py_tdb_hnd_unlock_bystring(PyObject *self, PyObject *args)
{
	tdb_hnd_object *obj = (tdb_hnd_object *)self;
	char *s;

        if (!obj->tdb) {
		PyErr_SetString(py_tdb_error, "tdb object has been closed"); 
		return NULL;
        }	

	if (!PyArg_ParseTuple(args, "s", &s))
		return NULL;

	tdb_unlock_bystring(obj->tdb, s);

	Py_INCREF(Py_None);
	return Py_None;
}

/* 
 * Method dispatch table for this module
 */

static PyMethodDef tdb_methods[] = {
	{ "open", (PyCFunction)py_tdb_open, METH_VARARGS | METH_KEYWORDS },
	{ "close", (PyCFunction)py_tdb_close, METH_VARARGS },
	{ NULL }
};

/* 
 * Methods on a tdb object
 */

static PyMethodDef tdb_hnd_methods[] = {
	{ "keys", (PyCFunction)py_tdb_hnd_keys, METH_VARARGS },
	{ "has_key", (PyCFunction)py_tdb_hnd_has_key, METH_VARARGS },
	{ "first_key", (PyCFunction)py_tdb_hnd_first_key, METH_VARARGS },
	{ "next_key", (PyCFunction)py_tdb_hnd_next_key, METH_VARARGS },
	{ "lock_all", (PyCFunction)py_tdb_hnd_lock_all, METH_VARARGS },
	{ "unlock_all", (PyCFunction)py_tdb_hnd_unlock_all, METH_VARARGS },
	{ "traverse", (PyCFunction)py_tdb_hnd_traverse, METH_VARARGS | METH_KEYWORDS },
	{ "chainlock", (PyCFunction)py_tdb_hnd_chainlock, METH_VARARGS | METH_KEYWORDS },
	{ "chainunlock", (PyCFunction)py_tdb_hnd_chainunlock, METH_VARARGS | METH_KEYWORDS },
	{ "lock_bystring", (PyCFunction)py_tdb_hnd_lock_bystring, METH_VARARGS | METH_KEYWORDS },
	{ "unlock_bystring", (PyCFunction)py_tdb_hnd_unlock_bystring, METH_VARARGS | METH_KEYWORDS },
	{ NULL }
};

/* Deallocate a tdb handle object */

static void tdb_hnd_dealloc(PyObject* self)
{
        tdb_hnd_object *hnd = (tdb_hnd_object *)self;

	if (hnd->tdb) {
		tdb_close(hnd->tdb);
		hnd->tdb = NULL;
	}
}

/* Return tdb handle attributes */

static PyObject *tdb_hnd_getattr(PyObject *self, char *attrname)
{
	return Py_FindMethod(tdb_hnd_methods, self, attrname);
}

static char tdb_hnd_type_doc[] = 
"Python wrapper for tdb.";

PyTypeObject tdb_hnd_type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"tdb",
	sizeof(tdb_hnd_object),
	0,
	tdb_hnd_dealloc,	/* tp_dealloc*/
	0,			/* tp_print*/
	tdb_hnd_getattr,	/* tp_getattr*/
	0,			/* tp_setattr*/
	0,			/* tp_compare*/
	0,			/* tp_repr*/
	0,			/* tp_as_number*/
	0,			/* tp_as_sequence*/
	&tdb_mapping,		/* tp_as_mapping*/
	0,			/* tp_hash */
	0,			/* tp_call */
	0,			/* tp_str */
	0,			/* tp_getattro */
	0,			/* tp_setattro */
	0,			/* tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,	/* tp_flags */
	tdb_hnd_type_doc,	/* tp_doc */
};

/* Constants */

static struct const_vals {
	char *name;
	uint32 value;
} module_const_vals[] = {

        /* Flags for tdb_open() */

	{ "TDB_DEFAULT", TDB_DEFAULT },
	{ "TDB_CLEAR_IF_FIRST", TDB_CLEAR_IF_FIRST },
	{ "TDB_INTERNAL", TDB_INTERNAL },
	{ "TDB_NOLOCK", TDB_NOLOCK },
	{ "TDB_NOMMAP", TDB_NOMMAP },
	{ "TDB_CONVERT", TDB_CONVERT },
	{ "TDB_BIGENDIAN", TDB_BIGENDIAN },
	
	{ NULL },
};

static void const_init(PyObject *dict)
{
	struct const_vals *tmp;
	PyObject *obj;

	for (tmp = module_const_vals; tmp->name; tmp++) {
		obj = PyInt_FromLong(tmp->value);
		PyDict_SetItemString(dict, tmp->name, obj);
		Py_DECREF(obj);
	}
}

/* Module initialisation */

void inittdb(void)
{
	PyObject *module, *dict;

	/* Initialise module */

	module = Py_InitModule("tdb", tdb_methods);
	dict = PyModule_GetDict(module);

	py_tdb_error = PyErr_NewException("tdb.error", NULL, NULL);
	PyDict_SetItemString(dict, "error", py_tdb_error);

	/* Initialise policy handle object */

	tdb_hnd_type.ob_type = &PyType_Type;

	PyDict_SetItemString(dict, "tdb.hnd", 
			     (PyObject *)&tdb_hnd_type);

	/* Initialise constants */

	const_init(dict);
}
