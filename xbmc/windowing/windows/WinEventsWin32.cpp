/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "WinEventsWin32.h"

#include "ServiceBroker.h"
#include "Util.h"
#include "WinKeyMap.h"
#include "application/AppInboundProtocol.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControl.h" // for EVENT_RESULT
#include "guilib/GUIWindowManager.h"
#include "input/InputManager.h"
#include "input/actions/Action.h"
#include "input/keyboard/Key.h"
#include "input/keyboard/KeyIDs.h"
#include "input/mouse/MouseStat.h"
#include "input/touch/generic/GenericTouchActionHandler.h"
#include "input/touch/generic/GenericTouchSwipeDetector.h"
#include "messaging/ApplicationMessenger.h"
#include "network/Zeroconf.h"
#include "network/ZeroconfBrowser.h"
#include "peripherals/Peripherals.h"
#include "rendering/dx/RenderContext.h"
#include "storage/MediaManager.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/win32/CharsetConverter.h"
#include "platform/win32/WIN32Util.h"
#include "platform/win32/powermanagement/Win32PowerSyscall.h"
#include "platform/win32/storage/Win32StorageProvider.h"

#include <array>
#include <math.h>

#include <Shlobj.h>
#include <dbt.h>

HWND g_hWnd = nullptr;

#ifndef LODWORD
#define LODWORD(longval) ((DWORD)((DWORDLONG)(longval)))
#endif

#define ROTATE_ANGLE_DEGREE(arg) GID_ROTATE_ANGLE_FROM_ARGUMENT(LODWORD(arg)) * 180 / M_PI

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

/* Masks for processing the windows KEYDOWN and KEYUP messages */
#define REPEATED_KEYMASK  (1<<30)
#define EXTENDED_KEYMASK  (1<<24)
#define EXTKEYPAD(keypad) ((scancode & 0x100)?(mvke):(keypad))

static GUID USB_HID_GUID = { 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

uint32_t g_uQueryCancelAutoPlay = 0;
bool g_sizeMoveSizing = false;
bool g_sizeMoveMoving = false;
int g_sizeMoveWidth = 0;
int g_sizeMoveHight = 0;
int g_sizeMoveX = -10000;
int g_sizeMoveY = -10000;

int CWinEventsWin32::m_originalZoomDistance = 0;
Pointer CWinEventsWin32::m_touchPointer;
CGenericTouchSwipeDetector* CWinEventsWin32::m_touchSwipeDetector = nullptr;

// register to receive SD card events (insert/remove)
// seen at http://www.codeproject.com/Messages/2897423/Re-No-message-triggered-on-SD-card-insertion-remov.aspx
#define WM_MEDIA_CHANGE (WM_USER + 666)
SHChangeNotifyEntry shcne;

static int XBMC_MapVirtualKey(int scancode, WPARAM vkey)
{
  int mvke = MapVirtualKeyEx(scancode & 0xFF, 1, nullptr);

  switch (vkey)
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
      /* Multimedia keys are already handled */
    case VK_BROWSER_BACK:
    case VK_BROWSER_FORWARD:
    case VK_BROWSER_REFRESH:
    case VK_BROWSER_STOP:
    case VK_BROWSER_SEARCH:
    case VK_BROWSER_FAVORITES:
    case VK_BROWSER_HOME:
    case VK_VOLUME_MUTE:
    case VK_VOLUME_DOWN:
    case VK_VOLUME_UP:
    case VK_MEDIA_NEXT_TRACK:
    case VK_MEDIA_PREV_TRACK:
    case VK_MEDIA_STOP:
    case VK_MEDIA_PLAY_PAUSE:
    case VK_LAUNCH_MAIL:
    case VK_LAUNCH_MEDIA_SELECT:
    case VK_LAUNCH_APP1:
    case VK_LAUNCH_APP2:
      return static_cast<int>(vkey);
    default:;
  }
  switch (mvke)
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
    default:;
  }
  return mvke ? mvke : static_cast<int>(vkey);
}


