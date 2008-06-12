/* -*- c-file-style: "python"; indent-tabs-mode: nil; -*-
	 
   Python wrapper for Samba tdb pack/unpack functions
   Copyright (C) Martin Pool 2002, 2003


   NOTE PYTHON STYLE GUIDE
   http://www.python.org/peps/pep-0007.html
   
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "Python.h"

/* This symbol is used in both config.h and Python.h which causes an
   annoying compiler warning. */

#ifdef HAVE_FSTAT
#undef HAVE_FSTAT
#endif

/* This module is supposed to be standalone, however for portability
   it would be good to use the FUNCTION_MACRO preprocessor define. */

#include "include/config.h"

#ifdef HAVE_FUNCTION_MACRO
#define FUNCTION_MACRO  (__FUNCTION__)
#else
#define FUNCTION_MACRO  (__FILE__)
#endif

static PyObject * pytdbpack_number(char ch, PyObject *val_iter, PyObject *packed_list);
static PyObject * pytdbpack_str(char ch,
				PyObject *val_iter, PyObject *packed_list,
				const char *encoding);
static PyObject * pytdbpack_buffer(PyObject *val_iter, PyObject *packed_list);

static PyObject *pytdbunpack_item(char, char **pbuf, int *plen, PyObject *);

static PyObject *pytdbpack_data(const char *format_str,
				     PyObject *val_seq,
				     PyObject *val_list);

static PyObject *
pytdbunpack_string(char **pbuf, int *plen, const char *encoding);

static void pack_le_uint32(unsigned long val_long, unsigned char *pbuf);


static PyObject *pytdbpack_bad_type(char ch,
				    const char *expected,
				    PyObject *val_obj);

static const char * pytdbpack_docstring =
"Convert between Python values and Samba binary encodings.\n"
"\n"
"This module is conceptually similar to the standard 'struct' module, but it\n"
"uses both a different binary format and a different description string.\n"
"\n"
"Samba's encoding is based on that used inside DCE-RPC and SMB: a\n"
"little-endian, unpadded, non-self-describing binary format.  It is intended\n"
"that these functions be as similar as possible to the routines in Samba's\n"
"tdb/tdbutil module, with appropriate adjustments for Python datatypes.\n"
"\n"
"Python strings are used to specify the format of data to be packed or\n"
"unpacked.\n"
"\n"
"String encodings are implied by the database format: they may be either DOS\n"
"codepage (currently hardcoded to 850), or Unix codepage (currently hardcoded\n"
"to be the same as the default Python encoding).\n"
"\n"
"tdbpack format strings:\n"
"\n"
"    'f': NUL-terminated string in codepage iso8859-1\n"
"   \n"
"    'P': same as 'f'\n"
"\n"
"    'F': NUL-terminated string in iso-8859-1\n"
"\n"
"    'd':  4 byte little-endian unsigned number\n"
"\n"
"    'w':  2 byte little-endian unsigned number\n"
"\n"
"    'P': \"Pointer\" value -- in the subset of DCERPC used by Samba, this is\n"
"          really just an \"exists\" or \"does not exist\" flag.  The boolean\n"
"          value of the Python object is used.\n"
"    \n"
"    'B': 4-byte LE length, followed by that many bytes of binary data.\n"
"         Corresponds to a Python integer giving the length, followed by a byte\n"
"         string of the appropriate length.\n"
"\n"
"    '$': Special flag indicating that the preceding format code should be\n"
"         repeated while data remains.  This is only supported for unpacking.\n"
"\n"
"    Every code corresponds to a single Python object, except 'B' which\n"
"    corresponds to two values (length and contents), and '$', which produces\n"
"    however many make sense.\n";

static char const pytdbpack_doc[] = 
"pack(format, values) -> buffer\n"
"Pack Python objects into Samba binary format according to format string.\n"
"\n"
"arguments:\n"
"    format -- string of tdbpack format characters\n"
"    values -- sequence of value objects corresponding 1:1 to format characters\n"
"\n"
"returns:\n"
"    buffer -- string containing packed data\n"
"\n"
"raises:\n"
"    IndexError -- if there are too few values for the format\n"
"    ValueError -- if any of the format characters is illegal\n"
"    TypeError  -- if the format is not a string, or values is not a sequence,\n"
"        or any of the values is of the wrong type for the corresponding\n"
"        format character\n"
"\n"
"notes:\n"
"    For historical reasons, it is not an error to pass more values than are consumed\n"
"    by the format.\n";


