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
#include "stdafx.h"
#include "WinEvents.h"
#include "Application.h"
#include "XBMC_vkeys.h"

#ifdef HAS_SDL

PHANDLE_EVENT_FUNC CWinEventsBase::m_pEventFunc = NULL;

void CWinEventsSDL::MessagePump()
{ 
  SDL_Event event;
  
  while (SDL_PollEvent(&event))
  {
    switch(event.type)
    {    
    case SDL_KEYDOWN:
      // process any platform specific shortcuts before handing off to XBMC
      //if (!ProcessOSShortcuts(event))
      {
        g_application.OnEvent(XBMC_PRESSED, (long unsigned int) &event.key.keysym, 0);
        // don't handle any more messages in the queue until we've handled keydown,
        // if a keyup is in the queue it will reset the keypress before it is handled.
      }
      break;
      
    case SDL_KEYUP:
      g_application.OnEvent(XBMC_RELEASED, (long unsigned int) &event.key.keysym, 0);
      break;
    }
  }
}

#endif
