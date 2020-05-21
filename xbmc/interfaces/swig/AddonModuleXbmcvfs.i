/*
 *  Copyright (C) 2005-2018 Team Kodi
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

%include "interfaces/legacy/swighelper.h"
%include "interfaces/legacy/AddonString.h"

%feature("python:strictUnicode") XBMCAddon::xbmcvfs::File::read "true"

%include "interfaces/legacy/File.h"

%rename ("st_atime") XBMCAddon::xbmcvfs::Stat::atime;
%rename ("st_mtime") XBMCAddon::xbmcvfs::Stat::mtime;
%rename ("st_ctime") XBMCAddon::xbmcvfs::Stat::ctime;
%include "interfaces/legacy/Stat.h"

%rename ("delete") XBMCAddon::xbmcvfs::deleteFile;
%include "interfaces/legacy/ModuleXbmcvfs.h"

