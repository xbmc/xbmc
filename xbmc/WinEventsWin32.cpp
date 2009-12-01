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

#include "utils/log.h"
#include "Windowsx.h"
#include "WinEvents.h"
#include "WIN32Util.h"
#include "Win32StorageProvider.h"
#include "Application.h"
#include "XBMC_vkeys.h"
#include "MouseStat.h"
#include "MediaManager.h"
#include "WindowingFactory.h"
#include <dbt.h>
#include "LocalizeStrings.h"

#ifdef _WIN32

/* Masks for processing the windows KEYDOWN and KEYUP messages */
#define REPEATED_KEYMASK  (1<<30)
#define EXTENDED_KEYMASK  (1<<24)
#define EXTKEYPAD(keypad) ((scancode & 0x100)?(mvke):(keypad))

static XBMCKey VK_keymap[XBMCK_LAST];
static HKL hLayoutUS = NULL;
static XBMCKey Arrows_keymap[4];

uint32_t g_uQueryCancelAutoPlay = 0;

int XBMC_TranslateUNICODE = 1;

PHANDLE_EVENT_FUNC CWinEventsBase::m_pEventFunc = NULL;

void DIB_InitOSKeymap()
{
  char current_layout[KL_NAMELENGTH];

  GetKeyboardLayoutName(current_layout);

  hLayoutUS = LoadKeyboardLayout("00000409", KLF_NOTELLSHELL);
  if (!hLayoutUS)
    hLayoutUS = GetKeyboardLayout(0);

  LoadKeyboardLayout(current_layout, KLF_ACTIVATE);

  /* Map the VK keysyms */
  for (int i = 0; i < XBMC_arraysize(VK_keymap); ++i)
    VK_keymap[i] = XBMCK_UNKNOWN;

  VK_keymap[VK_BACK] = XBMCK_BACKSPACE;
  VK_keymap[VK_TAB] = XBMCK_TAB;
  VK_keymap[VK_CLEAR] = XBMCK_CLEAR;
  VK_keymap[VK_RETURN] = XBMCK_RETURN;
  VK_keymap[VK_PAUSE] = XBMCK_PAUSE;
  VK_keymap[VK_ESCAPE] = XBMCK_ESCAPE;
  VK_keymap[VK_SPACE] = XBMCK_SPACE;
  VK_keymap[VK_APOSTROPHE] = XBMCK_QUOTE;
  VK_keymap[VK_COMMA] = XBMCK_COMMA;
  VK_keymap[VK_MINUS] = XBMCK_MINUS;
  VK_keymap[VK_PERIOD] = XBMCK_PERIOD;
  VK_keymap[VK_SLASH] = XBMCK_SLASH;
  VK_keymap[VK_0] = XBMCK_0;
  VK_keymap[VK_1] = XBMCK_1;
  VK_keymap[VK_2] = XBMCK_2;
  VK_keymap[VK_3] = XBMCK_3;
  VK_keymap[VK_4] = XBMCK_4;
  VK_keymap[VK_5] = XBMCK_5;
  VK_keymap[VK_6] = XBMCK_6;
  VK_keymap[VK_7] = XBMCK_7;
  VK_keymap[VK_8] = XBMCK_8;
  VK_keymap[VK_9] = XBMCK_9;
  VK_keymap[VK_SEMICOLON] = XBMCK_SEMICOLON;
  VK_keymap[VK_EQUALS] = XBMCK_EQUALS;
  VK_keymap[VK_LBRACKET] = XBMCK_LEFTBRACKET;
  VK_keymap[VK_BACKSLASH] = XBMCK_BACKSLASH;
  VK_keymap[VK_OEM_102] = XBMCK_LESS;
  VK_keymap[VK_RBRACKET] = XBMCK_RIGHTBRACKET;
  VK_keymap[VK_GRAVE] = XBMCK_BACKQUOTE;
  VK_keymap[VK_BACKTICK] = XBMCK_BACKQUOTE;
  VK_keymap[VK_A] = XBMCK_a;
  VK_keymap[VK_B] = XBMCK_b;
  VK_keymap[VK_C] = XBMCK_c;
  VK_keymap[VK_D] = XBMCK_d;
  VK_keymap[VK_E] = XBMCK_e;
  VK_keymap[VK_F] = XBMCK_f;
  VK_keymap[VK_G] = XBMCK_g;
  VK_keymap[VK_H] = XBMCK_h;
  VK_keymap[VK_I] = XBMCK_i;
  VK_keymap[VK_J] = XBMCK_j;
  VK_keymap[VK_K] = XBMCK_k;
  VK_keymap[VK_L] = XBMCK_l;
  VK_keymap[VK_M] = XBMCK_m;
  VK_keymap[VK_N] = XBMCK_n;
  VK_keymap[VK_O] = XBMCK_o;
  VK_keymap[VK_P] = XBMCK_p;
  VK_keymap[VK_Q] = XBMCK_q;
  VK_keymap[VK_R] = XBMCK_r;
  VK_keymap[VK_S] = XBMCK_s;
  VK_keymap[VK_T] = XBMCK_t;
  VK_keymap[VK_U] = XBMCK_u;
  VK_keymap[VK_V] = XBMCK_v;
  VK_keymap[VK_W] = XBMCK_w;
  VK_keymap[VK_X] = XBMCK_x;
  VK_keymap[VK_Y] = XBMCK_y;
  VK_keymap[VK_Z] = XBMCK_z;
  VK_keymap[VK_DELETE] = XBMCK_DELETE;

  VK_keymap[VK_NUMPAD0] = XBMCK_KP0;
  VK_keymap[VK_NUMPAD1] = XBMCK_KP1;
  VK_keymap[VK_NUMPAD2] = XBMCK_KP2;
  VK_keymap[VK_NUMPAD3] = XBMCK_KP3;
  VK_keymap[VK_NUMPAD4] = XBMCK_KP4;
  VK_keymap[VK_NUMPAD5] = XBMCK_KP5;
  VK_keymap[VK_NUMPAD6] = XBMCK_KP6;
  VK_keymap[VK_NUMPAD7] = XBMCK_KP7;
  VK_keymap[VK_NUMPAD8] = XBMCK_KP8;
  VK_keymap[VK_NUMPAD9] = XBMCK_KP9;
  VK_keymap[VK_DECIMAL] = XBMCK_KP_PERIOD;
  VK_keymap[VK_DIVIDE] = XBMCK_KP_DIVIDE;
  VK_keymap[VK_MULTIPLY] = XBMCK_KP_MULTIPLY;
  VK_keymap[VK_SUBTRACT] = XBMCK_KP_MINUS;
  VK_keymap[VK_ADD] = XBMCK_KP_PLUS;

  VK_keymap[VK_UP] = XBMCK_UP;
  VK_keymap[VK_DOWN] = XBMCK_DOWN;
  VK_keymap[VK_RIGHT] = XBMCK_RIGHT;
  VK_keymap[VK_LEFT] = XBMCK_LEFT;
  VK_keymap[VK_INSERT] = XBMCK_INSERT;
  VK_keymap[VK_HOME] = XBMCK_HOME;
  VK_keymap[VK_END] = XBMCK_END;
  VK_keymap[VK_PRIOR] = XBMCK_PAGEUP;
  VK_keymap[VK_NEXT] = XBMCK_PAGEDOWN;

  VK_keymap[VK_F1] = XBMCK_F1;
  VK_keymap[VK_F2] = XBMCK_F2;
  VK_keymap[VK_F3] = XBMCK_F3;
  VK_keymap[VK_F4] = XBMCK_F4;
  VK_keymap[VK_F5] = XBMCK_F5;
  VK_keymap[VK_F6] = XBMCK_F6;
  VK_keymap[VK_F7] = XBMCK_F7;
  VK_keymap[VK_F8] = XBMCK_F8;
  VK_keymap[VK_F9] = XBMCK_F9;
  VK_keymap[VK_F10] = XBMCK_F10;
  VK_keymap[VK_F11] = XBMCK_F11;
  VK_keymap[VK_F12] = XBMCK_F12;
  VK_keymap[VK_F13] = XBMCK_F13;
  VK_keymap[VK_F14] = XBMCK_F14;
  VK_keymap[VK_F15] = XBMCK_F15;

  VK_keymap[VK_NUMLOCK] = XBMCK_NUMLOCK;
  VK_keymap[VK_CAPITAL] = XBMCK_CAPSLOCK;
  VK_keymap[VK_SCROLL] = XBMCK_SCROLLOCK;
  VK_keymap[VK_RSHIFT] = XBMCK_RSHIFT;
  VK_keymap[VK_LSHIFT] = XBMCK_LSHIFT;
  VK_keymap[VK_RCONTROL] = XBMCK_RCTRL;
  VK_keymap[VK_LCONTROL] = XBMCK_LCTRL;
  VK_keymap[VK_RMENU] = XBMCK_RALT;
  VK_keymap[VK_LMENU] = XBMCK_LALT;
  VK_keymap[VK_RWIN] = XBMCK_RSUPER;
  VK_keymap[VK_LWIN] = XBMCK_LSUPER;

  VK_keymap[VK_HELP] = XBMCK_HELP;
#ifdef VK_PRINT
  VK_keymap[VK_PRINT] = XBMCK_PRINT;
#endif
  VK_keymap[VK_SNAPSHOT] = XBMCK_PRINT;
  VK_keymap[VK_CANCEL] = XBMCK_BREAK;
  VK_keymap[VK_APPS] = XBMCK_MENU;

  Arrows_keymap[3] = (XBMCKey)0x25;
  Arrows_keymap[2] = (XBMCKey)0x26;
  Arrows_keymap[1] = (XBMCKey)0x27;
  Arrows_keymap[0] = (XBMCKey)0x28;
}

