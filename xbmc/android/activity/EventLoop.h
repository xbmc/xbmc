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
 
#include <android_native_app_glue.h>

#include "IActivityHandler.h"
#include "IInputHandler.h"
 
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

  bool m_enabled;
  android_app* m_application;
  IActivityHandler* m_activityHandler;
  IInputHandler* m_inputHandler;
};

