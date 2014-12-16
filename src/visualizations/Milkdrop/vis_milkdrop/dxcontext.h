/*
  LICENSE
  -------
Copyright 2005 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer. 

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 

  * Neither the name of Nullsoft nor the names of its contributors may be used to 
    endorse or promote products derived from this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef __NULLSOFT_DX8_PLUGIN_SHELL_DXCONTEXT_H__
#define __NULLSOFT_DX8_PLUGIN_SHELL_DXCONTEXT_H__ 1

#include <windows.h>
//#include <xtl.h>
#include "shell_defines.h"
//#include "defines.h"
#include <d3d9.h>
#include <d3dx9.h>

typedef struct
{
    eScrMode screenmode;            // WINDOWED, FULLSCREEN, or FAKE FULLSCREEN
    int  nbackbuf;
    int  allow_page_tearing;
    GUID adapter_guid;
    D3DDISPLAYMODE display_mode;    // ONLY VALID FOR FULLSCREEN MODE.
    D3DMULTISAMPLE_TYPE multisamp;
    HWND parent_window;
    int m_dualhead_horz; // 0 = span both, 1 = left only, 2 = right only
    int m_dualhead_vert; // 0 = span both, 1 = top only, 2 = bottom only
    int m_skin; 
} 
DXCONTEXT_PARAMS;

#define MAX_DXC_ADAPTERS 32

class DXContext 
{
    public:
        // PUBLIC FUNCTIONS
		    DXContext(LPDIRECT3DDEVICE9 device, char* szIniFile);
//        DXContext(int hWndWinamp,HINSTANCE hInstance,LPCSTR szClassName,LPCSTR szWindowCaption,WNDPROC pProc,LONG uWindowLong, int minimize_winamp, char* szIniFile);
        ~DXContext();
        BOOL StartOrRestartDevice(DXCONTEXT_PARAMS *pParams); // also serves as Init() function
        BOOL OnUserResizeWindow(RECT *new_window_rect, RECT *new_client_rect);
        inline HWND GetHwnd() { return m_hwnd; };
        inline int  TempIgnoreDestroyMessages() { return m_ignore_wm_destroy; };
        void OnTrulyExiting() { m_truly_exiting = 1; }
        void UpdateMonitorWorkRect();
        int  GetBitDepth() { return m_bpp;     };
        inline D3DFORMAT GetZFormat() { return m_zFormat; };
        char* GetDriver() { return m_szDriver; };
        char* GetDesc()   { return m_szDesc; };
        void SaveWindow();

        // PUBLIC DATA - DO NOT WRITE TO THESE FROM OUTSIDE THE CLASS
        int m_ready;
        HRESULT m_lastErr;
        int m_window_width;
        int m_window_height;
        int m_client_width;
        int m_client_height;
        int m_fake_fs_covers_all;   
        int m_frame_delay;
        RECT m_all_monitors_rect;   // rect that encompasses all monitors that make up the desktop.  The primary monitor's upper-left corner is (0,0).
        RECT m_monitor_rect;        // rect for monitor the plugin is running on; for pseudo-multimon modes like 2048x768, if user decides to only run on half the monitor, this rect reflects that as well.
        RECT m_monitor_rect_orig;   //  same, but it's the original rect; does not account for pseudo-multimon modes like 2048x768
        RECT m_monitor_work_rect;   // same, but excludes the taskbar area.
        RECT m_monitor_work_rect_orig; // original work rect; does not account for pseudo-multimon modes like 2048x768
        DXCONTEXT_PARAMS       m_current_mode;
        LPDIRECT3DDEVICE9      m_lpDevice;
        D3DPRESENT_PARAMETERS  m_d3dpp;
        //LPDIRECT3D9            m_lpD3D;
        D3DCAPS9               m_caps;

    protected:
        D3DMULTISAMPLE_TYPE    m_multisamp;
        D3DFORMAT              m_zFormat;
        D3DFORMAT              m_orig_windowed_mode_format[MAX_DXC_ADAPTERS];
        HMODULE m_hmod_d3d8;
        int  m_ordinal_adapter;
        HWND m_hwnd;
        HWND m_hwnd_winamp;
        LONG m_uWindowLong;
        char m_szClassName[256];
        char m_szWindowCaption[512];
        char m_szIniFile[MAX_PATH];
        char m_szDriver[512];
        char m_szDesc[512];
        HINSTANCE m_hInstance;
        int  m_ignore_wm_destroy;
        int  m_minimize_winamp;
        int  m_winamp_minimized;
        int  m_truly_exiting;
        int  m_bpp;

        embedWindowState myWindowState;

        void WriteSafeWindowPos();
        int GetWindowedModeAutoSize(int iteration);
        BOOL TestDepth(int ordinal_adapter, D3DFORMAT fmt);
        BOOL TestFormat(int ordinal_adapter, D3DFORMAT fmt);
        int  CheckAndCorrectFullscreenDispMode(int ordinal_adapter, D3DDISPLAYMODE *pdm);
        void SetViewport();
        void MinimizeWinamp(HMONITOR hPluginMonitor);
        BOOL Internal_Init(DXCONTEXT_PARAMS *pParams, BOOL bFirstInit);
        void Internal_CleanUp();
        void RestoreWinamp();
};

#define DXC_ERR_REGWIN    -2
#define DXC_ERR_CREATEWIN -3
#define DXC_ERR_CREATE3D  -4
#define DXC_ERR_GETFORMAT -5
#define DXC_ERR_FORMAT    -6
#define DXC_ERR_CREATEDEV_PROBABLY_OUTOFVIDEOMEMORY -7
#define DXC_ERR_RESIZEFAILED -8
#define DXC_ERR_CAPSFAIL  -9
#define DXC_ERR_BAD_FS_DISPLAYMODE -10
#define DXC_ERR_USER_CANCELED -11
#define DXC_ERR_CREATEDEV_NOT_AVAIL -12
#define DXC_ERR_CREATEDDRAW  -13







#endif  