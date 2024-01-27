/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinEventsX11.h"

#include "ServiceBroker.h"
#include "application/AppInboundProtocol.h"
#include "application/Application.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/InputManager.h"
#include "input/mouse/MouseStat.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "windowing/WinEvents.h"
#include "windowing/X11/WinSystemX11.h"

#include <stdexcept>

#include <X11/XF86keysym.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <X11/keysymdef.h>

using namespace KODI::WINDOWING::X11;

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
, {XF86XK_Sleep, XBMCK_SLEEP}
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

CWinEventsX11::CWinEventsX11(CWinSystemX11& winSystem) : m_winSystem(winSystem)
{
}

CWinEventsX11::~CWinEventsX11()
{
  Quit();
}

bool CWinEventsX11::Init(Display *dpy, Window win)
{
  if (m_display)
    return true;

  m_display = dpy;
  m_window = win;
  m_keybuf_len = 32*sizeof(char);
  m_keybuf = (char*)malloc(m_keybuf_len);
  m_keymodState = 0;
  m_wmDeleteMessage = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  m_structureChanged = false;
  m_xrrEventPending = false;

  // open input method
  char *old_locale = NULL, *old_modifiers = NULL;
  char res_name[8];
  const char *p;

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
  m_xim = XOpenIM(m_display, NULL, res_name, res_name);

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

  m_xic = NULL;
  if (m_xim)
  {
    m_xic = XCreateIC(m_xim,
                      XNClientWindow, m_window,
                      XNFocusWindow, m_window,
                      XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                      XNResourceName, res_name,
                      XNResourceClass, res_name,
                      nullptr);
  }

  if (!m_xic)
    CLog::Log(LOGWARNING,"CWinEventsX11::Init - no input method found");

  // build Keysym lookup table
  for (const auto& symMapping : SymMappingsX11)
  {
    m_symLookupTable[symMapping[0]] = symMapping[1];
  }

  // register for xrandr events
  int iReturn;
  XRRQueryExtension(m_display, &m_RREventBase, &iReturn);
  int numScreens = XScreenCount(m_display);
  for (int i = 0; i < numScreens; i++)
  {
    XRRSelectInput(m_display, RootWindow(m_display, i), RRScreenChangeNotifyMask | RRCrtcChangeNotifyMask | RROutputChangeNotifyMask | RROutputPropertyNotifyMask);
  }

  return true;
}

void CWinEventsX11::Quit()
{
  free(m_keybuf);
  m_keybuf = nullptr;

  if (m_xic)
  {
    XUnsetICFocus(m_xic);
    XDestroyIC(m_xic);
    m_xic = nullptr;
  }

  if (m_xim)
  {
    XCloseIM(m_xim);
    m_xim = nullptr;
  }

  m_symLookupTable.clear();

  m_display = nullptr;
}

bool CWinEventsX11::HasStructureChanged()
{
  if (!m_display)
    return false;

  bool ret = m_structureChanged;
  m_structureChanged = false;
  return ret;
}

void CWinEventsX11::SetXRRFailSafeTimer(std::chrono::milliseconds duration)
{
  if (!m_display)
    return;

  m_xrrFailSafeTimer.Set(duration);
  m_xrrEventPending = true;
}

