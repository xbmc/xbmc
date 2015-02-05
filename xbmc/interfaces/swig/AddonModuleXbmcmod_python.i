/*
 *      Copyright (C) 2015 Team XBMC
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

%{
#include "system.h"

#ifdef HAS_WEB_SERVER
%}

%module xbmcmod_python

%{
#include "interfaces/legacy/mod_python/Cookie.h"
#include "interfaces/legacy/mod_python/HttpRequest.h"
#include "interfaces/legacy/mod_python/ModuleXbmcmod_python.h"

using namespace XBMCAddon;
using namespace xbmcmod_python;

#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

%}

// This is all about warning suppression. It's OK that these base classes are 
// not part of what swig parses.
%feature("knownbasetypes") XBMCAddon::xbmcaddon "AddonClass"

%feature("sequence") Table "contains";

%include "interfaces/legacy/swighelper.h"
%include "interfaces/legacy/AddonString.h"

%include "interfaces/legacy/mod_python/ModuleXbmcmod_python.h"

%include "interfaces/legacy/mod_python/HttpRequest.h"
%include "interfaces/legacy/mod_python/Cookie.h"

%{
#endif
%}

