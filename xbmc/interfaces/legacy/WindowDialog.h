 /*
 *      Copyright (C) 2005-2012 Team XBMC
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

#pragma once

#include "Window.h"
#include "WindowInterceptor.h"
#include "WindowDialogMixin.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    class WindowDialog : public Window, private WindowDialogMixin
    {

    public:
      WindowDialog() throw(WindowException);
      virtual ~WindowDialog();

#ifndef SWIG
      SWIGHIDDENVIRTUAL bool OnMessage(CGUIMessage& message);
      SWIGHIDDENVIRTUAL bool OnAction(const CAction &action);
      SWIGHIDDENVIRTUAL void OnDeinitWindow(int nextWindowID);

      SWIGHIDDENVIRTUAL bool IsDialogRunning() const { return WindowDialogMixin::IsDialogRunning(); }
      SWIGHIDDENVIRTUAL bool IsModalDialog() const { TRACE; return true; };
      SWIGHIDDENVIRTUAL bool IsDialog() const { TRACE; return true; };
#endif

      SWIGHIDDENVIRTUAL inline void show() { TRACE; WindowDialogMixin::show(); }
      SWIGHIDDENVIRTUAL inline void close() { TRACE; WindowDialogMixin::close(); }
    };
  }
}

