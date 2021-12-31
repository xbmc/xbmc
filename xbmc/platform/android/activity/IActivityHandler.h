/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
  virtual ~IActivityHandler() = default;

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

