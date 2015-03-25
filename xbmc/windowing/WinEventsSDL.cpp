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
#ifdef HAS_SDL_WIN_EVENTS

#include "WinEvents.h"
#include "WinEventsSDL.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "GUIUserMessages.h"
#include "settings/DisplaySettings.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#ifdef HAS_SDL_JOYSTICK
#include "input/SDLJoystick.h"
#endif
#include "input/InputManager.h"
#include "input/MouseStat.h"
#include "WindowingFactory.h"
#if defined(TARGET_DARWIN)
#include "osx/CocoaInterface.h"
#endif

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN) && !defined(TARGET_ANDROID)
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include "input/XBMC_keysym.h"
#include "utils/log.h"
#endif

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
// The following chunk of code is Linux specific. For keys that have
// with keysym.sym set to zero it checks the scan code, and sets the sym
// for some known scan codes. This is mostly the multimedia keys.
// Note that the scan code to sym mapping is different with and without
// the evdev driver so we need to check if evdev is loaded.

// m_bEvdev == true if evdev is loaded
static bool m_bEvdev = true;
// We need to initialise some local storage once. Set m_bEvdevInit to true
// once the initialisation has been done
static bool m_bEvdevInit = false;

// Mappings of scancodes to syms for the evdev driver
static uint16_t SymMappingsEvdev[][2] =
{ { 121, XBMCK_VOLUME_MUTE }         // Volume mute
, { 122, XBMCK_VOLUME_DOWN }         // Volume down
, { 123, XBMCK_VOLUME_UP }           // Volume up
, { 124, XBMCK_POWER }               // Power button on PC case
, { 127, XBMCK_SPACE }               // Pause
, { 135, XBMCK_MENU }                // Right click
, { 136, XBMCK_MEDIA_STOP }          // Stop
, { 138, 0x69 /* 'i' */}             // Info
, { 147, 0x6d /* 'm' */}             // Menu
, { 148, XBMCK_LAUNCH_APP2 }         // Launch app 2
, { 150, XBMCK_SLEEP }               // Sleep
, { 152, XBMCK_LAUNCH_APP1 }         // Launch app 1
, { 163, XBMCK_LAUNCH_MAIL }         // Launch Mail
, { 164, XBMCK_BROWSER_FAVORITES }   // Browser favorites
, { 166, XBMCK_BROWSER_BACK }        // Back
, { 167, XBMCK_BROWSER_FORWARD }     // Browser forward
, { 171, XBMCK_MEDIA_NEXT_TRACK }    // Next track
, { 172, XBMCK_MEDIA_PLAY_PAUSE }    // Play_Pause
, { 173, XBMCK_MEDIA_PREV_TRACK }    // Prev track
, { 174, XBMCK_MEDIA_STOP }          // Stop
, { 176, 0x72 /* 'r' */}             // Rewind
, { 179, XBMCK_LAUNCH_MEDIA_SELECT } // Launch media select
, { 180, XBMCK_BROWSER_HOME }        // Browser home
, { 181, XBMCK_BROWSER_REFRESH }     // Browser refresh
, { 208, XBMCK_MEDIA_PLAY_PAUSE }    // Play_Pause
, { 209, XBMCK_MEDIA_PLAY_PAUSE }    // Play_Pause
, { 214, XBMCK_ESCAPE }              // Close
, { 215, XBMCK_MEDIA_PLAY_PAUSE }    // Play_Pause
, { 216, 0x66 /* 'f' */}             // Forward
//, {167, 0xb3 } // Record
};

