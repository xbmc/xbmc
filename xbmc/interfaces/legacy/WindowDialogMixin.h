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

// These messages are a side effect of the way dialogs work through the
// main ApplicationMessenger. At some point it would be nice to remove
// the messenger and have direct (or even drive) communications.
#define HACK_CUSTOM_ACTION_CLOSING -3u
#define HACK_CUSTOM_ACTION_OPENING -4u

namespace XBMCAddon
{
  namespace xbmcgui
  {
    class WindowDialogMixin
    {
    private:
      Window* w;

    protected:
      inline WindowDialogMixin(Window* window) : w(window) {}

    public:
      SWIGHIDDENVIRTUAL void show();
      SWIGHIDDENVIRTUAL void close();

#ifndef SWIG
      SWIGHIDDENVIRTUAL bool IsDialogRunning() const;
      SWIGHIDDENVIRTUAL bool OnAction(const CAction &action);
#endif
    };
  }
}
