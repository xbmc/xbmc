/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

%module xbmcplugin

%{
#if defined(TARGET_WINDOWS)
#  include <windows.h>
#endif

#include "interfaces/legacy/ModuleXbmcplugin.h"

using namespace XBMCAddon;
using namespace xbmcplugin;

#if defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

%}

%include "kodi_common.i"

namespace XBMCAddon { namespace xbmcgui { class ListItem; } }
%traits_swigtype(XBMCAddon::xbmcgui::ListItem);
%fragment(SWIG_Traits_frag(XBMCAddon::xbmcgui::ListItem));

/* one line per shape crossing the boundary in this module */
%template() XBMCAddon::Tuple<std::string, XBMCAddon::xbmcgui::ListItem const*, bool>;
%template() std::vector<XBMCAddon::Tuple<std::string, XBMCAddon::xbmcgui::ListItem const*, bool> >;

%include "interfaces/legacy/swighelper.h"
%include "interfaces/legacy/AddonString.h"
%include "interfaces/legacy/ModuleXbmcplugin.h"