static char const pytdbunpack_doc[] =
"unpack(format, buffer) -> (values, rest)\n"
"Unpack Samba binary data according to format string.\n"
"\n"
"arguments:\n"
"    format -- string of tdbpack characters\n"
"    buffer -- string of packed binary data\n"
"\n"
"returns:\n"
"    2-tuple of:\n"
"        values -- sequence of values corresponding 1:1 to format characters\n"
"        rest -- string containing data that was not decoded, or '' if the\n"
"            whole string was consumed\n"
"\n"
"raises:\n"
"    IndexError -- if there is insufficient data in the buffer for the\n"
"        format (or if the data is corrupt and contains a variable-length\n"
"        field extending past the end)\n"
"    ValueError -- if any of the format characters is illegal\n"
"\n"
"notes:\n"
"    Because unconsumed data is returned, you can feed it back in to the\n"
"    unpacker to extract further fields.  Alternatively, if you wish to modify\n"
"    some fields near the start of the data, you may be able to save time by\n"
"    only unpacking and repacking the necessary part.\n";


const char *pytdb_dos_encoding = "cp850";

/* NULL, meaning that the Samba default encoding *must* be the same as the
   Python default encoding. */
const char *pytdb_unix_encoding = NULL;


/*
  * Pack objects to bytes.
  *
  * All objects are first individually encoded onto a list, and then the list
  * of strings is concatenated.  This is faster than concatenating strings,
  * and reasonably simple to code.
  */
static PyObject *
pytdbpack(PyObject *self,
	       PyObject *args)
{
	char *format_str;
	PyObject *val_seq, *val_iter = NULL,
		*packed_list = NULL, *packed_str = NULL,
		*empty_str = NULL;

	/* TODO: Test passing wrong types or too many arguments */
	if (!PyArg_ParseTuple(args, "sO", &format_str, &val_seq))
		return NULL;

	if (!(val_iter = PyObject_GetIter(val_seq)))
		goto out;

	/* Create list to hold strings until we're done, then join them all. */
	if (!(packed_list = PyList_New(0)))
		goto out;

	if (!pytdbpack_data(format_str, val_iter, packed_list))
		goto out;

	/* this function is not officially documented but it works */
	if (!(empty_str = PyString_InternFromString("")))
		goto out;
	
	packed_str = _PyString_Join(empty_str, packed_list);

  out:
	Py_XDECREF(empty_str);
	Py_XDECREF(val_iter);
	Py_XDECREF(packed_list);

	return packed_str;
}


/*
  Pack data according to FORMAT_STR from the elements of VAL_SEQ into
  PACKED_BUF.

  The string has already been checked out, so we know that VAL_SEQ is large
  enough to hold the packed data, and that there are enough value items.
  (However, their types may not have been thoroughly checked yet.)

  In addition, val_seq is a Python Fast sequence.

  Returns NULL for error (with exception set), or None.
*/
PyObject *
pytdbpack_data(const char *format_str,
		    PyObject *val_iter,
		    PyObject *packed_list)
{
	int format_i, val_i = 0;

	for (format_i = 0, val_i = 0; format_str[format_i]; format_i++) {
		char ch = format_str[format_i];

		switch (ch) {
			/* dispatch to the appropriate packer for this type,
			   which should pull things off the iterator, and
			   append them to the packed_list */
		case 'w':
		case 'd':
		case 'p':
			if (!(packed_list = pytdbpack_number(ch, val_iter, packed_list)))
				return NULL;
			break;

		case 'f':
		case 'P':
			if (!(packed_list = pytdbpack_str(ch, val_iter, packed_list, pytdb_unix_encoding)))
				return NULL;
			break;

		case 'B':
			if (!(packed_list = pytdbpack_buffer(val_iter, packed_list)))
				return NULL;
			break;

		default:
			PyErr_Format(PyExc_ValueError,
				     "%s: format character '%c' is not supported",
				     FUNCTION_MACRO, ch);
			return NULL;
		}
	}

	return packed_list;
}


