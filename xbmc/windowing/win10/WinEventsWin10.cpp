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

#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "input/MouseStat.h"
#include "input/touch/generic/GenericTouchActionHandler.h"
#include "input/touch/generic/GenericTouchInputHandler.h"
#include "rendering/dx/DeviceResources.h"
#include "utils/log.h"
#include "windowing/windows/WinKeyMap.h"
#include "windowing/WindowingFactory.h"
#include "WinEventsWin10.h"

#include <ppltasks.h>

using namespace PERIPHERALS;
using namespace KODI::MESSAGING;
using namespace Windows::Devices::Input;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::System;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;

static Point GetScreenPoint(Point point)
{
  auto dpi = DX::DeviceResources::Get()->GetDpi();
  return Point(DX::ConvertDipsToPixels(point.X, dpi), DX::ConvertDipsToPixels(point.Y, dpi));
}

void CWinEventsWin10::InitOSKeymap(void)
{
  KODI::WINDOWING::WINDOWS::DIB_InitOSKeymap();
}

void CWinEventsWin10::MessagePush(XBMC_Event *newEvent)
{
  // push input events in the queue they may init modal dialog which init
  // deeper message loop and call the deeper MessagePump from there.
  if ( newEvent->type == XBMC_KEYDOWN
    || newEvent->type == XBMC_KEYUP
    || newEvent->type == XBMC_MOUSEMOTION
    || newEvent->type == XBMC_MOUSEBUTTONDOWN
    || newEvent->type == XBMC_MOUSEBUTTONUP
    || newEvent->type == XBMC_TOUCH)
  {
    m_events.push(*newEvent);
  }
  else
    g_application.OnEvent(*newEvent);
}

bool CWinEventsWin10::MessagePump()
{
  bool ret = false;

  // processes all pending events and exits immediately
  CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

  XBMC_Event pumpEvent;
  while (m_events.try_pop(pumpEvent))
  {
    ret |= g_application.OnEvent(pumpEvent);

    if (pumpEvent.type == XBMC_MOUSEBUTTONUP)
      g_windowManager.SendMessage(GUI_MSG_UNFOCUS_ALL, 0, 0, 0, 0);
  }
  return ret;
}

size_t CWinEventsWin10::GetQueueSize()
{
  return m_events.unsafe_size();
}

void CWinEventsWin10::InitEventHandlers(CoreWindow^ window)
{
  //window->SetPointerCapture();

  // window
  window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(CWinEventsWin10::OnWindowSizeChanged);
  window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(CWinEventsWin10::OnVisibilityChanged);
  window->Activated += ref new TypedEventHandler<CoreWindow^, WindowActivatedEventArgs^>(CWinEventsWin10::OnWindowActivationChanged);
  window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(CWinEventsWin10::OnWindowClosed);
  // mouse, touch and pen
  window->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(CWinEventsWin10::OnPointerPressed);
  window->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(CWinEventsWin10::OnPointerMoved);
  window->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(CWinEventsWin10::OnPointerReleased);
  window->PointerExited += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(CWinEventsWin10::OnPointerExited);
  window->PointerWheelChanged += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(CWinEventsWin10::OnPointerWheelChanged);
  // keyboard
  window->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(CWinEventsWin10::OnKeyDown);
  window->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(CWinEventsWin10::OnKeyUp);
  window->CharacterReceived += ref new TypedEventHandler<CoreWindow^, CharacterReceivedEventArgs^>(CWinEventsWin10::OnCharacterReceived);
  // display
  DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();
  currentDisplayInformation->DpiChanged += ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(CWinEventsWin10::OnDpiChanged);
  currentDisplayInformation->OrientationChanged += ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(CWinEventsWin10::OnOrientationChanged);
  DisplayInformation::DisplayContentsInvalidated += ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(CWinEventsWin10::OnDisplayContentsInvalidated);
  // system
  SystemNavigationManager^ sysNavManager = SystemNavigationManager::GetForCurrentView();
  sysNavManager->BackRequested += ref new EventHandler<BackRequestedEventArgs^>(CWinEventsWin10::OnBackRequested);
}

