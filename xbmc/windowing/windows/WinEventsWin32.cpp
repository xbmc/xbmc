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
#include "windowing/WinEvents.h"
#include "WIN32Util.h"
#include "storage/windows/Win32StorageProvider.h"
#include "Application.h"
#include "input/XBMC_vkeys.h"
#include "input/MouseStat.h"
#include "storage/MediaManager.h"
#include "windowing/WindowingFactory.h"
#include <dbt.h>
#include "guilib/LocalizeStrings.h"
#include "input/KeyboardStat.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUIControl.h"       // for EVENT_RESULT
#include "powermanagement/windows/Win32PowerSyscall.h"
#include "Shlobj.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "peripherals/Peripherals.h"
#include "storage/DetectDVDType.h"
#include "utils/JobManager.h"

#ifdef _WIN32

using namespace PERIPHERALS;

#define XBMC_arraysize(array)	(sizeof(array)/sizeof(array[0]))

/* Masks for processing the windows KEYDOWN and KEYUP messages */
#define REPEATED_KEYMASK  (1<<30)
#define EXTENDED_KEYMASK  (1<<24)
#define EXTKEYPAD(keypad) ((scancode & 0x100)?(mvke):(keypad))

static XBMCKey VK_keymap[XBMCK_LAST];
static HKL hLayoutUS = NULL;

static GUID USB_HID_GUID = { 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

uint32_t g_uQueryCancelAutoPlay = 0;

int XBMC_TranslateUNICODE = 1;

PHANDLE_EVENT_FUNC CWinEventsBase::m_pEventFunc = NULL;
int CWinEventsWin32::m_lastGesturePosX = 0;
int CWinEventsWin32::m_lastGesturePosY = 0;

// register to receive SD card events (insert/remove)
// seen at http://www.codeproject.com/Messages/2897423/Re-No-message-triggered-on-SD-card-insertion-remov.aspx
#define WM_MEDIA_CHANGE (WM_USER + 666)
SHChangeNotifyEntry shcne;

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
  VK_keymap[VK_OEM_102] = XBMCK_BACKSLASH;
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

  // Only include the multimedia keys if they have been enabled in the
  // advanced settings
  if (g_advancedSettings.m_enableMultimediaKeys)
  {
    VK_keymap[VK_BROWSER_BACK]        = XBMCK_BROWSER_BACK;
    VK_keymap[VK_BROWSER_FORWARD]     = XBMCK_BROWSER_FORWARD;
    VK_keymap[VK_BROWSER_REFRESH]     = XBMCK_BROWSER_REFRESH;
    VK_keymap[VK_BROWSER_STOP]        = XBMCK_BROWSER_STOP;
    VK_keymap[VK_BROWSER_SEARCH]      = XBMCK_BROWSER_SEARCH;
    VK_keymap[VK_BROWSER_FAVORITES]   = XBMCK_BROWSER_FAVORITES;
    VK_keymap[VK_BROWSER_HOME]        = XBMCK_BROWSER_HOME;
    VK_keymap[VK_VOLUME_MUTE]         = XBMCK_VOLUME_MUTE;
    VK_keymap[VK_VOLUME_DOWN]         = XBMCK_VOLUME_DOWN;
    VK_keymap[VK_VOLUME_UP]           = XBMCK_VOLUME_UP;
    VK_keymap[VK_MEDIA_NEXT_TRACK]    = XBMCK_MEDIA_NEXT_TRACK;
    VK_keymap[VK_MEDIA_PREV_TRACK]    = XBMCK_MEDIA_PREV_TRACK;
    VK_keymap[VK_MEDIA_STOP]          = XBMCK_MEDIA_STOP;
    VK_keymap[VK_MEDIA_PLAY_PAUSE]    = XBMCK_MEDIA_PLAY_PAUSE;
    VK_keymap[VK_LAUNCH_MAIL]         = XBMCK_LAUNCH_MAIL;
    VK_keymap[VK_LAUNCH_MEDIA_SELECT] = XBMCK_LAUNCH_MEDIA_SELECT;
    VK_keymap[VK_LAUNCH_APP1]         = XBMCK_LAUNCH_APP1;
    VK_keymap[VK_LAUNCH_APP2]         = XBMCK_LAUNCH_APP2;
  }
}