static PyObject *
pytdbpack_number(char ch, PyObject *val_iter, PyObject *packed_list)
{
	unsigned long val_long;
	PyObject *val_obj = NULL, *long_obj = NULL, *result_obj = NULL;
	PyObject *new_list = NULL;
	unsigned char pack_buf[4];

	if (!(val_obj = PyIter_Next(val_iter)))
		goto out;

	if (!(long_obj = PyNumber_Long(val_obj))) {
		pytdbpack_bad_type(ch, "Number", val_obj);
		goto out;
	}

	val_long = PyLong_AsUnsignedLong(long_obj);
	pack_le_uint32(val_long, pack_buf);

	/* pack as 32-bit; if just packing a 'w' 16-bit word then only take
	   the first two bytes. */
	
	if (!(result_obj = PyString_FromStringAndSize(pack_buf, ch == 'w' ? 2 : 4)))
		goto out;

	if (PyList_Append(packed_list, result_obj) != -1)
		new_list = packed_list;

  out:
	Py_XDECREF(val_obj);
	Py_XDECREF(long_obj);
	Py_XDECREF(result_obj);

	return new_list;
}


/*
 * Take one string from the iterator val_iter, convert it to 8-bit, and return
 * it.
 *
 * If the input is neither a string nor Unicode, an exception is raised.
 *
 * If the input is Unicode, then it is converted to the appropriate encoding.
 *
 * If the input is a String, and encoding is not null, then it is converted to
 * Unicode using the default decoding method, and then converted to the
 * encoding.  If the encoding is NULL, then the string is written out as-is --
 * this is used when the default Python encoding is the same as the Samba
 * encoding.
 *
 * I hope this approach avoids being too fragile w.r.t. being passed either
 * Unicode or String objects.
 */
static PyObject *
pytdbpack_str(char ch,
	      PyObject *val_iter, PyObject *packed_list, const char *encoding)
{
	PyObject *val_obj = NULL;
	PyObject *unicode_obj = NULL;
	PyObject *coded_str = NULL;
	PyObject *nul_str = NULL;
	PyObject *new_list = NULL;

	if (!(val_obj = PyIter_Next(val_iter)))
		goto out;

	if (PyUnicode_Check(val_obj)) {
		if (!(coded_str = PyUnicode_AsEncodedString(val_obj, encoding, NULL)))
			goto out;
	}
	else if (PyString_Check(val_obj) && !encoding) {
		/* For efficiency, we assume that the Python interpreter has
		   the same default string encoding as Samba's native string
		   encoding.  On the PSA, both are always 8859-1. */
		coded_str = val_obj;
		Py_INCREF(coded_str);
	}
	else if (PyString_Check(val_obj)) {
		/* String, but needs to be converted */
		if (!(unicode_obj = PyString_AsDecodedObject(val_obj, NULL, NULL)))
			goto out;
		if (!(coded_str = PyUnicode_AsEncodedString(unicode_obj, encoding, NULL)))
			goto out;
	}
	else {
		pytdbpack_bad_type(ch, "String or Unicode", val_obj);
		goto out;
	}

	if (!nul_str)
		/* this is constant and often-used; hold it forever */
		if (!(nul_str = PyString_FromStringAndSize("", 1)))
			goto out;

	if ((PyList_Append(packed_list, coded_str) != -1)
	    && (PyList_Append(packed_list, nul_str) != -1))
		new_list = packed_list;

  out:
	Py_XDECREF(val_obj);
	Py_XDECREF(unicode_obj);
	Py_XDECREF(coded_str);

	return new_list;
}


/*
 * Pack (LENGTH, BUFFER) pair onto the list.
 *
 * The buffer must already be a String, not Unicode, because it contains 8-bit
 * untranslated data.  In some cases it will actually be UTF_16_LE data.
 */
static PyObject *
pytdbpack_buffer(PyObject *val_iter, PyObject *packed_list)
{
	PyObject *val_obj;
	PyObject *new_list = NULL;
	
	/* pull off integer and stick onto list */
	if (!(packed_list = pytdbpack_number('d', val_iter, packed_list)))
		return NULL;

	/* this assumes that the string is the right length; the old code did
	   the same. */
	if (!(val_obj = PyIter_Next(val_iter)))
		return NULL;

	if (!PyString_Check(val_obj)) {
		pytdbpack_bad_type('B', "String", val_obj);
		goto out;
	}
	
	if (PyList_Append(packed_list, val_obj) != -1)
		new_list = packed_list;

  out:
	Py_XDECREF(val_obj);
	return new_list;
}