void CWinEventsWin10::UpdateWindowSize()
{
  auto size = g_Windowing.GetOutputSize();

  CLog::Log(LOGDEBUG, __FUNCTION__": window resize event %f x %f (as:%s)", size.Width, size.Height, g_advancedSettings.m_fullScreen ? "true" : "false");

  auto appView = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView();
  appView->SetDesiredBoundsMode(Windows::UI::ViewManagement::ApplicationViewBoundsMode::UseCoreWindow);

  // seems app has lost FS mode it may occurs if an user use core window's button
  if (g_advancedSettings.m_fullScreen && !appView->IsFullScreenMode)
    g_advancedSettings.m_fullScreen = false;

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.type = XBMC_VIDEORESIZE;
  newEvent.resize.w = size.Width;
  newEvent.resize.h = size.Height;
  if (g_application.GetRenderGUI() && !g_Windowing.IsAlteringWindow() && newEvent.resize.w > 0 && newEvent.resize.h > 0)
    CWinEvents::MessagePush(&newEvent);
}

// Window event handlers.
void CWinEventsWin10::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
  g_Windowing.OnResize(args->Size.Width, args->Size.Height);
  UpdateWindowSize();
}

void CWinEventsWin10::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
  bool active = g_application.GetRenderGUI();
  g_application.SetRenderGUI(args->Visible);
  if (g_application.GetRenderGUI() != active)
    g_Windowing.NotifyAppActiveChange(g_application.GetRenderGUI());
  CLog::Log(LOGDEBUG, __FUNCTION__": window is %s", g_application.GetRenderGUI() ? "shown" : "hidden");
}

void CWinEventsWin10::OnWindowActivationChanged(CoreWindow ^ sender, WindowActivatedEventArgs ^ args)
{
  bool active = g_application.GetRenderGUI();
  if (args->WindowActivationState == CoreWindowActivationState::Deactivated)
  {
    g_application.SetRenderGUI(g_Windowing.WindowedMode());
  }
  else if (args->WindowActivationState == CoreWindowActivationState::PointerActivated
    || args->WindowActivationState == CoreWindowActivationState::CodeActivated)
  {
    g_application.SetRenderGUI(true);
  }
  if (g_application.GetRenderGUI() != active)
    g_Windowing.NotifyAppActiveChange(g_application.GetRenderGUI());
  CLog::Log(LOGDEBUG, __FUNCTION__": window is %s", g_application.GetRenderGUI() ? "active" : "inactive");
}

void CWinEventsWin10::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
  // send quit command to the application if it's still running
  if (!g_application.m_bStop)
  {
    XBMC_Event newEvent;
    memset(&newEvent, 0, sizeof(newEvent));
    newEvent.type = XBMC_QUIT;
    CWinEvents::MessagePush(&newEvent);
  }
}

void CWinEventsWin10::OnPointerPressed(CoreWindow ^ sender, PointerEventArgs ^ args)
{
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));

  PointerPoint^ point = args->CurrentPoint;
  auto position = GetScreenPoint(point->Position);

  if (point->PointerDevice->PointerDeviceType == PointerDeviceType::Touch)
  {
    CGenericTouchInputHandler::GetInstance().HandleTouchInput(TouchInputDown, position.X, position.Y, point->Timestamp, 0, 10);
    return;
  }
  else
  {
    newEvent.type = XBMC_MOUSEBUTTONDOWN;
    newEvent.button.x = position.X;
    newEvent.button.y = position.Y;
    if (point->PointerDevice->PointerDeviceType == PointerDeviceType::Mouse)
    {
      if (point->Properties->IsLeftButtonPressed)
        newEvent.button.button = XBMC_BUTTON_LEFT;
      else if (point->Properties->IsMiddleButtonPressed)
        newEvent.button.button = XBMC_BUTTON_MIDDLE;
      else if (point->Properties->IsRightButtonPressed)
        newEvent.button.button = XBMC_BUTTON_RIGHT;
    }
    else if (point->PointerDevice->PointerDeviceType == PointerDeviceType::Pen)
    {
      // pen
      // TODO
    }
  }
  CWinEvents::MessagePush(&newEvent);
}