static int XBMC_MapVirtualKey(int scancode, int vkey)
{
// It isn't clear why the US keyboard layout was being used. This causes
// problems with e.g. the \ key. I have provisionally switched the code
// to use the Windows layout.
// int mvke = MapVirtualKeyEx(scancode & 0xFF, 1, hLayoutUS);
  int mvke = MapVirtualKeyEx(scancode & 0xFF, 1, NULL);

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
{ uint16_t mod;
  uint8_t keystate[256];

  /* Set the keysym information */
  keysym->scancode = (unsigned char) scancode;
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

  // Attempt to convert the keypress to a UNICODE character
  GetKeyboardState(keystate);

  if ( pressed && XBMC_TranslateUNICODE )
  { uint16_t  wchars[2];

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

  // Set the modifier bitmap

  mod = (uint16_t) XBMCKMOD_NONE;

  // If left control and right alt are down this usually means that
  // AltGr is down
  if ((keystate[VK_LCONTROL] & 0x80) && (keystate[VK_RMENU] & 0x80))
  {
    mod |= XBMCKMOD_MODE;
  }
  else
  {
    if (keystate[VK_LCONTROL] & 0x80) mod |= XBMCKMOD_LCTRL;
    if (keystate[VK_RMENU]    & 0x80) mod |= XBMCKMOD_RALT;
  }

  // Check the remaining modifiers
  if (keystate[VK_LSHIFT]   & 0x80) mod |= XBMCKMOD_LSHIFT;
  if (keystate[VK_RSHIFT]   & 0x80) mod |= XBMCKMOD_RSHIFT;
  if (keystate[VK_RCONTROL] & 0x80) mod |= XBMCKMOD_RCTRL;
  if (keystate[VK_LMENU]    & 0x80) mod |= XBMCKMOD_LALT;
  if (keystate[VK_LWIN]     & 0x80) mod |= XBMCKMOD_LSUPER;
  if (keystate[VK_RWIN]     & 0x80) mod |= XBMCKMOD_LSUPER;
  keysym->mod = (XBMCMod) mod;

  // Return the updated keysym
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
  static HDEVNOTIFY hDeviceNotify;

  if (uMsg == WM_CREATE)
  {
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)(((LPCREATESTRUCT)lParam)->lpCreateParams));
    DIB_InitOSKeymap();
    g_uQueryCancelAutoPlay = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));
    shcne.pidl = NULL;
    shcne.fRecursive = TRUE;
    long fEvents = SHCNE_DRIVEADD | SHCNE_DRIVEREMOVED | SHCNE_MEDIAREMOVED | SHCNE_MEDIAINSERTED;
    SHChangeNotifyRegister(hWnd, SHCNRF_ShellLevel | SHCNRF_NewDelivery, fEvents, WM_MEDIA_CHANGE, 1, &shcne);
    RegisterDeviceInterfaceToHwnd(USB_HID_GUID, hWnd, &hDeviceNotify);
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
          if (LOWORD(wParam) != WA_INACTIVE)
          {
            if (GetWindowPlacement(hWnd, &lpwndpl))
              g_application.m_AppActive = lpwndpl.showCmd != SW_HIDE;
          }
          else
          {
            g_application.m_AppActive = g_Windowing.WindowedMode();
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
      g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
      if (uMsg == WM_KILLFOCUS)
      {
        CStdString procfile;
        if (CWIN32Util::GetFocussedProcess(procfile))
          CLog::Log(LOGDEBUG, __FUNCTION__": Focus switched to process %s", procfile.c_str());
      }
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
      newEvent.appcommand.action = GET_APPCOMMAND_LPARAM(lParam);
      if (m_pEventFunc(newEvent))
        return TRUE;
      else
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    case WM_GESTURENOTIFY:
    {
      OnGestureNotify(hWnd, lParam);
      return DefWindowProc(hWnd, WM_GESTURENOTIFY, wParam, lParam);
    }
    case WM_GESTURE:
    {
      OnGesture(hWnd, lParam);
      return 0;
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
      newEvent.resize.w = GET_X_LPARAM(lParam);
      newEvent.resize.h = GET_Y_LPARAM(lParam);

      CLog::Log(LOGDEBUG, __FUNCTION__": window resize event");

      if (!g_Windowing.IsAlteringWindow() && newEvent.resize.w > 0 && newEvent.resize.h > 0)
        m_pEventFunc(newEvent);

      return(0);
    case WM_MOVE:
      newEvent.type = XBMC_VIDEOMOVE;
      newEvent.move.x = GET_X_LPARAM(lParam);
      newEvent.move.y = GET_Y_LPARAM(lParam);

      CLog::Log(LOGDEBUG, __FUNCTION__": window move event");

      if (!g_Windowing.IsAlteringWindow())
        m_pEventFunc(newEvent);

      return(0);
    case WM_MEDIA_CHANGE:
      {
        // There may be multiple notifications for one event
        // There are also a few events we're not interested in, but they cause no harm
        // For example SD card reader insertion/removal
        long lEvent;
        PIDLIST_ABSOLUTE *ppidl;
        HANDLE hLock = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);

        if (hLock)
        {
          char drivePath[MAX_PATH+1];
          if (!SHGetPathFromIDList(ppidl[0], drivePath))
            break;

          switch(lEvent)
          {
            case SHCNE_DRIVEADD:
            case SHCNE_MEDIAINSERTED:
              CLog::Log(LOGDEBUG, __FUNCTION__": Drive %s Media has arrived.", drivePath);
              if (GetDriveType(drivePath) == DRIVE_CDROM)
                CJobManager::GetInstance().AddJob(new CDetectDisc(drivePath, true), NULL);
              else
                CWin32StorageProvider::SetEvent();
              break;

            case SHCNE_DRIVEREMOVED:
            case SHCNE_MEDIAREMOVED:
              CLog::Log(LOGDEBUG, __FUNCTION__": Drive %s Media was removed.", drivePath);
              if (GetDriveType(drivePath) == DRIVE_CDROM)
              {
                CMediaSource share;
                share.strPath = drivePath;
                share.strName = share.strPath;
                g_mediaManager.RemoveAutoSource(share);
              }
              else
                CWin32StorageProvider::SetEvent();
              break;
          }
          SHChangeNotification_Unlock(hLock);
        }
        break;
      }
    case WM_POWERBROADCAST:
      if (wParam==PBT_APMSUSPEND)
      {
        CLog::Log(LOGDEBUG,"WM_POWERBROADCAST: PBT_APMSUSPEND event was sent");
        CWin32PowerSyscall::SetOnSuspend();
      }
      else if(wParam==PBT_APMRESUMEAUTOMATIC)
      {
        CLog::Log(LOGDEBUG,"WM_POWERBROADCAST: PBT_APMRESUMEAUTOMATIC event was sent");
        CWin32PowerSyscall::SetOnResume();
      }
      break;
    case WM_DEVICECHANGE:
      {
        PDEV_BROADCAST_DEVICEINTERFACE b = (PDEV_BROADCAST_DEVICEINTERFACE) lParam;
        switch (wParam)
        {
          case DBT_DEVICEARRIVAL:
          case DBT_DEVICEREMOVECOMPLETE:
          case DBT_DEVNODES_CHANGED:
            g_peripherals.TriggerDeviceScan(PERIPHERAL_BUS_USB);
            break;
        }
        break;
      }
    case WM_PAINT:
      //some other app has painted over our window, mark everything as dirty
      g_windowManager.MarkDirty();
      break;
  }
  return(DefWindowProc(hWnd, uMsg, wParam, lParam));
}

