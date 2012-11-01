#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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

#include <stdint.h>

struct ANativeWindow;
typedef struct ANativeWindow ANativeWindow;

typedef enum
{
  ActivityOK    = 0,
  ActivityExit  = -1,
  ActivityError = -2,
  ActivityUnknown = 1
} ActivityResult;

class IActivityHandler
{
public:
  virtual ActivityResult onActivate() = 0;
  virtual void onDeactivate() = 0;

  virtual void onStart() {}
  virtual void onResume() {}
  virtual void onPause() {}
  virtual void onStop() {}
  virtual void onDestroy() {}

  virtual void onSaveState(void **data, size_t *size) {}
  virtual void onConfigurationChanged() {}
  virtual void onLowMemory() {}

  virtual void onCreateWindow(ANativeWindow* window) {}
  virtual void onResizeWindow() {}
  virtual void onDestroyWindow() {}
  virtual void onGainFocus() {}
  virtual void onLostFocus() {}
};

