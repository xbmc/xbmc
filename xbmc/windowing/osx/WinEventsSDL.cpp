/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinEventsSDL.h"

#include "AppInboundProtocol.h"
#include "Application.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/InputManager.h"
#include "input/Key.h"
#include "input/mouse/MouseStat.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/DisplaySettings.h"
#include "windowing/WinSystem.h"

#include "platform/darwin/osx/CocoaInterface.h"

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
          std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
          if (appPort)
            appPort->SetRenderGUI(event.active.gain != 0);
          CServiceBroker::GetWinSystem()->NotifyAppActiveChange(g_application.GetRenderGUI());
        }
        else if (event.active.state & SDL_APPINPUTFOCUS)
        {
          g_application.m_AppFocused = event.active.gain != 0;
          std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
          if (appPort && g_application.m_AppFocused)
            appPort->SetRenderGUI(g_application.m_AppFocused);
          CServiceBroker::GetWinSystem()->NotifyAppFocusChange(g_application.m_AppFocused);
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
        std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
        if (appPort)
          ret |= appPort->OnEvent(newEvent);
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

        std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
        if (appPort)
          ret |= appPort->OnEvent(newEvent);
        break;
      }

      case SDL_MOUSEBUTTONDOWN:
      {
        XBMC_Event newEvent;
        newEvent.type = XBMC_MOUSEBUTTONDOWN;
        newEvent.button.button = event.button.button;
        newEvent.button.x = event.button.x;
        newEvent.button.y = event.button.y;

        std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
        if (appPort)
          ret |= appPort->OnEvent(newEvent);
        break;
      }

      case SDL_MOUSEBUTTONUP:
      {
        XBMC_Event newEvent;
        newEvent.type = XBMC_MOUSEBUTTONUP;
        newEvent.button.button = event.button.button;
        newEvent.button.x = event.button.x;
        newEvent.button.y = event.button.y;

        std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
        if (appPort)
          ret |= appPort->OnEvent(newEvent);
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

        std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
        if (appPort)
          ret |= appPort->OnEvent(newEvent);
        break;
      }
      case SDL_VIDEORESIZE:
      {
        // Under newer osx versions sdl is so fucked up that it even fires resize events
        // that exceed the screen size (maybe some HiDP incompatibility in old SDL?)
        // ensure to ignore those events because it will mess with windowed size
        if((event.resize.w > CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP).iWidth) ||
           (event.resize.h > CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP).iHeight))
        {
          break;
        }
        XBMC_Event newEvent;
        newEvent.type = XBMC_VIDEORESIZE;
        newEvent.resize.w = event.resize.w;
        newEvent.resize.h = event.resize.h;
        std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
        if (appPort)
          ret |= appPort->OnEvent(newEvent);
        CServiceBroker::GetGUI()->GetWindowManager().MarkDirty();
        break;
      }
      case SDL_USEREVENT:
      {
        XBMC_Event newEvent;
        newEvent.type = XBMC_USEREVENT;
        newEvent.user.code = event.user.code;
        std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
        if (appPort)
          ret |= appPort->OnEvent(newEvent);
        break;
      }
      case SDL_VIDEOEXPOSE:
        CServiceBroker::GetGUI()->GetWindowManager().MarkDirty();
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
      CServiceBroker::GetWinSystem()->Hide();
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
