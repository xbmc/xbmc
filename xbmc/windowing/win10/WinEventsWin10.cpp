/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinEventsWin10.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "application/AppInboundProtocol.h"
#include "application/Application.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseStat.h"
#include "input/touch/generic/GenericTouchInputHandler.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/SystemInfo.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "windowing/windows/WinKeyMap.h"

#include "platform/win10/input/RemoteControlXbox.h"

#include <winrt/Windows.Devices.Input.h>

namespace winrt
{
  using namespace Windows::Foundation;
}
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::Devices::Input;
using namespace winrt::Windows::Graphics::Display;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::System;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::Input;
using namespace winrt::Windows::UI::ViewManagement;

using namespace PERIPHERALS;

static winrt::Point GetScreenPoint(winrt::Point point)
{
  auto dpi = DX::DeviceResources::Get()->GetDpi();
  return winrt::Point(DX::ConvertDipsToPixels(point.X, dpi), DX::ConvertDipsToPixels(point.Y, dpi));
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
  {
    std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
    if (appPort)
      appPort->OnEvent(*newEvent);
  }
}

bool CWinEventsWin10::MessagePump()
{
  bool ret = false;
  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();

  // processes all pending events and exits immediately
  CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

  XBMC_Event pumpEvent;
  while (m_events.try_pop(pumpEvent))
  {
    if (appPort)
      ret |= appPort->OnEvent(pumpEvent);

    if (pumpEvent.type == XBMC_MOUSEBUTTONUP)
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_UNFOCUS_ALL, 0, 0, 0, 0);
  }
  return ret;
}

size_t CWinEventsWin10::GetQueueSize()
{
  return m_events.unsafe_size();
}

void CWinEventsWin10::InitEventHandlers(const CoreWindow& window)
{
  CWinEventsWin10::InitOSKeymap();

  //window->SetPointerCapture();

  // window
  window.SizeChanged({ this, &CWinEventsWin10::OnWindowSizeChanged });
  window.ResizeStarted({ this, &CWinEventsWin10::OnWindowResizeStarted });
  window.ResizeCompleted({ this, &CWinEventsWin10::OnWindowResizeCompleted });
  window.Closed({ this, &CWinEventsWin10::OnWindowClosed});
  window.VisibilityChanged(CWinEventsWin10::OnVisibilityChanged);
  window.Activated(CWinEventsWin10::OnWindowActivationChanged);
  // mouse, touch and pen
  window.PointerPressed({ this, &CWinEventsWin10::OnPointerPressed });
  window.PointerMoved({ this, &CWinEventsWin10::OnPointerMoved });
  window.PointerReleased({ this, &CWinEventsWin10::OnPointerReleased });
  window.PointerExited({ this, &CWinEventsWin10::OnPointerExited });
  window.PointerWheelChanged({ this, &CWinEventsWin10::OnPointerWheelChanged });
  // keyboard
  window.Dispatcher().AcceleratorKeyActivated({ this, &CWinEventsWin10::OnAcceleratorKeyActivated });
  // display
  DisplayInformation currentDisplayInformation = DisplayInformation::GetForCurrentView();
  currentDisplayInformation.DpiChanged(CWinEventsWin10::OnDpiChanged);
  currentDisplayInformation.OrientationChanged(CWinEventsWin10::OnOrientationChanged);
  DisplayInformation::DisplayContentsInvalidated(CWinEventsWin10::OnDisplayContentsInvalidated);
  // system
  SystemNavigationManager sysNavManager = SystemNavigationManager::GetForCurrentView();
  sysNavManager.BackRequested(CWinEventsWin10::OnBackRequested);

  // requirement for backgroup playback
  m_smtc = SystemMediaTransportControls::GetForCurrentView();
  if (m_smtc)
  {
    m_smtc.IsPlayEnabled(true);
    m_smtc.IsPauseEnabled(true);
    m_smtc.IsStopEnabled(true);
    m_smtc.IsRecordEnabled(true);
    m_smtc.IsNextEnabled(true);
    m_smtc.IsPreviousEnabled(true);
    m_smtc.IsFastForwardEnabled(true);
    m_smtc.IsRewindEnabled(true);
    m_smtc.IsChannelUpEnabled(true);
    m_smtc.IsChannelDownEnabled(true);
    if (CSysInfo::GetWindowsDeviceFamily() != CSysInfo::WindowsDeviceFamily::Xbox)
    {
      m_smtc.ButtonPressed(CWinEventsWin10::OnSystemMediaButtonPressed);
    }
    m_smtc.IsEnabled(true);;
    CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);
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

  CLog::Log(LOGDEBUG, __FUNCTION__ ": window resize event {:f} x {:f} (as:{})", size.Width,
            size.Height,
            CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreen ? "true"
                                                                                        : "false");

  auto appView = ApplicationView::GetForCurrentView();
  appView.SetDesiredBoundsMode(ApplicationViewBoundsMode::UseCoreWindow);

  // seems app has lost FS mode it may occurs if an user use core window's button
  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreen && !appView.IsFullScreenMode())
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreen = false;

  XBMC_Event newEvent = {};
  newEvent.type = XBMC_VIDEORESIZE;
  newEvent.resize.w = size.Width;
  newEvent.resize.h = size.Height;
  if (g_application.GetRenderGUI() && !DX::Windowing()->IsAlteringWindow() && newEvent.resize.w > 0 && newEvent.resize.h > 0)
    MessagePush(&newEvent);
}

