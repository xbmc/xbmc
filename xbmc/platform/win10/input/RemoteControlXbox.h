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

#include <string>

class CRemoteControlXbox
{
public:
  CRemoteControlXbox();
  virtual ~CRemoteControlXbox();
  void Initialize();
  void Disconnect();
  bool IsRemoteDevice(const std::wstring &deviceId) const;

private:
  void HandleAcceleratorKey(Windows::UI::Core::CoreDispatcher^ sender, Windows::UI::Core::AcceleratorKeyEventArgs^ args);
  void HandleMediaButton(Windows::Media::SystemMediaTransportControlsButtonPressedEventArgs^ args);
  int32_t TranslateVirtualKey(Windows::System::VirtualKey vk);
  int32_t TranslateMediaKey(Windows::Media::SystemMediaTransportControlsButton mk);

  bool m_bInitialized;
  uint32_t m_firstClickTime;
  Windows::Foundation::EventRegistrationToken m_token;
  Windows::Foundation::EventRegistrationToken m_mediatoken;
  Windows::System::VirtualKey m_lastKey;
};
