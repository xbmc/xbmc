/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OSScreenSaverFreedesktop.h"

#include "CompileInfo.h"
#include "guilib/LocalizeStrings.h"

#include <sdbus-c++/sdbus-c++.h>

using namespace KODI::WINDOWING::LINUX;

namespace
{
constexpr auto SCREENSAVER_DEST{"org.freedesktop.ScreenSaver"};
constexpr auto SCREENSAVER_OBJECT{"/org/freedesktop/ScreenSaver"};
constexpr auto SCREENSAVER_INTERFACE{"org.freedesktop.ScreenSaver"};
}

bool COSScreenSaverFreedesktop::IsAvailable()
{
  bool available;
  auto proxy = sdbus::createProxy(SCREENSAVER_DEST, SCREENSAVER_OBJECT);

  try
  {
    proxy->callMethod("GetActive").onInterface(SCREENSAVER_INTERFACE).storeResultsTo(available);
  }
  catch (const sdbus::Error& e)
  {
    available = false;
  }

  // We do not care whether GetActive() is actually supported, we're just checking for the name to be there
  // (GNOME for example does not support GetActive)
  return available;
}

COSScreenSaverFreedesktop::COSScreenSaverFreedesktop()
{
  m_proxy = sdbus::createProxy(SCREENSAVER_DEST, SCREENSAVER_OBJECT);
}

void COSScreenSaverFreedesktop::Inhibit()
{
  if (m_inhibited)
    return;

  m_proxy->callMethod("Inhibit")
      .onInterface(SCREENSAVER_INTERFACE)
      .withArguments(CCompileInfo::GetAppName(), g_localizeStrings.Get(14086))
      .storeResultsTo(m_cookie);

  m_inhibited = true;
}

void COSScreenSaverFreedesktop::Uninhibit()
{
  if (!m_inhibited)
  {
    return;
  }

  m_proxy->callMethod("Inhibit").onInterface(SCREENSAVER_INTERFACE).withArguments(m_cookie);

  m_inhibited = false;
}