static int XBMC_MapVirtualKey(int scancode, int vkey)
{
  int mvke = MapVirtualKeyEx(scancode & 0xFF, 1, hLayoutUS);

  switch(vkey)
  { /* These are always correct */
    case VK_DIVIDE:
    case VK_MULTIPLY:
    case VK_SUBTRACT:
    case VK_ADD:
    case VK_LWIN:
    case VK_RWIN:
    case VK_APPS:
    /* These are already handled */
    case VK_LCONTROL:
    case VK_RCONTROL:
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_LMENU:
    case VK_RMENU:
    case VK_SNAPSHOT:
    case VK_PAUSE:
      return vkey;
  }
  switch(mvke)
  { /* Distinguish between keypad and extended keys */
    case VK_INSERT: return EXTKEYPAD(VK_NUMPAD0);
    case VK_DELETE: return EXTKEYPAD(VK_DECIMAL);
    case VK_END:    return EXTKEYPAD(VK_NUMPAD1);
    case VK_DOWN:   return EXTKEYPAD(VK_NUMPAD2);
    case VK_NEXT:   return EXTKEYPAD(VK_NUMPAD3);
    case VK_LEFT:   return EXTKEYPAD(VK_NUMPAD4);
    case VK_CLEAR:  return EXTKEYPAD(VK_NUMPAD5);
    case VK_RIGHT:  return EXTKEYPAD(VK_NUMPAD6);
    case VK_HOME:   return EXTKEYPAD(VK_NUMPAD7);
    case VK_UP:     return EXTKEYPAD(VK_NUMPAD8);
    case VK_PRIOR:  return EXTKEYPAD(VK_NUMPAD9);
  }
  return mvke ? mvke : vkey;
}

