/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OSScreenSaverX11.h"

#include "utils/log.h"
#include "windowing/linux/OSScreenSaverFreedesktop.h"

#include <cassert>

using namespace std::chrono_literals;

COSScreenSaverX11::COSScreenSaverX11(Display* dpy)
: m_dpy(dpy), m_screensaverResetTimer(std::bind(&COSScreenSaverX11::ResetScreenSaver, this))
{
  assert(m_dpy);

#if defined(HAS_DBUS)
  // Some session services, for example Steam on SteamOS, ignore XResetScreenSaver()
  if (KODI::WINDOWING::LINUX::COSScreenSaverFreedesktop::IsAvailable())
  {
    CLog::LogF(LOGINFO, "Also using freedesktop.org DBus interface for screen saver inhibition");
    m_freedesktopScreenSaver =
        std::make_unique<KODI::WINDOWING::LINUX::COSScreenSaverFreedesktop>();
  }
#endif
}

void COSScreenSaverX11::Inhibit()
{
  // disallow the screensaver by periodically calling XResetScreenSaver(),
  // for some reason setting a 0 timeout with XSetScreenSaver doesn't work with gnome
  m_screensaverResetTimer.Start(5000ms, true);
  if (m_freedesktopScreenSaver)
    m_freedesktopScreenSaver->Inhibit();
}

void COSScreenSaverX11::Uninhibit()
{
  m_screensaverResetTimer.Stop(true);
  if (m_freedesktopScreenSaver)
    m_freedesktopScreenSaver->Uninhibit();
}

void COSScreenSaverX11::ResetScreenSaver()
{
  XResetScreenSaver(m_dpy);
}
