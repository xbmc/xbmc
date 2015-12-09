/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "EventLoop.h"
#include "XBMCApp.h"
#include "AndroidExtra.h"

#include <dlfcn.h>

#define IS_FROM_SOURCE(v, s) ((v & s) == s)

CEventLoop::CEventLoop(android_app* application)
  : m_enabled(false),
    m_application(application),
    m_activityHandler(NULL), m_inputHandler(NULL)
{
  if (m_application == NULL)
    return;

  m_application->userData = this;
  m_application->onAppCmd = activityCallback;
  m_application->onInputEvent = inputCallback;
}

void CEventLoop::run(IActivityHandler &activityHandler, IInputHandler &inputHandler)
{
  int ident;
  int events;
  struct android_poll_source* source;

  m_activityHandler = &activityHandler;
  m_inputHandler = &inputHandler;

  CXBMCApp::android_printf("CEventLoop: starting event loop");
  while (1)
  {
    // We will block forever waiting for events.
    while ((ident = ALooper_pollAll(-1, NULL, &events, (void**)&source)) >= 0)
    {
      // Process this event.
      if (source != NULL)
        source->process(m_application, source);

      // Check if we are exiting.
      if (m_application->destroyRequested)
      {
        CXBMCApp::android_printf("CEventLoop: we are being destroyed");
        return;
      }
    }
  }
}

void CEventLoop::processActivity(int32_t command)
{
  switch (command)
  {
    case APP_CMD_CONFIG_CHANGED:
      m_activityHandler->onConfigurationChanged();
      break;

    case APP_CMD_INIT_WINDOW:
      // The window is being shown, get it ready.
      m_activityHandler->onCreateWindow(m_application->window);

      // set the proper DPI value
      m_inputHandler->setDPI(CXBMCApp::GetDPI());
      break;

    case APP_CMD_WINDOW_RESIZED:
      // The window has been resized
      m_activityHandler->onResizeWindow();
      break;

    case APP_CMD_TERM_WINDOW:
      // The window is being hidden or closed, clean it up.
      m_activityHandler->onDestroyWindow();
      break;

    case APP_CMD_GAINED_FOCUS:
      m_activityHandler->onGainFocus();
      break;

    case APP_CMD_LOST_FOCUS:
      m_activityHandler->onLostFocus();
      break;

    case APP_CMD_LOW_MEMORY:
      m_activityHandler->onLowMemory();
      break;

    case APP_CMD_START:
      m_activityHandler->onStart();
      break;

    case APP_CMD_RESUME:
      m_activityHandler->onResume();
      break;

    case APP_CMD_SAVE_STATE:
      // The system has asked us to save our current state. Do so.
      m_activityHandler->onSaveState(&m_application->savedState, &m_application->savedStateSize);
      break;

    case APP_CMD_PAUSE:
      m_activityHandler->onPause();
      break;

    case APP_CMD_STOP:
      m_activityHandler->onStop();
      break;

    case APP_CMD_DESTROY:
      m_activityHandler->onDestroy();
      break;

    default:
      break;
  }
}

int32_t CEventLoop::processInput(AInputEvent* event)
{
  int32_t rtn    = 0;
  int32_t type   = AInputEvent_getType(event);
  int32_t source = AInputEvent_getSource(event);

  switch(type)
  {
    case AINPUT_EVENT_TYPE_KEY:
      if (IS_FROM_SOURCE(source, AINPUT_SOURCE_GAMEPAD) || IS_FROM_SOURCE(source, AINPUT_SOURCE_JOYSTICK))
      {
        if (m_inputHandler->onJoyStickKeyEvent(event))
          return true;
      }
      rtn = m_inputHandler->onKeyboardEvent(event);
      break;
    case AINPUT_EVENT_TYPE_MOTION:
      if (IS_FROM_SOURCE(source, AINPUT_SOURCE_TOUCHSCREEN))
        rtn = m_inputHandler->onTouchEvent(event);
      else if (IS_FROM_SOURCE(source, AINPUT_SOURCE_MOUSE))
        rtn = m_inputHandler->onMouseEvent(event);
      else if (IS_FROM_SOURCE(source, AINPUT_SOURCE_GAMEPAD) || IS_FROM_SOURCE(source, AINPUT_SOURCE_JOYSTICK))
        rtn = m_inputHandler->onJoyStickMotionEvent(event);
      break;
  }

  return rtn;
}

void CEventLoop::activityCallback(android_app* application, int32_t command)
{
  if (application == NULL || application->userData == NULL)
    return;

  CEventLoop& eventLoop = *((CEventLoop*)application->userData);
  eventLoop.processActivity(command);
}

int32_t CEventLoop::inputCallback(android_app* application, AInputEvent* event)
{
  if (application == NULL || application->userData == NULL || event == NULL)
    return 0;

  CEventLoop& eventLoop = *((CEventLoop*)application->userData);

  return eventLoop.processInput(event);
}

