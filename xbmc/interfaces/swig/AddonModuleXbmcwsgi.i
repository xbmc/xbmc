/*
 *  Copyright (C) 2015-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

%begin %{
#if defined(TARGET_WINDOWS)
#  include <windows.h>
#endif
%}

/* HAS_WEB_SERVER guarded the whole generated file through an %insert("footer")
   #endif. The python backend has no footer section, so the guard belongs in the
   build system: compile this module only when HAS_WEB_SERVER is set. */

%module xbmcwsgi

%{
#include "interfaces/legacy/wsgi/WsgiErrorStream.h"
#include "interfaces/legacy/wsgi/WsgiInputStream.h"
#include "interfaces/legacy/wsgi/WsgiResponse.h"
#include "interfaces/legacy/wsgi/WsgiResponseBody.h"

using namespace XBMCAddon;
using namespace xbmcwsgi;

#if defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

%}

%include "kodi_common.i"

// declared before the header that returns it, fully qualified, so SWIG
// records a name the registering module also uses
namespace XBMCAddon { namespace xbmcwsgi { class WsgiResponseBody; } }

// construction in tp_new; see kodi_construct.i
KODI_CONSTRUCT(XBMCAddon::xbmcwsgi, WsgiErrorStream)
KODI_CONSTRUCT(XBMCAddon::xbmcwsgi, WsgiInputStream)
KODI_CONSTRUCT(XBMCAddon::xbmcwsgi, WsgiInputStreamIterator)
KODI_CONSTRUCT(XBMCAddon::xbmcwsgi, WsgiResponse)
KODI_CONSTRUCT(XBMCAddon::xbmcwsgi, WsgiResponseBody)

/* one line per shape crossing the boundary in this module */
%template() std::vector<std::string>;
%template() XBMCAddon::Tuple<std::string, std::string>;
%template() std::vector<XBMCAddon::Tuple<std::string, std::string> >;

/* -builtin routes operator() through the tp_call slot, whose closure drops the
   keyword dict and calls a two-argument wrapper. */
%feature("kwargs", "0") XBMCAddon::xbmcwsgi::WsgiResponse::operator();
%feature("kwargs", "0") XBMCAddon::xbmcwsgi::WsgiResponseBody::operator();

%include "interfaces/legacy/swighelper.h"
%include "interfaces/legacy/AddonString.h"

%include "interfaces/legacy/wsgi/WsgiErrorStream.h"
// stock SWIG needs explicit tp_iter/tp_iternext slots for iteration
%feature("kwargs", "0") XBMCAddon::xbmcwsgi::WsgiInputStream::__iter__;
%feature("kwargs", "0") XBMCAddon::xbmcwsgi::WsgiInputStreamIterator::__iter__;
%feature("kwargs", "0") XBMCAddon::xbmcwsgi::WsgiInputStreamIterator::__next__;
%feature("python:slot", "tp_iter",     functype="getiterfunc") XBMCAddon::xbmcwsgi::WsgiInputStream::__iter__;
%feature("python:slot", "tp_iter",     functype="getiterfunc") XBMCAddon::xbmcwsgi::WsgiInputStreamIterator::__iter__;
%feature("python:slot", "tp_iternext", functype="iternextfunc") XBMCAddon::xbmcwsgi::WsgiInputStreamIterator::__next__;
%newobject XBMCAddon::xbmcwsgi::WsgiInputStream::__iter__;
%extend XBMCAddon::xbmcwsgi::WsgiInputStream {
  XBMCAddon::xbmcwsgi::WsgiInputStreamIterator* __iter__() { return $self->begin(); }
}
%extend XBMCAddon::xbmcwsgi::WsgiInputStreamIterator {
  PyObject* __iter__() {
    return SWIG_Python_NewPointerObj(NULL, $self,
             SWIGTYPE_p_XBMCAddon__xbmcwsgi__WsgiInputStreamIterator, 0);
  }
  PyObject* __next__() {
    if ($self->end()) { PyErr_SetNone(PyExc_StopIteration); return NULL; }
    std::string line = *(*$self);
    ++(*$self);
    return PyUnicode_DecodeUTF8(line.c_str(), line.size(), "surrogateescape");
  }
}

%include "interfaces/legacy/wsgi/WsgiInputStream.h"
// WsgiResponse::operator() returns WsgiResponseBody*, so that class must be
// wrapped before it is named, or SWIG has no class node and %feature("ref")
// never fires: the call operator then hands python an owned pointer it leaks.
%include "interfaces/legacy/wsgi/WsgiResponseBody.h"
%include "interfaces/legacy/wsgi/WsgiResponse.h"


// for HTTPPythonWsgiInvoker; %wrapper has C linkage (no overloads); owned!=0: dealloc Releases the caller's Acquire
%wrapper %{
extern "C" PyObject* KodiSwig_wrapWsgiResponse(XBMCAddon::xbmcwsgi::WsgiResponse* obj, int owned)
{
  return SWIG_Python_NewPointerObj(NULL, obj, SWIGTYPE_p_XBMCAddon__xbmcwsgi__WsgiResponse,
                                   owned ? SWIG_POINTER_OWN : 0);
}
extern "C" PyObject* KodiSwig_wrapWsgiInputStream(XBMCAddon::xbmcwsgi::WsgiInputStream* obj, int owned)
{
  return SWIG_Python_NewPointerObj(NULL, obj, SWIGTYPE_p_XBMCAddon__xbmcwsgi__WsgiInputStream,
                                   owned ? SWIG_POINTER_OWN : 0);
}
extern "C" PyObject* KodiSwig_wrapWsgiErrorStream(XBMCAddon::xbmcwsgi::WsgiErrorStream* obj, int owned)
{
  return SWIG_Python_NewPointerObj(NULL, obj, SWIGTYPE_p_XBMCAddon__xbmcwsgi__WsgiErrorStream,
                                   owned ? SWIG_POINTER_OWN : 0);
}
%}