static PyObject *pytdbpack_bad_type(char ch,
				    const char *expected,
				    PyObject *val_obj)
{
	PyObject *r = PyObject_Repr(val_obj);
	if (!r)
		return NULL;
	PyErr_Format(PyExc_TypeError,
		     "tdbpack: format '%c' requires %s, not %s",
		     ch, expected, PyString_AS_STRING(r));
	Py_DECREF(r);
	return val_obj;
}


/*
  XXX: glib and Samba have quicker macro for doing the endianness conversions,
  but I don't know of one in plain libc, and it's probably not a big deal.  I
  realize this is kind of dumb because we'll almost always be on x86, but
  being safe is important.
*/
static void pack_le_uint32(unsigned long val_long, unsigned char *pbuf)
{
	pbuf[0] =         val_long & 0xff;
	pbuf[1] = (val_long >> 8)  & 0xff;
	pbuf[2] = (val_long >> 16) & 0xff;
	pbuf[3] = (val_long >> 24) & 0xff;
}


#if 0	/* not used */
static void pack_bytes(long len, const char *from,
		       unsigned char **pbuf)
{
	memcpy(*pbuf, from, len);
	(*pbuf) += len;
}
#endif


static PyObject *
pytdbunpack(PyObject *self,
		 PyObject *args)
{
	char *format_str, *packed_str, *ppacked;
	PyObject *val_list = NULL, *ret_tuple = NULL;
	PyObject *rest_string = NULL;
	int format_len, packed_len;
	char last_format = '#';	/* invalid */
	int i;
	
	/* get arguments */
	if (!PyArg_ParseTuple(args, "ss#", &format_str, &packed_str, &packed_len))
		return NULL;

	format_len = strlen(format_str);
	
	/* Allocate list to hold results.  Initially empty, and we append
	   results as we go along. */
	val_list = PyList_New(0);
	if (!val_list)
		goto failed;
	ret_tuple = PyTuple_New(2);
	if (!ret_tuple)
		goto failed;
	
	/* For every object, unpack.  */
	for (ppacked = packed_str, i = 0; i < format_len && format_str[i] != '$'; i++) {
		last_format = format_str[i];
		/* packed_len is reduced in place */
		if (!pytdbunpack_item(format_str[i], &ppacked, &packed_len, val_list))
			goto failed;
	}

	/* If the last character was '$', keep going until out of space */
	if (format_str[i] == '$') {
		if (i == 0) {
			PyErr_Format(PyExc_ValueError,
				     "%s: '$' may not be first character in format",
				     FUNCTION_MACRO);
			return NULL;
		} 
		while (packed_len > 0)
			if (!pytdbunpack_item(last_format, &ppacked, &packed_len, val_list))
				goto failed;
	}
	
	/* save leftovers for next time */
	rest_string = PyString_FromStringAndSize(ppacked, packed_len);
	if (!rest_string)
		goto failed;

	/* return (values, rest) tuple; give up references to them */
	PyTuple_SET_ITEM(ret_tuple, 0, val_list);
	val_list = NULL;
	PyTuple_SET_ITEM(ret_tuple, 1, rest_string);
	val_list = NULL;
	return ret_tuple;

  failed:
	/* handle failure: deallocate anything.  XDECREF forms handle NULL
	   pointers for objects that haven't been allocated yet. */
	Py_XDECREF(val_list);
	Py_XDECREF(ret_tuple);
	Py_XDECREF(rest_string);
	return NULL;
}


static void
pytdbunpack_err_too_short(void)
{
	PyErr_Format(PyExc_IndexError,
		     "%s: data too short for unpack format", FUNCTION_MACRO);
}


static PyObject *
pytdbunpack_uint32(char **pbuf, int *plen)
{
	unsigned long v;
	unsigned char *b;
	
	if (*plen < 4) {
		pytdbunpack_err_too_short();
		return NULL;
	}

	b = *pbuf;
	v = b[0] | b[1]<<8 | b[2]<<16 | b[3]<<24;
	
	(*pbuf) += 4;
	(*plen) -= 4;

	return PyLong_FromUnsignedLong(v);
}


static PyObject *pytdbunpack_int16(char **pbuf, int *plen)
{
	long v;
	unsigned char *b;
	
	if (*plen < 2) {
		pytdbunpack_err_too_short();
		return NULL;
	}

	b = *pbuf;
	v = b[0] | b[1]<<8;
	
	(*pbuf) += 2;
	(*plen) -= 2;

	return PyInt_FromLong(v);
}


