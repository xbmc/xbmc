/*
 *      Copyright (C) 2016 Canonical Ltd.
 *      brandon.schaefer@canonical.com
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


#pragma once

#include <mir_toolkit/mir_client_library.h>

#include "threads/CriticalSection.h"
#include "windowing/WinSystem.h"

class IDispResource;

class CWinSystemMir : public CWinSystemBase
{
public:
  CWinSystemMir();
  virtual ~CWinSystemMir() = default;

  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;

  bool CreateNewWindow(const std::string& name,
                       bool fullScreen,
                       RESOLUTION_INFO& res) override;

  bool DestroyWindow() override;

  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;

  void UpdateResolutions() override;

  bool Hide() override;
  bool Show(bool raise = true) override;
  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);

protected:
  MirConnection* m_connection;
  MirWindow* m_window;
  MirPixelFormat m_pixel_format;
};
