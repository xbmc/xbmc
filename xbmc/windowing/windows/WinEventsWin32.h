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

#ifndef WINDOW_EVENTS_WIN32_H
#define WINDOW_EVENTS_WIN32_H

#pragma once

#include "windowing/WinEvents.h"
#include "input/touch/TouchTypes.h"

class CGenericTouchSwipeDetector;

class CWinEventsWin32 : public IWinEvents
{
public:
  void MessagePush(XBMC_Event *newEvent);
  bool MessagePump();
  virtual size_t GetQueueSize();
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
  static void RegisterDeviceInterfaceToHwnd(GUID InterfaceClassGuid, HWND hWnd, HDEVNOTIFY *hDeviceNotify);
  static void WindowFromScreenCoords(HWND hWnd, POINT *point);
  static void OnGestureNotify(HWND hWnd, LPARAM lParam);
  static void OnGesture(HWND hWnd, LPARAM lParam);

  static PHANDLE_EVENT_FUNC m_pEventFunc;
  static int m_originalZoomDistance;
  static Pointer m_touchPointer;
  static CGenericTouchSwipeDetector *m_touchSwipeDetector;
};

#endif // WINDOW_EVENTS_WIN32_H
