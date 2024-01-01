/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

%module xbmcaddon

%{
#if defined(TARGET_WINDOWS)
#  include <windows.h>
#endif

#include "interfaces/legacy/Addon.h"
#include "interfaces/legacy/Settings.h"

using namespace XBMCAddon;
using namespace xbmcaddon;

#if defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

%}

// This is all about warning suppression. It's OK that these base classes are
// not part of what swig parses.
%feature("knownbasetypes") XBMCAddon::xbmcaddon "AddonClass"

%include "interfaces/legacy/swighelper.h"
%include "interfaces/legacy/AddonString.h"

%include "interfaces/legacy/Addon.h"
%nodefaultctor Settings;
%include "interfaces/legacy/Settings.h"