void CWinEventsWin10::OnResize(float width, float height)
{
  CLog::Log(LOGDEBUG, __FUNCTION__": window size changed.");
  m_logicalWidth = width;
  m_logicalHeight = height;
  m_bResized = true;

  if (m_sizeChanging)
    return;

  HandleWindowSizeChanged();
}

// Window event handlers.
void CWinEventsWin10::OnWindowSizeChanged(const CoreWindow&, const WindowSizeChangedEventArgs& args)
{
  OnResize(args.Size().Width, args.Size().Height);
}

void CWinEventsWin10::OnWindowResizeStarted(const CoreWindow& sender, const winrt::IInspectable&)
{
  CLog::Log(LOGDEBUG, __FUNCTION__": window resize started.");
  m_logicalPosX = sender.Bounds().X;
  m_logicalPosY = sender.Bounds().Y;
  m_sizeChanging = true;
}

void CWinEventsWin10::OnWindowResizeCompleted(const CoreWindow& sender, const winrt::IInspectable&)
{
  CLog::Log(LOGDEBUG, __FUNCTION__": window resize completed.");
  m_sizeChanging = false;

  if (m_logicalPosX != sender.Bounds().X || m_logicalPosY != sender.Bounds().Y)
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

void CWinEventsWin10::OnVisibilityChanged(const CoreWindow& sender, const VisibilityChangedEventArgs& args)
{
  bool active = g_application.GetRenderGUI();
  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->SetRenderGUI(args.Visible());

  if (g_application.GetRenderGUI() != active)
    DX::Windowing()->NotifyAppActiveChange(g_application.GetRenderGUI());
  CLog::Log(LOGDEBUG, __FUNCTION__ ": window is {}",
            g_application.GetRenderGUI() ? "shown" : "hidden");
}

void CWinEventsWin10::OnWindowActivationChanged(const CoreWindow& sender, const WindowActivatedEventArgs& args)
{
  bool active = g_application.GetRenderGUI();
  if (args.WindowActivationState() == CoreWindowActivationState::Deactivated)
  {
    std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
    if (appPort)
      appPort->SetRenderGUI(DX::Windowing()->WindowedMode());
  }
  else if (args.WindowActivationState() == CoreWindowActivationState::PointerActivated
    || args.WindowActivationState() == CoreWindowActivationState::CodeActivated)
  {
    std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
    if (appPort)
      appPort->SetRenderGUI(true);
  }
  if (g_application.GetRenderGUI() != active)
    DX::Windowing()->NotifyAppActiveChange(g_application.GetRenderGUI());

  if (CServiceBroker::IsLoggingUp())
    CLog::Log(LOGDEBUG, __FUNCTION__ ": window is {}",
              g_application.GetRenderGUI() ? "active" : "inactive");
}

void CWinEventsWin10::OnWindowClosed(const CoreWindow& sender, const CoreWindowEventArgs& args)
{
  // send quit command to the application if it's still running
  if (!g_application.m_bStop)
  {
    XBMC_Event newEvent = {};
    newEvent.type = XBMC_QUIT;
    MessagePush(&newEvent);
  }
}

void CWinEventsWin10::OnPointerPressed(const CoreWindow&, const PointerEventArgs& args)
{
  XBMC_Event newEvent = {};

  PointerPoint point = args.CurrentPoint();
  auto position = GetScreenPoint(point.Position());

  if (point.PointerDevice().PointerDeviceType() == PointerDeviceType::Touch)
  {
    CGenericTouchInputHandler::GetInstance().HandleTouchInput(TouchInputDown, position.X, position.Y, point.Timestamp(), 0, 10);
    return;
  }
  else
  {
    newEvent.type = XBMC_MOUSEBUTTONDOWN;
    newEvent.button.x = position.X;
    newEvent.button.y = position.Y;
    if (point.PointerDevice().PointerDeviceType() == PointerDeviceType::Mouse)
    {
      if (point.Properties().IsLeftButtonPressed())
        newEvent.button.button = XBMC_BUTTON_LEFT;
      else if (point.Properties().IsMiddleButtonPressed())
        newEvent.button.button = XBMC_BUTTON_MIDDLE;
      else if (point.Properties().IsRightButtonPressed())
        newEvent.button.button = XBMC_BUTTON_RIGHT;
    }
    else if (point.PointerDevice().PointerDeviceType() == PointerDeviceType::Pen)
    {
      // pen
      // TODO
    }
  }
  MessagePush(&newEvent);
}

void CWinEventsWin10::OnPointerMoved(const CoreWindow&, const PointerEventArgs& args)
{
  PointerPoint point = args.CurrentPoint();
  auto position = GetScreenPoint(point.Position());

  if (point.PointerDevice().PointerDeviceType() == PointerDeviceType::Touch)
  {
    if (point.IsInContact())
    {
      CGenericTouchInputHandler::GetInstance().UpdateTouchPointer(0, position.X, position.Y, point.Timestamp(), 10.f);
      CGenericTouchInputHandler::GetInstance().HandleTouchInput(TouchInputMove, position.X, position.Y, point.Timestamp(), 0, 10.f);
    }
    return;
  }

  XBMC_Event newEvent = {};
  newEvent.type = XBMC_MOUSEMOTION;
  newEvent.motion.x = position.X;
  newEvent.motion.y = position.Y;
  MessagePush(&newEvent);
}

void CWinEventsWin10::OnPointerReleased(const CoreWindow&, const PointerEventArgs& args)
{
  PointerPoint point = args.CurrentPoint();
  auto position = GetScreenPoint(point.Position());

  if (point.PointerDevice().PointerDeviceType() == PointerDeviceType::Touch)
  {
    CGenericTouchInputHandler::GetInstance().HandleTouchInput(TouchInputUp, position.X, position.Y, point.Timestamp(), 0, 10);
    return;
  }

  XBMC_Event newEvent = {};
  newEvent.type = XBMC_MOUSEBUTTONUP;
  newEvent.button.x = position.X;
  newEvent.button.y = position.Y;

  if (point.Properties().PointerUpdateKind() == PointerUpdateKind::LeftButtonReleased)
    newEvent.button.button = XBMC_BUTTON_LEFT;
  else if (point.Properties().PointerUpdateKind() == PointerUpdateKind::MiddleButtonReleased)
    newEvent.button.button = XBMC_BUTTON_MIDDLE;
  else if (point.Properties().PointerUpdateKind() == PointerUpdateKind::RightButtonReleased)
    newEvent.button.button = XBMC_BUTTON_RIGHT;

  MessagePush(&newEvent);
}

void CWinEventsWin10::OnPointerExited(const CoreWindow&, const PointerEventArgs& args)
{
  const PointerPoint& point = args.CurrentPoint();
  auto position = GetScreenPoint(point.Position());

  if (point.PointerDevice().PointerDeviceType() == PointerDeviceType::Touch)
  {
    CGenericTouchInputHandler::GetInstance().HandleTouchInput(TouchInputAbort, position.X, position.Y, point.Timestamp(), 0, 10);
  }
}

void CWinEventsWin10::OnPointerWheelChanged(const CoreWindow&, const PointerEventArgs& args)
{
  XBMC_Event newEvent = {};
  newEvent.type = XBMC_MOUSEBUTTONDOWN;
  newEvent.button.x = args.CurrentPoint().Position().X;
  newEvent.button.y = args.CurrentPoint().Position().Y;
  newEvent.button.button = args.CurrentPoint().Properties().MouseWheelDelta() > 0 ? XBMC_BUTTON_WHEELUP : XBMC_BUTTON_WHEELDOWN;
  MessagePush(&newEvent);
  newEvent.type = XBMC_MOUSEBUTTONUP;
  MessagePush(&newEvent);
}

void CWinEventsWin10::Kodi_KeyEvent(unsigned int vkey, unsigned scancode, unsigned keycode, bool isDown)
{
  using State = CoreVirtualKeyStates;

  XBMC_keysym keysym = {};
  keysym.scancode = scancode;
  keysym.sym = KODI::WINDOWING::WINDOWS::VK_keymap[vkey];
  keysym.unicode = keycode;

  auto window = CoreWindow::GetForCurrentThread();

  uint16_t mod = (uint16_t)XBMCKMOD_NONE;
  // If left control and right alt are down this usually means that AltGr is down
  if ((window.GetKeyState(VirtualKey::LeftControl) & State::Down) == State::Down
    && (window.GetKeyState(VirtualKey::RightMenu) & State::Down) == State::Down)
  {
    mod |= XBMCKMOD_MODE;
    mod |= XBMCKMOD_MODE;
  }
  else
  {
    if ((window.GetKeyState(VirtualKey::LeftControl) & State::Down) == State::Down)
      mod |= XBMCKMOD_LCTRL;
    if ((window.GetKeyState(VirtualKey::RightMenu) & State::Down) == State::Down)
      mod |= XBMCKMOD_RALT;
  }

  // Check the remaining modifiers
  if ((window.GetKeyState(VirtualKey::LeftShift) & State::Down) == State::Down)
    mod |= XBMCKMOD_LSHIFT;
  if ((window.GetKeyState(VirtualKey::RightShift) & State::Down) == State::Down)
    mod |= XBMCKMOD_RSHIFT;
  if ((window.GetKeyState(VirtualKey::RightControl) & State::Down) == State::Down)
    mod |= XBMCKMOD_RCTRL;
  if ((window.GetKeyState(VirtualKey::LeftMenu) & State::Down) == State::Down)
    mod |= XBMCKMOD_LALT;
  if ((window.GetKeyState(VirtualKey::LeftWindows) & State::Down) == State::Down)
    mod |= XBMCKMOD_LSUPER;
  if ((window.GetKeyState(VirtualKey::RightWindows) & State::Down) == State::Down)
    mod |= XBMCKMOD_LSUPER;

  keysym.mod = static_cast<XBMCMod>(mod);

  XBMC_Event newEvent = {};
  newEvent.type = isDown ? XBMC_KEYDOWN : XBMC_KEYUP;
  newEvent.key.keysym = keysym;
  MessagePush(&newEvent);
}

void CWinEventsWin10::OnAcceleratorKeyActivated(const CoreDispatcher&, const AcceleratorKeyEventArgs& args)
{
  static auto lockedState = CoreVirtualKeyStates::Locked;
  static VirtualKey keyStore = VirtualKey::None;

  // skip if device is remote control
  if (m_remote && m_remote->IsRemoteDevice(args.DeviceId().c_str()))
    return;

  bool isDown = false;
  unsigned keyCode = 0;
  unsigned vk = static_cast<unsigned>(args.VirtualKey());

  auto window = CoreWindow::GetForCurrentThread();
  bool numLockLocked = ((window.GetKeyState(VirtualKey::NumberKeyLock) & lockedState) == lockedState);

  switch (args.EventType())
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
      keyStore = args.VirtualKey();
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
    keyCode = static_cast<unsigned>(args.VirtualKey());
    // rewrite vk with stored value
    vk = static_cast<unsigned>(keyStore);
    // reset stored value
    keyStore = VirtualKey::None;
    isDown = true;
  }
  default:
    break;
  }

  Kodi_KeyEvent(vk, args.KeyStatus().ScanCode, keyCode, isDown);
  args.Handled(true);
}