// The non-evdev mappings need to be checked. At the moment XBMC will never
// use them anyway.
static uint16_t SymMappingsUbuntu[][2] =
{ { 0xf5, XBMCK_F13 } // F13 Launch help browser
, { 0x87, XBMCK_F14 } // F14 undo
, { 0x8a, XBMCK_F15 } // F15 redo
, { 0x89, 0x7f } // F16 new
, { 0xbf, 0x80 } // F17 open
, { 0xaf, 0x81 } // F18 close
, { 0xe4, 0x82 } // F19 reply
, { 0x8e, 0x83 } // F20 forward
, { 0xda, 0x84 } // F21 send
//, { 0x, 0x85 } // F22 check spell (doesn't work for me with ubuntu)
, { 0xd5, 0x86 } // F23 save
, { 0xb9, 0x87 } // 0x2a?? F24 print
        // end of special keys above F1 till F12

, { 234,  XBMCK_BROWSER_BACK } // Browser back
, { 233,  XBMCK_BROWSER_FORWARD } // Browser forward
, { 231,  XBMCK_BROWSER_REFRESH } // Browser refresh
//, { , XBMCK_BROWSER_STOP } // Browser stop
, { 122,  XBMCK_BROWSER_SEARCH } // Browser search
, { 0xe5, XBMCK_BROWSER_SEARCH } // Browser search
, { 230,  XBMCK_BROWSER_FAVORITES } // Browser favorites
, { 130,  XBMCK_BROWSER_HOME } // Browser home
, { 0xa0, XBMCK_VOLUME_MUTE } // Volume mute
, { 0xae, XBMCK_VOLUME_DOWN } // Volume down
, { 0xb0, XBMCK_VOLUME_UP } // Volume up
, { 0x99, XBMCK_MEDIA_NEXT_TRACK } // Next track
, { 0x90, XBMCK_MEDIA_PREV_TRACK } // Prev track
, { 0xa4, XBMCK_MEDIA_STOP } // Stop
, { 0xa2, XBMCK_MEDIA_PLAY_PAUSE } // Play_Pause
, { 0xec, XBMCK_LAUNCH_MAIL } // Launch mail
, { 129,  XBMCK_LAUNCH_MEDIA_SELECT } // Launch media_select
, { 198,  XBMCK_LAUNCH_APP1 } // Launch App1/PC icon
, { 0xa1, XBMCK_LAUNCH_APP2 } // Launch App2/Calculator
, { 34, 0x3b /* vkey 0xba */} // OEM 1: [ on us keyboard
, { 51, 0x2f /* vkey 0xbf */} // OEM 2: additional key on european keyboards between enter and ' on us keyboards
, { 47, 0x60 /* vkey 0xc0 */} // OEM 3: ; on us keyboards
, { 20, 0xdb } // OEM 4: - on us keyboards (between 0 and =)
, { 49, 0xdc } // OEM 5: ` on us keyboards (below ESC)
, { 21, 0xdd } // OEM 6: =??? on us keyboards (between - and backspace)
, { 48, 0xde } // OEM 7: ' on us keyboards (on right side of ;)
//, { 0, 0xdf } // OEM 8
, { 94, 0xe2 } // OEM 102: additional key on european keyboards between left shift and z on us keyboards
//, { 0xb2, 0x } // Ubuntu default setting for launch browser
//, { 0x76, 0x } // Ubuntu default setting for launch music player
//, { 0xcc, 0x } // Ubuntu default setting for eject
, { 117, 0x5d } // right click
};

// Called once. Checks whether evdev is loaded and sets m_bEvdev accordingly.
static void InitEvdev(void)
{
  // Set m_bEvdevInit to indicate we have been initialised
  m_bEvdevInit = true;

  Display* dpy = XOpenDisplay(NULL);
  if (!dpy)
  {
    CLog::Log(LOGERROR, "CWinEventsSDL::CWinEventsSDL - XOpenDisplay failed");
    return;
  }

  XkbDescPtr desc;
  char* symbols;

  desc = XkbGetKeyboard(dpy, XkbAllComponentsMask, XkbUseCoreKbd);
  if(!desc)
  {
    XCloseDisplay(dpy);
    CLog::Log(LOGERROR, "CWinEventsSDL::CWinEventsSDL - XkbGetKeyboard failed");
    return;
  }

  symbols = XGetAtomName(dpy, desc->names->symbols);
  if(symbols)
  {
    CLog::Log(LOGDEBUG, "CWinEventsSDL::CWinEventsSDL - XKb symbols %s", symbols);
    if(strstr(symbols, "(evdev)"))
      m_bEvdev = true;
    else
      m_bEvdev = false;
  }

  XFree(symbols);
  XkbFreeKeyboard(desc, XkbAllComponentsMask, True);
  XCloseDisplay(dpy);

  CLog::Log(LOGDEBUG, "CWinEventsSDL::CWinEventsSDL - m_bEvdev = %d", m_bEvdev);
}

// Check the scan code and return the matching sym, or zero if the scan code
// is unknown.
static uint16_t SymFromScancode(uint16_t scancode)
{
  unsigned int i;

  // We need to initialise m_bEvdev once
  if (!m_bEvdevInit)
    InitEvdev();

  // If evdev is loaded look up the scan code in SymMappingsEvdev
  if (m_bEvdev)
  {
    for (i = 0; i < sizeof(SymMappingsEvdev)/4; i++)
      if (scancode == SymMappingsEvdev[i][0])
        return SymMappingsEvdev[i][1];
  }

  // If evdev is not loaded look up the scan code in SymMappingsUbuntu
  else
  {
    for (i = 0; i < sizeof(SymMappingsUbuntu)/4; i++)
      if (scancode == SymMappingsUbuntu[i][0])
        return SymMappingsUbuntu[i][1];
  }

  // Scan code wasn't found, return zero
  return 0;
}
#endif // End of checks for keysym.sym == 0

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
          CApplicationMessenger::Get().Quit();
        break;

#ifdef HAS_SDL_JOYSTICK
      case SDL_JOYBUTTONUP:
      case SDL_JOYBUTTONDOWN:
      case SDL_JOYAXISMOTION:
      case SDL_JOYBALLMOTION:
      case SDL_JOYHATMOTION:
      case SDL_JOYDEVICEADDED:
      case SDL_JOYDEVICEREMOVED:
        CInputManager::Get().UpdateJoystick(event);
        ret = true;
        break;
