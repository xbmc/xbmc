//-----------------------------------------------------------------------------
// File: XBMC_PC.cpp
//
// Desc: This is the first tutorial for using Direct3D. In this tutorial, all
//       we are doing is creating a Direct3D device and using it to clear the
//       window.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "../../xbmc/stdafx.h"
#include "XBMC_PC.h"
#include <d3d8.h>
#include "../../xbmc/Application.h"

//-----------------------------------------------------------------------------
// Resource defines
//-----------------------------------------------------------------------------

#include "resource.h"
#define IDI_MAIN_ICON          101 // Application icon
#define IDR_MAIN_ACCEL         113 // Keyboard accelerator

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

CApplication g_application;
CXBMC_PC *g_xbmcPC;

CXBMC_PC::CXBMC_PC()
{
  m_hWnd = NULL;
  m_hAccel = NULL;
  m_dwWindowStyle = 0;
  m_dwCreationWidth = 720;
  m_dwCreationHeight = 576;
  m_strWindowTitle = "XBoxMediaCenter PC Skin Preview";
  m_active = false;
  m_focused = false;
  m_closing = false;
  m_mouseEnabled = true;
  m_inDialog = false;
  m_fullscreen = false;
}

CXBMC_PC::~CXBMC_PC()
{
  // todo: deinitialization code
}

LRESULT WINAPI WinProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  return g_xbmcPC->MsgProc(hWnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Name: WinProc()
// Desc: The window's message handler
//-----------------------------------------------------------------------------
LRESULT CXBMC_PC::MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch( msg )
  {
    case WM_DESTROY:
      Cleanup();
      SaveSettings();
      PostQuitMessage( 0 );
      return 0;

    case WM_PAINT:
      Render();
      ValidateRect( hWnd, NULL );
      return 0;

    case WM_SIZE:
      // Check to see if we are losing our window...
      if( SIZE_MAXHIDE==wParam || SIZE_MINIMIZED==wParam )
      {
 //       if( m_bClipCursorWhenFullscreen && !m_bWindowed )
 //         ClipCursor( NULL );
        m_active = FALSE;
      }
      else
      {
        m_active = TRUE;
      }
      break;
    case WM_MOUSEACTIVATE:
      {
        return MA_ACTIVATEANDEAT;
      }
      break;
    case WM_ACTIVATE:
      {
        m_focused = !(WA_INACTIVE==wParam);
        // we have focus - grab the keyboard and mouse
        g_Keyboard.Acquire();
      }
      break;
    case WM_SETCURSOR:
      {
        if(m_focused && m_mouseEnabled)
        {
          POINT point;
          g_Mouse.Acquire();
          if (GetCursorPos(point))
            return TRUE;
        }
      }
      break;
    case WM_COMMAND:
			switch( LOWORD(wParam) )
      {
				case ID_SETTINGS_MOUSE:
          // mouse disabled/enabled
          m_mouseEnabled = !m_mouseEnabled;
          {
            HMENU menu = GetMenu(m_hWnd);
            CheckMenuItem(menu, ID_SETTINGS_MOUSE, m_mouseEnabled ? MF_CHECKED : MF_UNCHECKED);
          }
					break;
        case ID_RESOLUTION_PAL4X3:
          g_applicationMessenger.ExecBuiltIn("Resolution(PAL)");
          break;
        case ID_RESOLUTION_PAL16X9:
          g_applicationMessenger.ExecBuiltIn("Resolution(PAL16x9)");
          break;
        case ID_RESOLUTION_NTSC4X3:
          g_applicationMessenger.ExecBuiltIn("Resolution(NTSC)");
          break;
        case ID_RESOLUTION_NTSC16X9:
          g_applicationMessenger.ExecBuiltIn("Resolution(NTSC16x9)");
          break;
        case ID_RESOLUTION_720P:
          g_applicationMessenger.ExecBuiltIn("Resolution(720p)");
          break;
        case ID_RESOLUTION_1080I:
          g_applicationMessenger.ExecBuiltIn("Resolution(1080i)");
          break;
        case ID_SKIN_RELOAD:
          g_applicationMessenger.ExecBuiltIn("ReloadSkin");
          break;
        case ID_SKIN_ACTIVATEWINDOW:
          OnActivateWindow();
          break;
        case ID_RESIZEWINDOW_TOPIXELSIZE:
          OnResizeToPixel();
          break;
        case ID_RESIZEWINDOW_TOASPECTRATIO:
          OnResizeToAspectRatio();
          break;
        case ID_EXECUTE_BUILTIN:
          OnExecuteBuiltin();
          break;
        case ID_EXECUTE_QUIT:
          Cleanup();
          PostQuitMessage( 0 );
          return 0;
          break;
			}
			break;
  }

  return DefWindowProc( hWnd, msg, wParam, lParam );
}