// DisplayInformation event handlers.
void CWinEventsWin10::OnDpiChanged(const DisplayInformation& sender, const winrt::IInspectable&)
{
  // Note: The value for LogicalDpi retrieved here may not match the effective DPI of the app
  // if it is being scaled for high resolution devices. Once the DPI is set on DeviceResources,
  // you should always retrieve it using the GetDpi method.
  // See DeviceResources.cpp for more details.
  //critical_section::scoped_lock lock(m_deviceResources->GetCriticalSection());
  RECT resizeRect = { 0,0,0,0 };
  DX::Windowing()->DPIChanged(sender.LogicalDpi(), resizeRect);
  CGenericTouchInputHandler::GetInstance().SetScreenDPI(DX::DisplayMetrics::Dpi100);
}

void CWinEventsWin10::OnOrientationChanged(const DisplayInformation&, const winrt::IInspectable&)
{
  //critical_section::scoped_lock lock(m_deviceResources->GetCriticalSection());
  //m_deviceResources->SetCurrentOrientation(sender->CurrentOrientation);

  //auto size = DX::DeviceResources::Get()->GetOutputSize();
  //UpdateWindowSize(size.Width, size.Height);
}

void CWinEventsWin10::OnDisplayContentsInvalidated(const DisplayInformation&, const winrt::IInspectable&)
{
  CLog::Log(LOGDEBUG, __FUNCTION__": onevent.");
  DX::DeviceResources::Get()->ValidateDevice();
}

