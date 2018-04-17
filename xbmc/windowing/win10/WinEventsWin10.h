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

#include "interfaces/IAnnouncer.h"
#include "windowing/WinEvents.h"
#include <concurrent_queue.h>
#include <cmath>

class CRemoteControlXbox;

class CWinEventsWin10 : public IWinEvents
                      , public ANNOUNCEMENT::IAnnouncer
{
public:
  CWinEventsWin10();
  virtual ~CWinEventsWin10();

  void MessagePush(XBMC_Event *newEvent);
  bool MessagePump() override;
  virtual size_t GetQueueSize();

  // initialization 
  void InitEventHandlers(Windows::UI::Core::CoreWindow^ window);
  static void InitOSKeymap(void);

  // Window event handlers.
  void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
  void OnWindowResizeStarted(Windows::UI::Core::CoreWindow^ sender, Platform::Object^ args);
  void OnWindowResizeCompleted(Windows::UI::Core::CoreWindow^ sender, Platform::Object^ args);
  void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);
  static void OnWindowActivationChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowActivatedEventArgs^ args);
  static void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
  // touch mouse and pen
  void OnPointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  void OnPointerMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  void OnPointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  void OnPointerExited(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  void OnPointerWheelChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  // keyboard
  void OnAcceleratorKeyActivated(Windows::UI::Core::CoreDispatcher^ sender, Windows::UI::Core::AcceleratorKeyEventArgs^ args);

  // DisplayInformation event handlers.
  static void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
  static void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
  static void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
  // system
  static void OnBackRequested(Platform::Object^ sender, Windows::UI::Core::BackRequestedEventArgs^ args);
  // system media handlers
  static void OnSystemMediaButtonPressed(Windows::Media::SystemMediaTransportControls^, Windows::Media::SystemMediaTransportControlsButtonPressedEventArgs^);
  // IAnnouncer overrides
  void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data) override;

private:
  friend class CWinSystemWin10;

  void UpdateWindowSize();
  void Kodi_KeyEvent(unsigned int vkey, unsigned scancode, unsigned keycode, bool isDown);
  void HandleWindowSizeChanged();

  Concurrency::concurrent_queue<XBMC_Event> m_events;
  Windows::Media::SystemMediaTransportControls^ m_smtc{ nullptr };
  bool m_bResized{ false };
  bool m_bMoved{ false };
  bool m_sizeChanging{ false };
  float m_logicalWidth{ 0 };
  float m_logicalHeight{ 0 };
  float m_logicalPosX{ 0 };
  float m_logicalPosY{ 0 };
  std::unique_ptr<CRemoteControlXbox> m_remote;
};
