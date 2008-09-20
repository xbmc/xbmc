#ifndef DINPUT_MOUSE_H
#define DINPUT_MOUSE_H

#ifndef HAS_SDL

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "../system.h"  // only really need dinput.h

#define MOUSE_DOUBLE_CLICK_LENGTH 500L
#define MOUSE_ACTIVE_LENGTH   5000L
#include "Mouse.h"

class CDirectInputMouse : public IMouseDevice
{
public:
  CDirectInputMouse();
  ~CDirectInputMouse();
  virtual void Initialize(void *appData = NULL);
  virtual void Acquire();
  virtual bool Update(MouseState &state);
  virtual void ShowPointer(bool show) {};
private:
  LPDIRECTINPUTDEVICE m_mouse;
};
#endif

#endif