void CWinEventsWin10::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
  PointerPoint^ point = args->CurrentPoint;
  auto position = GetScreenPoint(point->Position);

  if (point->PointerDevice->PointerDeviceType == PointerDeviceType::Touch)
  {
    if (point->IsInContact)
    {
      CGenericTouchInputHandler::GetInstance().UpdateTouchPointer(0, position.X, position.Y, point->Timestamp, 10.f);
      CGenericTouchInputHandler::GetInstance().HandleTouchInput(TouchInputMove, position.X, position.Y, point->Timestamp, 0, 10.f);
    }
    return;
  }

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.type = XBMC_MOUSEMOTION;
  newEvent.motion.x = position.X;
  newEvent.motion.y = position.Y;
  CWinEvents::MessagePush(&newEvent);
}

void CWinEventsWin10::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
  PointerPoint^ point = args->CurrentPoint;
  auto position = GetScreenPoint(point->Position);

  if (point->PointerDevice->PointerDeviceType == PointerDeviceType::Touch)
  {
    CGenericTouchInputHandler::GetInstance().HandleTouchInput(TouchInputUp, position.X, position.Y, point->Timestamp, 0, 10);
    return;
  }

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.type = XBMC_MOUSEBUTTONUP;
  newEvent.button.x = position.X;
  newEvent.button.y = position.Y;

  if (point->Properties->PointerUpdateKind == Windows::UI::Input::PointerUpdateKind::LeftButtonReleased)
    newEvent.button.button = XBMC_BUTTON_LEFT;
  else if (point->Properties->PointerUpdateKind == Windows::UI::Input::PointerUpdateKind::MiddleButtonReleased)
    newEvent.button.button = XBMC_BUTTON_MIDDLE;
  else if (point->Properties->PointerUpdateKind == Windows::UI::Input::PointerUpdateKind::RightButtonReleased)
    newEvent.button.button = XBMC_BUTTON_RIGHT;

  CWinEvents::MessagePush(&newEvent);
}

void CWinEventsWin10::OnPointerExited(CoreWindow^ sender, PointerEventArgs^ args)
{
  PointerPoint^ point = args->CurrentPoint;
  auto position = GetScreenPoint(point->Position);

  if (point->PointerDevice->PointerDeviceType == PointerDeviceType::Touch)
  {
    CGenericTouchInputHandler::GetInstance().HandleTouchInput(TouchInputAbort, position.X, position.Y, point->Timestamp, 0, 10);
    return;
  }
}

void CWinEventsWin10::OnPointerWheelChanged(CoreWindow^ sender, PointerEventArgs^ args)
{
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.type = XBMC_MOUSEBUTTONDOWN;
  newEvent.button.x = args->CurrentPoint->Position.X;
  newEvent.button.y = args->CurrentPoint->Position.Y;
  newEvent.button.button = args->CurrentPoint->Properties->MouseWheelDelta > 0 ? XBMC_BUTTON_WHEELUP : XBMC_BUTTON_WHEELDOWN;
  CWinEvents::MessagePush(&newEvent);
  newEvent.type = XBMC_MOUSEBUTTONUP;
  CWinEvents::MessagePush(&newEvent);
}

