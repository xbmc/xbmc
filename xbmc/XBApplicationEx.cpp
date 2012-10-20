/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "system.h"
#include "XBApplicationEx.h"
#include "utils/log.h"
#include "threads/SystemClock.h"
#ifdef HAS_PERFORMANCE_SAMPLE
#include "utils/PerformanceSample.h"
#else
#define MEASURE_FUNCTION
#endif
#include "commons/Exception.h"

// Put this here for easy enable and disable
#ifndef _DEBUG
#define XBMC_TRACK_EXCEPTIONS
#endif

CXBApplicationEx::CXBApplicationEx()
{
  // Variables to perform app timing
  m_bStop = false;
  m_AppActive = true;
  m_AppFocused = true;
  m_ExitCode = EXITCODE_QUIT;
}

CXBApplicationEx::~CXBApplicationEx()
{
}

/* Create the app */
bool CXBApplicationEx::Create()
{
  // Variables to perform app timing
  m_bStop = false;
  m_AppActive = true;
  m_AppFocused = true;
  m_ExitCode = EXITCODE_QUIT;

  // Initialize the app's device-dependent objects
  if (!Initialize())
  {
    CLog::Log(LOGERROR, "XBAppEx: Call to Initialize() failed!" );
    return false;
  }

  return true;
}

/* Destroy the app */
VOID CXBApplicationEx::Destroy()
{
  CLog::Log(LOGNOTICE, "destroy");
  // Perform app-specific cleanup
  Cleanup();
}

/* Function that runs the application */
INT CXBApplicationEx::Run(bool renderGUI)
{
  CLog::Log(LOGNOTICE, "Running the application..." );

  unsigned int lastFrameTime = 0;
  unsigned int frameTime = 0;
  const unsigned int noRenderFrameTime = 15;  // Simulates ~66fps
  m_renderGUI = renderGUI;

#ifdef XBMC_TRACK_EXCEPTIONS
  BYTE processExceptionCount = 0;
  BYTE frameMoveExceptionCount = 0;
  BYTE renderExceptionCount = 0;
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
#ifdef XBMC_TRACK_EXCEPTIONS
    try
    {
#endif
      lastFrameTime = XbmcThreads::SystemClockMillis();
      Process();
      //reset exception count
#ifdef XBMC_TRACK_EXCEPTIONS
      processExceptionCount = 0;

    }
    catch (const XbmcCommons::UncheckedException &e)
    {
      e.LogThrowMessage("CApplication::Process()");
      processExceptionCount++;
      //MAX_EXCEPTION_COUNT exceptions in a row? -> bail out
      if (processExceptionCount > MAX_EXCEPTION_COUNT)
      {
        CLog::Log(LOGERROR, "CApplication::Process(), too many exceptions");
        throw;
      }
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
#ifdef XBMC_TRACK_EXCEPTIONS
    try
    {
#endif
      if (!m_bStop) FrameMove(true, m_renderGUI);
      //reset exception count
#ifdef XBMC_TRACK_EXCEPTIONS
      frameMoveExceptionCount = 0;

    }
    catch (const XbmcCommons::UncheckedException &e)
    {
      e.LogThrowMessage("CApplication::FrameMove()");
      frameMoveExceptionCount++;
      //MAX_EXCEPTION_COUNT exceptions in a row? -> bail out
      if (frameMoveExceptionCount > MAX_EXCEPTION_COUNT)
      {
        CLog::Log(LOGERROR, "CApplication::FrameMove(), too many exceptions");
        throw;
      }
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
#ifdef XBMC_TRACK_EXCEPTIONS
    try
    {
#endif
      if (m_renderGUI && !m_bStop) Render();
      else if (!m_renderGUI)
      {
        frameTime = XbmcThreads::SystemClockMillis() - lastFrameTime;
        if(frameTime < noRenderFrameTime)
          Sleep(noRenderFrameTime - frameTime);
      }
#ifdef XBMC_TRACK_EXCEPTIONS
      //reset exception count
      renderExceptionCount = 0;

    }
    catch (const XbmcCommons::UncheckedException &e)
    {
      e.LogThrowMessage("CApplication::Render()");
      renderExceptionCount++;
      //MAX_EXCEPTION_COUNT exceptions in a row? -> bail out
      if (renderExceptionCount > MAX_EXCEPTION_COUNT)
      {
        CLog::Log(LOGERROR, "CApplication::Render(), too many exceptions");
        throw;
      }
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
  return m_ExitCode;
}