static XBMC_keysym *TranslateKey(WPARAM vkey, UINT scancode, XBMC_keysym *keysym, int pressed)
{
  /* Set the keysym information */
  keysym->scancode = (unsigned char) scancode;
  keysym->mod = XBMCKMOD_NONE;
  keysym->unicode = 0;

  if ((vkey == VK_RETURN) && (scancode & 0x100)) 
  {
    /* No VK_ code for the keypad enter key */
    keysym->sym = XBMCK_KP_ENTER;
  }
  else 
  {
    keysym->sym = VK_keymap[XBMC_MapVirtualKey(scancode, vkey)];
  }

  if ( pressed && XBMC_TranslateUNICODE ) {

    uint8_t   keystate[256];
    uint16_t  wchars[2];

    GetKeyboardState(keystate);
    /* Numlock isn't taken into account in ToUnicode,
    * so we handle it as a special case here */
    if ((keystate[VK_NUMLOCK] & 1) && vkey >= VK_NUMPAD0 && vkey <= VK_NUMPAD9)
    {
      keysym->unicode = vkey - VK_NUMPAD0 + '0';
    }
    else if (ToUnicode((UINT)vkey, scancode, keystate, (LPWSTR)wchars, sizeof(wchars)/sizeof(wchars[0]), 0) > 0)
    {
      keysym->unicode = wchars[0];
    }
  }
  return(keysym);
}

