//-----------------------------------------------------------------------------
// File: XBApplicationEx.cpp
//
// Desc: Application class for the XBox samples.
//
// Hist: 11.01.00 - New for November XDK release
//       12.15.00 - Changes for December XDK release
//       12.19.01 - Changes for March XDK release
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "XBApplicationEx.h"
#include "XBVideoConfig.h"
#include "Settings.h"
#include "Application.h"
#ifdef HAS_PERFORMANCE_SAMPLE
#include "utils/PerformanceSample.h"
#else
#define MEASURE_FUNCTION
#endif
#include "GUIFontManager.h"
#ifdef _WIN32PC
#include <dbt.h>
#include "SDL/SDL_syswm.h"
#include "GUIWindowManager.h"
#include "WIN32Util.h"
#endif


//-----------------------------------------------------------------------------
// Global access to common members
//-----------------------------------------------------------------------------
CXBApplicationEx* g_pXBApp = NULL;
#ifndef HAS_SDL
static LPDIRECT3DDEVICE8 g_pd3dDevice = NULL;
#endif

// Deadzone for the gamepad inputs
const SHORT XINPUT_DEADZONE = (SHORT)( 0.24f * FLOAT(0x7FFF) );




//-----------------------------------------------------------------------------
// Name: CXBApplication()
// Desc: Constructor
//-----------------------------------------------------------------------------
CXBApplicationEx::CXBApplicationEx()
{
  // Initialize member variables
  g_pXBApp = this;

#ifndef HAS_SDL
  // Direct3D variables
  m_pD3D = NULL;
  m_pd3dDevice = NULL;
  //    m_pDepthBuffer    = NULL;
  m_pBackBuffer = NULL;
#endif

  // Variables to perform app timing
  m_bPaused = FALSE;
  m_fTime = 0.0f;
  m_fElapsedTime = 0.0f;
  m_fAppTime = 0.0f;
  m_fElapsedAppTime = 0.0f;
  m_strFrameRate[0] = L'\0';
  m_bStop = false;
  m_AppActive = true;

#ifndef HAS_SDL
  // Set up the presentation parameters for a double-buffered, 640x480,
  // 32-bit display using depth-stencil. Override these parameters in
  // your derived class as your app requires.
  ZeroMemory( &m_d3dpp, sizeof(m_d3dpp) );
  m_d3dpp.BackBufferWidth = 720;
  m_d3dpp.BackBufferHeight = 576;
  m_d3dpp.BackBufferFormat = D3DFMT_LIN_A8R8G8B8;
  m_d3dpp.BackBufferCount = 1;
  m_d3dpp.EnableAutoDepthStencil = FALSE;
  m_d3dpp.AutoDepthStencilFormat = D3DFMT_LIN_D16;
  m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  m_d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
#endif
}




//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Create the app
//-----------------------------------------------------------------------------
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




//-----------------------------------------------------------------------------
// Name: Destroy()
// Desc: Cleanup objects
//-----------------------------------------------------------------------------
VOID CXBApplicationEx::Destroy()
{
  CLog::Log(LOGNOTICE, "destroy");
  // Perform app-specific cleanup
  Cleanup();

#ifndef HAS_SDL
  // Release display objects
  SAFE_RELEASE( m_pd3dDevice );
  SAFE_RELEASE( m_pD3D );
#endif
}




