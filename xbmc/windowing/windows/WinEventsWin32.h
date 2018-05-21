/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
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
  bool MessagePump() override;
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
  static void RegisterDeviceInterfaceToHwnd(GUID InterfaceClassGuid, HWND hWnd, HDEVNOTIFY *hDeviceNotify);
  static void WindowFromScreenCoords(HWND hWnd, POINT *point);
  static void OnGestureNotify(HWND hWnd, LPARAM lParam);
  static void OnGesture(HWND hWnd, LPARAM lParam);

  static int m_originalZoomDistance;
  static Pointer m_touchPointer;
  static CGenericTouchSwipeDetector *m_touchSwipeDetector;
};

#endif // WINDOW_EVENTS_WIN32_H
