/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "system.h"

#include "WinEventsSDL.h"
#include "Application.h"
#include "messaging/ApplicationMessenger.h"
#include "GUIUserMessages.h"
#include "settings/DisplaySettings.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "input/InputManager.h"
#include "input/MouseStat.h"
#include "windowing/WindowingFactory.h"
#include "platform/darwin/osx/CocoaInterface.h"
#include "ServiceBroker.h"

using namespace KODI::MESSAGING;

bool CWinEventsSDL::MessagePump()
{
  SDL_Event event;
  bool ret = false;

  while (SDL_PollEvent(&event))
  {
    switch(event.type)
    {
      case SDL_QUIT:
        if (!g_application.m_bStop)
          CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);
        break;

      case SDL_ACTIVEEVENT:
        //If the window was inconified or restored
        if( event.active.state & SDL_APPACTIVE )
        {
          g_application.SetRenderGUI(event.active.gain != 0);
          g_Windowing.NotifyAppActiveChange(g_application.GetRenderGUI());
        }
        else if (event.active.state & SDL_APPINPUTFOCUS)
        {
          g_application.m_AppFocused = event.active.gain != 0;
          g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
        }
        break;

      case SDL_KEYDOWN:
      {
        // process any platform specific shortcuts before handing off to XBMC
        if (ProcessOSXShortcuts(event))
        {
          ret = true;
          break;
        }

        XBMC_Event newEvent;
        newEvent.type = XBMC_KEYDOWN;
        newEvent.key.keysym.scancode = event.key.keysym.scancode;
        newEvent.key.keysym.sym = (XBMCKey) event.key.keysym.sym;
        newEvent.key.keysym.unicode = event.key.keysym.unicode;

        // Check if the Windows keys are down because SDL doesn't flag this.
        uint16_t mod = event.key.keysym.mod;
        uint8_t* keystate = SDL_GetKeyState(NULL);
        if (keystate[SDLK_LSUPER] || keystate[SDLK_RSUPER])
          mod |= XBMCKMOD_LSUPER;
        newEvent.key.keysym.mod = (XBMCMod) mod;

        // don't handle any more messages in the queue until we've handled keydown,
        // if a keyup is in the queue it will reset the keypress before it is handled.
        ret |= g_application.OnEvent(newEvent);
        break;
      }

      case SDL_KEYUP:
      {
        XBMC_Event newEvent;
        newEvent.type = XBMC_KEYUP;
        newEvent.key.keysym.scancode = event.key.keysym.scancode;
        newEvent.key.keysym.sym = (XBMCKey) event.key.keysym.sym;
        newEvent.key.keysym.mod =(XBMCMod) event.key.keysym.mod;
        newEvent.key.keysym.unicode = event.key.keysym.unicode;

        ret |= g_application.OnEvent(newEvent);
        break;
      }

      case SDL_MOUSEBUTTONDOWN:
      {
        XBMC_Event newEvent;
        newEvent.type = XBMC_MOUSEBUTTONDOWN;
        newEvent.button.button = event.button.button;
        newEvent.button.x = event.button.x;
        newEvent.button.y = event.button.y;

        ret |= g_application.OnEvent(newEvent);
        break;
      }

      case SDL_MOUSEBUTTONUP:
      {
        XBMC_Event newEvent;
        newEvent.type = XBMC_MOUSEBUTTONUP;
        newEvent.button.button = event.button.button;
        newEvent.button.x = event.button.x;
        newEvent.button.y = event.button.y;

        ret |= g_application.OnEvent(newEvent);
        break;
      }

      case SDL_MOUSEMOTION:
      {
        if (0 == (SDL_GetAppState() & SDL_APPMOUSEFOCUS))
        {
          CServiceBroker::GetInputManager().SetMouseActive(false);
          // See CApplication::ProcessSlow() for a description as to why we call Cocoa_HideMouse.
          // this is here to restore the pointer when toggling back to window mode from fullscreen.
          Cocoa_ShowMouse();
          break;
        }
        XBMC_Event newEvent;
        newEvent.type = XBMC_MOUSEMOTION;
        newEvent.motion.x = event.motion.x;
        newEvent.motion.y = event.motion.y;

        ret |= g_application.OnEvent(newEvent);
        break;
      }
      case SDL_VIDEORESIZE:
      {
        // Under newer osx versions sdl is so fucked up that it even fires resize events
        // that exceed the screen size (maybe some HiDP incompatibility in old SDL?)
        // ensure to ignore those events because it will mess with windowed size
        int RES_SCREEN = g_Windowing.DesktopResolution(g_Windowing.GetCurrentScreen());
        if((event.resize.w > CDisplaySettings::GetInstance().GetResolutionInfo(RES_SCREEN).iWidth) ||
           (event.resize.h > CDisplaySettings::GetInstance().GetResolutionInfo(RES_SCREEN).iHeight))
        {
          break;
        }
        XBMC_Event newEvent;
        newEvent.type = XBMC_VIDEORESIZE;
        newEvent.resize.w = event.resize.w;
        newEvent.resize.h = event.resize.h;
        ret |= g_application.OnEvent(newEvent);
        g_windowManager.MarkDirty();
        break;
      }
      case SDL_USEREVENT:
      {
        XBMC_Event newEvent;
        newEvent.type = XBMC_USEREVENT;
        newEvent.user.code = event.user.code;
        ret |= g_application.OnEvent(newEvent);
        break;
      }
      case SDL_VIDEOEXPOSE:
        g_windowManager.MarkDirty();
        break;
    }
    memset(&event, 0, sizeof(SDL_Event));
  }

  return ret;
}

bool CWinEventsSDL::ProcessOSXShortcuts(SDL_Event& event)
{
  static bool shift = false, cmd = false;

  cmd   = !!(SDL_GetModState() & (KMOD_LMETA  | KMOD_RMETA ));
  shift = !!(SDL_GetModState() & (KMOD_LSHIFT | KMOD_RSHIFT));

  if (cmd && event.key.type == SDL_KEYDOWN)
  {
    char keysymbol = event.key.keysym.sym;

    // if the unicode is in the ascii range
    // use this instead for getting the real
    // character based on the used keyboard layout
    // see http://lists.libsdl.org/pipermail/sdl-libsdl.org/2004-May/043716.html
    if (!(event.key.keysym.unicode & 0xff80))
      keysymbol = event.key.keysym.unicode;

    switch(keysymbol)
    {
    case SDLK_q:  // CMD-q to quit
      if (!g_application.m_bStop)
        CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);
      return true;

    case SDLK_f: // CMD-f to toggle fullscreen
      g_application.OnAction(CAction(ACTION_TOGGLE_FULLSCREEN));
      return true;

    case SDLK_s: // CMD-3 to take a screenshot
      g_application.OnAction(CAction(ACTION_TAKE_SCREENSHOT));
      return true;

    case SDLK_h: // CMD-h to hide
      g_Windowing.Hide();
      return true;

    case SDLK_m: // CMD-m to minimize
      CApplicationMessenger::GetInstance().PostMsg(TMSG_MINIMIZE);
      return true;

    default:
      return false;
    }
  }

  return false;
}