#endif

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
#ifdef TARGET_DARWIN_OSX
        if (ProcessOSXShortcuts(event))
        {
          ret = true;
          break;
        }
#endif

        XBMC_Event newEvent;
        newEvent.type = XBMC_KEYDOWN;
        newEvent.key.keysym.scancode = event.key.keysym.scancode;
        newEvent.key.keysym.sym = (XBMCKey) event.key.keysym.sym;
        newEvent.key.keysym.unicode = event.key.keysym.unicode;
        newEvent.key.state = event.key.state;
        newEvent.key.type = event.key.type;
        newEvent.key.which = event.key.which;

        // Check if the Windows keys are down because SDL doesn't flag this.
        uint16_t mod = event.key.keysym.mod;
        uint8_t* keystate = SDL_GetKeyState(NULL);
        if (keystate[SDLK_LSUPER] || keystate[SDLK_RSUPER])
          mod |= XBMCKMOD_LSUPER;
        newEvent.key.keysym.mod = (XBMCMod) mod;

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
        // If the keysym.sym is zero try to get it from the scan code
        if (newEvent.key.keysym.sym == 0)
          newEvent.key.keysym.sym = (XBMCKey) SymFromScancode(newEvent.key.keysym.scancode);
#endif

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
        newEvent.key.state = event.key.state;
        newEvent.key.type = event.key.type;
        newEvent.key.which = event.key.which;

        ret |= g_application.OnEvent(newEvent);
        break;
      }

      case SDL_MOUSEBUTTONDOWN:
      {
        XBMC_Event newEvent;
        newEvent.type = XBMC_MOUSEBUTTONDOWN;
        newEvent.button.button = event.button.button;
        newEvent.button.state = event.button.state;
        newEvent.button.type = event.button.type;
        newEvent.button.which = event.button.which;
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
        newEvent.button.state = event.button.state;
        newEvent.button.type = event.button.type;
        newEvent.button.which = event.button.which;
        newEvent.button.x = event.button.x;
        newEvent.button.y = event.button.y;

        ret |= g_application.OnEvent(newEvent);
        break;
      }

      case SDL_MOUSEMOTION:
      {
        if (0 == (SDL_GetAppState() & SDL_APPMOUSEFOCUS))
        {
          CInputManager::Get().SetMouseActive(false);
#if defined(TARGET_DARWIN_OSX)
          // See CApplication::ProcessSlow() for a description as to why we call Cocoa_HideMouse.
          // this is here to restore the pointer when toggling back to window mode from fullscreen.
          Cocoa_ShowMouse();
#endif
          break;
        }
        XBMC_Event newEvent;
        newEvent.type = XBMC_MOUSEMOTION;
        newEvent.motion.xrel = event.motion.xrel;
        newEvent.motion.yrel = event.motion.yrel;
        newEvent.motion.state = event.motion.state;
        newEvent.motion.type = event.motion.type;
        newEvent.motion.which = event.motion.which;
        newEvent.motion.x = event.motion.x;
        newEvent.motion.y = event.motion.y;

        ret |= g_application.OnEvent(newEvent);
        break;
      }
      case SDL_VIDEORESIZE:
      {
        // Under linux returning from fullscreen, SDL sends an extra event to resize to the desktop
        // resolution causing the previous window dimensions to be lost. This is needed to rectify
        // that problem.
        if(!g_Windowing.IsFullScreen())
        {
          int RES_SCREEN = g_Windowing.DesktopResolution(g_Windowing.GetCurrentScreen());
          if((event.resize.w == CDisplaySettings::Get().GetResolutionInfo(RES_SCREEN).iWidth) &&
              (event.resize.h == CDisplaySettings::Get().GetResolutionInfo(RES_SCREEN).iHeight))
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

size_t CWinEventsSDL::GetQueueSize()
{
  int ret;
  SDL_Event event;

  if (-1 == (ret = SDL_PeepEvents(&event, 0, SDL_PEEKEVENT, ~0)))
    ret = 0;

  return ret;
}

#ifdef TARGET_DARWIN_OSX
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
        CApplicationMessenger::Get().Quit();
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
      CApplicationMessenger::Get().Minimize();
      return true;

    default:
      return false;
    }
  }

  return false;
}

#elif defined(TARGET_POSIX)

bool CWinEventsSDL::ProcessLinuxShortcuts(SDL_Event& event)
{
  bool alt = false;

  alt = !!(SDL_GetModState() & (XBMCKMOD_LALT  | XBMCKMOD_RALT));

  if (alt && event.key.type == SDL_KEYDOWN)
  {
    switch(event.key.keysym.sym)
    {
      case SDLK_TAB:  // ALT+TAB to minimize/hide
        g_application.Minimize();
        return true;
      default:
        break;
    }
  }
  return false;
}
#endif

#endif
