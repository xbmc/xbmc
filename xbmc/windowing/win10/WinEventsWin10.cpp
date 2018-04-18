/*
*      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "WinEventsWin10.h"
#include "Application.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/mouse/MouseStat.h"
#include "input/touch/generic/GenericTouchActionHandler.h"
#include "input/touch/generic/GenericTouchInputHandler.h"
#include "input/Action.h"
#include "input/ActionIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "platform/win10/AsyncHelpers.h"
#include "platform/win10/input/RemoteControlXbox.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "utils/SystemInfo.h"
#include "utils/Variant.h"
#include "windowing/windows/WinKeyMap.h"
#include "xbmc/GUIUserMessages.h"

using namespace Windows::Devices::Input;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::Media;
using namespace Windows::System;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;

using namespace ANNOUNCEMENT;
using namespace PERIPHERALS;
using namespace KODI::MESSAGING;

static Point GetScreenPoint(Point point)
{
  auto dpi = DX::DeviceResources::Get()->GetDpi();
  return Point(DX::ConvertDipsToPixels(point.X, dpi), DX::ConvertDipsToPixels(point.Y, dpi));
}

CWinEventsWin10::CWinEventsWin10() = default;
CWinEventsWin10::~CWinEventsWin10() = default;

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
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_UNFOCUS_ALL, 0, 0, 0, 0);
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
  window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>([&](CoreWindow^ wnd, WindowSizeChangedEventArgs^ args) {
    OnWindowSizeChanged(wnd, args);
  });
#if defined(NTDDI_WIN10_RS2) && (NTDDI_VERSION >= NTDDI_WIN10_RS2)
  try
  {
    window->ResizeStarted += ref new TypedEventHandler<CoreWindow^, Platform::Object^>([&](CoreWindow^ wnd, Platform::Object^ args) {
      OnWindowResizeStarted(wnd, args);
    });
    window->ResizeCompleted += ref new TypedEventHandler<CoreWindow^, Platform::Object^>([&](CoreWindow^ wnd, Platform::Object^ args) {
      OnWindowResizeCompleted(wnd, args);
    });
  } 
  catch (Platform::Exception^ ex)
  {
    // Win10 Creators Update is required
  }
#endif
  window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>([&](CoreWindow^ wnd, CoreWindowEventArgs^ args) {
    OnWindowClosed(wnd, args);
  });
  window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(CWinEventsWin10::OnVisibilityChanged);
  window->Activated += ref new TypedEventHandler<CoreWindow^, WindowActivatedEventArgs^>(CWinEventsWin10::OnWindowActivationChanged);
  // mouse, touch and pen
  window->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>([&](CoreWindow^ wnd, PointerEventArgs^ args) {
    OnPointerPressed(wnd, args);
  });
  window->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>([&](CoreWindow^ wnd, PointerEventArgs^ args) {
    OnPointerMoved(wnd, args);
  });
  window->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>([&](CoreWindow^ wnd, PointerEventArgs^ args) {
    OnPointerReleased(wnd, args);
  });
  window->PointerExited += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>([&](CoreWindow^ wnd, PointerEventArgs^ args) {
    OnPointerExited(wnd, args);
  });
  window->PointerWheelChanged += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>([&](CoreWindow^ wnd, PointerEventArgs^ args) {
    OnPointerWheelChanged(wnd, args);
  });
  // keyboard
  window->Dispatcher->AcceleratorKeyActivated += ref new TypedEventHandler<CoreDispatcher^, AcceleratorKeyEventArgs^>([&](CoreDispatcher^ disp, AcceleratorKeyEventArgs^ args) {
    OnAcceleratorKeyActivated(disp, args);
  });
  // display
  DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();
  currentDisplayInformation->DpiChanged += ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(CWinEventsWin10::OnDpiChanged);
  currentDisplayInformation->OrientationChanged += ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(CWinEventsWin10::OnOrientationChanged);
  DisplayInformation::DisplayContentsInvalidated += ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(CWinEventsWin10::OnDisplayContentsInvalidated);
  // system
  SystemNavigationManager^ sysNavManager = SystemNavigationManager::GetForCurrentView();
  sysNavManager->BackRequested += ref new EventHandler<BackRequestedEventArgs^>(CWinEventsWin10::OnBackRequested);

  // requirement for backgroup playback
  m_smtc = SystemMediaTransportControls::GetForCurrentView();
  if (m_smtc)
  {
    m_smtc->IsPlayEnabled = true;
    m_smtc->IsPauseEnabled = true;
    m_smtc->IsStopEnabled = true;
    m_smtc->IsRecordEnabled = true;
    m_smtc->IsNextEnabled = true;
    m_smtc->IsPreviousEnabled = true;
    m_smtc->IsFastForwardEnabled = true;
    m_smtc->IsRewindEnabled = true;
    m_smtc->IsChannelUpEnabled = true;
    m_smtc->IsChannelDownEnabled = true;
    if (CSysInfo::GetWindowsDeviceFamily() != CSysInfo::WindowsDeviceFamily::Xbox)
    {
      m_smtc->ButtonPressed += ref new TypedEventHandler<SystemMediaTransportControls^, SystemMediaTransportControlsButtonPressedEventArgs^>
                               (CWinEventsWin10::OnSystemMediaButtonPressed);
    }
    m_smtc->IsEnabled = true;
    CAnnouncementManager::GetInstance().AddAnnouncer(this);
  }
  if (CSysInfo::GetWindowsDeviceFamily() == CSysInfo::WindowsDeviceFamily::Xbox)
  {
    m_remote = std::make_unique<CRemoteControlXbox>();
    m_remote->Initialize();
  }
}

void CWinEventsWin10::UpdateWindowSize()
{
  auto size = DX::DeviceResources::Get()->GetOutputSize();

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
  if (g_application.GetRenderGUI() && !DX::Windowing()->IsAlteringWindow() && newEvent.resize.w > 0 && newEvent.resize.h > 0)
    MessagePush(&newEvent);
}

// Window event handlers.
void CWinEventsWin10::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
  CLog::Log(LOGDEBUG, __FUNCTION__": window size changed.");
  m_logicalWidth = args->Size.Width;
  m_logicalHeight = args->Size.Height;
  m_bResized = true;

  if (m_sizeChanging)
    return;

  HandleWindowSizeChanged();
}

void CWinEventsWin10::OnWindowResizeStarted(Windows::UI::Core::CoreWindow^ sender, Platform::Object ^ args)
{
  CLog::Log(LOGDEBUG, __FUNCTION__": window resize started.");
  m_logicalPosX = sender->Bounds.X;
  m_logicalPosY = sender->Bounds.Y;
  m_sizeChanging = true;
}

void CWinEventsWin10::OnWindowResizeCompleted(Windows::UI::Core::CoreWindow^ sender, Platform::Object ^ args)
{
  CLog::Log(LOGDEBUG, __FUNCTION__": window resize completed.");
  m_sizeChanging = false;

  if (m_logicalPosX != sender->Bounds.X || m_logicalPosY != sender->Bounds.Y)
    m_bMoved = true;

  HandleWindowSizeChanged();
}

void CWinEventsWin10::HandleWindowSizeChanged()
{
  CLog::Log(LOGDEBUG, __FUNCTION__": window size/move handled.");
  if (m_bMoved)
  {
    // it will get position from CoreWindow
    DX::Windowing()->OnMove(0, 0);
  }
  if (m_bResized)
  {
    DX::Windowing()->OnResize(m_logicalWidth, m_logicalHeight);
    UpdateWindowSize();
  }
  m_bResized = false;
  m_bMoved = false;
}

void CWinEventsWin10::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
  bool active = g_application.GetRenderGUI();
  g_application.SetRenderGUI(args->Visible);
  if (g_application.GetRenderGUI() != active)
    DX::Windowing()->NotifyAppActiveChange(g_application.GetRenderGUI());
  CLog::Log(LOGDEBUG, __FUNCTION__": window is %s", g_application.GetRenderGUI() ? "shown" : "hidden");
}

void CWinEventsWin10::OnWindowActivationChanged(CoreWindow ^ sender, WindowActivatedEventArgs ^ args)
{
  bool active = g_application.GetRenderGUI();
  if (args->WindowActivationState == CoreWindowActivationState::Deactivated)
  {
    g_application.SetRenderGUI(DX::Windowing()->WindowedMode());
  }
  else if (args->WindowActivationState == CoreWindowActivationState::PointerActivated
    || args->WindowActivationState == CoreWindowActivationState::CodeActivated)
  {
    g_application.SetRenderGUI(true);
  }
  if (g_application.GetRenderGUI() != active)
    DX::Windowing()->NotifyAppActiveChange(g_application.GetRenderGUI());
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
    MessagePush(&newEvent);
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
  MessagePush(&newEvent);
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
  MessagePush(&newEvent);
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

  MessagePush(&newEvent);
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
  MessagePush(&newEvent);
  newEvent.type = XBMC_MOUSEBUTTONUP;
  MessagePush(&newEvent);
}

void CWinEventsWin10::Kodi_KeyEvent(unsigned int vkey, unsigned scancode, unsigned keycode, bool isDown)
{
  static auto downState = CoreVirtualKeyStates::Down;

  XBMC_keysym keysym;
  memset(&keysym, 0, sizeof(keysym));
  keysym.scancode = scancode;
  keysym.sym = KODI::WINDOWING::WINDOWS::VK_keymap[vkey];
  keysym.unicode = keycode;

  auto window = CoreWindow::GetForCurrentThread();

  uint16_t mod = (uint16_t)XBMCKMOD_NONE;
  // If left control and right alt are down this usually means that AltGr is down
  if ((window->GetKeyState(VirtualKey::LeftControl) & downState) == downState
    && (window->GetKeyState(VirtualKey::RightMenu) & downState) == downState)
  {
    mod |= XBMCKMOD_MODE;
    mod |= XBMCKMOD_MODE;
  }
  else
  {
    if ((window->GetKeyState(VirtualKey::LeftControl) & downState) == downState)
      mod |= XBMCKMOD_LCTRL;
    if ((window->GetKeyState(VirtualKey::RightMenu) & downState) == downState)
      mod |= XBMCKMOD_RALT;
  }

  // Check the remaining modifiers
  if ((window->GetKeyState(VirtualKey::LeftShift) & downState) == downState)
    mod |= XBMCKMOD_LSHIFT;
  if ((window->GetKeyState(VirtualKey::RightShift) & downState) == downState)
    mod |= XBMCKMOD_RSHIFT;
  if ((window->GetKeyState(VirtualKey::RightControl) & downState) == downState)
    mod |= XBMCKMOD_RCTRL;
  if ((window->GetKeyState(VirtualKey::LeftMenu) & downState) == downState)
    mod |= XBMCKMOD_LALT;
  if ((window->GetKeyState(VirtualKey::LeftWindows) & downState) == downState)
    mod |= XBMCKMOD_LSUPER;
  if ((window->GetKeyState(VirtualKey::RightWindows) & downState) == downState)
    mod |= XBMCKMOD_LSUPER;

  keysym.mod = (XBMCMod)mod;

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.type = isDown ? XBMC_KEYDOWN : XBMC_KEYUP;
  newEvent.key.keysym = keysym;
  MessagePush(&newEvent);
}

void CWinEventsWin10::OnAcceleratorKeyActivated(CoreDispatcher^ sender, AcceleratorKeyEventArgs^ args)
{
  static auto lockedState = CoreVirtualKeyStates::Locked;
  static VirtualKey keyStore = VirtualKey::None;

  // skip if device is remote control
  if (m_remote && m_remote->IsRemoteDevice(args->DeviceId->Data()))
    return;

  bool isDown = false;
  unsigned keyCode = 0;
  unsigned vk = static_cast<unsigned>(args->VirtualKey);

  auto window = CoreWindow::GetForCurrentThread();
  bool numLockLocked = ((window->GetKeyState(VirtualKey::NumberKeyLock) & lockedState) == lockedState);

  switch (args->EventType)
  {
  case CoreAcceleratorKeyEventType::KeyDown:
  case CoreAcceleratorKeyEventType::SystemKeyDown:
  {
    if ( (vk == 0x08) // VK_BACK
      || (vk == 0x09) // VK_TAB
      || (vk == 0x0C) // VK_CLEAR
      || (vk == 0x0D) // VK_RETURN
      || (vk == 0x1B) // VK_ESCAPE
      || (vk == 0x20) // VK_SPACE
      || (vk >= 0x30 && vk <= 0x39) // numeric keys
      || (vk >= 0x41 && vk <= 0x5A) // alphabetic keys
      || (vk >= 0x60 && vk <= 0x69 && numLockLocked) // keypad numeric (if numlock is on)
      || (vk >= 0x6A && vk <= 0x6F) // keypad keys except numeric
      || (vk >= 0x92 && vk <= 0x96) // OEM specific
      || (vk >= 0xBA && vk <= 0xC0) // OEM specific
      || (vk >= 0xDB && vk <= 0xDF) // OEM specific
      || (vk >= 0xE1 && vk <= 0xF5 && vk != 0xE5 && vk != 0xE7 && vk != 0xE8) // OEM specific
      )
    {
      // store this for character events, because VirtualKey is key code on character event.
      keyStore = args->VirtualKey;
      return;
    }
    isDown = true;
    break;
  }
  case CoreAcceleratorKeyEventType::KeyUp:
  case CoreAcceleratorKeyEventType::SystemKeyUp:
    break;
  case CoreAcceleratorKeyEventType::Character:
  case CoreAcceleratorKeyEventType::SystemCharacter:
  case CoreAcceleratorKeyEventType::UnicodeCharacter:
  case CoreAcceleratorKeyEventType::DeadCharacter:
  case CoreAcceleratorKeyEventType::SystemDeadCharacter:
  {
    // VirtualKey is KeyCode
    keyCode = static_cast<unsigned>(args->VirtualKey);
    // rewrite vk with stored value
    vk = static_cast<unsigned>(keyStore);
    // reset stored value
    keyStore = VirtualKey::None;
    isDown = true;
  }
  default:
    break;
  }

  Kodi_KeyEvent(vk, args->KeyStatus.ScanCode, keyCode, isDown);
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
  DX::Windowing()->DPIChanged(sender->LogicalDpi, resizeRect);
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
  CLog::Log(LOGDEBUG, __FUNCTION__": onevent.");
  DX::DeviceResources::Get()->ValidateDevice();
}

void CWinEventsWin10::OnBackRequested(Platform::Object^ sender, Windows::UI::Core::BackRequestedEventArgs^ args)
{
  // handle this only on windows mobile
  if (CSysInfo::GetWindowsDeviceFamily() == CSysInfo::WindowsDeviceFamily::Mobile)
  {
    CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_NAV_BACK)));
  }
  args->Handled = true;
}

void CWinEventsWin10::OnSystemMediaButtonPressed(SystemMediaTransportControls^ sender, SystemMediaTransportControlsButtonPressedEventArgs^ args)
{
  int action = ACTION_NONE;
  switch (args->Button)
  {
  case SystemMediaTransportControlsButton::ChannelDown:
    action = ACTION_CHANNEL_DOWN;
    break;
  case SystemMediaTransportControlsButton::ChannelUp:
    action = ACTION_CHANNEL_UP;
    break;
  case SystemMediaTransportControlsButton::FastForward:
    action = ACTION_PLAYER_FORWARD;
    break;
  case SystemMediaTransportControlsButton::Rewind:
    action = ACTION_PLAYER_REWIND;
    break;
  case SystemMediaTransportControlsButton::Next:
    action = ACTION_NEXT_ITEM;
    break;
  case SystemMediaTransportControlsButton::Previous:
    action = ACTION_PREV_ITEM;
    break;
  case SystemMediaTransportControlsButton::Pause:
  case SystemMediaTransportControlsButton::Play:
    action = ACTION_PLAYER_PLAYPAUSE;
    break;
  case SystemMediaTransportControlsButton::Stop:
    action = ACTION_STOP;
    break;
  case SystemMediaTransportControlsButton::Record:
    action = ACTION_RECORD;
    break;
  default:
    break;
  }
  if (action != ACTION_NONE)
  {
    CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(action)));
  }
}

void CWinEventsWin10::Announce(AnnouncementFlag flag, const char * sender, const char * message, const CVariant & data)
{
  if (flag & AnnouncementFlag::Player)
  {
    double speed = 1.0;
    if (data.isMember("player") && data["player"].isMember("speed"))
      speed = data["player"]["speed"].asDouble(1.0);

    bool changed = false;
    MediaPlaybackStatus status = MediaPlaybackStatus::Changing;

    if (strcmp(message, "OnPlay") == 0 || strcmp(message, "OnResume") == 0)
    {
      changed = true;
      status = MediaPlaybackStatus::Playing;
    }
    else if (strcmp(message, "OnStop") == 0)
    {
      changed = true;
      status = MediaPlaybackStatus::Stopped;
    }
    else if (strcmp(message, "OnPause") == 0)
    {
      changed = true;
      status = MediaPlaybackStatus::Paused;
    }
    else if (strcmp(message, "OnSpeedChanged") == 0)
    {
      changed = true;
      status = speed != 0.0 ? MediaPlaybackStatus::Playing : MediaPlaybackStatus::Paused;
    }

    if (changed)
    {
      auto dispatcher = Windows::ApplicationModel::Core::CoreApplication::MainView->Dispatcher;
      dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([status, speed]
      {
        auto smtc = SystemMediaTransportControls::GetForCurrentView();
        if (!smtc)
          return;

        smtc->PlaybackStatus = status;
        smtc->PlaybackRate = speed;
      }));
    }
  }
}