void CXBMC_PC::OnActivateWindow()
{
  m_inDialog = true;
  CStdString window = "Enter Window Name or ID";
  // Prompt the user to change the mode
  if( IDOK == DialogBoxParam( (HINSTANCE)GetModuleHandle(NULL),
                              MAKEINTRESOURCE(IDD_ACTIVATE_WINDOW), m_hWnd,
                              ActivateWindowProc, (LPARAM)&window ) )
  {
    // now activate the window
    if (!window.IsEmpty())
    {
      CStdString command;
      command.Format("ActivateWindow(%s)", window.c_str());
      g_applicationMessenger.ExecBuiltIn(command);
    }
  }
  m_inDialog = false;
}

void CXBMC_PC::OnExecuteBuiltin()
{
  m_inDialog = true;
  CStdString command = "Enter Built In Function to Execute";
  // Prompt the user to change the mode
  if( IDOK == DialogBoxParam( (HINSTANCE)GetModuleHandle(NULL),
                              MAKEINTRESOURCE(IDD_ACTIVATE_WINDOW), m_hWnd,
                              ActivateWindowProc, (LPARAM)&command ) )
  {
    // now activate the window
    if (!command.IsEmpty())
    {
      g_applicationMessenger.ExecBuiltIn(command);
    }
  }
  m_inDialog = false;
}

HRESULT CXBMC_PC::Render()
{
  // we check for g_application.m_bStop here, as if we have a dialog open, we're likely
  // to be in Process() or FrameMove(), and if the window's close button is pushed, this
  // will cause us to deinit
  if (!g_application.m_bStop) g_application.Process();
  if (!g_application.m_bStop) g_application.FrameMove();
  if (!g_application.m_bStop) g_application.Render();
  Sleep(0);
  return S_OK;
}

HRESULT CXBMC_PC::Cleanup()
{
  m_active = false;
  m_closing = true;
//  if (!g_application.m_bStop)
//    g_application.Stop();
  return S_OK;
}

void CXBMC_PC::SaveSettings()
{
  WINDOWPLACEMENT WndStatus;
  WndStatus.length = sizeof(WINDOWPLACEMENT);
  GetWindowPlacement(m_hWnd, &WndStatus);

  HKEY hKey;
  RegOpenKeyEx(HKEY_CURRENT_USER, "", 0, KEY_WRITE, &hKey);

  HKEY winKey;
  RegCreateKeyEx(hKey, "XBMC_PC\\window", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &winKey, NULL);
  RegSetValueEx(winKey, "flags", 0, REG_DWORD, (BYTE *)&WndStatus.flags, sizeof(DWORD));
  RegSetValueEx(winKey, "showcmd", 0, REG_DWORD, (BYTE *)&WndStatus.showCmd, sizeof(DWORD));
  RegSetValueEx(winKey, "left", 0, REG_DWORD, (BYTE *)&WndStatus.rcNormalPosition.left, sizeof(DWORD));
  RegSetValueEx(winKey, "right", 0, REG_DWORD, (BYTE *)&WndStatus.rcNormalPosition.right, sizeof(DWORD));
  RegSetValueEx(winKey, "top", 0, REG_DWORD, (BYTE *)&WndStatus.rcNormalPosition.top, sizeof(DWORD));
  RegSetValueEx(winKey, "bottom", 0, REG_DWORD, (BYTE *)&WndStatus.rcNormalPosition.bottom, sizeof(DWORD));
  RegCloseKey(winKey);
  RegCreateKeyEx(hKey, "XBMC_PC\\settings", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &winKey, NULL);
  RegSetValueEx(winKey, "mouse", 0, REG_DWORD, (BYTE *)&m_mouseEnabled, sizeof(BYTE));
  RegCloseKey(winKey);
  RegCloseKey(hKey);
}

