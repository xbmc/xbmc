/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IActivityHandler.h"
#include "IInputHandler.h"

#include <android_native_app_glue.h>

class CEventLoop
{
public:
  CEventLoop(android_app* application);

  void run(IActivityHandler &activityHandler, IInputHandler &inputHandler);

protected:
  void activate();
  void deactivate();

  void processActivity(int32_t command);
  int32_t processInput(AInputEvent* event);

private:
  static void activityCallback(android_app* application, int32_t command);
  static int32_t inputCallback(android_app* application, AInputEvent* event);

  bool m_enabled = false;
  android_app* m_application;
  IActivityHandler* m_activityHandler;
  IInputHandler* m_inputHandler;
};

