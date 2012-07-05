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

#include "system.h"

#ifdef HAS_X11_WIN_EVENTS

#include "WinEvents.h"
#include "WinEventsX11.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include <X11/Xlib.h>
#include "X11/WinSystemX11GL.h"
#include "X11/keysymdef.h"
#include "X11/XF86keysym.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "guilib/GUIWindowManager.h"
#include "input/MouseStat.h"

#if defined(HAS_XRANDR)
#include <X11/extensions/Xrandr.h>
#endif

#ifdef HAS_SDL_JOYSTICK
#include "input/SDLJoystick.h"
#endif

CWinEventsX11Imp* CWinEventsX11Imp::WinEvents = 0;

static uint32_t SymMappingsX11[][2] =
{
  {XK_BackSpace, XBMCK_BACKSPACE}
, {XK_Tab, XBMCK_TAB}
, {XK_Clear, XBMCK_CLEAR}
, {XK_Return, XBMCK_RETURN}
, {XK_Pause, XBMCK_PAUSE}
, {XK_Escape, XBMCK_ESCAPE}
, {XK_Delete, XBMCK_DELETE}
// multi-media keys
, {XF86XK_Back, XBMCK_BROWSER_BACK}
, {XF86XK_Forward, XBMCK_BROWSER_FORWARD}
, {XF86XK_Refresh, XBMCK_BROWSER_REFRESH}
, {XF86XK_Stop, XBMCK_BROWSER_STOP}
, {XF86XK_Search, XBMCK_BROWSER_SEARCH}
, {XF86XK_Favorites, XBMCK_BROWSER_FAVORITES}
, {XF86XK_HomePage, XBMCK_BROWSER_HOME}
, {XF86XK_AudioMute, XBMCK_VOLUME_MUTE}
, {XF86XK_AudioLowerVolume, XBMCK_VOLUME_DOWN}
, {XF86XK_AudioRaiseVolume, XBMCK_VOLUME_UP}
, {XF86XK_AudioNext, XBMCK_MEDIA_NEXT_TRACK}
, {XF86XK_AudioPrev, XBMCK_MEDIA_PREV_TRACK}
, {XF86XK_AudioStop, XBMCK_MEDIA_STOP}
, {XF86XK_AudioPause, XBMCK_MEDIA_PLAY_PAUSE}
, {XF86XK_Mail, XBMCK_LAUNCH_MAIL}
, {XF86XK_Select, XBMCK_LAUNCH_MEDIA_SELECT}
, {XF86XK_Launch0, XBMCK_LAUNCH_APP1}
, {XF86XK_Launch1, XBMCK_LAUNCH_APP2}
, {XF86XK_WWW, XBMCK_LAUNCH_FILE_BROWSER}
, {XF86XK_AudioMedia, XBMCK_LAUNCH_MEDIA_CENTER }
  // Numeric keypad
, {XK_KP_0, XBMCK_KP0}
, {XK_KP_1, XBMCK_KP1}
, {XK_KP_2, XBMCK_KP2}
, {XK_KP_3, XBMCK_KP3}
, {XK_KP_4, XBMCK_KP4}
, {XK_KP_5, XBMCK_KP5}
, {XK_KP_6, XBMCK_KP6}
, {XK_KP_7, XBMCK_KP7}
, {XK_KP_8, XBMCK_KP8}
, {XK_KP_9, XBMCK_KP9}
, {XK_KP_Separator, XBMCK_KP_PERIOD}
, {XK_KP_Divide, XBMCK_KP_DIVIDE}
, {XK_KP_Multiply, XBMCK_KP_MULTIPLY}
, {XK_KP_Subtract, XBMCK_KP_MINUS}
, {XK_KP_Add, XBMCK_KP_PLUS}
, {XK_KP_Enter, XBMCK_KP_ENTER}
, {XK_KP_Equal, XBMCK_KP_EQUALS}
  // Arrows + Home/End pad
, {XK_Up, XBMCK_UP}
, {XK_Down, XBMCK_DOWN}
, {XK_Right, XBMCK_RIGHT}
, {XK_Left, XBMCK_LEFT}
, {XK_Insert, XBMCK_INSERT}
, {XK_Home, XBMCK_HOME}
, {XK_End, XBMCK_END}
, {XK_Page_Up, XBMCK_PAGEUP}
, {XK_Page_Down, XBMCK_PAGEDOWN}
  // Function keys
, {XK_F1, XBMCK_F1}
, {XK_F2, XBMCK_F2}
, {XK_F3, XBMCK_F3}
, {XK_F4, XBMCK_F4}
, {XK_F5, XBMCK_F5}
, {XK_F6, XBMCK_F6}
, {XK_F7, XBMCK_F7}
, {XK_F8, XBMCK_F8}
, {XK_F9, XBMCK_F9}
, {XK_F10, XBMCK_F10}
, {XK_F11, XBMCK_F11}
, {XK_F12, XBMCK_F12}
, {XK_F13, XBMCK_F13}
, {XK_F14, XBMCK_F14}
, {XK_F15, XBMCK_F15}
  // Key state modifier keys
, {XK_Num_Lock, XBMCK_NUMLOCK}
, {XK_Caps_Lock, XBMCK_CAPSLOCK}
, {XK_Scroll_Lock, XBMCK_SCROLLOCK}
, {XK_Shift_R, XBMCK_RSHIFT}
, {XK_Shift_L, XBMCK_LSHIFT}
, {XK_Control_R, XBMCK_RCTRL}
, {XK_Control_L, XBMCK_LCTRL}
, {XK_Alt_R, XBMCK_RALT}
, {XK_Alt_L, XBMCK_LALT}
, {XK_Meta_R, XBMCK_RMETA}
, {XK_Meta_L, XBMCK_LMETA}
, {XK_Super_L, XBMCK_LSUPER}
, {XK_Super_R, XBMCK_RSUPER}
, {XK_Mode_switch, XBMCK_MODE}
, {XK_Multi_key, XBMCK_COMPOSE}
  // Miscellaneous function keys
, {XK_Help, XBMCK_HELP}
, {XK_Print, XBMCK_PRINT}
//, {0, XBMCK_SYSREQ}
, {XK_Break, XBMCK_BREAK}
, {XK_Menu, XBMCK_MENU}
, {XF86XK_PowerOff, XBMCK_POWER}
, {XK_EcuSign, XBMCK_EURO}
, {XK_Undo, XBMCK_UNDO}
  /* Media keys */
, {XF86XK_Eject, XBMCK_EJECT}
, {XF86XK_Stop, XBMCK_STOP}
, {XF86XK_AudioRecord, XBMCK_RECORD}
, {XF86XK_AudioRewind, XBMCK_REWIND}
, {XF86XK_Phone, XBMCK_PHONE}
, {XF86XK_AudioPlay, XBMCK_PLAY}
, {XF86XK_AudioRandomPlay, XBMCK_SHUFFLE}
, {XF86XK_AudioForward, XBMCK_FASTFORWARD}
};