void CWinEventsWin10::OnBackRequested(const winrt::IInspectable&, const BackRequestedEventArgs& args)
{
  // handle this only on windows mobile
  if (CSysInfo::GetWindowsDeviceFamily() == CSysInfo::WindowsDeviceFamily::Mobile)
  {
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                               static_cast<void*>(new CAction(ACTION_NAV_BACK)));
  }
  args.Handled(true);
}

void CWinEventsWin10::OnSystemMediaButtonPressed(const SystemMediaTransportControls&, const SystemMediaTransportControlsButtonPressedEventArgs& args)
{
  int action = ACTION_NONE;
  switch (args.Button())
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
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                               static_cast<void*>(new CAction(action)));
  }
}

void CWinEventsWin10::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                               const std::string& sender,
                               const std::string& message,
                               const CVariant& data)
{
  if (flag & ANNOUNCEMENT::Player)
  {
    double speed = 1.0;
    if (data.isMember("player") && data["player"].isMember("speed"))
      speed = data["player"]["speed"].asDouble(1.0);

    bool changed = false;
    MediaPlaybackStatus status = MediaPlaybackStatus::Changing;

    if (message == "OnPlay" || message == "OnResume")
    {
      changed = true;
      status = MediaPlaybackStatus::Playing;
    }
    else if (message == "OnStop")
    {
      changed = true;
      status = MediaPlaybackStatus::Stopped;
    }
    else if (message == "OnPause")
    {
      changed = true;
      status = MediaPlaybackStatus::Paused;
    }
    else if (message == "OnSpeedChanged")
    {
      changed = true;
      status = speed != 0.0 ? MediaPlaybackStatus::Playing : MediaPlaybackStatus::Paused;
    }

    if (changed)
    {
      try
      {
        auto dispatcher = CoreApplication::MainView().Dispatcher();
        if (dispatcher)
        {
          dispatcher.RunAsync(CoreDispatcherPriority::Normal, DispatchedHandler([status, speed]
          {
            auto smtc = SystemMediaTransportControls::GetForCurrentView();
            if (!smtc)
              return;

            smtc.PlaybackStatus(status);
            smtc.PlaybackRate(speed);
          }));
        }
      }
      catch (const winrt::hresult_error&)
      {
      }
    }
  }
}