//-----------------------------------------------------------------------------
// Name: Run()
// Desc:
//-----------------------------------------------------------------------------
INT CXBApplicationEx::Run()
{
  CLog::Log(LOGNOTICE, "Running the application..." );

  // Get the frequency of the timer
  LARGE_INTEGER qwTicksPerSec;
  QueryPerformanceFrequency( &qwTicksPerSec );
  FLOAT fSecsPerTick = 1.0f / (FLOAT)qwTicksPerSec.QuadPart;

  // Save the start time
  LARGE_INTEGER qwTime, qwLastTime, qwElapsedTime;
  QueryPerformanceCounter( &qwTime );
  qwLastTime.QuadPart = qwTime.QuadPart;

  LARGE_INTEGER qwAppTime, qwElapsedAppTime;
  qwAppTime.QuadPart = 0;
  qwElapsedTime.QuadPart = 0;
  qwElapsedAppTime.QuadPart = 0;

  BYTE processExceptionCount = 0;
  BYTE frameMoveExceptionCount = 0;
  BYTE renderExceptionCount = 0;

#ifndef _DEBUG
  const BYTE MAX_EXCEPTION_COUNT = 10;
#endif

#ifdef _WIN32PC
  SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
#endif

  // Run the game loop, animating and rendering frames
  while (!m_bStop)
  {
#ifdef HAS_PERFORMANCE_SAMPLE
    CPerformanceSample sampleLoop("XBApplicationEx-loop");
#endif

    //-----------------------------------------
    // Perform app timing
    //-----------------------------------------

    // Check Start button
#ifdef HAS_GAMEPAD
    if ( m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_START )
#endif
      m_bPaused = !m_bPaused;

    // Get the current time (keep in LARGE_INTEGER format for precision)
    QueryPerformanceCounter( &qwTime );
    qwElapsedTime.QuadPart = qwTime.QuadPart - qwLastTime.QuadPart;
    qwLastTime.QuadPart = qwTime.QuadPart;
    if ( m_bPaused )
      qwElapsedAppTime.QuadPart = 0;
    else
      qwElapsedAppTime.QuadPart = qwElapsedTime.QuadPart;
    qwAppTime.QuadPart += qwElapsedAppTime.QuadPart;

    // Store the current time values as floating point
    m_fTime = fSecsPerTick * ((FLOAT)(qwTime.QuadPart));
    m_fElapsedTime = fSecsPerTick * ((FLOAT)(qwElapsedTime.QuadPart));
    m_fAppTime = fSecsPerTick * ((FLOAT)(qwAppTime.QuadPart));
    m_fElapsedAppTime = fSecsPerTick * ((FLOAT)(qwElapsedAppTime.QuadPart));

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

  }
  Destroy();

  CLog::Log(LOGNOTICE, "application stopped..." );
  return 0;
}




inline float DeadZone(float &f)
{
  if (f > g_advancedSettings.m_controllerDeadzone)
    return (f - g_advancedSettings.m_controllerDeadzone)/(1.0f - g_advancedSettings.m_controllerDeadzone);
  else if (f < -g_advancedSettings.m_controllerDeadzone)
    return (f + g_advancedSettings.m_controllerDeadzone)/(1.0f - g_advancedSettings.m_controllerDeadzone);
  else
    return 0.0f;
}