void CWinEventsWin32::RegisterDeviceInterfaceToHwnd(GUID InterfaceClassGuid, HWND hWnd, HDEVNOTIFY *hDeviceNotify)
{
  DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

  ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
  NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
  NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
  NotificationFilter.dbcc_classguid = InterfaceClassGuid;

  *hDeviceNotify = RegisterDeviceNotification( 
      hWnd,                       // events recipient
      &NotificationFilter,        // type of device
      DEVICE_NOTIFY_WINDOW_HANDLE // type of recipient handle
      );
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

void CWinEventsWin32::OnGestureNotify(HWND hWnd, LPARAM lParam)
{
  // convert to window coordinates
  PGESTURENOTIFYSTRUCT gn = (PGESTURENOTIFYSTRUCT)(lParam);
  POINT point = { gn->ptsLocation.x, gn->ptsLocation.y };
  WindowFromScreenCoords(hWnd, &point);

  // by default that we want no gestures
  GESTURECONFIG gc[] = {{ GID_ZOOM, 0, GC_ZOOM},
                        { GID_ROTATE, 0, GC_ROTATE},
                        { GID_PAN, 0, GC_PAN},
                        { GID_TWOFINGERTAP, 0, GC_TWOFINGERTAP },
                        { GID_PRESSANDTAP, 0, GC_PRESSANDTAP }};

  // send a message to see if a control wants any
  CGUIMessage message(GUI_MSG_GESTURE_NOTIFY, 0, 0, point.x, point.y);
  if (g_windowManager.SendMessage(message))
  {
    int gestures = message.GetParam1();
    if (gestures == EVENT_RESULT_ZOOM)
      gc[0].dwWant |= GC_ZOOM;
    if (gestures == EVENT_RESULT_ROTATE)
      gc[1].dwWant |= GC_ROTATE;
    if (gestures == EVENT_RESULT_PAN_VERTICAL)
      gc[2].dwWant |= GC_PAN_WITH_SINGLE_FINGER_VERTICALLY | GC_PAN_WITH_GUTTER | GC_PAN_WITH_INERTIA;
    if (gestures == EVENT_RESULT_PAN_VERTICAL_WITHOUT_INERTIA)
      gc[2].dwWant |= GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;
    if (gestures == EVENT_RESULT_PAN_HORIZONTAL)
      gc[2].dwWant |= GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY | GC_PAN_WITH_GUTTER | GC_PAN_WITH_INERTIA;
    if (gestures == EVENT_RESULT_PAN_HORIZONTAL_WITHOUT_INERTIA)
      gc[2].dwWant |= GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;
    gc[0].dwBlock = gc[0].dwWant ^ 1;
    gc[1].dwBlock = gc[1].dwWant ^ 1;
    gc[2].dwBlock = gc[2].dwWant ^ 30;
  }
  if (g_Windowing.PtrSetGestureConfig)
    g_Windowing.PtrSetGestureConfig(hWnd, 0, 5, gc, sizeof(GESTURECONFIG));
}

void CWinEventsWin32::OnGesture(HWND hWnd, LPARAM lParam)
{
  if (!g_Windowing.PtrGetGestureInfo)
    return;

  GESTUREINFO gi = {0};
  gi.cbSize = sizeof(gi);
  g_Windowing.PtrGetGestureInfo((HGESTUREINFO)lParam, &gi);

  // convert to window coordinates
  POINT point = { gi.ptsLocation.x, gi.ptsLocation.y };
  WindowFromScreenCoords(hWnd, &point);
  switch (gi.dwID)
  {
  case GID_BEGIN:
    {
      m_lastGesturePosX = point.x;
      m_lastGesturePosY = point.y;
      g_application.OnAction(CAction(ACTION_GESTURE_BEGIN, 0, (float)point.x, (float)point.y, 0, 0));
    }
    break;
  case GID_END:
    {
      g_application.OnAction(CAction(ACTION_GESTURE_END));
    }
    break;
  case GID_PAN:
    {
      g_application.OnAction(CAction(ACTION_GESTURE_PAN, 0, (float)point.x, (float)point.y,
                                    (float)(point.x - m_lastGesturePosX), (float)(point.y - m_lastGesturePosY)));
      m_lastGesturePosX = point.x;
      m_lastGesturePosY = point.y;
    }
    break;
  case GID_ROTATE:
    CLog::Log(LOGDEBUG, "%s - Rotating", __FUNCTION__);
    break;
  case GID_ZOOM:
    CLog::Log(LOGDEBUG, "%s - Zooming", __FUNCTION__);
    break;
  case GID_TWOFINGERTAP:
  case GID_PRESSANDTAP:
  default:
    // You have encountered an unknown gesture
    break;
  }
  g_Windowing.PtrCloseGestureInfoHandle((HGESTUREINFO)lParam);
}

#endif