bool CWinEventsX11::MessagePump()
{
  if (!m_display)
    return false;

  bool ret = false;
  XEvent xevent;
  unsigned long serial = 0;
  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();

  while (m_display && XPending(m_display))
  {
    memset(&xevent, 0, sizeof (XEvent));
    XNextEvent(m_display, &xevent);

    if (m_display && (xevent.type == m_RREventBase + RRScreenChangeNotify))
    {
      if (xevent.xgeneric.serial == serial)
        continue;

      if (m_xrrEventPending)
      {
        m_winSystem.NotifyXRREvent();
        m_xrrEventPending = false;
        serial = xevent.xgeneric.serial;
      }

      continue;
    }
    else if (m_display && (xevent.type == m_RREventBase + RRNotify))
    {
      if (xevent.xgeneric.serial == serial)
        continue;

      XRRNotifyEvent* rrEvent = reinterpret_cast<XRRNotifyEvent*>(&xevent);
      if (rrEvent->subtype == RRNotify_OutputChange)
      {
        XRROutputChangeNotifyEvent* changeEvent = reinterpret_cast<XRROutputChangeNotifyEvent*>(&xevent);
        if (changeEvent->connection == RR_Connected ||
            changeEvent->connection == RR_Disconnected)
        {
          m_winSystem.NotifyXRREvent();
          CServiceBroker::GetActiveAE()->DeviceChange();
          serial = xevent.xgeneric.serial;
        }
      }

      continue;
    }

    if (XFilterEvent(&xevent, None))
      continue;

    switch (xevent.type)
    {
      case MapNotify:
      {
        if (appPort)
          appPort->SetRenderGUI(true);
        break;
      }

      case UnmapNotify:
      {
        if (appPort)
          appPort->SetRenderGUI(false);
        break;
      }

      case FocusIn:
      {
        if (m_xic)
          XSetICFocus(m_xic);
        g_application.m_AppFocused = true;
        m_keymodState = 0;
        if (serial == xevent.xfocus.serial)
          break;
        m_winSystem.NotifyAppFocusChange(g_application.m_AppFocused);
        break;
      }

      case FocusOut:
      {
        if (m_xic)
          XUnsetICFocus(m_xic);
        g_application.m_AppFocused = false;
        m_winSystem.NotifyAppFocusChange(g_application.m_AppFocused);
        serial = xevent.xfocus.serial;
        break;
      }

      case Expose:
      {
        CServiceBroker::GetGUI()->GetWindowManager().MarkDirty();
        break;
      }

      case ConfigureNotify:
      {
        if (xevent.xconfigure.window != m_window)
          break;

        m_structureChanged = true;
        XBMC_Event newEvent = {};
        newEvent.type = XBMC_VIDEORESIZE;
        newEvent.resize.w = xevent.xconfigure.width;
        newEvent.resize.h = xevent.xconfigure.height;
        if (appPort)
          ret |= appPort->OnEvent(newEvent);
        CServiceBroker::GetGUI()->GetWindowManager().MarkDirty();
        break;
      }

      case ClientMessage:
      {
        if ((unsigned int)xevent.xclient.data.l[0] == m_wmDeleteMessage)
          if (!g_application.m_bStop)
            CServiceBroker::GetAppMessenger()->PostMsg(TMSG_QUIT);
        break;
      }

      case KeyPress:
      {
        XBMC_Event newEvent = {};
        newEvent.type = XBMC_KEYDOWN;
        KeySym xkeysym;

        // fallback if we have no IM
        if (!m_xic)
        {
          static XComposeStatus state;
          char keybuf[32];
          XLookupString(&xevent.xkey, NULL, 0, &xkeysym, NULL);
          newEvent.key.keysym.sym = LookupXbmcKeySym(xkeysym);
          newEvent.key.keysym.scancode = xevent.xkey.keycode;
          if (XLookupString(&xevent.xkey, keybuf, sizeof(keybuf), NULL, &state))
          {
            newEvent.key.keysym.unicode = keybuf[0];
          }
          ret |= ProcessKey(newEvent);
          break;
        }

        Status status;
        int len;
        len = Xutf8LookupString(m_xic, &xevent.xkey,
                                m_keybuf, m_keybuf_len,
                                &xkeysym, &status);
        if (status == XBufferOverflow)
        {
          m_keybuf_len = len;
          m_keybuf = (char*)realloc(m_keybuf, m_keybuf_len);
          if (m_keybuf == nullptr)
            throw std::runtime_error("Failed to realloc memory, insufficient memory available");
          len = Xutf8LookupString(m_xic, &xevent.xkey,
                                  m_keybuf, m_keybuf_len,
                                  &xkeysym, &status);
        }
        switch (status)
        {
          case XLookupNone:
            break;
          case XLookupChars:
          case XLookupBoth:
          {
            std::string data(m_keybuf, len);
            std::wstring keys;
            g_charsetConverter.utf8ToW(data, keys, false);

            if (keys.length() == 0)
            {
              break;
            }

            for (unsigned int i = 0; i < keys.length() - 1; i++)
            {
              newEvent.key.keysym.sym = XBMCK_UNKNOWN;
              newEvent.key.keysym.unicode = keys[i];
              ret |= ProcessKey(newEvent);
            }
            if (keys.length() > 0)
            {
              newEvent.key.keysym.scancode = xevent.xkey.keycode;
              XLookupString(&xevent.xkey, NULL, 0, &xkeysym, NULL);
              newEvent.key.keysym.sym = LookupXbmcKeySym(xkeysym);
              newEvent.key.keysym.unicode = keys[keys.length() - 1];

              ret |= ProcessKey(newEvent);
            }
            break;
          }

          case XLookupKeySym:
          {
            newEvent.key.keysym.scancode = xevent.xkey.keycode;
            newEvent.key.keysym.sym = LookupXbmcKeySym(xkeysym);
            ret |= ProcessKey(newEvent);
            break;
          }

        }// switch status
        break;
      } //KeyPress

      case KeyRelease:
      {
        // if we have a queued press directly after, this is a repeat
        if (XEventsQueued(m_display, QueuedAfterReading))
        {
          XEvent next_event;
          XPeekEvent(m_display, &next_event);
          if (next_event.type == KeyPress &&
              next_event.xkey.window == xevent.xkey.window &&
              next_event.xkey.keycode == xevent.xkey.keycode &&
              (next_event.xkey.time - xevent.xkey.time < 2))
            continue;
        }

        XBMC_Event newEvent = {};
        KeySym xkeysym;
        newEvent.type = XBMC_KEYUP;
        xkeysym = XLookupKeysym(&xevent.xkey, 0);
        newEvent.key.keysym.scancode = xevent.xkey.keycode;
        newEvent.key.keysym.sym = LookupXbmcKeySym(xkeysym);
        ret |= ProcessKey(newEvent);
        break;
      }

      case EnterNotify:
      {
        break;
      }

      // lose mouse coverage
      case LeaveNotify:
      {
        CServiceBroker::GetInputManager().SetMouseActive(false);
        break;
      }

      case MotionNotify:
      {
        if (xevent.xmotion.window != m_window)
          break;
        XBMC_Event newEvent = {};
        newEvent.type = XBMC_MOUSEMOTION;
        newEvent.motion.x = (int16_t)xevent.xmotion.x;
        newEvent.motion.y = (int16_t)xevent.xmotion.y;
        if (appPort)
          ret |= appPort->OnEvent(newEvent);
        break;
      }

      case ButtonPress:
      {
        XBMC_Event newEvent = {};
        newEvent.type = XBMC_MOUSEBUTTONDOWN;
        newEvent.button.button = (unsigned char)xevent.xbutton.button;
        newEvent.button.x = (int16_t)xevent.xbutton.x;
        newEvent.button.y = (int16_t)xevent.xbutton.y;
        if (appPort)
          ret |= appPort->OnEvent(newEvent);
        break;
      }

      case ButtonRelease:
      {
        XBMC_Event newEvent = {};
        newEvent.type = XBMC_MOUSEBUTTONUP;
        newEvent.button.button = (unsigned char)xevent.xbutton.button;
        newEvent.button.x = (int16_t)xevent.xbutton.x;
        newEvent.button.y = (int16_t)xevent.xbutton.y;
        if (appPort)
          ret |= appPort->OnEvent(newEvent);
        break;
      }

      default:
      {
        break;
      }
    }// switch event.type
  }// while

  if (m_display && m_xrrEventPending && m_xrrFailSafeTimer.IsTimePast())
  {
    CLog::Log(LOGERROR,"CWinEventsX11::MessagePump - missed XRR Events");
    m_winSystem.NotifyXRREvent();
    m_xrrEventPending = false;
  }

  return ret;
}