static void TranslateKey(CoreWindow^ window, XBMC_keysym &keysym, VirtualKey vkey, unsigned scancode, unsigned keycode)
{
  switch (vkey)
  {
  case Windows::System::VirtualKey::GamepadA:
  case Windows::System::VirtualKey::GamepadLeftThumbstickButton:
  case Windows::System::VirtualKey::GamepadRightThumbstickButton:
    keysym.sym = XBMCK_RETURN;
    break;
  case Windows::System::VirtualKey::GamepadB:
    keysym.sym = XBMCK_BACKSPACE;
    break;
  case Windows::System::VirtualKey::GamepadDPadUp:
  case Windows::System::VirtualKey::GamepadLeftThumbstickUp:
  case Windows::System::VirtualKey::GamepadRightThumbstickUp:
  case Windows::System::VirtualKey::NavigationUp:
    keysym.sym = XBMCK_UP;
    break;
  case Windows::System::VirtualKey::GamepadDPadDown:
  case Windows::System::VirtualKey::GamepadLeftThumbstickDown:
  case Windows::System::VirtualKey::GamepadRightThumbstickDown:
  case Windows::System::VirtualKey::NavigationDown:
    keysym.sym = XBMCK_DOWN;
    break;
  case Windows::System::VirtualKey::GamepadDPadLeft:
  case Windows::System::VirtualKey::GamepadLeftThumbstickLeft:
  case Windows::System::VirtualKey::GamepadRightThumbstickLeft:
  case Windows::System::VirtualKey::NavigationLeft:
    keysym.sym = XBMCK_LEFT;
    break;
  case Windows::System::VirtualKey::GamepadDPadRight:
  case Windows::System::VirtualKey::GamepadLeftThumbstickRight:
  case Windows::System::VirtualKey::GamepadRightThumbstickRight:
  case Windows::System::VirtualKey::NavigationRight:
    keysym.sym = XBMCK_RIGHT;
    break;
  default:
    keysym.sym = KODI::WINDOWING::WINDOWS::VK_keymap[static_cast<UINT>(vkey)];
    break;
  }
  keysym.unicode = 0;
  keysym.scancode = scancode;

  if (window->GetKeyState(VirtualKey::NumberKeyLock) == CoreVirtualKeyStates::Locked
    && vkey >= VirtualKey::NumberPad0 && vkey <= VirtualKey::NumberPad9)
  {
    keysym.unicode = static_cast<UINT>(vkey - VirtualKey::NumberPad0) + '0';
  }
  else
  {
    keysym.unicode = keycode;
  }

  uint16_t mod = (uint16_t)XBMCKMOD_NONE;

  // If left control and right alt are down this usually means that AltGr is down
  if (window->GetKeyState(VirtualKey::LeftControl) == CoreVirtualKeyStates::Down
    && window->GetKeyState(VirtualKey::RightMenu) == CoreVirtualKeyStates::Down)
  {
    mod |= XBMCKMOD_MODE;
    mod |= XBMCKMOD_MODE;
  }
  else
  {
    if (window->GetKeyState(VirtualKey::LeftControl) == CoreVirtualKeyStates::Down)
      mod |= XBMCKMOD_LCTRL;
    if (window->GetKeyState(VirtualKey::RightMenu) == CoreVirtualKeyStates::Down)
      mod |= XBMCKMOD_RALT;
  }

  // Check the remaining modifiers
  if (window->GetKeyState(VirtualKey::LeftShift) == CoreVirtualKeyStates::Down)
    mod |= XBMCKMOD_LSHIFT;
  if (window->GetKeyState(VirtualKey::RightShift) == CoreVirtualKeyStates::Down)
    mod |= XBMCKMOD_RSHIFT;
  if (window->GetKeyState(VirtualKey::RightControl) == CoreVirtualKeyStates::Down)
    mod |= XBMCKMOD_RCTRL;
  if (window->GetKeyState(VirtualKey::LeftMenu) == CoreVirtualKeyStates::Down)
    mod |= XBMCKMOD_LALT;
  if (window->GetKeyState(VirtualKey::LeftWindows) == CoreVirtualKeyStates::Down)
    mod |= XBMCKMOD_LSUPER;
  if (window->GetKeyState(VirtualKey::RightWindows) == CoreVirtualKeyStates::Down)
    mod |= XBMCKMOD_LSUPER;
  keysym.mod = (XBMCMod)mod;
}

VirtualKey keyDown = VirtualKey::None;

