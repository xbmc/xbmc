/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "GUIInfoManager.h"
#include "GUIPassword.h"
#include "PartyModeManager.h"
#include "SectionLoader.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/DllLibCurl.h"
#include "utils/AlarmClock.h"
#include "utils/LangCodeExpander.h"
#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif

#include "filesystem/SpecialProtocol.h"
std::map<std::string, std::string> CSpecialProtocol::m_pathMap;

CLangCodeExpander g_LangCodeExpander;

XFILE::CDirectoryCache g_directoryCache;

CGUIPassword g_passwordManager;

XCURL::DllLibCurlGlobal g_curlInterface;
CPartyModeManager g_partyModeManager;

CAlarmClock g_alarmClock;
CSectionLoader g_sectionLoader;
