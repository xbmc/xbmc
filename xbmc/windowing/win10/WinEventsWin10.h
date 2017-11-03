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

#ifndef WINDOW_EVENTS_WIN10_H
#define WINDOW_EVENTS_WIN10_H

#pragma once

#include "windowing/WinEvents.h"
#include <concurrent_queue.h>

class CWinEventsWin10 : public IWinEvents
{
public:
  void MessagePush(XBMC_Event *newEvent);
  bool MessagePump();
  virtual size_t GetQueueSize();

  // initialization 
  static void InitEventHandlers(Windows::UI::Core::CoreWindow^ window);
  static void InitOSKeymap(void);

  // Window event handlers.
  static void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
  static void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
  static void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);
  static void OnWindowActivationChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowActivatedEventArgs^ args);
  // touch mouse and pen
  static void OnPointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  static void OnPointerMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  static void OnPointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  static void OnPointerExited(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  static void OnPointerWheelChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  // keyboard
  static void OnAcceleratorKeyActivated(Windows::UI::Core::CoreDispatcher^ sender, Windows::UI::Core::AcceleratorKeyEventArgs^ args);
  // DisplayInformation event handlers.
  static void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
  static void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
  static void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
  // system
  static void OnBackRequested(Platform::Object^ sender, Windows::UI::Core::BackRequestedEventArgs^ args);

private:
  static void UpdateWindowSize();
  Concurrency::concurrent_queue<XBMC_Event> m_events;
};

#endif // WINDOW_EVENTS_WIN10_H
