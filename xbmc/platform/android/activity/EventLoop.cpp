/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EventLoop.h"

#include "XBMCApp.h"

#define IS_FROM_SOURCE(v, s) ((v & s) == s)

CEventLoop::CEventLoop(android_app* application)
  : m_application(application), m_activityHandler(NULL), m_inputHandler(NULL)
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
  while (true)
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
      m_inputHandler->setDPI(CXBMCApp::Get().GetDPI());
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

  // handle joystick input
  if (IS_FROM_SOURCE(source, AINPUT_SOURCE_GAMEPAD) || IS_FROM_SOURCE(source, AINPUT_SOURCE_JOYSTICK))
  {
    if (m_inputHandler->onJoyStickEvent(event))
      return true;
  }

  switch(type)
  {
    case AINPUT_EVENT_TYPE_KEY:
      rtn = m_inputHandler->onKeyboardEvent(event);
      break;
    case AINPUT_EVENT_TYPE_MOTION:
      if (IS_FROM_SOURCE(source, AINPUT_SOURCE_TOUCHSCREEN))
        rtn = m_inputHandler->onTouchEvent(event);
      else if (IS_FROM_SOURCE(source, AINPUT_SOURCE_MOUSE))
        rtn = m_inputHandler->onMouseEvent(event);
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

