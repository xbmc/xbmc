/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

%module xbmcvfs

%{
#if defined(TARGET_WINDOWS)
#  include <windows.h>
#endif

#include "interfaces/legacy/ModuleXbmcvfs.h"
#include "interfaces/legacy/File.h"
#include "interfaces/legacy/Stat.h"
#include "utils/log.h"

using namespace XBMCAddon;
using namespace xbmcvfs;

#if defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

%}


%include "kodi_common.i"

// construction in tp_new; see kodi_construct.i
KODI_CONSTRUCT(XBMCAddon::xbmcvfs, File)
KODI_CONSTRUCT(XBMCAddon::xbmcvfs, Stat)

/* one line per shape crossing the boundary in this module */
%template() std::vector<std::string>;
%template() XBMCAddon::Tuple<std::vector<std::string>, std::vector<std::string> >;

%include "interfaces/legacy/swighelper.h"
%include "interfaces/legacy/AddonString.h"

// The shipped bindings decode File.read strictly (%feature("python:strictUnicode"),
// a Kodi-invented feature stock SWIG drops). Stock SWIG_FromCharPtrAndSize uses
// surrogateescape, which turns binary content into surrogates instead of raising,
// so a caller reading a non-text file gets silent mojibake rather than an error.
%typemap(out) XBMCAddon::String XBMCAddon::xbmcvfs::File::read {
  $result = PyUnicode_DecodeUTF8($1.c_str(), (Py_ssize_t)$1.size(), "strict");
  if (!$result)
    SWIG_fail;
}

%include "interfaces/legacy/File.h"

%rename ("st_atime") XBMCAddon::xbmcvfs::Stat::atime;
%rename ("st_mtime") XBMCAddon::xbmcvfs::Stat::mtime;
%rename ("st_ctime") XBMCAddon::xbmcvfs::Stat::ctime;
%include "interfaces/legacy/Stat.h"

%rename ("delete") XBMCAddon::xbmcvfs::deleteFile;
%include "interfaces/legacy/ModuleXbmcvfs.h"