bool CWinEventsX11::MessagePump()
{
  return CWinEventsX11Imp::MessagePump();
}

size_t CWinEventsX11::GetQueueSize()
{
  return CWinEventsX11Imp::GetQueueSize();
}

CWinEventsX11Imp::CWinEventsX11Imp()
{
  m_display = 0;
  m_window = 0;
  m_keybuf = 0;
}

CWinEventsX11Imp::~CWinEventsX11Imp()
{
  if (m_keybuf);
  {
    free(m_keybuf);
    m_keybuf = 0;
  }

  if (m_xic)
  {
    XUnsetICFocus(m_xic);
    XDestroyIC(m_xic);
    m_xic = 0;
  }

  if (m_xim)
  {
    XCloseIM(m_xim);
    m_xim = 0;
  }

  m_symLookupTable.clear();
}

bool CWinEventsX11Imp::Init(Display *dpy, Window win)
{
  if (WinEvents)
    return true;

  WinEvents = new CWinEventsX11Imp();
  WinEvents->m_display = dpy;
  WinEvents->m_window = win;
  WinEvents->m_keybuf = (char*)malloc(32*sizeof(char));
  WinEvents->m_keymodState = 0;
  WinEvents->m_wmDeleteMessage = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  WinEvents->m_structureChanged = false;
  WinEvents->m_xrrEventPending = false;
  memset(&(WinEvents->m_lastKey), 0, sizeof(XBMC_Event));

  // open input method
  char *old_locale = NULL, *old_modifiers = NULL;
  char res_name[8];
  const char *p;
  size_t n;

  // set resource name to xbmc, not used
  strcpy(res_name, "xbmc");

  // save current locale, this should be "C"
  p = setlocale(LC_ALL, NULL);
  if (p)
  {
    old_locale = (char*)malloc(strlen(p) +1);
    strcpy(old_locale, p);
  }
  p = XSetLocaleModifiers(NULL);
  if (p)
  {
    old_modifiers = (char*)malloc(strlen(p) +1);
    strcpy(old_modifiers, p);
  }

  // set users preferences and open input method
  p = setlocale(LC_ALL, "");
  XSetLocaleModifiers("");
  WinEvents->m_xim = XOpenIM(WinEvents->m_display, NULL, res_name, res_name);

  // restore old locale
  if (old_locale)
  {
    setlocale(LC_ALL, old_locale);
    free(old_locale);
  }
  if (old_modifiers)
  {
    XSetLocaleModifiers(old_modifiers);
    free(old_modifiers);
  }

  WinEvents->m_xic = NULL;
  if (WinEvents->m_xim)
  {
    WinEvents->m_xic = XCreateIC(WinEvents->m_xim,
                                 XNClientWindow, WinEvents->m_window,
                                 XNFocusWindow, WinEvents->m_window,
                                 XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                                 XNResourceName, res_name,
                                 XNResourceClass, res_name,
                                 NULL);
  }

  if (!WinEvents->m_xic)
    CLog::Log(LOGWARNING,"CWinEventsX11::Init - no input method found");

  // build Keysym lookup table
  for (unsigned int i = 0; i < sizeof(SymMappingsX11)/(2*sizeof(uint32_t)); ++i)
  {
    WinEvents->m_symLookupTable[SymMappingsX11[i][0]] = SymMappingsX11[i][1];
  }

  // register for xrandr events
#if defined(HAS_XRANDR)
  int iReturn;
  XRRQueryExtension(WinEvents->m_display, &WinEvents->m_RREventBase, &iReturn);
  XRRSelectInput(WinEvents->m_display, WinEvents->m_window, RRScreenChangeNotifyMask);
#endif

  return true;
}

