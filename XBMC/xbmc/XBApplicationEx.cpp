/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "stdafx.h"
#include "XBApplicationEx.h"
#include "Settings.h"
#include "Application.h"
#ifdef HAS_PERFORMANCE_SAMPLE
#include "utils/PerformanceSample.h"
#else
#define MEASURE_FUNCTION
#endif
#include "GUIFontManager.h"


//-----------------------------------------------------------------------------
// Name: CXBApplication()
// Desc: Constructor
//-----------------------------------------------------------------------------
CXBApplicationEx::CXBApplicationEx()
{
  // Variables to perform app timing
  m_bStop = false;
  m_AppActive = true;
  m_AppFocused = true;
}

/* CXBApplicationEx() destructor */
CXBApplicationEx::~CXBApplicationEx()
{}

/* Create the app */
HRESULT CXBApplicationEx::Create(HWND hWnd)
{
  HRESULT hr;

  // Initialize the app's device-dependent objects
  if ( FAILED( hr = Initialize() ) )
  {
    CLog::Log(LOGERROR, "XBAppEx: Call to Initialize() failed!" );
    return hr;
  }

  return S_OK;
}

/* Destroy the app */
VOID CXBApplicationEx::Destroy()
{
  CLog::Log(LOGNOTICE, "destroy");
  // Perform app-specific cleanup
  Cleanup();
}

/* Function that runs the application */
INT CXBApplicationEx::Run()
{
  CLog::Log(LOGNOTICE, "Running the application..." );

  BYTE processExceptionCount = 0;
  BYTE frameMoveExceptionCount = 0;
  BYTE renderExceptionCount = 0;

#ifndef _DEBUG
  const BYTE MAX_EXCEPTION_COUNT = 10;
#endif

  // Run xbmc
  while (!m_bStop)
  {
#ifdef HAS_PERFORMANCE_SAMPLE
    CPerformanceSample sampleLoop("XBApplicationEx-loop");
#endif
    //-----------------------------------------
    // Animate and render a frame
    //-----------------------------------------
#ifndef _DEBUG
    try
    {
#endif
      Process();
      //reset exception count
      processExceptionCount = 0;

#ifndef _DEBUG

    }
    catch (...)
    {
      CLog::Log(LOGERROR, "exception in CApplication::Process()");
      processExceptionCount++;
      //MAX_EXCEPTION_COUNT exceptions in a row? -> bail out
      if (processExceptionCount > MAX_EXCEPTION_COUNT)
      {
        CLog::Log(LOGERROR, "CApplication::Process(), too many exceptions");
        throw;
      }
    }
#endif
    // Frame move the scene
#ifndef _DEBUG
    try
    {
#endif
      if (!m_bStop) FrameMove();
      //reset exception count
      frameMoveExceptionCount = 0;

#ifndef _DEBUG

    }
    catch (...)
    {
      CLog::Log(LOGERROR, "exception in CApplication::FrameMove()");
      frameMoveExceptionCount++;
      //MAX_EXCEPTION_COUNT exceptions in a row? -> bail out
      if (frameMoveExceptionCount > MAX_EXCEPTION_COUNT)
      {
        CLog::Log(LOGERROR, "CApplication::FrameMove(), too many exceptions");
        throw;
      }
    }
#endif

    // Render the scene
#ifndef _DEBUG
    try
    {
#endif
      if (!m_bStop) Render();
      //reset exception count
      renderExceptionCount = 0;

#ifndef _DEBUG

    }
    catch (...)
    {
      CLog::Log(LOGERROR, "exception in CApplication::Render()");
      renderExceptionCount++;
      //MAX_EXCEPTION_COUNT exceptions in a row? -> bail out
      if (renderExceptionCount > MAX_EXCEPTION_COUNT)
      {
        CLog::Log(LOGERROR, "CApplication::Render(), too many exceptions");
        throw;
      }
    }
#endif

  } // while (!m_bStop)
  Destroy();

  CLog::Log(LOGNOTICE, "application stopped..." );
  return 0;
}

/* Function for handling input */

