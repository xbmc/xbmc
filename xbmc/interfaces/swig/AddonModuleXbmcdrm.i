/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

%module xbmcdrm

%{
#if defined(TARGET_WINDOWS)
#  include <windows.h>
#endif

#include "interfaces/legacy/DrmCryptoSession.h"
#include "utils/log.h"

using namespace XBMCAddon;
using namespace xbmcdrm;

#if defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

%}

%include "kodi_common.i"

// construction in tp_new; see kodi_construct.i
KODI_CONSTRUCT(XBMCAddon::xbmcdrm, CryptoSession)

/* one line per shape crossing the boundary in this module */
%template() std::map<std::string, std::string>;

%include "interfaces/legacy/swighelper.h"
%include "interfaces/legacy/AddonString.h"
%include "interfaces/legacy/DrmCryptoSession.h"