void CXBMC_PC::LoadSettings()
{
  HKEY hKey;
  RegOpenKeyEx(HKEY_CURRENT_USER, "XBMC_PC", 0, KEY_WRITE, &hKey);
  if (hKey)
  {
    WINDOWPLACEMENT WndStatus;
    WndStatus.length = sizeof(WINDOWPLACEMENT);

    HKEY winKey;
    RegCreateKeyEx(hKey, "window", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE, NULL, &winKey, NULL);
    DWORD size = sizeof(DWORD);
    RegQueryValueEx(winKey, "flags", 0, NULL, (BYTE *)&WndStatus.flags, &size);
    RegQueryValueEx(winKey, "showcmd", 0, NULL, (BYTE *)&WndStatus.showCmd, &size);
    RegQueryValueEx(winKey, "left", 0, NULL, (BYTE *)&WndStatus.rcNormalPosition.left, &size);
    RegQueryValueEx(winKey, "right", 0, NULL, (BYTE *)&WndStatus.rcNormalPosition.right, &size);
    RegQueryValueEx(winKey, "top", 0, NULL, (BYTE *)&WndStatus.rcNormalPosition.top, &size);
    RegQueryValueEx(winKey, "bottom", 0, NULL, (BYTE *)&WndStatus.rcNormalPosition.bottom, &size);
    RegCloseKey(winKey);
    RegCreateKeyEx(hKey, "settings", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE, NULL, &winKey, NULL);
    size = sizeof(BOOL);
    RegQueryValueEx(winKey, "mouse", 0, NULL, (BYTE *)&m_mouseEnabled, &size);
    RegCloseKey(hKey);
    HMENU menu = GetMenu(m_hWnd);
    CheckMenuItem(menu, ID_SETTINGS_MOUSE, m_mouseEnabled ? MF_CHECKED : MF_UNCHECKED);

    WndStatus.ptMinPosition.x = 0;
    WndStatus.ptMinPosition.y = 0;
    WndStatus.ptMaxPosition.x = -GetSystemMetrics(SM_CXBORDER);
    WndStatus.ptMaxPosition.y =-GetSystemMetrics(SM_CYBORDER);
    if (!this->m_fullscreen)
      SetWindowPlacement(m_hWnd, &WndStatus);
  }
}

HRESULT CXBMC_PC::Create( HINSTANCE hInstance )
{
  m_hInstance = hInstance;
  HRESULT hr = S_OK;

  // Create the Direct3D object
//  if( m_pD3D == NULL )
//    return DisplayErrorMsg( D3DAPPERR_NODIRECT3D, MSGERR_APPMUSTEXIT );

  // Build a list of Direct3D adapters, modes and devices. The
  // ConfirmDevice() callback is used to confirm that only devices that
  // meet the app's requirements are considered.
//  if( FAILED( hr = BuildDeviceList() ) )
//  {
//   SAFE_RELEASE( m_pD3D );
//  return DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
//  }
  int nArgs;
  LPWSTR * szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
  for(int  i=0; i<nArgs; i++)  {
      if (lstrcmpW(szArglist[i], L"-fs") == 0) {
          m_fullscreen = true;
      }
  }
  // Free memory allocated for CommandLineToArgvW arguments.
   LocalFree(szArglist);

  // Unless a substitute hWnd has been specified, create a window to
  // render into
  if( m_hWnd == NULL)
  {
    // Register the windows class
    WNDCLASS wndClass = { 0, WinProc, 0, 0, hInstance,
                          LoadIcon( hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON) ),
                          LoadCursor( NULL, IDC_ARROW ),
                          (HBRUSH)GetStockObject(WHITE_BRUSH),
                          NULL, _T("XBoxMediaCenterPC") };
    RegisterClass( &wndClass );
    HMENU toolMenu;
    // Set the window's initial style
    if (m_fullscreen) {
      m_dwWindowStyle = WS_POPUP | WS_VISIBLE;
      m_dwCreationWidth = GetSystemMetrics(SM_CXSCREEN);
      m_dwCreationHeight = GetSystemMetrics(SM_CYSCREEN);
      toolMenu = NULL;
    } else {
      m_dwWindowStyle = WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|
                        WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_VISIBLE;  
      toolMenu = LoadMenu( hInstance, MAKEINTRESOURCE(IDR_MENU) );                        
    }

    // Set the window's initial width 
    RECT rc;
    SetRect( &rc, 0, 0, m_dwCreationWidth, m_dwCreationHeight );
    AdjustWindowRect( &rc, m_dwWindowStyle, !m_fullscreen );

    // Create the render window
    //int foo  = rc.bottom-rc.top;
    m_hWnd = CreateWindow(_T("XBoxMediaCenterPC"), m_strWindowTitle.c_str(), m_dwWindowStyle,
                            0, 0, (rc.right-rc.left), (rc.bottom-rc.top), 0L, toolMenu , hInstance, 0L );
  }

  // The focus window can be a specified to be a different window than the
  // device window.  If not, use the device window as the focus window.
  //if( m_hWndFocus == NULL )
  //    m_hWndFocus = m_hWnd;
  LoadSettings();

  // Save window properties
  m_dwWindowStyle = GetWindowLong( m_hWnd, GWL_STYLE );
  GetWindowRect( m_hWnd, &m_rcWindowBounds );
  GetClientRect( m_hWnd, &m_rcWindowClient );

  // Create the D3D Device
  g_application.Create(m_hWnd);

  // Initialize the application timer
