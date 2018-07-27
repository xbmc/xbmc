/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

%begin %{
#if defined(TARGET_WINDOWS)
#  include <windows.h>
#endif

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

#if defined(__GNUG__)
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