/* elis
void CXBApplicationEx::ReadInput(XBMC_Event newEvent)
{
  MEASURE_FUNCTION;

#ifdef HAS_SDL
#ifndef __APPLE__
  static RESOLUTION windowres = WINDOW;
#endif

  // Read the input from the mouse
  g_Mouse.Update();

  XBMC_Event event;
  bool bProcessNextEvent = true;
  while (bProcessNextEvent && XBMC_PollEvent(&event))
  {
    switch(event.type)
    {
    case SDL_QUIT:
      if (!g_application.m_bStop) g_application.getApplicationMessenger().Quit();
      break;

    case SDL_VIDEORESIZE:
#if !defined(__APPLE__) && !defined(_WIN32PC)
      g_settings.m_ResInfo[WINDOW].iWidth = event.resize.w;
      g_settings.m_ResInfo[WINDOW].iHeight = event.resize.h;
      g_graphicsContext.ResetOverscan(g_settings.m_ResInfo[WINDOW]);
      g_graphicsContext.SetVideoResolution(windowres, FALSE, false);
      g_Mouse.SetResolution(g_settings.m_ResInfo[WINDOW].iWidth, g_settings.m_ResInfo[WINDOW].iHeight, 1, 1);
      g_fontManager.ReloadTTFFonts();
#endif
      break;

#ifdef HAS_SDL_JOYSTICK
    case SDL_JOYBUTTONUP:
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYAXISMOTION:
    case SDL_JOYBALLMOTION:
    case SDL_JOYHATMOTION:
      g_Joystick.Update(event);
      break;
#endif
    case SDL_KEYDOWN:
      // process any platform specific shortcuts before handing off to XBMC
      if (!ProcessOSShortcuts(event))
      {
        g_Keyboard.Update(event);
        // don't handle any more messages in the queue until we've handled keydown,
        // if a keyup is in the queue it will reset the keypress before it is handled.
        bProcessNextEvent = false;
      }
      break;
    case SDL_KEYUP:
      g_Keyboard.Update(event);
      break;
    case SDL_ACTIVEEVENT:
      //If the window was inconified or restored
      if( event.active.state & SDL_APPACTIVE )
      {
        m_AppActive = event.active.gain != 0;
        if (m_AppActive) g_application.Minimize(false);
      }
      if (event.active.state & SDL_APPINPUTFOCUS)
      {
        m_AppFocused = event.active.gain != 0;
        g_graphicsContext.NotifyAppFocusChange(m_AppFocused);
      }
      break;
    case SDL_MOUSEBUTTONDOWN:
      // mouse scroll wheel.
      if (event.button.button == 4)
        g_Mouse.UpdateMouseWheel(1);
      else if (event.button.button == 5)
        g_Mouse.UpdateMouseWheel(-1);
      break;
    }
  }

  // Read the input from the mouse
  g_Mouse.Update();
#endif // HAS_SDL

#ifdef HAS_LIRC
  // Read the input from a remote
  g_RemoteControl.Update();
#endif
}

*/

/*
#ifdef HAS_SDL
bool CXBApplicationEx::ProcessOSShortcuts(SDL_Event& event)
{
#ifdef __APPLE__
  return ProcessOSXShortcuts(event);
#elif defined(_LINUX)
  return ProcessLinuxShortcuts(event);
#else
  return ProcessWin32Shortcuts(event);
#endif
}

bool CXBApplicationEx::ProcessWin32Shortcuts(SDL_Event& event)
{
  static bool alt = false;
  static CAction action;

  alt = !!(SDL_GetModState() & (XBMCXBMCKMOD_LALT | XBMCXBMCKMOD_RALT));

  if (event.key.type == SDL_KEYDOWN)
  {
    if(alt)
    {
      switch(event.key.keysym.sym)
      {
      case SDLK_F4:  // alt-F4 to quit
        if (!g_application.m_bStop)
          g_application.getApplicationMessenger().Quit();
      case SDLK_RETURN:  // alt-Return to toggle fullscreen
        {
          action.wID = ACTION_TOGGLE_FULLSCREEN;
          g_application.OnAction(action);
          return true;
        }
        return false;
      default:
        return false;
      }
    }
  }
  return false;
}

bool CXBApplicationEx::ProcessLinuxShortcuts(SDL_Event& event)
{
  bool alt = false;

  alt = !!(SDL_GetModState() & (XBMCXBMCKMOD_LALT  | XBMCXBMCKMOD_RALT));

  if (alt && event.key.type == SDL_KEYDOWN)
  {
    switch(event.key.keysym.sym)
    {
      case SDLK_TAB:  // ALT+TAB to minimize/hide
        g_application.Minimize();
        return true;
      default:
        break;
    }
  }
  return false;
 
}

bool CXBApplicationEx::ProcessOSXShortcuts(SDL_Event& event)
{
  static bool shift = false, cmd = false;
  static CAction action;

  cmd   = !!(SDL_GetModState() & (XBMCXBMCKMOD_LMETA  | XBMCXBMCKMOD_RMETA ));
  shift = !!(SDL_GetModState() & (XBMCXBMCKMOD_LSHIFT | XBMCXBMCKMOD_RSHIFT));

  if (cmd && event.key.type == SDL_KEYDOWN)
  {
    switch(event.key.keysym.sym)
    {
    case SDLK_q:  // CMD-q to quit
    case SDLK_w:  // CMD-w to quit
      if (!g_application.m_bStop)
        g_application.getApplicationMessenger().Quit();
      return true;

    case SDLK_f: // CMD-f to toggle fullscreen
      action.wID = ACTION_TOGGLE_FULLSCREEN;
      g_application.OnAction(action);
      return true;

    case SDLK_s: // CMD-3 to take a screenshot
      action.wID = ACTION_TAKE_SCREENSHOT;
      g_application.OnAction(action);
      return true;

    case SDLK_h: // CMD-h to hide (but we minimize for now)
    case SDLK_m: // CMD-m to minimize
      g_application.getApplicationMessenger().Minimize();
      return true;

    default:
      return false;
    }
  }
  return false;
  
}
#endif // HAS_SDL
*/
