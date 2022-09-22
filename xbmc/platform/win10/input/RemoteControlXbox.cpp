/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RemoteControlXbox.h"

#include "ServiceBroker.h"
#include "application/AppInboundProtocol.h"
#include "input/remote/IRRemote.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

#define XBOX_REMOTE_DEVICE_ID L"GIP:0000F50000000001"
#define XBOX_REMOTE_DEVICE_NAME "Xbox One Game Controller"

namespace winrt
{
  using namespace Windows::Foundation;
}
using namespace winrt::Windows::Media;
using namespace winrt::Windows::System;
using namespace winrt::Windows::UI::Core;

CRemoteControlXbox::CRemoteControlXbox()
  : m_bInitialized(false)
  , m_repeatCount(0)
{
}

CRemoteControlXbox::~CRemoteControlXbox()
{
  if (m_bInitialized)
    Disconnect();
}

bool CRemoteControlXbox::IsRemoteDevice(const std::wstring &deviceId) const
{
  return deviceId.compare(XBOX_REMOTE_DEVICE_ID) == 0;
}

void CRemoteControlXbox::Disconnect()
{
  m_bInitialized = false;
  auto coreWindow = CoreWindow::GetForCurrentThread();
  if (!coreWindow) // window is destroyed already
    return;

  // unregister our key handler
  coreWindow.Dispatcher().AcceleratorKeyActivated(m_token);

  auto smtc = SystemMediaTransportControls::GetForCurrentView();
  if (smtc)
  {
    smtc.ButtonPressed(m_mediatoken);
  }
}

void CRemoteControlXbox::Initialize()
{
  auto dispatcher = CoreWindow::GetForCurrentThread().Dispatcher();
  m_token = dispatcher.AcceleratorKeyActivated([this](const CoreDispatcher& sender, const AcceleratorKeyEventArgs& args)
  {
    if (IsRemoteDevice(args.DeviceId().c_str()))
      HandleAcceleratorKey(sender, args);
  });

  auto smtc = SystemMediaTransportControls::GetForCurrentView();
  if (smtc)
  {
    m_mediatoken = smtc.ButtonPressed([this](auto&&, const SystemMediaTransportControlsButtonPressedEventArgs& args)
    {
      HandleMediaButton(args);
    });
    smtc.IsEnabled(true);
  }
  m_bInitialized = true;
}

void CRemoteControlXbox::HandleAcceleratorKey(const CoreDispatcher& sender, const AcceleratorKeyEventArgs& args)
{
  auto button = TranslateVirtualKey(args.VirtualKey());
  if (!button)
    return;

  XBMC_Event newEvent = {};
  newEvent.type = XBMC_BUTTON;
  newEvent.keybutton.button = button;
  newEvent.keybutton.holdtime = 0;

  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();

  switch (args.EventType())
  {
  case CoreAcceleratorKeyEventType::KeyDown:
  case CoreAcceleratorKeyEventType::SystemKeyDown:
  {
    if (!args.KeyStatus().WasKeyDown) // first occurrence
    {
      m_firstClickTime = std::chrono::steady_clock::now();
      if (appPort)
        appPort->OnEvent(newEvent);
    }
    else
    {
      m_repeatCount++;
      if (m_repeatCount > CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_remoteDelay)
      {
        auto now = std::chrono::steady_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - m_firstClickTime);

        newEvent.keybutton.holdtime = duration.count();
        if (appPort)
          appPort->OnEvent(newEvent);
      }
    }
    break;
  }
  case CoreAcceleratorKeyEventType::KeyUp:
  case CoreAcceleratorKeyEventType::SystemKeyUp:
  {
    m_repeatCount = 0;
    break;
  }
  case CoreAcceleratorKeyEventType::Character:
  case CoreAcceleratorKeyEventType::SystemCharacter:
  case CoreAcceleratorKeyEventType::UnicodeCharacter:
  case CoreAcceleratorKeyEventType::DeadCharacter:
  case CoreAcceleratorKeyEventType::SystemDeadCharacter:
  default:
    break;
  }
  args.Handled(true);
}

