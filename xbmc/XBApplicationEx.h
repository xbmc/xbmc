/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef XBAPPLICATIONEX_H
#define XBAPPLICATIONEX_H

#include "guilib/IWindowManagerCallback.h"

class CXBApplicationEx : public IWindowManagerCallback
{
public:
  CXBApplicationEx();
  ~CXBApplicationEx();

  // Variables for timing
  bool m_bStop;
  bool m_AppActive;
  bool m_AppFocused;

  // Overridable functions for the 3D scene created by the app
  virtual bool Initialize() { return true; }
  virtual bool Cleanup() { return true; }

public:
  // Functions to create, run, and clean up the application
  virtual bool Create();
  INT Run();
  VOID Destroy();

private:
};

#endif /* XBAPPLICATIONEX_H */
