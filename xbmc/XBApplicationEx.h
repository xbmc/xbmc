// XBApplicationEx.h: interface for the CXBApplicationEx class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XBAPPLICATIONEX_H__B5474945_70C7_4084_B345_0F1874AC77BA__INCLUDED_)
#define AFX_XBAPPLICATIONEX_H__B5474945_70C7_4084_B345_0F1874AC77BA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef HAS_GAMEPAD
#include "XBInput.h"
#include "XBInputEx.h"
#endif
#include "IWindowManagerCallback.h"
#include "common/Mouse.h"
#ifdef _XBOX
#include "common/XBoxKeyboard.h"
#else
#include "common/DirectInputKeyboard.h"
#endif
//-----------------------------------------------------------------------------
// Global access to common members
//-----------------------------------------------------------------------------
//extern LPDIRECT3DDEVICE8 g_pd3dDevice;




//-----------------------------------------------------------------------------
// Error codes
//-----------------------------------------------------------------------------
#define XBAPPERR_MEDIANOTFOUND       0x82000003




//-----------------------------------------------------------------------------
// Name: class CXBApplicationEx
// Desc: A base class for creating sample Xbox applications. To create a simple
//       Xbox application, simply derive this class and override the following
//       functions:
//          Initialize()          - To initialize the device-dependant objects
//          FrameMove()           - To animate the scene
//          Render()              - To render the scene
//-----------------------------------------------------------------------------
class CXBApplicationEx : public IWindowManagerCallback
{
public:
  D3DPRESENT_PARAMETERS m_d3dpp;

  // Main objects used for creating and rendering the 3D scene
  LPDIRECT3D8 m_pD3D;              // The D3D enumerator object
  LPDIRECT3DDEVICE8 m_pd3dDevice;        // The D3D rendering device
  LPDIRECT3DSURFACE8 m_pBackBuffer;       // The back buffer
  //LPDIRECT3DSURFACE8    m_pDepthBuffer;      // The depth buffer

  // Variables for timing
  FLOAT m_fTime;             // Current absolute time in seconds
  FLOAT m_fElapsedTime;      // Elapsed absolute time since last frame
  FLOAT m_fAppTime;          // Current app time in seconds
  FLOAT m_fElapsedAppTime;   // Elapsed app time since last frame
  BOOL m_bPaused;           // Whether app time is paused by user
  WCHAR m_strFrameRate[20];  // Frame rate written to a CStdString
  HANDLE m_hFrameCounter;     // Handle to frame rate perf counter
  bool m_bStop;
#ifdef HAS_GAMEPAD
  // Members to init the XINPUT devices.
  XDEVICE_PREALLOC_TYPE* m_InputDeviceTypes;
  DWORD m_dwNumInputDeviceTypes;
  XBGAMEPAD* m_Gamepad;
  XBGAMEPAD m_DefaultGamepad;
#endif
#ifdef HAS_IR_REMOTE
  // XBMP 6.0 - START
  XBIR_REMOTE m_IR_Remote[4];
  XBIR_REMOTE m_DefaultIR_Remote;
  // XBMP 6.0 - END
#endif

  // Overridable functions for the 3D scene created by the app
  virtual HRESULT Initialize() { return S_OK; }
  virtual HRESULT Cleanup() { return S_OK; }
  void ReadInput();

public:
  // Functions to create, run, and clean up the application
  virtual HRESULT Create(HWND hWnd);
  INT Run();
  VOID Destroy();
  virtual void Process();
  // Internal constructor
  CXBApplicationEx();
};

#endif // !defined(AFX_XBAPPLICATIONEX_H__B5474945_70C7_4084_B345_0F1874AC77BA__INCLUDED_)
