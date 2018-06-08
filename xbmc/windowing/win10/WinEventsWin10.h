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
#include <winrt/Windows.Media.h>

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
  void InitEventHandlers(const winrt::Windows::UI::Core::CoreWindow&);
  static void InitOSKeymap(void);

  // Window event handlers.
  void OnWindowSizeChanged(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::WindowSizeChangedEventArgs&);
  void OnWindowResizeStarted(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::Foundation::IInspectable&);
  void OnWindowResizeCompleted(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::Foundation::IInspectable&);
  void OnWindowClosed(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::CoreWindowEventArgs&);
  static void OnWindowActivationChanged(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::WindowActivatedEventArgs&);
  static void OnVisibilityChanged(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::VisibilityChangedEventArgs&);
  // touch mouse and pen
  void OnPointerPressed(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::PointerEventArgs&);
  void OnPointerMoved(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::PointerEventArgs&);
  void OnPointerReleased(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::PointerEventArgs&);
  void OnPointerExited(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::PointerEventArgs&);
  void OnPointerWheelChanged(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::PointerEventArgs&);
  // keyboard
  void OnAcceleratorKeyActivated(const winrt::Windows::UI::Core::CoreDispatcher&, const winrt::Windows::UI::Core::AcceleratorKeyEventArgs&);

  // DisplayInformation event handlers.
  static void OnDpiChanged(const winrt::Windows::Graphics::Display::DisplayInformation&, const winrt::Windows::Foundation::IInspectable&);
  static void OnOrientationChanged(const winrt::Windows::Graphics::Display::DisplayInformation&, const winrt::Windows::Foundation::IInspectable&);
  static void OnDisplayContentsInvalidated(const winrt::Windows::Graphics::Display::DisplayInformation&, const winrt::Windows::Foundation::IInspectable&);
  // system
  static void OnBackRequested(const winrt::Windows::Foundation::IInspectable&, const winrt::Windows::UI::Core::BackRequestedEventArgs&);
  // system media handlers
  static void OnSystemMediaButtonPressed(const winrt::Windows::Media::SystemMediaTransportControls&
                                       , const winrt::Windows::Media::SystemMediaTransportControlsButtonPressedEventArgs&);
  // IAnnouncer overrides
  void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data) override;

private:
  friend class CWinSystemWin10;

  void OnResize(float width, float height);
  void UpdateWindowSize();
  void Kodi_KeyEvent(unsigned int vkey, unsigned scancode, unsigned keycode, bool isDown);
  void HandleWindowSizeChanged();

  Concurrency::concurrent_queue<XBMC_Event> m_events;
  winrt::Windows::Media::SystemMediaTransportControls m_smtc{ nullptr };
  bool m_bResized{ false };
  bool m_bMoved{ false };
  bool m_sizeChanging{ false };
  float m_logicalWidth{ 0 };
  float m_logicalHeight{ 0 };
  float m_logicalPosX{ 0 };
  float m_logicalPosY{ 0 };
  std::unique_ptr<CRemoteControlXbox> m_remote;
};
