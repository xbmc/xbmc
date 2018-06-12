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