void CWinEventsX11Imp::Quit()
{
  if (!WinEvents)
    return;

  delete WinEvents;
  WinEvents = 0;
}

bool CWinEventsX11Imp::HasStructureChanged()
{
  if (!WinEvents)
    return false;

  bool ret = WinEvents->m_structureChanged;
  WinEvents->m_structureChanged = false;
  return ret;
}

void CWinEventsX11Imp::SetXRRFailSafeTimer(int millis)
{
  if (!WinEvents)
    return;

  WinEvents->m_xrrFailSafeTimer.Set(millis);
  WinEvents->m_xrrEventPending = true;
}

bool CWinEventsX11Imp::MessagePump()
{
  if (!WinEvents)
    return false;

  bool ret = false;
  XEvent xevent;
  unsigned long serial = 0;

  while (WinEvents && XPending(WinEvents->m_display))
  {
    memset(&xevent, 0, sizeof (XEvent));
    XNextEvent(WinEvents->m_display, &xevent);

    //  ignore events generated by auto-repeat
    if (xevent.type == KeyRelease && XPending(WinEvents->m_display))
    {
      XEvent peekevent;
      XPeekEvent(WinEvents->m_display, &peekevent);
      if ((peekevent.type == KeyPress) &&
          (peekevent.xkey.keycode == xevent.xkey.keycode) &&
          ((peekevent.xkey.time - xevent.xkey.time) < 2))
      {
        XNextEvent(WinEvents->m_display, &peekevent);
        continue;
      }
    }

    if (XFilterEvent(&xevent, None))
      continue;

    switch (xevent.type)
    {
      case MapNotify:
      {
        g_application.SetRenderGUI(true);
        break;
      }

      case UnmapNotify:
      {
        g_application.SetRenderGUI(false);
        break;
      }

      case FocusIn:
      {
        if (WinEvents->m_xic)
          XSetICFocus(WinEvents->m_xic);
        g_application.m_AppFocused = true;
        memset(&(WinEvents->m_lastKey), 0, sizeof(XBMC_Event));
        WinEvents->m_keymodState = 0;
        if (serial == xevent.xfocus.serial)
          break;
        g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
        break;
      }

      case FocusOut:
      {
        if (WinEvents->m_xic)
          XUnsetICFocus(WinEvents->m_xic);
        g_application.m_AppFocused = false;
        memset(&(WinEvents->m_lastKey), 0, sizeof(XBMC_Event));
        g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
        serial = xevent.xfocus.serial;
        break;
      }

      case Expose:
      {
        g_windowManager.MarkDirty();
        break;
      }

      case ConfigureNotify:
      {
        if (xevent.xconfigure.window != WinEvents->m_window)
          break;

        WinEvents->m_structureChanged = true;
        XBMC_Event newEvent;
        memset(&newEvent, 0, sizeof(newEvent));
        newEvent.type = XBMC_VIDEORESIZE;
        newEvent.resize.w = xevent.xconfigure.width;
        newEvent.resize.h = xevent.xconfigure.height;
        ret |= g_application.OnEvent(newEvent);
        g_windowManager.MarkDirty();
        break;
      }

      case ClientMessage:
      {
        if (xevent.xclient.data.l[0] == WinEvents->m_wmDeleteMessage)
          if (!g_application.m_bStop) CApplicationMessenger::Get().Quit();
        break;
      }

      case KeyPress:
      {
        XBMC_Event newEvent;
        memset(&newEvent, 0, sizeof(newEvent));
        newEvent.type = XBMC_KEYDOWN;
        KeySym xkeysym;

        // fallback if we have no IM
        if (!WinEvents->m_xic)
        {
          static XComposeStatus state;
          char keybuf[32];
          xkeysym = XLookupKeysym(&xevent.xkey, 0);
          newEvent.key.keysym.sym = LookupXbmcKeySym(xkeysym);
          newEvent.key.keysym.scancode = xevent.xkey.keycode;
          newEvent.key.state = xevent.xkey.state;
          newEvent.key.type = xevent.xkey.type;
          if (XLookupString(&xevent.xkey, keybuf, sizeof(keybuf), NULL, &state))
          {
            newEvent.key.keysym.unicode = keybuf[0];
          }
          ret |= ProcessKey(newEvent, 500);
          break;
        }

        Status status;
        int len;
        len = Xutf8LookupString(WinEvents->m_xic, &xevent.xkey,
                                WinEvents->m_keybuf, sizeof(WinEvents->m_keybuf),
                                &xkeysym, &status);
        if (status == XBufferOverflow)
        {
          WinEvents->m_keybuf = (char*)realloc(WinEvents->m_keybuf, len*sizeof(char));
          len = Xutf8LookupString(WinEvents->m_xic, &xevent.xkey,
                                  WinEvents->m_keybuf, sizeof(WinEvents->m_keybuf),
                                  &xkeysym, &status);
        }
        switch (status)
        {
          case XLookupNone:
            break;
          case XLookupChars:
          case XLookupBoth:
          {
            CStdString   data(WinEvents->m_keybuf, len);
            CStdStringW keys;
            g_charsetConverter.utf8ToW(data, keys, false);

            if (keys.length() == 0)
            {
              break;
            }

            for (unsigned int i = 0; i < keys.length() - 1; i++)
            {
              newEvent.key.keysym.sym = XBMCK_UNKNOWN;
              newEvent.key.keysym.unicode = keys[i];
              newEvent.key.state = xevent.xkey.state;
              newEvent.key.type = xevent.xkey.type;
              ret |= ProcessKey(newEvent, 500);
            }
            if (keys.length() > 0)
            {
              newEvent.key.keysym.scancode = xevent.xkey.keycode;
              xkeysym = XLookupKeysym(&xevent.xkey, 0);
              newEvent.key.keysym.sym = LookupXbmcKeySym(xkeysym);
              newEvent.key.keysym.unicode = keys[keys.length() - 1];
              newEvent.key.state = xevent.xkey.state;
              newEvent.key.type = xevent.xkey.type;

              ret |= ProcessKey(newEvent, 500);
            }
            break;
          }

          case XLookupKeySym:
          {
            newEvent.key.keysym.scancode = xevent.xkey.keycode;
            newEvent.key.keysym.sym = LookupXbmcKeySym(xkeysym);
            newEvent.key.state = xevent.xkey.state;
            newEvent.key.type = xevent.xkey.type;
            ret |= ProcessKey(newEvent, 500);
            break;
          }

        }// switch status
        break;
      } //KeyPress

      case KeyRelease:
      {
        XBMC_Event newEvent;
        KeySym xkeysym;
        memset(&newEvent, 0, sizeof(newEvent));
        newEvent.type = XBMC_KEYUP;
        xkeysym = XLookupKeysym(&xevent.xkey, 0);
        newEvent.key.keysym.scancode = xevent.xkey.keycode;
        newEvent.key.keysym.sym = LookupXbmcKeySym(xkeysym);
        newEvent.key.state = xevent.xkey.state;
        newEvent.key.type = xevent.xkey.type;
        ret |= ProcessKey(newEvent, 0);
        break;
      }

      case EnterNotify:
      {
        g_Windowing.NotifyMouseCoverage(true);
        break;
      }

      // lose mouse coverage
      case LeaveNotify:
      {
        g_Windowing.NotifyMouseCoverage(false);
        g_Mouse.SetActive(false);
        break;
      }

      case MotionNotify:
      {
        XBMC_Event newEvent;
        memset(&newEvent, 0, sizeof(newEvent));
        newEvent.type = XBMC_MOUSEMOTION;
        newEvent.motion.xrel = (int16_t)xevent.xmotion.x_root;
        newEvent.motion.yrel = (int16_t)xevent.xmotion.y_root;
        newEvent.motion.x = (int16_t)xevent.xmotion.x;
        newEvent.motion.y = (int16_t)xevent.xmotion.y;
        ret |= g_application.OnEvent(newEvent);
        break;
      }

      case ButtonPress:
      {
        XBMC_Event newEvent;
        memset(&newEvent, 0, sizeof(newEvent));
        newEvent.type = XBMC_MOUSEBUTTONDOWN;
        newEvent.button.button = (unsigned char)xevent.xbutton.button;
        newEvent.button.state = XBMC_PRESSED;
        newEvent.button.x = (int16_t)xevent.xbutton.x;
        newEvent.button.y = (int16_t)xevent.xbutton.y;
        ret |= g_application.OnEvent(newEvent);
        break;
      }

      case ButtonRelease:
      {
        XBMC_Event newEvent;
        memset(&newEvent, 0, sizeof(newEvent));
        newEvent.type = XBMC_MOUSEBUTTONUP;
        newEvent.button.button = (unsigned char)xevent.xbutton.button;
        newEvent.button.state = XBMC_RELEASED;
        newEvent.button.x = (int16_t)xevent.xbutton.x;
        newEvent.button.y = (int16_t)xevent.xbutton.y;
        ret |= g_application.OnEvent(newEvent);
        break;
      }

      default:
      {
        break;
      }
    }// switch event.type

#if defined(HAS_XRANDR)
    if (WinEvents && (xevent.type == WinEvents->m_RREventBase + RRScreenChangeNotify))
    {
      XRRUpdateConfiguration(&xevent);
      if (xevent.xgeneric.serial != serial)
        g_Windowing.NotifyXRREvent();
      WinEvents->m_xrrEventPending = false;
      serial = xevent.xgeneric.serial;
    }
#endif

  }// while

  ret |= ProcessKeyRepeat();

#if defined(HAS_XRANDR)
  if (WinEvents && WinEvents->m_xrrEventPending && WinEvents->m_xrrFailSafeTimer.IsTimePast())
  {
    CLog::Log(LOGERROR,"CWinEventsX11::MessagePump - missed XRR Events");
    g_Windowing.NotifyXRREvent();
    WinEvents->m_xrrEventPending = false;
  }
#endif

#ifdef HAS_SDL_JOYSTICK
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    switch(event.type)
    {
      case SDL_JOYBUTTONUP:
      case SDL_JOYBUTTONDOWN:
      case SDL_JOYAXISMOTION:
      case SDL_JOYBALLMOTION:
      case SDL_JOYHATMOTION:
        g_Joystick.Update(event);
        ret = true;
        break;

      default:
        break;
    }
    memset(&event, 0, sizeof(SDL_Event));
  }
