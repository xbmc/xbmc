/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

%{
#include "utils/log.h"
%}

/* 'None' is accepted as the empty string, and bytes are taken verbatim,
   including as a dict value or list element. */
//! @todo Drop the 'None' acceptance after v22; kodi_typing.i annotates these as str.
%fragment(SWIG_AsVal_frag(std::string), "header", fragment=SWIG_AsPtr_frag(std::string)) {
SWIGINTERN int
SWIG_AsVal_dec(std::string)(SWIG_Object obj, std::string* val)
{
  if (obj == Py_None)
  {
    CLog::Log(LOGWARNING, "Passing None where a string is expected is deprecated and might be "
                          "removed in future Kodi versions. Please pass an empty string instead.");
    if (val)
      val->clear();
    return SWIG_OK;
  }

  /* Stock SWIG_AsCharPtrAndSize takes bytes only when the whole binding is
     built for them, so a str is all it would accept here, while the typecheck
     typemap below offers this conversion bytes as well. Add-ons do pass them:
     anything storing binary through the string API, such as a pickle. */
  if (PyBytes_Check(obj))
  {
    if (val)
      val->assign(PyBytes_AS_STRING(obj), PyBytes_GET_SIZE(obj));
    return SWIG_OK;
  }

  std::string* v = 0;
  int res = SWIG_AsPtr(std::string)(obj, &v);
  if (!SWIG_IsOK(res))
    return res;
  if (v)
  {
    if (val)
      *val = *v;
    if (SWIG_IsNewObj(res))
    {
      delete v;
      res = SWIG_DelNewMask(res);
    }
    return res;
  }
  return SWIG_ERROR;
}
}

/* Fragments are first-definition-wins, typemaps last-declaration-wins. */
%include <std_string.i>

/* Kodi declares optional string parameters as `const String& x = emptyString`.
   Stock std_string.i routes `const std::string&` through %typemaps_asptrfromn
   (typemaps/ptrtypes.swg), whose in-typemap heap-allocates and whose freearg is
       if (SWIG_IsNewObj(res$argnum)) delete $1;
   Because the defaulted parameter starts out pointing at the emptyString global,
   the compiler cannot prove that delete is never reached with the global, and
   warns. The combination is in fact unreachable, but the build must be
   warning-free, so the argument is converted into a stack temporary instead. */
%typemap(in, fragment=SWIG_AsVal_frag(std::string)) const std::string & (std::string swig_temp)
{
  int swig_res = SWIG_AsVal_std_string($input, &swig_temp);
  if (!SWIG_IsOK(swig_res))
    SWIG_exception_fail(SWIG_ArgError(swig_res),
      "in method '$symname', argument $argnum of type '$type'");
  $1 = &swig_temp;
}
%typemap(typecheck, precedence=SWIG_TYPECHECK_STRING) const std::string & {
  $1 = ($input == Py_None || PyUnicode_Check($input) || PyBytes_Check($input)) ? 1 : 0;
}
%typemap(freearg) const std::string & ""

/* The same defect reaches container parameters that carry a default value.
   Exactly two exist in the API, both on Dialog, and both default to an empty
   vector, so the argument pointer starts out aimed at a stack temporary that
   the stock freearg would then delete. Convert into a local instead. */
%define %kodi_defaulted_container(TYPE...)
%typemap(in) const TYPE & (TYPE swig_temp) {
  TYPE *swig_p = 0;
  int swig_res = swig::asptr($input, &swig_p);
  if (!SWIG_IsOK(swig_res) || !swig_p)
    SWIG_exception_fail(SWIG_ArgError(SWIG_TypeError),
      "in method '$symname', argument $argnum of type '$type'");
  swig_temp = *swig_p;
  if (SWIG_IsNewObj(swig_res)) delete swig_p;
  $1 = &swig_temp;
}
%typemap(freearg) const TYPE & ""
%enddef