//  DXUtil_Timer( TIMER_START );

  // Initialize the app's custom scene stuff
//  if( FAILED( hr = OneTimeSceneInit() ) )
//  {
//      SAFE_RELEASE( m_pD3D );
//      return DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
//  }

//  // Initialize the 3D environment for the app
//  if( FAILED( hr = Initialize3DEnvironment() ) )
//  {
//      SAFE_RELEASE( m_pD3D );
//      return DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
//  }

  // The app is ready to go
  m_active = TRUE;
//  m_bReady = TRUE;

  return S_OK;
}


INT CXBMC_PC::Run()
{
  // Load keyboard accelerators
  m_hAccel = LoadAccelerators( NULL, MAKEINTRESOURCE(IDR_MAIN_ACCEL) );

  // Now we're ready to recieve and process Windows messages.
  MSG  msg;
  msg.message = WM_NULL;
  PeekMessage( &msg, NULL, 0U, 0U, PM_NOREMOVE );

  while( WM_QUIT != msg.message  )
  {
    // Use PeekMessage() if the app is active, so we can use idle time to
    // render the scene. Else, use GetMessage() to avoid eating CPU time.
    if (!ProcessMessage(&msg))
    {
      // Render a frame during idle time (no messages are waiting)
      if( m_active && !m_closing)//m_bActive && m_bReady )
      {
        if( FAILED( Render() ) )
          SendMessage( m_hWnd, WM_CLOSE, 0, 0 );
      }
    }
  }

  return (INT)msg.wParam;
}

BOOL CXBMC_PC::ProcessMessage(MSG *msg)
{
  MSG backup;
  backup.message = WM_NULL;
  if (!msg)
    msg = &backup;
  BOOL bGotMsg;
  if( m_active || m_closing)
    bGotMsg = PeekMessage( msg, NULL, 0U, 0U, PM_REMOVE );
  else
    bGotMsg = GetMessage( msg, NULL, 0U, 0U );

  if (msg->message == WM_QUIT)
    PostQuitMessage( 0 );   // we can get called from a dialog within our app - this is to make sure we die.

  if(msg->message == WM_KEYDOWN && msg->wParam == VK_F11)
  {
    m_fullscreen = !m_fullscreen;
    if (m_fullscreen)
    {
       SetWindowLong (m_hWnd, GWL_STYLE, WS_POPUP|WS_VISIBLE);
       OnResizeToPixel();
    }
    else
    {
      SetWindowLong( m_hWnd, GWL_STYLE, WS_POPUPWINDOW|WS_CAPTION| WS_THICKFRAME|WS_VISIBLE|WS_MINIMIZEBOX|WS_MAXIMIZEBOX);
      OnResizeToAspectRatio();
    }
  }

  if( bGotMsg )
  {
    // Translate and dispatch the message
    if( 0 == TranslateAccelerator( m_hWnd, m_hAccel, msg ) )
    {
      TranslateMessage( msg );
      DispatchMessage( msg );
    }
  }
  return bGotMsg;
}

bool CXBMC_PC::GetCursorPos(POINT &point)
{
  ::GetCursorPos(&point);
  POINT client = point;
  ScreenToClient(m_hWnd, &client);
  RECT rect;
  GetClientRect(m_hWnd, &rect);
  if (!m_inDialog && m_mouseEnabled && client.x >= rect.left && client.y >= rect.top && client.x < rect.right && client.y < rect.bottom)
  {
    client.x -= rect.left;
    client.y -= rect.top;
    point.x = (client.x * g_graphicsContext.GetWidth()) / (rect.right - rect.left);
    point.y = (client.y * g_graphicsContext.GetHeight()) / (rect.bottom - rect.top);
    SetCursor(NULL);
    return true;
  }
  return false;
}