#endif

  return ret;
}

size_t CWinEventsX11Imp::GetQueueSize()
{
  int ret = 0;

  if (WinEvents)
    ret = XPending(WinEvents->m_display);

  return ret;
}

bool CWinEventsX11Imp::ProcessKey(XBMC_Event &event, int repeatDelay)
{
  if (event.type == XBMC_KEYDOWN)
  {
    // check key modifiers
    switch(event.key.keysym.sym)
    {
      case XBMCK_LSHIFT:
        WinEvents->m_keymodState |= XBMCKMOD_LSHIFT;
        break;
      case XBMCK_RSHIFT:
        WinEvents->m_keymodState |= XBMCKMOD_RSHIFT;
        break;
      case XBMCK_LCTRL:
        WinEvents->m_keymodState |= XBMCKMOD_LCTRL;
        break;
      case XBMCK_RCTRL:
        WinEvents->m_keymodState |= XBMCKMOD_RCTRL;
        break;
      case XBMCK_LALT:
        WinEvents->m_keymodState |= XBMCKMOD_LALT;
        break;
      case XBMCK_RALT:
        WinEvents->m_keymodState |= XBMCKMOD_RCTRL;
        break;
      case XBMCK_LMETA:
        WinEvents->m_keymodState |= XBMCKMOD_LMETA;
        break;
      case XBMCK_RMETA:
        WinEvents->m_keymodState |= XBMCKMOD_RMETA;
        break;
      case XBMCK_MODE:
        WinEvents->m_keymodState |= XBMCKMOD_MODE;
        break;
      default:
        break;
    }
    event.key.keysym.mod = (XBMCMod)WinEvents->m_keymodState;
    memcpy(&(WinEvents->m_lastKey), &event, sizeof(event));
    WinEvents->m_repeatKeyTimeout.Set(repeatDelay);

    bool ret = ProcessShortcuts(event);
    if (ret)
      return ret;
  }
  else if (event.type == XBMC_KEYUP)
  {
    switch(event.key.keysym.sym)
    {
      case XBMCK_LSHIFT:
        WinEvents->m_keymodState &= ~XBMCKMOD_LSHIFT;
        break;
      case XBMCK_RSHIFT:
        WinEvents->m_keymodState &= ~XBMCKMOD_RSHIFT;
        break;
      case XBMCK_LCTRL:
        WinEvents->m_keymodState &= ~XBMCKMOD_LCTRL;
        break;
      case XBMCK_RCTRL:
        WinEvents->m_keymodState &= ~XBMCKMOD_RCTRL;
        break;
      case XBMCK_LALT:
        WinEvents->m_keymodState &= ~XBMCKMOD_LALT;
        break;
      case XBMCK_RALT:
        WinEvents->m_keymodState &= ~XBMCKMOD_RCTRL;
        break;
      case XBMCK_LMETA:
        WinEvents->m_keymodState &= ~XBMCKMOD_LMETA;
        break;
      case XBMCK_RMETA:
        WinEvents->m_keymodState &= ~XBMCKMOD_RMETA;
        break;
      case XBMCK_MODE:
        WinEvents->m_keymodState &= ~XBMCKMOD_MODE;
        break;
      default:
        break;
    }
    event.key.keysym.mod = (XBMCMod)WinEvents->m_keymodState;
    memset(&(WinEvents->m_lastKey), 0, sizeof(event));
  }

  return g_application.OnEvent(event);
}

