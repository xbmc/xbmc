/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIPassword.h"
#include "PartyModeManager.h"
#include "SectionLoader.h"
#include "filesystem/BlurayDiscCache.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/DllLibCurl.h"
#include "guilib/LocalizeStrings.h"
#include "utils/AlarmClock.h"
#include "utils/LangCodeExpander.h"
#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif

// Guarantee that CSpecialProtocol is initialized before and uninitialized after ZipManager
#include "filesystem/SpecialProtocol.h"
std::map<std::string, std::string> CSpecialProtocol::m_pathMap;

#include "filesystem/ZipManager.h"

CLangCodeExpander g_LangCodeExpander;
CLocalizeStrings g_localizeStrings;
CLocalizeStrings g_localizeStringsTemp;

XFILE::CDirectoryCache g_directoryCache;

#ifdef HAVE_LIBBLURAY
XFILE::CBlurayDiscCache g_blurayDiscCache;
#endif

CGUIPassword g_passwordManager;

XCURL::DllLibCurlGlobal g_curlInterface;
CPartyModeManager g_partyModeManager;

CAlarmClock g_alarmClock;
CSectionLoader g_sectionLoader;

CZipManager g_ZipManager;