void CWinEventsWin10::OnKeyDown(CoreWindow^ sender, KeyEventArgs^ args)
{
  int vk = static_cast<unsigned>(args->VirtualKey);
  if ((vk == 0x08) // VK_BACK
    || (vk == 0x09) // VK_TAB
    || (vk == 0x0C) // VK_CLEAR
    || (vk == 0x0D) // VK_RETURN
    || (vk == 0x1B) // VK_ESCAPE
    || (vk == 0x20) // VK_SPACE
    || (vk >= 0x30 && vk <= 0x39) // numeric keys
    || (vk >= 0x41 && vk <= 0x5A) // alphabetic keys
    || (vk >= 0x60 && vk <= 0x6F) // keypad keys
    || (vk >= 0x92 && vk <= 0x96) // OEM specific
    || (vk >= 0xBA && vk <= 0xC0) // OEM specific
    || (vk >= 0xDB && vk <= 0xDF) // OEM specific
    || (vk >= 0xE1 && vk <= 0xF5 && vk != 0xE5 && vk != 0xE7 && vk != 0xE8) // OEM specific
    )
  {
    // Those will be handled in OnCharacterReceived
    // store VirtualKey for future processing
    keyDown = args->VirtualKey;
    return;
  }

  XBMC_keysym keysym;
  TranslateKey(sender, keysym, args->VirtualKey, args->KeyStatus.ScanCode, 0);

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.type = XBMC_KEYDOWN;
  newEvent.key.keysym = keysym;
  CWinEvents::MessagePush(&newEvent);
}

void CWinEventsWin10::OnKeyUp(CoreWindow^ sender, KeyEventArgs^ args)
{
  XBMC_keysym keysym;
  TranslateKey(sender, keysym, args->VirtualKey, args->KeyStatus.ScanCode, 0);

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.type = XBMC_KEYUP;
  newEvent.key.keysym = keysym;
  CWinEvents::MessagePush(&newEvent);
}

void CWinEventsWin10::OnCharacterReceived(CoreWindow^ sender, CharacterReceivedEventArgs^ args)
{
  XBMC_keysym keysym;
  TranslateKey(sender, keysym, keyDown, args->KeyStatus.ScanCode, args->KeyCode);

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.key.keysym = keysym;
  newEvent.type = XBMC_KEYDOWN;
  CWinEvents::MessagePush(&newEvent);
}

// DisplayInformation event handlers.
void CWinEventsWin10::OnDpiChanged(DisplayInformation^ sender, Platform::Object^ args)
{
  // Note: The value for LogicalDpi retrieved here may not match the effective DPI of the app
  // if it is being scaled for high resolution devices. Once the DPI is set on DeviceResources,
  // you should always retrieve it using the GetDpi method.
  // See DeviceResources.cpp for more details.
  //critical_section::scoped_lock lock(m_deviceResources->GetCriticalSection());
  RECT resizeRect = { 0,0,0,0 };
  g_Windowing.DPIChanged(sender->LogicalDpi, resizeRect);
  CGenericTouchInputHandler::GetInstance().SetScreenDPI(DX::DisplayMetrics::Dpi100);
}

void CWinEventsWin10::OnOrientationChanged(DisplayInformation^ sender, Platform::Object^ args)
{
  //critical_section::scoped_lock lock(m_deviceResources->GetCriticalSection());
  //m_deviceResources->SetCurrentOrientation(sender->CurrentOrientation);

  //auto size = DX::DeviceResources::Get()->GetOutputSize();
  //UpdateWindowSize(size.Width, size.Height);
}

void CWinEventsWin10::OnDisplayContentsInvalidated(DisplayInformation^ sender, Platform::Object^ args)
{
  //critical_section::scoped_lock lock(m_deviceResources->GetCriticalSection());
  DX::DeviceResources::Get()->ValidateDevice();
}

void CWinEventsWin10::OnBackRequested(Platform::Object^ sender, Windows::UI::Core::BackRequestedEventArgs^ args)
{
  XBMC_keysym keysym;
  keysym.sym = XBMCK_BACKSPACE;

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.key.keysym = keysym;

  newEvent.type = XBMC_KEYDOWN;
  CWinEvents::MessagePush(&newEvent);

  newEvent.type = XBMC_KEYUP;
  CWinEvents::MessagePush(&newEvent);

  args->Handled = true;
}
