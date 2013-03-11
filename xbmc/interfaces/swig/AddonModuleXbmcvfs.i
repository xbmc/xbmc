/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

%module xbmcvfs

%{
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

%include "interfaces/legacy/File.h"

%rename ("st_atime") XBMCAddon::xbmcvfs::Stat::atime;
%rename ("st_mtime") XBMCAddon::xbmcvfs::Stat::mtime;
%rename ("st_ctime") XBMCAddon::xbmcvfs::Stat::ctime;
%include "interfaces/legacy/Stat.h"

%rename ("delete") XBMCAddon::xbmcvfs::deleteFile;
%include "interfaces/legacy/ModuleXbmcvfs.h"

