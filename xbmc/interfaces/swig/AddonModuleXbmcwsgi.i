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

%begin %{
#include "system.h"

#ifdef HAS_WEB_SERVER
%}

%module xbmcwsgi

%{
#include "interfaces/legacy/wsgi/WsgiErrorStream.h"
#include "interfaces/legacy/wsgi/WsgiInputStream.h"
#include "interfaces/legacy/wsgi/WsgiResponse.h"
#include "interfaces/legacy/wsgi/WsgiResponseBody.h"

using namespace XBMCAddon;
using namespace xbmcwsgi;

#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

%}

// This is all about warning suppression. It's OK that these base classes are 
// not part of what swig parses.
%feature("knownbasetypes") XBMCAddon::xbmcaddon "AddonClass"

%feature("iterator") WsgiInputStreamIterator "std::string"
%feature("iterable") WsgiInputStream "XBMCAddon::xbmcwsgi::WsgiInputStreamIterator"

%include "interfaces/legacy/swighelper.h"
%include "interfaces/legacy/AddonString.h"

%include "interfaces/legacy/wsgi/WsgiErrorStream.h"
%include "interfaces/legacy/wsgi/WsgiInputStream.h"
%include "interfaces/legacy/wsgi/WsgiResponse.h"
%include "interfaces/legacy/wsgi/WsgiResponseBody.h"

%insert("footer") %{
#endif
%}