void CXBMC_PC::OnResizeToAspectRatio()
{
  RECT border = { 0, 0, 0, 0 };
  AdjustWindowRect(&border, m_dwWindowStyle, TRUE);
  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  float pixelRatio = g_graphicsContext.GetPixelRatio(res);
  float frameRatio = (pixelRatio * g_graphicsContext.GetWidth()) / g_graphicsContext.GetHeight();
  // resize so that it fits within the window the user has
  RECT rect;
  
  GetClientRect(m_hWnd, &rect);
  if (rect.bottom * frameRatio > rect.right)
    rect.bottom = (int)(rect.right / frameRatio);
  else
    rect.right = (int)(rect.bottom * frameRatio);
  // and set the new bounds
  RECT window;
  GetWindowRect(m_hWnd, &window);
  AdjustWindowRect(&rect, m_dwWindowStyle, TRUE);
  SetWindowPos( m_hWnd, HWND_NOTOPMOST,
              window.left, window.top,
              rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW );
}

void CXBMC_PC::OnResizeToPixel()
{
  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  RECT window;
  GetWindowRect(m_hWnd, &window);
  RECT client = { 0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight() };

  if (m_fullscreen)
  {
    RECT client2 = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    AdjustWindowRect(&client2, WS_POPUP | WS_MAXIMIZE, TRUE);
    SetWindowPos( m_hWnd, HWND_NOTOPMOST,0,0,client2.right, client2.bottom, SWP_SHOWWINDOW );
  }
  else 
  {
    AdjustWindowRect(&client, m_dwWindowStyle, TRUE);
    SetWindowPos( m_hWnd, HWND_NOTOPMOST,
              window.left, window.top,
              client.right - client.left, client.bottom - client.top, SWP_SHOWWINDOW );
  }
}

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
  CXBMC_PC myApp;

  g_xbmcPC = &myApp;

  if (GetDriveType("Q:\\") == DRIVE_NO_ROOT_DIR)
  {
    MessageBox(NULL, "No Q:\\ Drive found, Exiting XBMC_PC", "XBMC_PC: Fatal Error", MB_OK);
    return 0;
  }
  if (GetDriveType("Q:\\") == DRIVE_CDROM)
  {
    MessageBox(NULL, "Q:\\ Drive can not be DVD/CD Drive, Exiting XBMC_PC", "XBMC_PC: Fatal Error", MB_OK);
    return 0;
  }

  if (FAILED(myApp.Create(hInst)))
    return 1;

  return myApp.Run();
}

