#pragma once
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

#include "input/remote/IRemoteControl.h"
#include <string>

class CRemoteControlXbox : public KODI::REMOTE::IRemoteControl
{
public:
  CRemoteControlXbox();
  virtual ~CRemoteControlXbox();
  void Initialize() override;
  void Disconnect() override;
  void Reset() override;
  void Update() override;
  uint16_t GetButton() const override;
  uint32_t GetHoldTimeMs() const  override;
  bool IsInitialized() const override { return m_bInitialized; }
  std::string GetMapFile() override;

  void SetEnabled(bool) override { }
  void SetDeviceName(const std::string&) override { }
  void AddSendCommand(const std::string&) override { }
  bool IsInUse() const override { return false; }

  static KODI::REMOTE::IRemoteControl* CreateInstance();
  static void Register();

  static bool IsRemoteControlId(const std::wstring &deviceId);

private:
  void HandleAcceleratorKey(Windows::UI::Core::CoreDispatcher^ sender, Windows::UI::Core::AcceleratorKeyEventArgs^ args);
  void HandleMediaButton(Windows::Media::SystemMediaTransportControlsButtonPressedEventArgs^ args);
  int32_t TranslateVirtualKey(Windows::System::VirtualKey vk);
  int32_t TranslateMediaKey(Windows::Media::SystemMediaTransportControlsButton mk);

  int32_t  m_button;
  int32_t  m_lastButton;
  uint32_t m_holdTime;
  uint32_t m_firstClickTime;
  bool m_bInitialized;
  std::string m_deviceName;
  Windows::Foundation::EventRegistrationToken m_token;
  Windows::Foundation::EventRegistrationToken m_mediatoken;
  Windows::System::VirtualKey m_lastKey;
  Windows::Media::SystemMediaTransportControlsButton m_lastMediaButton;
};
