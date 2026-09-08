/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/* Kodi declares optional string parameters as `const String& x = emptyString`.
   Stock std_string.i routes `const std::string&` through %typemaps_asptrfromn
   (typemaps/ptrtypes.swg), whose in-typemap heap-allocates and whose freearg is
       if (SWIG_IsNewObj(res$argnum)) delete $1;
   Because the defaulted parameter starts out pointing at the emptyString global,
   the compiler cannot prove that delete is never reached with the global, and
   warns. The combination is in fact unreachable, but the build must be
   warning-free, so the argument is converted into a stack temporary instead:
   nothing is allocated, nothing is freed, and the freearg goes away.
   SWIG_AsVal_std_string is stock, supplied by std_string.i. */
%typemap(in, fragment=SWIG_AsVal_frag(std::string)) const std::string & (std::string swig_temp)
{
  /* The shipped bindings coerce None to the empty string (PyXBMCGetUnicodeString
     returns XBMCAddon::emptyString for Py_None). Scrapers rely on it: they pass
     dict.get(...) straight into setters, and a missing key is None. */
  if ($input != Py_None)
  {
    int swig_res = SWIG_AsVal_std_string($input, &swig_temp);
    if (!SWIG_IsOK(swig_res))
      SWIG_exception_fail(SWIG_ArgError(swig_res),
        "in method '$symname', argument $argnum of type '$type'");
  }
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
