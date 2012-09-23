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
#include <X11/Xlib.h>
#include "X11/WinSystemX11GL.h"
#include "X11/keysymdef.h"
#include "X11/XF86keysym.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "guilib/GUIWindowManager.h"
#include "input/MouseStat.h"
#include "ApplicationMessenger.h"

#if defined(HAS_XRANDR)
#include <X11/extensions/Xrandr.h>
#endif

#ifdef HAS_SDL_JOYSTICK
#include "input/SDLJoystick.h"
#endif

CWinEventsX11* CWinEventsX11::WinEvents = 0;

static const uint32_t SymMappingsX11[][2] =
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

CWinEventsX11::~CWinEventsX11()
{
  free(m_keybuf);
  m_keybuf = 0;

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

bool CWinEventsX11::Init(Display *dpy, Window win)
{
  if (WinEvents)
    return true;
  WinEvents = new CWinEventsX11(dpy, win);
  return true;
}

CWinEventsX11::CWinEventsX11(Display *dpy, Window win)
{
  m_display = dpy;
  m_window = win;
  m_keybuf_len = 32*sizeof(char);
  m_keybuf = (char*)malloc(m_keybuf_len);
  m_keymodState = 0;
  m_wmDeleteMessage = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  memset(&m_compose, 0, sizeof(m_compose));

  // open input method
  CStdString old_locale, old_modifiers;
  char res_name[] = "xbmc";

  // save current locale, this should be "C"
  old_locale    = setlocale(LC_ALL, NULL);
  old_modifiers = XSetLocaleModifiers(NULL);

  // set users preferences and open input method
  setlocale(LC_ALL, "");
  XSetLocaleModifiers("");

  m_xim = XOpenIM(m_display, NULL, res_name, res_name);

  // restore original locale
  XSetLocaleModifiers(old_modifiers.c_str());
  setlocale(LC_ALL, old_locale.c_str());

  m_xic = NULL;
  if (m_xim)
  {
    m_xic = XCreateIC(m_xim,
                      XNClientWindow , m_window,
                      XNFocusWindow  , m_window,
                      XNInputStyle   , XIMPreeditNothing | XIMStatusNothing,
                      XNResourceName , res_name,
                      XNResourceClass, res_name,
                      NULL);
  }

  if (!m_xic)
    CLog::Log(LOGWARNING,"CWinEventsX11::Init - no input method found");

  // build Keysym lookup table
  for (unsigned int i = 0; i < sizeof(SymMappingsX11)/(2*sizeof(uint32_t)); ++i)
  {
    m_symLookupTable[SymMappingsX11[i][0]] = SymMappingsX11[i][1];
  }

  // register for xrandr events
#if defined(HAS_XRANDR)
  int iReturn;
  XRRQueryExtension(m_display, &m_RREventBase, &iReturn);
  XRRSelectInput(m_display, m_window, RRScreenChangeNotifyMask);
#endif
}

void CWinEventsX11::Quit()
{
  if (!WinEvents)
    return;

  delete WinEvents;
  WinEvents = NULL;
}

bool CWinEventsX11::MessagePump()
{
  if (!WinEvents)
    return false;
  return WinEvents->Process();
}

bool CWinEventsX11::ProcessKeyPress(XKeyEvent& xevent)
{
  XBMC_Event newEvent = {0};
  KeySym xkeysym;
  bool ret = false;
  Status status;
  int len;
  CStdString data;
  CStdStringW keys;

  // fallback if we have no IM
  if (m_xic)
  {
    len = Xutf8LookupString(m_xic, &xevent,
                            m_keybuf, m_keybuf_len,
                            &xkeysym, &status);
    if (status == XBufferOverflow)
    {
      m_keybuf_len = len;
      m_keybuf = (char*)realloc(m_keybuf, m_keybuf_len);
      len = Xutf8LookupString(m_xic, &xevent,
                              m_keybuf, m_keybuf_len,
                              &xkeysym, &status);
    }
    data.assign(m_keybuf, len);
    g_charsetConverter.utf8ToW(data, keys, false);
  }
  else
  {
    len = XLookupString(&xevent, m_keybuf, m_keybuf_len, &xkeysym, &m_compose);
    if(len > 0)
      status = XLookupBoth;
    else
      status = XLookupKeySym;
    data.assign(m_keybuf, len);
    g_charsetConverter.toW(data, keys, "LATIN1");
  }


  /* apparently Xutf8LookupString will not always return
   * the expected keysym for Ctrl+S and will instead
   * return Ctrl+s (lowercase). This causes keymap
   * issues. Investigation needed to see if it can be
   * removed
   */
  xkeysym = XLookupKeysym(&xevent, 0);

  switch (status)
  {
    case XLookupNone:
      break;
    case XLookupChars:
    case XLookupBoth:
    {
      if (keys.length() == 0)
      {
        break;
      }

      for (unsigned int i = 0; i < keys.length() - 1; i++)
      {
        newEvent.key.keysym.sym     = XBMCK_UNKNOWN;
        newEvent.key.keysym.unicode = keys[i];
        newEvent.key.state          = XBMC_PRESSED;
        newEvent.key.type           = XBMC_KEYDOWN;
        ret |= ProcessKey(newEvent);
      }
      if (keys.length() > 0)
      {
        newEvent.key.keysym.scancode = xevent.keycode;
        newEvent.key.keysym.sym     = LookupXbmcKeySym(xkeysym);
        newEvent.key.keysym.unicode = keys[keys.length() - 1];
        newEvent.key.state          = XBMC_PRESSED;
        newEvent.key.type           = XBMC_KEYDOWN;
        ret |= ProcessKey(newEvent);
      }
      break;
    }

    case XLookupKeySym:
    {
      newEvent.key.keysym.scancode = xevent.keycode;
      newEvent.key.keysym.sym      = LookupXbmcKeySym(xkeysym);
      newEvent.key.state           = XBMC_PRESSED;
      newEvent.key.type            = XBMC_KEYDOWN;
      ret |= ProcessKey(newEvent);
      break;
    }

  }

  return ret;
}

bool CWinEventsX11::ProcessKeyRelease(XKeyEvent& xkey)
{
  /* if we have a queued press directly after, this is a repeat */
  if( XEventsQueued( m_display, QueuedAfterReading ) )
  {
    XEvent next_event;
    XPeekEvent( m_display, &next_event );
    if(next_event.type == KeyPress
    && next_event.xkey.window  == xkey.window
    && next_event.xkey.keycode == xkey.keycode
    && next_event.xkey.time    == xkey.time )
      return false;
  }

  XBMC_Event newEvent = {0};
  newEvent.key.keysym.scancode = xkey.keycode;
  newEvent.key.keysym.sym      = LookupXbmcKeySym(XLookupKeysym(&xkey, 0));
  newEvent.key.state           = XBMC_RELEASED;
  newEvent.key.type            = XBMC_KEYUP;
  return ProcessKey(newEvent);
}

bool CWinEventsX11::ProcessConfigure (XConfigureEvent& xevent)
{
  if (xevent.window != m_window)
    return false;

  bool ret = false;

  /* find the last configure event in the queue */
  while(XCheckTypedWindowEvent(m_display, m_window, ConfigureNotify, (XEvent*)&xevent))
    ;

  /* check for resize */
  if((int)g_Windowing.GetWidth()  != xevent.width
  || (int)g_Windowing.GetHeight() != xevent.height)
  {
    XBMC_Event newEvent = {0};
    newEvent.type = XBMC_VIDEORESIZE;
    newEvent.resize.w = xevent.width;
    newEvent.resize.h = xevent.height;
    ret |= g_application.OnEvent(newEvent);
  }

  /* real events have position relative to parent */
  if(!xevent.send_event) {
    XWindowAttributes attr;
    Window child;
    XGetWindowAttributes(xevent.display, xevent.window, &attr);
    XTranslateCoordinates(xevent.display
                        , xevent.window, attr.root
                        , xevent.x, xevent.x
                        , &xevent.x, &xevent.y
                        , &child);
  }

  /* check for move */
  if(g_Windowing.GetLeft()  != xevent.x
  || g_Windowing.GetTop()   != xevent.y)
  {
    XBMC_Event newEvent = {0};
    newEvent.type = XBMC_VIDEOMOVE;
    newEvent.move.x = xevent.x;
    newEvent.move.y = xevent.y;
    ret |= g_application.OnEvent(newEvent);
  }
  return ret;
}

bool CWinEventsX11::ProcessMotion(XMotionEvent& xmotion)
{
  XBMC_Event newEvent = {0};
  newEvent.type = XBMC_MOUSEMOTION;
  newEvent.motion.xrel = (int16_t)xmotion.x_root;
  newEvent.motion.yrel = (int16_t)xmotion.y_root;
  newEvent.motion.x    = (int16_t)xmotion.x;
  newEvent.motion.y    = (int16_t)xmotion.y;
  return g_application.OnEvent(newEvent);
}

bool CWinEventsX11::ProcessEnter(XCrossingEvent& xcrossing)
{
  return true;
}

bool CWinEventsX11::ProcessLeave(XCrossingEvent& xcrossing)
{
  g_Mouse.SetActive(false);
  return true;
}

bool CWinEventsX11::ProcessButtonPress(XButtonEvent& xbutton)
{
  XBMC_Event newEvent = {0};
  newEvent.type = XBMC_MOUSEBUTTONDOWN;
  newEvent.button.button = (unsigned char)(xbutton.button);
  newEvent.button.state  = XBMC_PRESSED;
  newEvent.button.x      = (int16_t)(xbutton.x);
  newEvent.button.y      = (int16_t)(xbutton.y);
  return g_application.OnEvent(newEvent);
}

bool CWinEventsX11::ProcessButtonRelease(XButtonEvent& xbutton)
{
  XBMC_Event newEvent = {0};
  newEvent.type = XBMC_MOUSEBUTTONUP;
  newEvent.button.button = (unsigned char)(xbutton.button);
  newEvent.button.state  = XBMC_RELEASED;
  newEvent.button.x      = (int16_t)(xbutton.x);
  newEvent.button.y      = (int16_t)(xbutton.y);
  return g_application.OnEvent(newEvent);
}

bool CWinEventsX11::ProcessClientMessage(XClientMessageEvent& xclient)
{
  if ((Atom)(xclient.data.l[0]) == m_wmDeleteMessage)
  {
    if (!g_application.m_bStop)
      CApplicationMessenger::Get().Quit();
    return true;
  }
  return false;
}

bool CWinEventsX11::ProcessFocusIn(XFocusInEvent& xfocus)
{
  if (m_xic)
    XSetICFocus(m_xic);

  g_application.m_AppFocused = true;
  m_keymodState = 0;
  g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
  return true;
}

bool CWinEventsX11::ProcessFocusOut(XFocusOutEvent& xfocus)
{
  if (m_xic)
    XUnsetICFocus(m_xic);

  g_application.m_AppFocused = false;
  g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
  return true;
}

bool CWinEventsX11::Process()
{
  bool ret = false;
  XEvent xevent;

  while (WinEvents && XPending(m_display))
  {
    memset(&xevent, 0, sizeof (XEvent));
    XNextEvent(m_display, &xevent);

    if (XFilterEvent(&xevent, None))
      continue;


    if(!g_Windowing.IsWindowManagerControlled())
    {
      switch (xevent.type)
      {
        case ButtonPress:
        case ButtonRelease:
          XSetInputFocus(m_display, m_window, RevertToParent, xevent.xbutton.time);
          break;
        case KeyPress:
        case KeyRelease:
          XSetInputFocus(m_display, m_window, RevertToParent, xevent.xkey.time);
          break;
        case MapNotify:
          XSetInputFocus(m_display, m_window, RevertToParent, CurrentTime);
          break;
      }
    }

    switch (xevent.type)
    {
      case MapNotify:
        CLog::Log(LOGDEBUG,"CWinEventsX11::map %ld", xevent.xmap.window);
        g_application.m_AppActive = true;
        break;

      case UnmapNotify:
        g_application.m_AppActive = false;
        break;

      case FocusIn:
        CLog::Log(LOGDEBUG,"CWinEventsX11::focus in %ld %d %d", xevent.xfocus.window, xevent.xfocus.mode, xevent.xfocus.detail);
        ret |= ProcessFocusIn(xevent.xfocus);
        break;

      case FocusOut:
        CLog::Log(LOGDEBUG,"CWinEventsX11::focus out %ld %d %d", xevent.xfocus.window, xevent.xfocus.mode, xevent.xfocus.detail);
        ret |= ProcessFocusOut(xevent.xfocus);
        break;

      case Expose:
        g_windowManager.MarkDirty();
        break;

      case ConfigureNotify:
        ret |= ProcessConfigure(xevent.xconfigure);
        break;

      case ClientMessage:
        ret |= ProcessClientMessage(xevent.xclient);
        break;

      case KeyPress:
        ret |= ProcessKeyPress(xevent.xkey);
        break;

      case KeyRelease:
        ret |= ProcessKeyRelease(xevent.xkey);
        break;

      case EnterNotify:
        CLog::Log(LOGDEBUG,"CWinEventsX11::enter %ld %d", xevent.xcrossing.window, xevent.xcrossing.mode);
        ret |= ProcessEnter(xevent.xcrossing);
        break;

      case LeaveNotify:
        CLog::Log(LOGDEBUG,"CWinEventsX11::leave %ld %d", xevent.xcrossing.window, xevent.xcrossing.mode);
        ret |= ProcessLeave(xevent.xcrossing);
        break;

      case MotionNotify:
        ret |= ProcessMotion(xevent.xmotion);
        break;

      case ButtonPress:
        ret |= ProcessButtonPress(xevent.xbutton);
        break;

      case ButtonRelease:
        ret |= ProcessButtonRelease(xevent.xbutton);
        break;

      default:
        break;
    }// switch event.type

#if defined(HAS_XRANDR)
    if (WinEvents && xevent.type == m_RREventBase + RRScreenChangeNotify)
    {
      XRRUpdateConfiguration(&xevent);
      g_Windowing.NotifyXRREvent();
    }
#endif

  }// while

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

    bool ret = ProcessShortcuts(event);
    if (ret)
      return ret;
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

  return g_application.OnEvent(event);
}

bool CWinEventsX11::ProcessShortcuts(XBMC_Event& event)
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
    return (XBMCKey)(keysym & 0xFF);

  return (XBMCKey)keysym;
}
#endif