static PyObject *
pytdbunpack_string(char **pbuf, int *plen, const char *encoding)
{
	int len;
	char *nul_ptr, *start;

	start = *pbuf;
	
	nul_ptr = memchr(start, '\0', *plen);
	if (!nul_ptr) {
		pytdbunpack_err_too_short();
		return NULL;
	}

	len = nul_ptr - start;

	*pbuf += len + 1;	/* skip \0 */
	*plen -= len + 1;

	return PyString_Decode(start, len, encoding, NULL);
}


static PyObject *
pytdbunpack_buffer(char **pbuf, int *plen, PyObject *val_list)
{
	/* first get 32-bit len */
	long slen;
	unsigned char *b;
	unsigned char *start;
	PyObject *str_obj = NULL, *len_obj = NULL;
	
	if (*plen < 4) {
		pytdbunpack_err_too_short();
		return NULL;
	}
	
	b = *pbuf;
	slen = b[0] | b[1]<<8 | b[2]<<16 | b[3]<<24;

	if (slen < 0) { /* surely you jest */
		PyErr_Format(PyExc_ValueError,
			     "%s: buffer seems to have negative length", FUNCTION_MACRO);
		return NULL;
	}

	(*pbuf) += 4;
	(*plen) -= 4;
	start = *pbuf;

	if (*plen < slen) {
		PyErr_Format(PyExc_IndexError,
			     "%s: not enough data to unpack buffer: "
			     "need %d bytes, have %d", FUNCTION_MACRO,
			     (int) slen, *plen);
		return NULL;
	}

	(*pbuf) += slen;
	(*plen) -= slen;

	if (!(len_obj = PyInt_FromLong(slen)))
		goto failed;

	if (PyList_Append(val_list, len_obj) == -1)
		goto failed;
	
	if (!(str_obj = PyString_FromStringAndSize(start, slen)))
		goto failed;
	
	if (PyList_Append(val_list, str_obj) == -1)
		goto failed;
	
	Py_DECREF(len_obj);
	Py_DECREF(str_obj);
	
	return val_list;

  failed:
	Py_XDECREF(len_obj);	/* handles NULL */
	Py_XDECREF(str_obj);
	return NULL;
}


/* Unpack a single field from packed data, according to format character CH.
   Remaining data is at *PBUF, of *PLEN.

   *PBUF is advanced, and *PLEN reduced to reflect the amount of data that has
   been consumed.

   Returns a reference to None, or NULL for failure.
*/
static PyObject *pytdbunpack_item(char ch,
				  char **pbuf,
				  int *plen,
				  PyObject *val_list)
{
	PyObject *unpacked;
	
	if (ch == 'w') {	/* 16-bit int */
		unpacked = pytdbunpack_int16(pbuf, plen);
	}
	else if (ch == 'd' || ch == 'p') { /* 32-bit int */
		/* pointers can just come through as integers */
		unpacked = pytdbunpack_uint32(pbuf, plen);
	}
	else if (ch == 'f' || ch == 'P') { /* nul-term string  */
		unpacked = pytdbunpack_string(pbuf, plen, pytdb_unix_encoding);
	}
	else if (ch == 'B') { /* length, buffer */
		return pytdbunpack_buffer(pbuf, plen, val_list);
	}
	else {
		PyErr_Format(PyExc_ValueError,
			     "%s: format character '%c' is not supported", 
                             FUNCTION_MACRO, ch);
		
		return NULL;
	}

	/* otherwise OK */
	if (!unpacked)
		return NULL;

	if (PyList_Append(val_list, unpacked) == -1)
		val_list = NULL;

	/* PyList_Append takes a new reference to the inserted object.
	   Therefore, we no longer need the original reference. */
	Py_DECREF(unpacked);
	
	return val_list;
}






static PyMethodDef pytdbpack_methods[] = {
	{ "pack", pytdbpack, METH_VARARGS, (char *) pytdbpack_doc },
	{ "unpack", pytdbunpack, METH_VARARGS, (char *) pytdbunpack_doc },
};

DL_EXPORT(void)
inittdbpack(void)
{
	Py_InitModule3("tdbpack", pytdbpack_methods,
		       (char *) pytdbpack_docstring);
}