//-----------------------------------------------------------------------------
// Name: SelectDeviceProc()
// Desc: Windows message handling function for the device select dialog
//-----------------------------------------------------------------------------
INT_PTR CALLBACK CXBMC_PC::ActivateWindowProc( HWND hDlg, UINT msg,
                                                    WPARAM wParam, LPARAM lParam )
{
    // Get access to the UI controls
    HWND hWndEditControl        = GetDlgItem( hDlg, IDC_EDIT1 );
    BOOL bUpdateDlgControls     = FALSE;

    // Static state for our members
    static CStdString *pString;

    // Working variables

    // Handle the initialization message
    if( WM_INITDIALOG == msg )
    {
      // Old state
      pString = (CStdString *)lParam;
      // New state is initially the same as the old state

      SetWindowText(hDlg, pString->c_str());
      pString->Empty();

      // Set flag to update dialog controls below
      bUpdateDlgControls = TRUE;
    }

    if( WM_SHOWWINDOW == msg )
    {
      SetFocus(hWndEditControl);
    }

    if( WM_COMMAND == msg )
    {
      // Get current UI state
//        bNewWindowed  = Button_GetCheck( hwndWindowedRadio );

      if( IDOK == LOWORD(wParam) )
      {
        // Handle the case when the user hits the OK button. Check if any
        // of the options were changed
        EndDialog( hDlg, IDOK );
//        EndDialog( hDlg, IDCANCEL );
        return TRUE;
      }
      else if( IDCANCEL == LOWORD(wParam) )
      {
        // Handle the case when the user hits the Cancel button
        EndDialog( hDlg, IDCANCEL );
        return TRUE;
      }
      else if ( IDC_EDIT1 == LOWORD(wParam) )
      {
        // Edit box
        char text[256];
        GetWindowText(hWndEditControl, text, 256);
        *pString = text;
      }
      /* combo box
      else if( CBN_SELENDOK == HIWORD(wParam) )
      {
        if( LOWORD(wParam) == IDC_ADAPTER_COMBO )
        {
          dwNewAdapter = ComboBox_GetCurSel( hwndAdapterList );
          pAdapter     = &pd3dApp->m_Adapters[dwNewAdapter];

          dwNewDevice  = pAdapter->dwCurrentDevice;
          dwNewMode    = pAdapter->devices[dwNewDevice].dwCurrentMode;
          bNewWindowed = pAdapter->devices[dwNewDevice].bWindowed;
        }
        else if( LOWORD(wParam) == IDC_DEVICE_COMBO )
        {
          pAdapter     = &pd3dApp->m_Adapters[dwNewAdapter];

          dwNewDevice  = ComboBox_GetCurSel( hwndDeviceList );
          dwNewMode    = pAdapter->devices[dwNewDevice].dwCurrentMode;
          bNewWindowed = pAdapter->devices[dwNewDevice].bWindowed;
        }
        else if( LOWORD(wParam) == IDC_FULLSCREENMODES_COMBO )
        {
          dwNewMode = ComboBox_GetCurSel( hwndFullscreenModeList );
        }
        else if( LOWORD(wParam) == IDC_MULTISAMPLE_COMBO )
        {
          DWORD dwItem = ComboBox_GetCurSel( hwndMultiSampleList );
          if( bNewWindowed )
              NewMultiSampleTypeWindowed = (D3DMULTISAMPLE_TYPE)ComboBox_GetItemData( hwndMultiSampleList, dwItem );
          else
              NewMultiSampleTypeFullscreen = (D3DMULTISAMPLE_TYPE)ComboBox_GetItemData( hwndMultiSampleList, dwItem );
        }
*/
        // Keep the UI current
        bUpdateDlgControls = TRUE;
    }

    // Update the dialog controls
    if( bUpdateDlgControls )
    {
/*      // Reset the content in each of the combo boxes
      ComboBox_ResetContent( hwndAdapterList );
      ComboBox_ResetContent( hwndDeviceList );
      ComboBox_ResetContent( hwndFullscreenModeList );
      ComboBox_ResetContent( hwndMultiSampleList );

      pAdapter = &pd3dApp->m_Adapters[dwNewAdapter];
      pDevice  = &pAdapter->devices[dwNewDevice];

      // Add a list of adapters to the adapter combo box
      for( DWORD a=0; a < pd3dApp->m_dwNumAdapters; a++ )
      {
          // Add device name to the combo box
          DWORD dwItem = ComboBox_AddString( hwndAdapterList,
                            pd3dApp->m_Adapters[a].d3dAdapterIdentifier.Description );

          // Set the item data to identify this adapter
          ComboBox_SetItemData( hwndAdapterList, dwItem, a );

          // Set the combobox selection on the current adapater
          if( a == dwNewAdapter )
              ComboBox_SetCurSel( hwndAdapterList, dwItem );
      }

      // Add a list of devices to the device combo box
      for( DWORD d=0; d < pAdapter->dwNumDevices; d++ )
      {
        // Add device name to the combo box
        DWORD dwItem = ComboBox_AddString( hwndDeviceList,
                                            pAdapter->devices[d].strDesc );

        // Set the item data to identify this device
        ComboBox_SetItemData( hwndDeviceList, dwItem, d );

        // Set the combobox selection on the current device
        if( d == dwNewDevice )
          ComboBox_SetCurSel( hwndDeviceList, dwItem );
      }

      if( bNewWindowed )
      {
          Button_SetCheck( hwndWindowedRadio,   TRUE );
          Button_SetCheck( hwndFullscreenRadio, FALSE );
          EnableWindow( hwndFullscreenModeList, FALSE );
      }
      else
      {
          Button_SetCheck( hwndWindowedRadio,   FALSE );
          Button_SetCheck( hwndFullscreenRadio, TRUE );
          EnableWindow( hwndFullscreenModeList, TRUE );
      }
*/
      return TRUE;
    }

    return FALSE;
}