void CRemoteControlXbox::HandleMediaButton(const SystemMediaTransportControlsButtonPressedEventArgs& args)
{
  XBMC_Event newEvent = {};
  newEvent.type = XBMC_BUTTON;
  newEvent.keybutton.button = TranslateMediaKey(args.Button());
  newEvent.keybutton.holdtime = 0;
  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->OnEvent(newEvent);
}

int32_t CRemoteControlXbox::TranslateVirtualKey(VirtualKey vk)
{
  switch (vk)
  {
  case VirtualKey::GamepadDPadLeft:
    return XINPUT_IR_REMOTE_LEFT;
  case VirtualKey::GamepadDPadUp:
    return XINPUT_IR_REMOTE_UP;
  case VirtualKey::GamepadDPadRight:
    return XINPUT_IR_REMOTE_RIGHT;
  case VirtualKey::GamepadDPadDown:
    return XINPUT_IR_REMOTE_DOWN;
  case VirtualKey::GamepadA:
    return XINPUT_IR_REMOTE_SELECT;
  case VirtualKey::GamepadB:
    return XINPUT_IR_REMOTE_BACK;
  case VirtualKey::GamepadX:
    return XINPUT_IR_REMOTE_CONTENTS_MENU;
  case VirtualKey::GamepadY:
    return XINPUT_IR_REMOTE_INFO;
  case VirtualKey::Clear:
    return XINPUT_IR_REMOTE_CLEAR;
  case VirtualKey::PageDown:
    return XINPUT_IR_REMOTE_CHANNEL_MINUS;
  case VirtualKey::PageUp:
    return XINPUT_IR_REMOTE_CHANNEL_PLUS;
  case VirtualKey::Number0:
    return XINPUT_IR_REMOTE_0;
  case VirtualKey::Number1:
    return XINPUT_IR_REMOTE_1;
  case VirtualKey::Number2:
    return XINPUT_IR_REMOTE_2;
  case VirtualKey::Number3:
    return XINPUT_IR_REMOTE_3;
  case VirtualKey::Number4:
    return XINPUT_IR_REMOTE_4;
  case VirtualKey::Number5:
    return XINPUT_IR_REMOTE_5;
  case VirtualKey::Number6:
    return XINPUT_IR_REMOTE_6;
  case VirtualKey::Number7:
    return XINPUT_IR_REMOTE_7;
  case VirtualKey::Number8:
    return XINPUT_IR_REMOTE_8;
  case VirtualKey::Number9:
    return XINPUT_IR_REMOTE_9;
  case VirtualKey::Decimal:
    return XINPUT_IR_REMOTE_STAR;
  case VirtualKey::GamepadView:
    return XINPUT_IR_REMOTE_DISPLAY;
  case VirtualKey::GamepadMenu:
    return XINPUT_IR_REMOTE_MENU;
  default:
    CLog::LogF(LOGDEBUG, "unknown vrtual key {}", static_cast<int>(vk));
    return 0;
  }
}

int32_t CRemoteControlXbox::TranslateMediaKey(SystemMediaTransportControlsButton mk)
{
  switch (mk)
  {
  case SystemMediaTransportControlsButton::ChannelDown:
    return XINPUT_IR_REMOTE_CHANNEL_MINUS;
    break;
  case SystemMediaTransportControlsButton::ChannelUp:
    return XINPUT_IR_REMOTE_CHANNEL_PLUS;
  case SystemMediaTransportControlsButton::FastForward:
    return XINPUT_IR_REMOTE_FORWARD;
  case SystemMediaTransportControlsButton::Rewind:
    return XINPUT_IR_REMOTE_REVERSE;
  case SystemMediaTransportControlsButton::Next:
    return XINPUT_IR_REMOTE_SKIP_PLUS;
  case SystemMediaTransportControlsButton::Previous:
    return XINPUT_IR_REMOTE_SKIP_MINUS;
  case SystemMediaTransportControlsButton::Pause:
    return XINPUT_IR_REMOTE_PAUSE;
  case SystemMediaTransportControlsButton::Play:
    return XINPUT_IR_REMOTE_PLAY;
  case SystemMediaTransportControlsButton::Stop:
    return XINPUT_IR_REMOTE_STOP;
  case SystemMediaTransportControlsButton::Record:
    return XINPUT_IR_REMOTE_RECORD;
  default:
    return 0;
  }
}