bool CWinEventsX11Imp::ProcessShortcuts(XBMC_Event& event)
{
  if (event.key.keysym.mod & XBMCKMOD_ALT)
  {
    switch(event.key.keysym.sym)
    {
      case XBMCK_TAB:  // ALT+TAB to minimize/hide
        g_application.Minimize();
        return true;

      default:
        return false;
    }
  }
  return false;
}

bool CWinEventsX11Imp::ProcessKeyRepeat()
{
  if (WinEvents && (WinEvents->m_lastKey.type == XBMC_KEYDOWN))
  {
    if (WinEvents->m_repeatKeyTimeout.IsTimePast())
    {
      return ProcessKey(WinEvents->m_lastKey, 10);
    }
  }
  return false;
}

XBMCKey CWinEventsX11Imp::LookupXbmcKeySym(KeySym keysym)
{
  // try direct mapping first
  std::map<uint32_t, uint32_t>::iterator it;
  it = WinEvents->m_symLookupTable.find(keysym);
  if (it != WinEvents->m_symLookupTable.end())
  {
    return (XBMCKey)(it->second);
  }

  // try ascii mappings
  if (keysym>>8 == 0x00)
    return (XBMCKey)(keysym & 0xFF);

  return (XBMCKey)keysym;
}
#endif
