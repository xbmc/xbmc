/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/* Everything every Kodi python module needs, in the one order that works.
   Include this before any %include of a Kodi header. */

/* 0. Keep SWIG plumbing and Kodi internals out of the module namespace.
      Without these, every module publishes SwigPyIterator (SWIG's runtime
      iterator type), cvar (SWIG's variable-link container), and the sentinel
      and internal types that Tuple.h and AddonString.h declare. None of them
      were ever part of the addon API. Anonymous %template() covers the rest by
      installing typemaps without creating a python class. */
%ignore swig::SwigPyIterator;
%ignore XBMCAddon::TupleBase;
%ignore XBMCAddon::tuple_null_type;
%ignore XBMCAddon::emptyString;


/* 1. python list (not tuple) for sequence returns. MUST come before any stock
      include, because SWIG fragments are first-definition-wins. */
%include "kodi_list.i"

/* 2. stock library */
%include <std_string.i>
%include "kodi_string.i"

/* 3. Kodi's generic vocabulary types: Tuple, Alternative, Dictionary */
%include "kodi_generics.i"

/* 4. leaf types */
%{
#include "commons/Buffer.h"
%}
%include "kodi_buffer.i"

/* 5. most-derived return type via RTTI */
%include "kodi_dyncast.i"

/* 6. build-time failure for any unconverted container */
%include "kodi_guard.i"

/* 7b. director callbacks must not let a python exception reach std::terminate */
%include "kodi_addonclass.i"
%include "kodi_construct.i"

/* 7d. PEP 484 annotations for the -pyi stubs */
%include "kodi_typing.i"

/* 7c. The Groovy generator published these five per module via
   PyModule_AddStringConstant. No bundled addon reads them, but they are cheap
   and third-party code may. Values copied from PythonSwig.cpp.template. */
%{
#include "CompileInfo.h"
%}
%init %{
  PyModule_AddStringConstant(m, "__author__", "Team Kodi <http://kodi.tv>");
  PyModule_AddStringConstant(m, "__date__", CCompileInfo::GetBuildDate().c_str());
  PyModule_AddStringConstant(m, "__version__", "3.0.2");
  PyModule_AddStringConstant(m, "__credits__", "Team Kodi");
  PyModule_AddStringConstant(m, "__platform__", "ALL");
%}

/* 7. every addon call site uses keyword arguments */
%feature("kwargs");

/* 8. commons/Exception.h is not %included, so SWIG would otherwise parse each
      XBMCCOMMONS_STANDARD_EXCEPTION(E) as a global of an undeclared type.
      The exception classes are not part of the python API. */
#define XBMCCOMMONS_STANDARD_EXCEPTION(E)


namespace XBMCAddon
{
typedef std::string String;
typedef String StringOrInt;
}
