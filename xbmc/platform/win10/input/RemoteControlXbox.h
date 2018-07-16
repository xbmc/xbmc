/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <winrt/Windows.Media.h>

class CRemoteControlXbox
{
public:
  CRemoteControlXbox();
  virtual ~CRemoteControlXbox();
  void Initialize();
  void Disconnect();
  bool IsRemoteDevice(const std::wstring &deviceId) const;

private:
  void HandleAcceleratorKey(const winrt::Windows::UI::Core::CoreDispatcher&, const winrt::Windows::UI::Core::AcceleratorKeyEventArgs&);
  void HandleMediaButton(const winrt::Windows::Media::SystemMediaTransportControlsButtonPressedEventArgs&);
  int32_t TranslateVirtualKey(winrt::Windows::System::VirtualKey vk);
  int32_t TranslateMediaKey(winrt::Windows::Media::SystemMediaTransportControlsButton mk);

  bool m_bInitialized;
  uint32_t m_firstClickTime;
  uint32_t m_repeatCount;
  winrt::event_token m_token;
  winrt::event_token m_mediatoken;
};