bool CWinEventsWin32::MessagePump()
{ 
  MSG  msg;
  while( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
  {
    TranslateMessage( &msg );
    DispatchMessage( &msg );
  }
  return true;
}

LRESULT CALLBACK CWinEventsWin32::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  XBMC_Event newEvent;
  ZeroMemory(&newEvent, sizeof(newEvent));

  if (uMsg == WM_CREATE)
  {
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)(((LPCREATESTRUCT)lParam)->lpCreateParams));
    DIB_InitOSKeymap();
    g_uQueryCancelAutoPlay = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));
    return 0;
  }

  m_pEventFunc = (PHANDLE_EVENT_FUNC)GetWindowLongPtr(hWnd, GWLP_USERDATA);
  if (!m_pEventFunc)
    return DefWindowProc(hWnd, uMsg, wParam, lParam);

  if(g_uQueryCancelAutoPlay != 0 && uMsg == g_uQueryCancelAutoPlay)
    return S_FALSE;

  switch (uMsg) 
  {
    case WM_CLOSE:
    case WM_QUIT:
    case WM_DESTROY:
      newEvent.type = XBMC_QUIT;
      m_pEventFunc(newEvent);
      break;
    case WM_SHOWWINDOW:
      {
        bool active = g_application.m_AppActive;
        g_application.m_AppActive = wParam != 0;
        if (g_application.m_AppActive != active)
          g_Windowing.NotifyAppActiveChange(g_application.m_AppActive);
        CLog::Log(LOGDEBUG, __FUNCTION__"Window is %s", g_application.m_AppActive ? "shown" : "hidden");
      }
      break;
    case WM_ACTIVATE:
      {
        bool active = g_application.m_AppActive;
        if (HIWORD(wParam))
        {
          g_application.m_AppActive = false;
        }
        else
        {
          WINDOWPLACEMENT lpwndpl;
          lpwndpl.length = sizeof(lpwndpl);
          if (LOWORD(wParam) != WA_INACTIVE && GetWindowPlacement(hWnd, &lpwndpl))
          {
            g_application.m_AppActive = lpwndpl.showCmd != SW_HIDE;
          }
        }
        if (g_application.m_AppActive != active)
          g_Windowing.NotifyAppActiveChange(g_application.m_AppActive);
        CLog::Log(LOGDEBUG, __FUNCTION__"Window is %s", g_application.m_AppActive ? "active" : "inactive");
      }
      break;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
      g_application.m_AppFocused = uMsg == WM_SETFOCUS;
      CLog::Log(LOGDEBUG, __FUNCTION__"Window %s focus", g_application.m_AppFocused ? "gained" : "lost");
      g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
      break;
    case WM_SYSKEYDOWN:
      switch (wParam) 
      {
        case VK_F4: //alt-f4, default event quit.
          return(DefWindowProc(hWnd, uMsg, wParam, lParam));
        case VK_RETURN: //alt-return
          if ((lParam & REPEATED_KEYMASK) == 0)
            g_graphicsContext.ToggleFullScreenRoot();
          return 0;
      }
      //deliberate fallthrough
    case WM_KEYDOWN: 
    {
      switch (wParam) 
      {
        case VK_CONTROL:
          if ( lParam & EXTENDED_KEYMASK )
            wParam = VK_RCONTROL;
          else
            wParam = VK_LCONTROL;
          break;
        case VK_SHIFT:
          /* EXTENDED trick doesn't work here */
          if (GetKeyState(VK_LSHIFT) & 0x8000)
            wParam = VK_LSHIFT;
          else if (GetKeyState(VK_RSHIFT) & 0x8000)
            wParam = VK_RSHIFT;
          break;
        case VK_MENU:
          if ( lParam & EXTENDED_KEYMASK )
            wParam = VK_RMENU;
          else
            wParam = VK_LMENU;
          break;
      }
      XBMC_keysym keysym;
      TranslateKey(wParam, HIWORD(lParam), &keysym, 1);

      newEvent.type = XBMC_KEYDOWN;
      newEvent.key.keysym = keysym;
      m_pEventFunc(newEvent);
    }
    return(0);

    case WM_SYSKEYUP:
    case WM_KEYUP: 
      {
      switch (wParam) 
      {
        case VK_CONTROL:
          if ( lParam&EXTENDED_KEYMASK )
            wParam = VK_RCONTROL;
          else
            wParam = VK_LCONTROL;
          break;
        case VK_SHIFT:
          {
            uint32_t scanCodeL = MapVirtualKey(VK_LSHIFT, MAPVK_VK_TO_VSC);
            uint32_t scanCodeR = MapVirtualKey(VK_RSHIFT, MAPVK_VK_TO_VSC);
            uint32_t keyCode = (uint32_t)((lParam & 0xFF0000) >> 16);
            if (keyCode == scanCodeL) 
              wParam = VK_LSHIFT;
            else if (keyCode == scanCodeR)
              wParam = VK_RSHIFT;
          }
          break;
        case VK_MENU:
          if ( lParam&EXTENDED_KEYMASK )
            wParam = VK_RMENU;
          else
            wParam = VK_LMENU;
          break;
      }
      XBMC_keysym keysym;
      TranslateKey(wParam, HIWORD(lParam), &keysym, 1);

      if (wParam == VK_SNAPSHOT)
        newEvent.type = XBMC_KEYDOWN;
      else
        newEvent.type = XBMC_KEYUP;
      newEvent.key.keysym = keysym;
      m_pEventFunc(newEvent);
    }
    return(0);
    case WM_APPCOMMAND: // MULTIMEDIA keys are mapped to APPCOMMANDS
    {
      CLog::Log(LOGDEBUG, "WinEventsWin32.cpp: APPCOMMAND %d", GET_APPCOMMAND_LPARAM(lParam));
      newEvent.appcommand.type = XBMC_APPCOMMAND;
      newEvent.appcommand.action = 0;

      switch (GET_APPCOMMAND_LPARAM(lParam))
      {
        case APPCOMMAND_MEDIA_PLAY:
          newEvent.appcommand.action = ACTION_PLAYER_PLAY;
          break;
        case APPCOMMAND_MEDIA_PAUSE:
          newEvent.appcommand.action = ACTION_PAUSE;
          break;
        case APPCOMMAND_MEDIA_PLAY_PAUSE:
          if (g_application.IsPaused())
            newEvent.appcommand.action = ACTION_PLAYER_PLAY;
          else
            newEvent.appcommand.action = ACTION_PAUSE;
          break;
        case APPCOMMAND_BROWSER_BACKWARD:
          newEvent.appcommand.action = ACTION_PARENT_DIR;
          break;
        case APPCOMMAND_MEDIA_STOP:
          newEvent.appcommand.action = ACTION_STOP;
          break;
        case APPCOMMAND_MEDIA_PREVIOUSTRACK:
          newEvent.appcommand.action = ACTION_PREV_ITEM;
          break;
        case APPCOMMAND_MEDIA_NEXTTRACK:
          newEvent.appcommand.action = ACTION_NEXT_ITEM;
          break;
        case APPCOMMAND_LAUNCH_MEDIA_SELECT:
          // disable launch of external media players
          return 1;
      }
      if (newEvent.appcommand.action != 0)
      {
        m_pEventFunc(newEvent);
        return 1; // should return TRUE if application handled the event
      }
      break;
    }
    case WM_SYSCHAR:
      if (wParam == VK_RETURN) //stop system beep on alt-return
        return 0;
      break;
    case WM_SETCURSOR:
      if (HTCLIENT != LOWORD(lParam))
        g_Windowing.ShowOSMouse(true);
      break;
    case WM_MOUSEMOVE:
      newEvent.type = XBMC_MOUSEMOTION;
      newEvent.motion.x = GET_X_LPARAM(lParam);
      newEvent.motion.y = GET_Y_LPARAM(lParam);
      newEvent.motion.state = 0;
      m_pEventFunc(newEvent);
      return(0);
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
      newEvent.type = XBMC_MOUSEBUTTONDOWN;
      newEvent.button.state = XBMC_PRESSED;
      newEvent.button.x = GET_X_LPARAM(lParam);
      newEvent.button.y = GET_Y_LPARAM(lParam);
      newEvent.button.button = 0;
      if (uMsg == WM_LBUTTONDOWN) newEvent.button.button = XBMC_BUTTON_LEFT;
      else if (uMsg == WM_MBUTTONDOWN) newEvent.button.button = XBMC_BUTTON_MIDDLE;
      else if (uMsg == WM_RBUTTONDOWN) newEvent.button.button = XBMC_BUTTON_RIGHT;
      m_pEventFunc(newEvent);
      return(0);
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
      newEvent.type = XBMC_MOUSEBUTTONUP;
      newEvent.button.state = XBMC_RELEASED;
      newEvent.button.x = GET_X_LPARAM(lParam);
      newEvent.button.y = GET_Y_LPARAM(lParam);
      newEvent.button.button = 0;
      if (uMsg == WM_LBUTTONUP) newEvent.button.button = XBMC_BUTTON_LEFT;
      else if (uMsg == WM_MBUTTONUP) newEvent.button.button = XBMC_BUTTON_MIDDLE;
      else if (uMsg == WM_RBUTTONUP) newEvent.button.button = XBMC_BUTTON_RIGHT;
      m_pEventFunc(newEvent);
      return(0);
    case WM_MOUSEWHEEL:
      {
        // SDL, which our events system is based off, sends a MOUSEBUTTONDOWN message
        // followed by a MOUSEBUTTONUP message.  As this is a momentary event, we just
        // react on the MOUSEBUTTONUP message, resetting the state after processing.
        newEvent.type = XBMC_MOUSEBUTTONDOWN;
        newEvent.button.state = XBMC_PRESSED;
        // the coordinates in WM_MOUSEWHEEL are screen, not client coordinates
        POINT point;
        point.x = GET_X_LPARAM(lParam);
        point.y = GET_Y_LPARAM(lParam);
        WindowFromScreenCoords(hWnd, &point);
        newEvent.button.x = (uint16_t)point.x;
        newEvent.button.y = (uint16_t)point.y;
        newEvent.button.button = GET_Y_LPARAM(wParam) > 0 ? XBMC_BUTTON_WHEELUP : XBMC_BUTTON_WHEELDOWN;
        m_pEventFunc(newEvent);
        newEvent.type = XBMC_MOUSEBUTTONUP;
        newEvent.button.state = XBMC_RELEASED;
        m_pEventFunc(newEvent);
      }
      return(0);
    case WM_SIZE:
      newEvent.type = XBMC_VIDEORESIZE;
      newEvent.resize.type = XBMC_VIDEORESIZE;
      newEvent.resize.w = GET_X_LPARAM(lParam);
      newEvent.resize.h = GET_Y_LPARAM(lParam);
      if (newEvent.resize.w * newEvent.resize.h)
        m_pEventFunc(newEvent);
      return(0);
    case WM_DEVICECHANGE:
      PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam;
      switch(wParam)
      {
        case DBT_DEVICEARRIVAL:
           if (lpdb -> dbch_devicetype == DBT_DEVTYP_VOLUME)
           {
              PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;

              // Check whether a CD or DVD was inserted into a drive.
              if (lpdbv -> dbcv_flags & DBTF_MEDIA)
              {
                CLog::Log(LOGDEBUG, "%s: Drive %c: Media has arrived.\n", __FUNCTION__, CWIN32Util::FirstDriveFromMask(lpdbv ->dbcv_unitmask));
                CStdString strDevice;
                strDevice.Format("%c:",CWIN32Util::FirstDriveFromMask(lpdbv ->dbcv_unitmask));
                g_application.getApplicationMessenger().OpticalMount(strDevice, true);
              }
              else
              {
                // USB drive inserted
                CWin32StorageProvider::SetEvent();
              }
           }
           break;

        case DBT_DEVICEREMOVECOMPLETE:
           if (lpdb -> dbch_devicetype == DBT_DEVTYP_VOLUME)
           {
              PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
          
              // Check whether a CD or DVD was removed from a drive.
              if (lpdbv -> dbcv_flags & DBTF_MEDIA)
              {
                CLog::Log(LOGDEBUG,"%s: Drive %c: Media was removed.\n", __FUNCTION__, CWIN32Util::FirstDriveFromMask(lpdbv ->dbcv_unitmask));
                CStdString strDevice;
                strDevice.Format("%c:",CWIN32Util::FirstDriveFromMask(lpdbv ->dbcv_unitmask));
                g_application.getApplicationMessenger().OpticalUnMount(strDevice);
              }
              else
              {
                // USB drive was removed
                CWin32StorageProvider::SetEvent();
              }
           }
           break;
      }
  }
  return(DefWindowProc(hWnd, uMsg, wParam, lParam));
}

void CWinEventsWin32::WindowFromScreenCoords(HWND hWnd, POINT *point)
{
  if (!point) return;
  RECT clientRect;
  GetClientRect(hWnd, &clientRect);
  POINT windowPos;
  windowPos.x = clientRect.left;
  windowPos.y = clientRect.top;
  ClientToScreen(hWnd, &windowPos);
  point->x -= windowPos.x;
  point->y -= windowPos.y;
}

#endif
