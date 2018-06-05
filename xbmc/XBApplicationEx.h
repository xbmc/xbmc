/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "guilib/IWindowManagerCallback.h"

class CAppParamParser;

// Do not change the numbering, external scripts depend on them
enum {
  EXITCODE_QUIT      = 0,
  EXITCODE_POWERDOWN = 64,
  EXITCODE_RESTARTAPP= 65,
  EXITCODE_REBOOT    = 66,
};

class CXBApplicationEx : public IWindowManagerCallback
{
public:
  CXBApplicationEx();
  ~CXBApplicationEx() override;

  // Variables for timing
  bool m_bStop;
  int  m_ExitCode;
  bool m_AppFocused;
  bool m_renderGUI;

  // Overridable functions for the 3D scene created by the app
  virtual bool Initialize() { return true; }
  virtual bool Cleanup() { return true; }

public:
  int Run(const CAppParamParser &params);
  void Destroy();

private:
};