#ifdef HAS_GAMEPAD
inline float MaxTrigger(XBGAMEPAD &gamepad)
{
  float max = fabs(gamepad.fX1);
  if (fabs(gamepad.fX2) > max) max = fabs(gamepad.fX2);
  if (fabs(gamepad.fY1) > max) max = fabs(gamepad.fY1);
  if (fabs(gamepad.fY2) > max) max = fabs(gamepad.fY2);
  return max;
}
#endif
void CXBApplicationEx::ReadInput()
{
  MEASURE_FUNCTION;

  //-----------------------------------------
  // Handle input
  //-----------------------------------------

  // Read the input from the IR remote
#ifdef HAS_IR_REMOTE
  XBInput_GetInput( m_IR_Remote );
  ZeroMemory( &m_DefaultIR_Remote, sizeof(m_DefaultIR_Remote) );

  for ( DWORD i = 0; i < 4; i++ )
  {
    if ( m_IR_Remote[i].hDevice)
    {
      m_DefaultIR_Remote.wButtons = m_IR_Remote[i].wButtons;
    }
  }
#endif

#ifdef HAS_SDL
  //SDL_PumpEvents();

  static RESOLUTION windowres = WINDOW;

  // Read the input from the mouse
  g_Mouse.Update();

  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    switch(event.type)
    {
#ifdef _WIN32PC
    case SDL_SYSWMEVENT:
      {
        if (event.syswm.msg->wParam == DBT_DEVICEARRIVAL)
        {
          CMediaSource share;
          CStdString strDrive = CWIN32Util::GetChangedDrive();
          share.strName.Format("%s (%s)", g_localizeStrings.Get(437), strDrive);
          share.strName.Replace(":\\",":");
          share.strPath = strDrive;
          share.m_iDriveType = CMediaSource::SOURCE_TYPE_REMOVABLE;
          g_settings.AddShare("files",share);
          CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
          m_gWindowManager.SendThreadMessage( msg );
        }
        if (event.syswm.msg->wParam == DBT_DEVICEREMOVECOMPLETE)
        {     
          CStdString strDrive = CWIN32Util::GetChangedDrive();
          CStdString strName;
          strName.Format("%s (%s)", g_localizeStrings.Get(437), strDrive);
          strName.Replace(":\\",":");
          g_settings.DeleteSource("files",strName,strDrive);
          CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REMOVED_MEDIA);
          m_gWindowManager.SendThreadMessage( msg );
        }
      }
      break;
#endif

    case SDL_QUIT:
      if (!g_application.m_bStop) g_application.getApplicationMessenger().Shutdown();
      break;

    case SDL_VIDEORESIZE:
#ifndef __APPLE__
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
      g_Keyboard.Update(event);
      break;
    case SDL_ACTIVEEVENT:
      //If the window was inconified or restored
      if( event.active.state & SDL_APPACTIVE )
      {
        m_AppActive = event.active.gain != 0;
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
#else
  // Read the input from the keyboard
  g_Keyboard.Update();

  // Read the input from the mouse
  g_Mouse.Update();
#endif

#ifdef HAS_LIRC
  g_RemoteControl.Update();
#endif

#ifdef HAS_GAMEPAD
  // Read the input for all connected gampads
  XBInput_GetInput( m_Gamepad );

  // Lump inputs of all connected gamepads into one common structure.
  // This is done so apps that need only one gamepad can function with
  // any gamepad.
  ZeroMemory( &m_DefaultGamepad, sizeof(m_DefaultGamepad) );

  float maxTrigger = 0.0f;
  for ( DWORD i = 0; i < 4; i++ )
  {
    if ( m_Gamepad[i].hDevice )
    {
      if (maxTrigger < MaxTrigger(m_Gamepad[i]))
      {
        maxTrigger = MaxTrigger(m_Gamepad[i]);
        m_DefaultGamepad.fX1 = m_Gamepad[i].fX1;
        m_DefaultGamepad.fY1 = m_Gamepad[i].fY1;
        m_DefaultGamepad.fX2 = m_Gamepad[i].fX2;
        m_DefaultGamepad.fY2 = m_Gamepad[i].fY2;
      }
      m_DefaultGamepad.wButtons |= m_Gamepad[i].wButtons;
      m_DefaultGamepad.wPressedButtons |= m_Gamepad[i].wPressedButtons;
      m_DefaultGamepad.wLastButtons |= m_Gamepad[i].wLastButtons;

      for ( DWORD b = 0; b < 8; b++ )
      {
        m_DefaultGamepad.bAnalogButtons[b] |= m_Gamepad[i].bAnalogButtons[b];
        m_DefaultGamepad.bPressedAnalogButtons[b] |= m_Gamepad[i].bPressedAnalogButtons[b];
        m_DefaultGamepad.bLastAnalogButtons[b] |= m_Gamepad[i].bLastAnalogButtons[b];
      }
    }
  }

  // Secure the deadzones of the analog sticks
  m_DefaultGamepad.fX1 = DeadZone(m_DefaultGamepad.fX1);
  m_DefaultGamepad.fY1 = DeadZone(m_DefaultGamepad.fY1);
  m_DefaultGamepad.fX2 = DeadZone(m_DefaultGamepad.fX2);
  m_DefaultGamepad.fY2 = DeadZone(m_DefaultGamepad.fY2);
#endif
}

void CXBApplicationEx::Process()
{}
