/*
 *      Copyright (C) 2017 Team XBMC
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

#include "OSScreenSaverX11.h"

#include <cassert>

COSScreenSaverX11::COSScreenSaverX11(Display* dpy)
: m_dpy(dpy), m_screensaverResetTimer(std::bind(&COSScreenSaverX11::ResetScreenSaver, this))
{
  assert(m_dpy);
}

void COSScreenSaverX11::Inhibit()
{
  // disallow the screensaver by periodically calling XResetScreenSaver(),
  // for some reason setting a 0 timeout with XSetScreenSaver doesn't work with gnome
  m_screensaverResetTimer.Start(5000, true);
}

void COSScreenSaverX11::Uninhibit()
{
  m_screensaverResetTimer.Stop(true);
}

void COSScreenSaverX11::ResetScreenSaver()
{
  XResetScreenSaver(m_dpy);
}
