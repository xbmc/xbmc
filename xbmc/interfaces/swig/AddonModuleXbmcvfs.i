/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

%module xbmcvfs

%{
#if defined(TARGET_WINDOWS) || defined(TARGET_WIN10)
#  if !defined(WIN32_LEAN_AND_MEAN)
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

#include "interfaces/legacy/ModuleXbmcvfs.h"
#include "interfaces/legacy/File.h"
#include "interfaces/legacy/Stat.h"
#include "utils/log.h"

using namespace XBMCAddon;
using namespace xbmcvfs;

#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

%}

%include "interfaces/legacy/swighelper.h"
%include "interfaces/legacy/AddonString.h"
%include "interfaces/legacy/File.h"

%rename ("st_atime") XBMCAddon::xbmcvfs::Stat::atime;
%rename ("st_mtime") XBMCAddon::xbmcvfs::Stat::mtime;
%rename ("st_ctime") XBMCAddon::xbmcvfs::Stat::ctime;
%include "interfaces/legacy/Stat.h"

%rename ("delete") XBMCAddon::xbmcvfs::deleteFile;
%include "interfaces/legacy/ModuleXbmcvfs.h"