static XBMC_keysym *TranslateKey(WPARAM vkey, UINT scancode, XBMC_keysym *keysym, int pressed)
{
  using namespace KODI::WINDOWING::WINDOWS;

  uint8_t keystate[256];

  /* Set the keysym information */
  keysym->scancode = static_cast<unsigned char>(scancode);
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
  if (GetKeyboardState(keystate) == FALSE)
  {
    CLog::LogF(LOGERROR, "GetKeyboardState error {}", GetLastError());
    return keysym;
  }

  if (pressed)
  {
    std::array<uint16_t, 2> wchars;

    /* Numlock isn't taken into account in ToUnicode,
    * so we handle it as a special case here */
    if ((keystate[VK_NUMLOCK] & 1) && vkey >= VK_NUMPAD0 && vkey <= VK_NUMPAD9)
    {
      keysym->unicode = static_cast<uint16_t>(vkey - VK_NUMPAD0 + '0');
    }
    else if (ToUnicode(static_cast<UINT>(vkey), scancode, keystate,
                       reinterpret_cast<LPWSTR>(wchars.data()), static_cast<int>(wchars.size()),
                       0) > 0)
    {
      keysym->unicode = wchars[0];
    }
  }

  // Set the modifier bitmap

  uint16_t mod = static_cast<uint16_t>(XBMCKMOD_NONE);

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
  keysym->mod = static_cast<XBMCMod>(mod);

  // Return the updated keysym
  return(keysym);
}


bool CWinEventsWin32::MessagePump()
{
  MSG  msg;
  while( PeekMessage( &msg, nullptr, 0U, 0U, PM_REMOVE ) )
  {
    TranslateMessage( &msg );
    DispatchMessage( &msg );
  }
  return true;
}

