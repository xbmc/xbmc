/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/* XbmcCommons::Buffer is a leaf type: it does not recurse, so it needs only a
   traits specialisation, not a template pattern. Behaviour copied from
   xbmc/interfaces/python/typemaps/python.buffer.intm and python.buffer.outtm:
   accept str / bytes / bytearray on input, always yield a bytearray on output. */

%fragment(SWIG_Traits_frag(XbmcCommons::Buffer), "header", fragment="StdTraits") {
namespace swig {
  template <> struct traits< XbmcCommons::Buffer > {
    typedef value_category category;
    static const char* type_name() { return "XbmcCommons::Buffer"; }
  };

  template <> struct traits_from< XbmcCommons::Buffer > {
    static PyObject *from(const XbmcCommons::Buffer& b) {
      XbmcCommons::Buffer& nb = const_cast<XbmcCommons::Buffer&>(b);
      return PyByteArray_FromStringAndSize((char*)nb.curPosition(), nb.remaining());
    }
  };

  template <> struct traits_asval< XbmcCommons::Buffer > {
    static int asval(PyObject *obj, XbmcCommons::Buffer *val) {
      const char *str = 0;
      Py_ssize_t size = 0;
      if (PyUnicode_Check(obj))       { str = PyUnicode_AsUTF8AndSize(obj, &size); }
      else if (PyBytes_Check(obj))    { str = PyBytes_AS_STRING(obj); size = PyBytes_GET_SIZE(obj); }
      else if (PyByteArray_Check(obj)){ str = PyByteArray_AsString(obj); size = PyByteArray_Size(obj); }
      else return SWIG_TypeError;
      if (!str) return SWIG_TypeError;
      if (val) {
        val->allocate((size_t)size);
        val->put(str, (size_t)size);
        val->flip();
      }
      return SWIG_OK;
    }
  };
}
}

%typemap_traits(SWIG_TYPECHECK_STRING, XbmcCommons::Buffer);

/* File::write takes a non-const Buffer&, which %typemap_traits does not cover
   (it generates Type and const Type& only), so it fell through to the opaque
   pointer typemap and python could not call write() at all. The buffer is
   read by the callee, so a stack temporary is sufficient. */
%typemap(in, fragment=SWIG_Traits_frag(XbmcCommons::Buffer))
  XbmcCommons::Buffer & (XbmcCommons::Buffer swig_temp)
{
  int swig_res = swig::traits_asval<XbmcCommons::Buffer>::asval($input, &swig_temp);
  if (!SWIG_IsOK(swig_res))
    SWIG_exception_fail(SWIG_ArgError(swig_res),
      "in method '$symname', argument $argnum of type '$type'");
  $1 = &swig_temp;
}
%typemap(typecheck, precedence=SWIG_TYPECHECK_STRING) XbmcCommons::Buffer & {
  $1 = (PyBytes_Check($input) || PyByteArray_Check($input) || PyUnicode_Check($input)) ? 1 : 0;
}
%typemap(freearg) XbmcCommons::Buffer & ""
