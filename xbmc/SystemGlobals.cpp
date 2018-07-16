/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "SectionLoader.h"
#include "utils/AlarmClock.h"
#include "GUIInfoManager.h"
#include "filesystem/DllLibCurl.h"
#include "filesystem/DirectoryCache.h"
#include "GUIPassword.h"
#include "utils/LangCodeExpander.h"
#include "PartyModeManager.h"
#include "guilib/LocalizeStrings.h"
#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif

// Guarantee that CSpecialProtocol is initialized before and uninitialized after ZipManager
#include "filesystem/SpecialProtocol.h"
std::map<std::string, std::string> CSpecialProtocol::m_pathMap;

#include "filesystem/ZipManager.h"

#ifdef TARGET_RASPBERRY_PI
#include "platform/linux/RBP.h"
#endif

  CLangCodeExpander  g_LangCodeExpander;
  CLocalizeStrings   g_localizeStrings;
  CLocalizeStrings   g_localizeStringsTemp;

  XFILE::CDirectoryCache g_directoryCache;

  CGUIPassword       g_passwordManager;

  XCURL::DllLibCurlGlobal g_curlInterface;
  CPartyModeManager     g_partyModeManager;

  CAlarmClock        g_alarmClock;
  CSectionLoader     g_sectionLoader;

#ifdef TARGET_RASPBERRY_PI
  CRBP               g_RBP;
#endif

  CZipManager g_ZipManager;