LRESULT CALLBACK CWinEventsWin32::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  using KODI::PLATFORM::WINDOWS::FromW;

  XBMC_Event newEvent = {};
  static HDEVNOTIFY hDeviceNotify;

  if (uMsg == WM_CREATE)
  {
    g_hWnd = hWnd;
    // need to set windows handle before WM_SIZE processing
    DX::Windowing()->SetWindow(hWnd);

    KODI::WINDOWING::WINDOWS::DIB_InitOSKeymap();

    g_uQueryCancelAutoPlay = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));
    shcne.pidl = nullptr;
    shcne.fRecursive = TRUE;
    long fEvents = SHCNE_DRIVEADD | SHCNE_DRIVEREMOVED | SHCNE_MEDIAREMOVED | SHCNE_MEDIAINSERTED;
    SHChangeNotifyRegister(hWnd, SHCNRF_ShellLevel | SHCNRF_NewDelivery, fEvents, WM_MEDIA_CHANGE, 1, &shcne);
    RegisterDeviceInterfaceToHwnd(USB_HID_GUID, hWnd, &hDeviceNotify);
    return 0;
  }

  if (uMsg == WM_DESTROY)
    g_hWnd = nullptr;

  if(g_uQueryCancelAutoPlay != 0 && uMsg == g_uQueryCancelAutoPlay)
    return S_FALSE;

  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();

  switch (uMsg)
  {
    case WM_CLOSE:
    case WM_QUIT:
    case WM_DESTROY:
      if (hDeviceNotify)
      {
        if (UnregisterDeviceNotification(hDeviceNotify))
          hDeviceNotify = nullptr;
        else
          CLog::LogF(LOGINFO, "UnregisterDeviceNotification failed ({})", GetLastError());
      }
      newEvent.type = XBMC_QUIT;
      if (appPort)
        appPort->OnEvent(newEvent);
      break;
    case WM_SHOWWINDOW:
      {
        const auto& components = CServiceBroker::GetAppComponents();
        const auto appPower = components.GetComponent<CApplicationPowerHandling>();
        bool active = appPower->GetRenderGUI();
        if (appPort)
          appPort->SetRenderGUI(wParam != 0);
        if (appPower->GetRenderGUI() != active)
          DX::Windowing()->NotifyAppActiveChange(appPower->GetRenderGUI());
        CLog::LogFC(LOGDEBUG, LOGWINDOWING, "WM_SHOWWINDOW -> window is {}",
                    wParam != 0 ? "shown" : "hidden");
      }
      break;
    case WM_ACTIVATE:
      {
        CLog::LogFC(LOGDEBUG, LOGWINDOWING, "WM_ACTIVATE -> window is {}",
                    LOWORD(wParam) != WA_INACTIVE ? "active" : "inactive");
        const auto& components = CServiceBroker::GetAppComponents();
        const auto appPower = components.GetComponent<CApplicationPowerHandling>();
        bool active = appPower->GetRenderGUI();
        if (HIWORD(wParam))
        {
          if (appPort)
            appPort->SetRenderGUI(false);
        }
        else
        {
          WINDOWPLACEMENT lpwndpl;
          lpwndpl.length = sizeof(lpwndpl);
          if (LOWORD(wParam) != WA_INACTIVE)
          {
            if (GetWindowPlacement(hWnd, &lpwndpl))
            {
              if (appPort)
                appPort->SetRenderGUI(lpwndpl.showCmd != SW_HIDE);
            }
          }
          else
          {

          }
        }
        if (appPower->GetRenderGUI() != active)
          DX::Windowing()->NotifyAppActiveChange(appPower->GetRenderGUI());
        CLog::LogFC(LOGDEBUG, LOGWINDOWING, "window is {}",
                    appPower->GetRenderGUI() ? "active" : "inactive");
      }
      break;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
      g_application.m_AppFocused = uMsg == WM_SETFOCUS;
      CLog::LogFC(LOGDEBUG, LOGWINDOWING, "window focus {}",
                  g_application.m_AppFocused ? "set" : "lost");

      DX::Windowing()->NotifyAppFocusChange(g_application.m_AppFocused);
      if (uMsg == WM_KILLFOCUS)
      {
        std::string procfile;
        if (CWIN32Util::GetFocussedProcess(procfile))
          CLog::LogFC(LOGDEBUG, LOGWINDOWING, "Focus switched to process {}", procfile);
      }
      break;
    /* needs to be reviewed after frodo. we reset the system idle timer
       and the display timer directly now (see m_screenSaverTimer).
    case WM_SYSCOMMAND:
      switch( wParam&0xFFF0 )
      {
        case SC_MONITORPOWER:
          if (g_application.GetAppPlayer().IsPlaying() || g_application.GetAppPlayer().IsPausedPlayback())
            return 0;
          else if(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_POWERMANAGEMENT_DISPLAYSOFF) == 0)
            return 0;
          break;
        case SC_SCREENSAVE:
          return 0;
      }
      break;*/
    case WM_SYSKEYDOWN:
      switch (wParam)
      {
        case VK_F4: //alt-f4, default event quit.
          return(DefWindowProc(hWnd, uMsg, wParam, lParam));
        case VK_RETURN: //alt-return
          if ((lParam & REPEATED_KEYMASK) == 0)
            CServiceBroker::GetAppMessenger()->PostMsg(TMSG_TOGGLEFULLSCREEN);
          return 0;
        default:;
      }
      [[fallthrough]];
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
        default:;
      }
      XBMC_keysym keysym;
      TranslateKey(wParam, HIWORD(lParam), &keysym, 1);

      newEvent.type = XBMC_KEYDOWN;
      newEvent.key.keysym = keysym;
      if (appPort)
        appPort->OnEvent(newEvent);
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
            uint32_t keyCode = static_cast<uint32_t>((lParam & 0xFF0000) >> 16);
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
        default:;
      }
      XBMC_keysym keysym;
      TranslateKey(wParam, HIWORD(lParam), &keysym, 1);

      if (wParam == VK_SNAPSHOT)
        newEvent.type = XBMC_KEYDOWN;
      else
        newEvent.type = XBMC_KEYUP;
      newEvent.key.keysym = keysym;
      if (appPort)
        appPort->OnEvent(newEvent);
    }
    return(0);
    case WM_APPCOMMAND: // MULTIMEDIA keys are mapped to APPCOMMANDS
    {
      const unsigned int appcmd = GET_APPCOMMAND_LPARAM(lParam);

      CLog::LogFC(LOGDEBUG, LOGWINDOWING, "APPCOMMAND {}", appcmd);

      // Reset the screen saver
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPower = components.GetComponent<CApplicationPowerHandling>();
      appPower->ResetScreenSaver();

      // If we were currently in the screen saver wake up and don't process the
      // appcommand
      if (appPower->WakeUpScreenSaverAndDPMS())
        return true;

      // Retrieve the action associated with this appcommand from the mapping table
      CKey key(appcmd | KEY_APPCOMMAND, 0U);
      int iWin = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog();

      CAction appcmdaction = CServiceBroker::GetInputManager().GetAction(iWin, key);
      if (appcmdaction.GetID())
      {
        CLog::LogFC(LOGDEBUG, LOGWINDOWING, "appcommand {}, action {}", appcmd,
                    appcmdaction.GetName());
        CServiceBroker::GetInputManager().QueueAction(appcmdaction);
        return true;
      }

      CLog::LogFC(LOGDEBUG, LOGWINDOWING, "unknown appcommand {}", appcmd);
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
        DX::Windowing()->ShowOSMouse(true);
      break;
    case WM_MOUSEMOVE:
      newEvent.type = XBMC_MOUSEMOTION;
      newEvent.motion.x = GET_X_LPARAM(lParam);
      newEvent.motion.y = GET_Y_LPARAM(lParam);
      if (appPort)
        appPort->OnEvent(newEvent);
      return(0);
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
      newEvent.type = XBMC_MOUSEBUTTONDOWN;
      newEvent.button.x = GET_X_LPARAM(lParam);
      newEvent.button.y = GET_Y_LPARAM(lParam);
      newEvent.button.button = 0;
      if (uMsg == WM_LBUTTONDOWN) newEvent.button.button = XBMC_BUTTON_LEFT;
      else if (uMsg == WM_MBUTTONDOWN) newEvent.button.button = XBMC_BUTTON_MIDDLE;
      else if (uMsg == WM_RBUTTONDOWN) newEvent.button.button = XBMC_BUTTON_RIGHT;
      if (appPort)
        appPort->OnEvent(newEvent);
      return(0);
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
      newEvent.type = XBMC_MOUSEBUTTONUP;
      newEvent.button.x = GET_X_LPARAM(lParam);
      newEvent.button.y = GET_Y_LPARAM(lParam);
      newEvent.button.button = 0;
      if (uMsg == WM_LBUTTONUP) newEvent.button.button = XBMC_BUTTON_LEFT;
      else if (uMsg == WM_MBUTTONUP) newEvent.button.button = XBMC_BUTTON_MIDDLE;
      else if (uMsg == WM_RBUTTONUP) newEvent.button.button = XBMC_BUTTON_RIGHT;
      if (appPort)
        appPort->OnEvent(newEvent);
      return(0);
    case WM_MOUSEWHEEL:
      {
        // SDL, which our events system is based off, sends a MOUSEBUTTONDOWN message
        // followed by a MOUSEBUTTONUP message.  As this is a momentary event, we just
        // react on the MOUSEBUTTONUP message, resetting the state after processing.
        newEvent.type = XBMC_MOUSEBUTTONDOWN;
        // the coordinates in WM_MOUSEWHEEL are screen, not client coordinates
        POINT point;
        point.x = GET_X_LPARAM(lParam);
        point.y = GET_Y_LPARAM(lParam);
        WindowFromScreenCoords(hWnd, &point);
        newEvent.button.x = static_cast<uint16_t>(point.x);
        newEvent.button.y = static_cast<uint16_t>(point.y);
        newEvent.button.button = GET_Y_LPARAM(wParam) > 0 ? XBMC_BUTTON_WHEELUP : XBMC_BUTTON_WHEELDOWN;
        if (appPort)
        {
          appPort->OnEvent(newEvent);
          newEvent.type = XBMC_MOUSEBUTTONUP;
          appPort->OnEvent(newEvent);
        }
    }
      return(0);
    case WM_DPICHANGED:
    // This message tells the program that most of its window is on a
    // monitor with a new DPI. The wParam contains the new DPI, and the
    // lParam contains a rect which defines the window rectangle scaled
    // the new DPI.
    {
      // get the suggested size of the window on the new display with a different DPI
      uint16_t  dpi = HIWORD(wParam);
      RECT rc = *reinterpret_cast<RECT*>(lParam);
      CLog::LogFC(LOGDEBUG, LOGWINDOWING, "dpi changed event -> {} ({}, {}, {}, {})", dpi, rc.left,
                  rc.top, rc.right, rc.bottom);
      DX::Windowing()->DPIChanged(dpi, rc);
      return(0);
    }
    case WM_DISPLAYCHANGE:
    {
      CLog::LogFC(LOGDEBUG, LOGWINDOWING, "display change event");
      if (DX::Windowing()->IsTogglingHDR() || DX::Windowing()->IsAlteringWindow())
        return (0);

      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPower = components.GetComponent<CApplicationPowerHandling>();
      if (appPower->GetRenderGUI() && GET_X_LPARAM(lParam) > 0 && GET_Y_LPARAM(lParam) > 0)
      {
        DX::Windowing()->UpdateResolutions();
      }
      return(0);
    }
    case WM_ENTERSIZEMOVE:
      {
        DX::Windowing()->SetSizeMoveMode(true);
      }
      return(0);
    case WM_EXITSIZEMOVE:
      {
        DX::Windowing()->SetSizeMoveMode(false);
        if (g_sizeMoveMoving)
        {
          g_sizeMoveMoving = false;
          newEvent.type = XBMC_VIDEOMOVE;
          newEvent.move.x = g_sizeMoveX;
          newEvent.move.y = g_sizeMoveY;

          // tell the device about new position
          DX::Windowing()->OnMove(newEvent.move.x, newEvent.move.y);
          // tell the application about new position
          const auto& components = CServiceBroker::GetAppComponents();
          const auto appPower = components.GetComponent<CApplicationPowerHandling>();
          if (appPower->GetRenderGUI() && !DX::Windowing()->IsAlteringWindow())
          {
            if (appPort)
              appPort->OnEvent(newEvent);
          }
        }
        if (g_sizeMoveSizing)
        {
          g_sizeMoveSizing = false;
          newEvent.type = XBMC_VIDEORESIZE;
          newEvent.resize.w = g_sizeMoveWidth;
          newEvent.resize.h = g_sizeMoveHight;

          // tell the device about new size
          DX::Windowing()->OnResize(newEvent.resize.w, newEvent.resize.h);
          // tell the application about new size
          const auto& components = CServiceBroker::GetAppComponents();
          const auto appPower = components.GetComponent<CApplicationPowerHandling>();
          if (appPower->GetRenderGUI() && !DX::Windowing()->IsAlteringWindow() &&
              newEvent.resize.w > 0 && newEvent.resize.h > 0)
          {
            if (appPort)
              appPort->OnEvent(newEvent);
          }
        }
      }
    return(0);
    case WM_SIZE:
      if (wParam == SIZE_MINIMIZED)
      {
        if (!DX::Windowing()->IsMinimized())
        {
          DX::Windowing()->SetMinimized(true);
          const auto& components = CServiceBroker::GetAppComponents();
          const auto appPower = components.GetComponent<CApplicationPowerHandling>();
          if (appPower->GetRenderGUI())
          {
            if (appPort)
              appPort->SetRenderGUI(false);
          }
        }
      }
      else if (DX::Windowing()->IsMinimized())
      {
        DX::Windowing()->SetMinimized(false);
        const auto& components = CServiceBroker::GetAppComponents();
        const auto appPower = components.GetComponent<CApplicationPowerHandling>();
        if (!appPower->GetRenderGUI())
        {
          if (appPort)
            appPort->SetRenderGUI(true);
        }
      }
      else
      {
        g_sizeMoveWidth = GET_X_LPARAM(lParam);
        g_sizeMoveHight = GET_Y_LPARAM(lParam);
        if (DX::Windowing()->IsInSizeMoveMode())
        {
          // If an user is dragging the resize bars, we don't resize
          // the buffers and don't rise XBMC_VIDEORESIZE here because
          // as the user continuously resize the window, a lot of WM_SIZE
          // messages are sent to the proc, and it'd be pointless (and slow)
          // to resize for each WM_SIZE message received from dragging.
          // So instead, we reset after the user is done resizing the
          // window and releases the resize bars, which ends with WM_EXITSIZEMOVE.
          g_sizeMoveSizing = true;
        }
        else
        {
          // API call such as SetWindowPos or SwapChain->SetFullscreenState
          newEvent.type = XBMC_VIDEORESIZE;
          newEvent.resize.w = g_sizeMoveWidth;
          newEvent.resize.h = g_sizeMoveHight;

          CLog::LogFC(LOGDEBUG, LOGWINDOWING, "window resize event {} x {}", newEvent.resize.w,
                      newEvent.resize.h);
          // tell device about new size
          DX::Windowing()->OnResize(newEvent.resize.w, newEvent.resize.h);
          // tell application about size changes
          const auto& components = CServiceBroker::GetAppComponents();
          const auto appPower = components.GetComponent<CApplicationPowerHandling>();
          if (appPower->GetRenderGUI() && !DX::Windowing()->IsAlteringWindow() &&
              newEvent.resize.w > 0 && newEvent.resize.h > 0)
          {
            if (appPort)
              appPort->OnEvent(newEvent);
          }
        }
      }
      return(0);
    case WM_MOVE:
      {
        g_sizeMoveX = GET_X_LPARAM(lParam);
        g_sizeMoveY = GET_Y_LPARAM(lParam);
        if (DX::Windowing()->IsInSizeMoveMode())
        {
          // the same as WM_SIZE
          g_sizeMoveMoving = true;
        }
        else
        {
          newEvent.type = XBMC_VIDEOMOVE;
          newEvent.move.x = g_sizeMoveX;
          newEvent.move.y = g_sizeMoveY;

          CLog::LogFC(LOGDEBUG, LOGWINDOWING, "window move event");

          // tell the device about new position
          DX::Windowing()->OnMove(newEvent.move.x, newEvent.move.y);
          const auto& components = CServiceBroker::GetAppComponents();
          const auto appPower = components.GetComponent<CApplicationPowerHandling>();
          if (appPower->GetRenderGUI() && !DX::Windowing()->IsAlteringWindow())
          {
            if (appPort)
              appPort->OnEvent(newEvent);
          }
        }
      }
      return(0);
    case WM_MEDIA_CHANGE:
      {
        // This event detects media changes of usb, sd card and optical media.
        // It only works if the explorer.exe process is started. Because this
        // isn't the case for all setups we use WM_DEVICECHANGE for usb and
        // optical media because this event is also triggered without the
        // explorer process. Since WM_DEVICECHANGE doesn't detect sd card changes
        // we still use this event only for sd.
        long lEvent;
        PIDLIST_ABSOLUTE *ppidl;
        HANDLE hLock = SHChangeNotification_Lock(reinterpret_cast<HANDLE>(wParam), static_cast<DWORD>(lParam), &ppidl, &lEvent);

        if (hLock)
        {
          wchar_t drivePath[MAX_PATH+1];
          if (!SHGetPathFromIDList(ppidl[0], drivePath))
            break;

          switch(lEvent)
          {
            case SHCNE_DRIVEADD:
            case SHCNE_MEDIAINSERTED:
              if (GetDriveType(drivePath) != DRIVE_CDROM)
              {
                CLog::LogF(LOGDEBUG, "Drive {} Media has arrived.", FromW(drivePath));
                CWin32StorageProvider::SetEvent();
              }
              break;

            case SHCNE_DRIVEREMOVED:
            case SHCNE_MEDIAREMOVED:
              if (GetDriveType(drivePath) != DRIVE_CDROM)
              {
                CLog::LogF(LOGDEBUG, "Drive {} Media was removed.", FromW(drivePath));
                CWin32StorageProvider::SetEvent();
              }
              break;
            default:;
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
        switch(wParam)
        {
          case DBT_DEVNODES_CHANGED:
            CServiceBroker::GetPeripherals().TriggerDeviceScan(PERIPHERALS::PERIPHERAL_BUS_USB);
            break;
          case DBT_DEVICEARRIVAL:
          case DBT_DEVICEREMOVECOMPLETE:
            if (((_DEV_BROADCAST_HEADER*) lParam)->dbcd_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
            {
              CServiceBroker::GetPeripherals().TriggerDeviceScan(PERIPHERALS::PERIPHERAL_BUS_USB);
            }
            // check if an usb or optical media was inserted or removed
            if (((_DEV_BROADCAST_HEADER*) lParam)->dbcd_devicetype == DBT_DEVTYP_VOLUME)
            {
              PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)((_DEV_BROADCAST_HEADER*) lParam);
              // optical medium
              if (lpdbv -> dbcv_flags & DBTF_MEDIA)
              {
                std::string strdrive = StringUtils::Format(
                    "{}:", CWIN32Util::FirstDriveFromMask(lpdbv->dbcv_unitmask));
                if(wParam == DBT_DEVICEARRIVAL)
                {
                  CLog::LogF(LOGDEBUG, "Drive {} Media has arrived.", strdrive);
                  CServiceBroker::GetJobManager()->AddJob(new CDetectDisc(strdrive, true), nullptr);
                }
                else
                {
                  CLog::LogF(LOGDEBUG, "Drive {} Media was removed.", strdrive);
                  CMediaSource share;
                  share.strPath = strdrive;
                  share.strName = share.strPath;
                  CServiceBroker::GetMediaManager().RemoveAutoSource(share);
                }
              }
              else
                CWin32StorageProvider::SetEvent();
            }
            break;
          default:;
        }
        break;
      }
    case WM_PAINT:
    {
      //some other app has painted over our window, mark everything as dirty
      CGUIComponent* component = CServiceBroker::GetGUI();
      if (component)
        component->GetWindowManager().MarkDirty();
      break;
    }
    case BONJOUR_EVENT:
      CZeroconf::GetInstance()->ProcessResults();
      break;
    case BONJOUR_BROWSER_EVENT:
      CZeroconfBrowser::GetInstance()->ProcessResults();
      break;
    case TRAY_ICON_NOTIFY:
    {
      switch (LOWORD(lParam))
      {
        case WM_LBUTTONDBLCLK:
        {
          DX::Windowing()->SetMinimized(false);
          const auto& components = CServiceBroker::GetAppComponents();
          const auto appPower = components.GetComponent<CApplicationPowerHandling>();
          if (!appPower->GetRenderGUI())
          {
            if (appPort)
              appPort->SetRenderGUI(true);
          }
          break;
        }
      }
      break;
    }
    case WM_TIMER:
    {
      if (wParam == ID_TIMER_HDR)
      {
        CLog::LogFC(LOGDEBUG, LOGWINDOWING, "finish toggling HDR event");
        DX::Windowing()->SetTogglingHDR(false);
        KillTimer(hWnd, wParam);
      }
      break;
    }
    case WM_INITMENU:
    {
      HMENU hm = GetSystemMenu(hWnd, FALSE);
      if (hm)
      {
        if (DX::Windowing()->IsFullScreen())
          EnableMenuItem(hm, SC_MOVE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        else
          EnableMenuItem(hm, SC_MOVE, MF_BYCOMMAND | MF_ENABLED);
      }
      break;
    }
    default:
      break;
  }
  return(DefWindowProc(hWnd, uMsg, wParam, lParam));
}

void CWinEventsWin32::RegisterDeviceInterfaceToHwnd(GUID InterfaceClassGuid, HWND hWnd, HDEVNOTIFY *hDeviceNotify)
{
  DEV_BROADCAST_DEVICEINTERFACE NotificationFilter = {};

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
  PGESTURENOTIFYSTRUCT gn = reinterpret_cast<PGESTURENOTIFYSTRUCT>(lParam);
  POINT point = { gn->ptsLocation.x, gn->ptsLocation.y };
  WindowFromScreenCoords(hWnd, &point);

  // by default we only want twofingertap and pressandtap gestures
  // the other gestures are enabled best on supported gestures
  GESTURECONFIG gc[] = {{ GID_ZOOM, 0, GC_ZOOM},
                        { GID_ROTATE, 0, GC_ROTATE},
                        { GID_PAN, 0, GC_PAN},
                        { GID_TWOFINGERTAP, GC_TWOFINGERTAP, GC_TWOFINGERTAP },
                        { GID_PRESSANDTAP, GC_PRESSANDTAP, GC_PRESSANDTAP }};

  // send a message to see if a control wants any
  int gestures;
  if ((gestures = CGenericTouchActionHandler::GetInstance().QuerySupportedGestures(static_cast<float>(point.x), static_cast<float>(point.y))) != EVENT_RESULT_UNHANDLED)
  {
    if (gestures & EVENT_RESULT_ZOOM)
      gc[0].dwWant |= GC_ZOOM;
    if (gestures & EVENT_RESULT_ROTATE)
      gc[1].dwWant |= GC_ROTATE;
    if (gestures & EVENT_RESULT_PAN_VERTICAL)
      gc[2].dwWant |= GC_PAN | GC_PAN_WITH_SINGLE_FINGER_VERTICALLY | GC_PAN_WITH_GUTTER | GC_PAN_WITH_INERTIA;
    if (gestures & EVENT_RESULT_PAN_VERTICAL_WITHOUT_INERTIA)
      gc[2].dwWant |= GC_PAN | GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;
    if (gestures & EVENT_RESULT_PAN_HORIZONTAL)
      gc[2].dwWant |= GC_PAN | GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY | GC_PAN_WITH_GUTTER | GC_PAN_WITH_INERTIA;
    if (gestures & EVENT_RESULT_PAN_HORIZONTAL_WITHOUT_INERTIA)
      gc[2].dwWant |= GC_PAN | GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;
    if (gestures & EVENT_RESULT_SWIPE)
    {
      gc[2].dwWant |= GC_PAN | GC_PAN_WITH_SINGLE_FINGER_VERTICALLY | GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY | GC_PAN_WITH_GUTTER;

      // create a new touch swipe detector
      m_touchSwipeDetector = new CGenericTouchSwipeDetector(&CGenericTouchActionHandler::GetInstance(), 160.0f);
    }

    gc[0].dwBlock = gc[0].dwWant ^ 0x01;
    gc[1].dwBlock = gc[1].dwWant ^ 0x01;
    gc[2].dwBlock = gc[2].dwWant ^ 0x1F;
  }
  SetGestureConfig(hWnd, 0, 5, gc, sizeof(GESTURECONFIG));
}

void CWinEventsWin32::OnGesture(HWND hWnd, LPARAM lParam)
{
  GESTUREINFO gi = {};
  gi.cbSize = sizeof(gi);
  GetGestureInfo(reinterpret_cast<HGESTUREINFO>(lParam), &gi);

  // convert to window coordinates
  POINT point = { gi.ptsLocation.x, gi.ptsLocation.y };
  WindowFromScreenCoords(hWnd, &point);

  if (gi.dwID == GID_BEGIN)
    m_touchPointer.reset();

  // if there's a "current" touch from a previous event, copy it to "last"
  if (m_touchPointer.current.valid())
    m_touchPointer.last = m_touchPointer.current;

  // set the "current" touch
  m_touchPointer.current.x = static_cast<float>(point.x);
  m_touchPointer.current.y = static_cast<float>(point.y);
  m_touchPointer.current.time = time(nullptr);

  switch (gi.dwID)
  {
  case GID_BEGIN:
    {
      // set the "down" touch
      m_touchPointer.down = m_touchPointer.current;
      m_originalZoomDistance = 0;

      CGenericTouchActionHandler::GetInstance().OnTouchGestureStart(static_cast<float>(point.x), static_cast<float>(point.y));
    }
    break;

  case GID_END:
    CGenericTouchActionHandler::GetInstance().OnTouchGestureEnd(static_cast<float>(point.x), static_cast<float>(point.y), 0.0f, 0.0f, 0.0f, 0.0f);
    break;

  case GID_PAN:
    {
      if (!m_touchPointer.moving)
        m_touchPointer.moving = true;

      // calculate the velocity of the pan gesture
      float velocityX, velocityY;
      m_touchPointer.velocity(velocityX, velocityY);

      CGenericTouchActionHandler::GetInstance().OnTouchGesturePan(m_touchPointer.current.x, m_touchPointer.current.y,
                                                          m_touchPointer.current.x - m_touchPointer.last.x, m_touchPointer.current.y - m_touchPointer.last.y,
                                                          velocityX, velocityY);

      if (m_touchSwipeDetector != nullptr)
      {
        if (gi.dwFlags & GF_BEGIN)
        {
          m_touchPointer.down = m_touchPointer.current;
          m_touchSwipeDetector->OnTouchDown(0, m_touchPointer);
        }
        else if (gi.dwFlags & GF_END)
        {
          m_touchSwipeDetector->OnTouchUp(0, m_touchPointer);

          delete m_touchSwipeDetector;
          m_touchSwipeDetector = nullptr;
        }
        else
          m_touchSwipeDetector->OnTouchMove(0, m_touchPointer);
      }
    }
    break;

  case GID_ROTATE:
    {
      if (gi.dwFlags == GF_BEGIN)
        break;

      CGenericTouchActionHandler::GetInstance().OnRotate(static_cast<float>(point.x), static_cast<float>(point.y),
                                                        -static_cast<float>(ROTATE_ANGLE_DEGREE(gi.ullArguments)));
    }
    break;

  case GID_ZOOM:
    {
      if (gi.dwFlags == GF_BEGIN)
      {
        m_originalZoomDistance = static_cast<int>(LODWORD(gi.ullArguments));
        break;
      }

      // avoid division by 0
      if (m_originalZoomDistance == 0)
        break;

      CGenericTouchActionHandler::GetInstance().OnZoomPinch(static_cast<float>(point.x), static_cast<float>(point.y),
                                                            static_cast<float>(LODWORD(gi.ullArguments)) / static_cast<float>(m_originalZoomDistance));
    }
    break;

  case GID_TWOFINGERTAP:
    CGenericTouchActionHandler::GetInstance().OnTap(static_cast<float>(point.x), static_cast<float>(point.y), 2);
    break;

  case GID_PRESSANDTAP:
  default:
    // You have encountered an unknown gesture
    break;
  }
  CloseGestureInfoHandle(reinterpret_cast<HGESTUREINFO>(lParam));
}