bool CWinEventsX11::ProcessKey(XBMC_Event &event)
{
  if (event.type == XBMC_KEYDOWN)
  {
    // check key modifiers
    switch(event.key.keysym.sym)
    {
      case XBMCK_LSHIFT:
        m_keymodState |= XBMCKMOD_LSHIFT;
        break;
      case XBMCK_RSHIFT:
        m_keymodState |= XBMCKMOD_RSHIFT;
        break;
      case XBMCK_LCTRL:
        m_keymodState |= XBMCKMOD_LCTRL;
        break;
      case XBMCK_RCTRL:
        m_keymodState |= XBMCKMOD_RCTRL;
        break;
      case XBMCK_LALT:
        m_keymodState |= XBMCKMOD_LALT;
        break;
      case XBMCK_RALT:
        m_keymodState |= XBMCKMOD_RCTRL;
        break;
      case XBMCK_LMETA:
        m_keymodState |= XBMCKMOD_LMETA;
        break;
      case XBMCK_RMETA:
        m_keymodState |= XBMCKMOD_RMETA;
        break;
      case XBMCK_MODE:
        m_keymodState |= XBMCKMOD_MODE;
        break;
      default:
        break;
    }
    event.key.keysym.mod = (XBMCMod)m_keymodState;
  }
  else if (event.type == XBMC_KEYUP)
  {
    switch(event.key.keysym.sym)
    {
      case XBMCK_LSHIFT:
        m_keymodState &= ~XBMCKMOD_LSHIFT;
        break;
      case XBMCK_RSHIFT:
        m_keymodState &= ~XBMCKMOD_RSHIFT;
        break;
      case XBMCK_LCTRL:
        m_keymodState &= ~XBMCKMOD_LCTRL;
        break;
      case XBMCK_RCTRL:
        m_keymodState &= ~XBMCKMOD_RCTRL;
        break;
      case XBMCK_LALT:
        m_keymodState &= ~XBMCKMOD_LALT;
        break;
      case XBMCK_RALT:
        m_keymodState &= ~XBMCKMOD_RCTRL;
        break;
      case XBMCK_LMETA:
        m_keymodState &= ~XBMCKMOD_LMETA;
        break;
      case XBMCK_RMETA:
        m_keymodState &= ~XBMCKMOD_RMETA;
        break;
      case XBMCK_MODE:
        m_keymodState &= ~XBMCKMOD_MODE;
        break;
      default:
        break;
    }
    event.key.keysym.mod = (XBMCMod)m_keymodState;
  }

  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->OnEvent(event);
  return true;
}

XBMCKey CWinEventsX11::LookupXbmcKeySym(KeySym keysym)
{
  // try direct mapping first
  std::map<uint32_t, uint32_t>::iterator it;
  it = m_symLookupTable.find(keysym);
  if (it != m_symLookupTable.end())
  {
    return (XBMCKey)(it->second);
  }

  // try ascii mappings
  if (keysym>>8 == 0x00)
    return (XBMCKey)tolower(keysym & 0xFF);

  return (XBMCKey)keysym;
}
