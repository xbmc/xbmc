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

/*
    TO DO
    -----

    -to do/v1.06:
        -FFT: high freq. data kinda sucks because of the 8-bit samples we get in;
            look for justin to put 16-bit vis data into wa5.
        -make an 'advanced view' button on config panel; hide complicated stuff
            til they click that.
        -put an asterisk(*) next to the 'max framerate' values that
            are ideal (given the current windows display mode or selected FS dispmode).
        -or add checkbox: "smart sync"
            -> matches FPS limit to nearest integer divisor of refresh rate.
        -debug.txt/logging support!
        -audio: make it a DSP plugin? then we could get the complete, continuous waveform
            and overlap our waveform windows, so we'd never miss a brief high note.
        -bugs:
            -vms plugins sometimes freeze after a several-minute pause; I've seen it
                with most of them.  hard to repro, though.
            -running FS on monitor 2, hit ALT-TAB -> minimizes!!!
                -but only if you let go of TAB first.  Let go of ALT first and it's fine!
                -> means it's related to the keyup...
            -fix delayloadhelper leak; one for each launch to config panel/plugin.
            -also, delayload(d3d8.dll) still leaks, if plugin has error initializing and
                quits by returning false from PluginInitialize().
        -add config panel option to ignore fake-fullscreen tips
            -"tip" boxes in dxcontext.cpp
            -"notice" box on WM_ACTIVATEAPP?
        -desktop mode:
            -icon context menus: 'send to', 'cut', and 'copy' links do nothing.
                -http://netez.com/2xExplorer/shellFAQ/bas_context.html
            -create a 2nd texture to render all icon text labels into
                (they're the sole reason that desktop mode is slow)
            -in UpdateIconBitmaps, don't read the whole bitmap and THEN
                realize it's a dupe; try to compare icon filename+index or somethign?
            -DRAG AND DROP.  COMPLICATED; MANY DETAILS.
                -http://netez.com/2xExplorer/shellFAQ/adv_drag.html
                -http://www.codeproject.com/shell/explorerdragdrop.asp
                -hmm... you can't drag icons between the 2 desktops (ugh)
            -multiple delete/open/props/etc
            -delete + enter + arrow keys.
            -try to solve mysteries w/ShellExecuteEx() and desktop *shortcuts* (*.lnk).
            -(notice that when icons are selected, they get modulated by the
                highlight color, when they should be blended 50% with that color.)

    ---------------------------
    final touches:
        -Tests:
            -make sure desktop still functions/responds properly when winamp paused
            -desktop mode + multimon:
                -try desktop mode on all monitors
                -try moving taskbar around; make sure icons are in the
                    right place, that context menus (general & for
                    specific icons) pop up in the right place, and that
                    text-off-left-edge is ok.
                -try setting the 2 monitors to different/same resolutions
        -check tab order of config panel controls!
        -Clean All
        -build in release mode to include in the ZIP
        -leave only one file open in workspace: README.TXT.
        -TEMPORARILY "ATTRIB -R" ALL FILES BEFORE ZIPPING THEM!

    ---------------------------
    KEEP IN VIEW:
        -EMBEDWND:
            -kiv: on resize of embedwnd, it's out of our control; winamp
                resizes the child every time the mouse position changes,
                and we have to cleanup & reallocate everything, b/c we
                can't tell when the resize begins & ends.
                [justin said he'd fix in wa5, though]
            -kiv: with embedded windows of any type (plugin, playlist, etc.)
                you can't place the winamp main wnd over them.
            -kiv: tiny bug (IGNORE): when switching between embedwnd &
                no-embedding, the window gets scooted a tiny tiny bit.
        -kiv: fake fullscreen mode w/multiple monitors: there is no way
            to keep the taskbar from popping up [potentially overtop of
            the plugin] when you click on something besides the plugin.
            To get around this, use true fullscreen mode.
        -kiv: max_fps implementation assumptions:
            -that most computers support high-precision timer
            -that no computers [regularly] sleep for more than 1-2 ms
                when you call Sleep(1) after timeBeginPeriod(1).
        -reminder: if vms_desktop.dll's interface needs changed,
            it will have to be renamed!  (version # upgrades are ok
            as long as it won't break on an old version; if the
            new functionality is essential, rename the DLL.)

    ---------------------------
    REMEMBER:
        -GF2MX + GF4 have icon scooting probs in desktop mode
            (when taskbar is on upper or left edge of screen)
        -Radeon is the one w/super slow text probs @ 1280x1024.
            (it goes unstable after you show playlist AND helpscr; -> ~1 fps)
        -Mark's win98 machine has hidden cursor (in all modes),
            but no one else seems to have this problem.
        -links:
            -win2k-only-style desktop mode: (uses VirtualAllocEx, vs. DLL Injection)
                http://www.digiwar.com/scripts/renderpage.php?section=2&subsection=2
            -http://www.experts-exchange.com/Programming/Programming_Platforms/Win_Prog/Q_20096218.html
*/

//#include <xtl.h>
#include <windows.h>
#include "pluginshell.h"
#include "utility.h"
#include "defines.h"
#include "shell_defines.h"
//#include "resource.h"
//#include "vis.h"
#include <time.h>
#include <stdio.h>
#include <math.h>
//#include <multimon.h>
//#pragma comment(lib,"winmm.lib")    // for timeGetTime

// STATE VALUES & VERTEX FORMATS FOR HELP SCREEN TEXTURE:
#define TEXT_SURFACE_NOT_READY  0
#define TEXT_SURFACE_REQUESTED  1
#define TEXT_SURFACE_READY      2
#define TEXT_SURFACE_ERROR      3
typedef struct _HELPVERTEX
{
    float x, y;      // screen position
    float z;         // Z-buffer depth
    DWORD Diffuse;   // diffuse color. also acts as filler; aligns struct to 16 bytes (good for random access/indexed prims)
    float tu, tv;    // texture coordinates for texture #0
} HELPVERTEX, *LPHELPVERTEX;
#define HELP_VERTEX_FORMAT (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)
typedef struct _SIMPLEVERTEX
{
    float x, y;      // screen position
    float z;         // Z-buffer depth
    DWORD Diffuse;   // diffuse color. also acts as filler; aligns struct to 16 bytes (good for random access/indexed prims)
} SIMPLEVERTEX, *LPSIMPLEVERTEX;
#define SIMPLE_VERTEX_FORMAT (D3DFVF_XYZ | D3DFVF_DIFFUSE)

extern char g_szHelp[];
//extern winampVisModule mod1;

// resides in vms_desktop.dll/lib:
void getItemData(int x);


CPluginShell::CPluginShell()
{
    // this should remain empty!
}

CPluginShell::~CPluginShell()
{
    // this should remain empty!
}

eScrMode  CPluginShell::GetScreenMode()     { return m_screenmode; };
int       CPluginShell::GetFrame()          { return m_frame;      };
float     CPluginShell::GetTime()           { return m_time;       };
float     CPluginShell::GetFps()            { return m_fps;        };
HWND      CPluginShell::GetPluginWindow()   { if (m_lpDX) return m_lpDX->GetHwnd();       else return NULL; };
int       CPluginShell::GetWidth()          { if (m_lpDX) return m_lpDX->m_client_width;  else return 0; };
int       CPluginShell::GetHeight()         { if (m_lpDX) return m_lpDX->m_client_height; else return 0; };
HWND      CPluginShell::GetWinampWindow()   { return m_hWndWinamp; };
HINSTANCE CPluginShell::GetInstance()       { return m_hInstance;  };
char*     CPluginShell::GetPluginsDirPath() { return m_szPluginsDirPath;  };
char*     CPluginShell::GetConfigIniFile()  { return m_szConfigIniFile;   };
//int       CPluginShell::GetFontHeight(eFontIndex idx) { if (idx >= 0 && idx < NUM_BASIC_FONTS + NUM_EXTRA_FONTS) return m_fontinfo[idx].nSize; else return 0; };
int       CPluginShell::GetBitDepth()       { return m_lpDX->GetBitDepth(); };
LPDIRECT3DDEVICE9 CPluginShell::GetDevice() { if (m_lpDX) return m_lpDX->m_lpDevice; else return NULL; };
D3DCAPS9* CPluginShell::GetCaps()           { if (m_lpDX) return &(m_lpDX->m_caps);  else return NULL; };
D3DFORMAT CPluginShell::GetBackBufFormat()  { if (m_lpDX) return m_lpDX->m_current_mode.display_mode.Format; else return D3DFMT_UNKNOWN; };
D3DFORMAT CPluginShell::GetBackBufZFormat() { if (m_lpDX) return m_lpDX->GetZFormat(); else return D3DFMT_UNKNOWN; };
//LPD3DXFONT CPluginShell::GetFont(eFontIndex idx) { if (idx >= 0 && idx < NUM_BASIC_FONTS + NUM_EXTRA_FONTS) return m_d3dx_font[idx]; else return NULL; };
char* CPluginShell::GetDriverFilename()    { if (m_lpDX) return m_lpDX->GetDriver(); else return NULL; };
char* CPluginShell::GetDriverDescription() { if (m_lpDX) return m_lpDX->GetDesc(); else return NULL; };


int CPluginShell::InitNonDx8Stuff()
{
//    timeBeginPeriod(1);
    m_fftobj.Init(512, NUM_FREQUENCIES);
    if (!InitGDIStuff()) return false;
    return AllocateMyNonDx8Stuff();
}

void CPluginShell::CleanUpNonDx8Stuff()
{
//    timeEndPeriod(1);
    CleanUpMyNonDx8Stuff();
    CleanUpGDIStuff();
    m_fftobj.CleanUp();
}

int CPluginShell::InitGDIStuff()
{
#if 0
    // note: messagebox parent window should be NULL here, because lpDX is still NULL!

    for (int i=0; i<NUM_BASIC_FONTS + NUM_EXTRA_FONTS; i++)
    {
        if (!(m_font[i] = CreateFont(m_fontinfo[i].nSize, 0, 0, 0, m_fontinfo[i].bBold ? 900 : 400, m_fontinfo[i].bItalic, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, m_fontinfo[i].bAntiAliased ? ANTIALIASED_QUALITY : DEFAULT_QUALITY, DEFAULT_PITCH, m_fontinfo[i].szFace)))
        {
            MessageBox(NULL, "Error creating GDI fonts", "ERROR", MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
            return false;
        }
    }

    if (!(m_main_menu = LoadMenu(m_hInstance, MAKEINTRESOURCE(IDR_WINDOWED_CONTEXT_MENU))))
    {
        MessageBox(NULL, "Error loading main menu", "ERROR", MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
        return false;
    }

    if (!(m_context_menu = GetSubMenu(m_main_menu, 0)))
    {
        MessageBox(NULL, "Error loading context menu", "ERROR", MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
        return false;
    }
#endif
    return true;//InitMyGDIStuff();
}

void CPluginShell::CleanUpGDIStuff()
{
#if 0
    for (int i=0; i<NUM_BASIC_FONTS + NUM_EXTRA_FONTS; i++)
    {
        if (m_font[i])
        {
            DeleteObject(m_font[i]);
            m_font[i] = NULL;
        }
    }

    /*if (m_context_menu)
    {
        DestroyMenu(m_context_menu);
        m_context_menu = NULL;
    }*/

    if (m_main_menu)
    {
        DestroyMenu(m_main_menu);
        m_main_menu = NULL;
    }

    //CleanUpMyGDIStuff();

#endif
}

int CPluginShell::InitVJStuff(RECT* pClientRect)
{
#if 0
    // Init VJ mode (second window for text):
	if (m_vj_mode)
	{
        DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU;
        POINT upper_left_corner;
        upper_left_corner.x = 0;
        upper_left_corner.y = 0;

        // Create direct 3d & get some infos
        if (!(m_vjd3d8 = Direct3DCreate8(D3D_SDK_VERSION)))
        {
			MessageBox(NULL,"Error creating Direct3D device for VJ mode;\rDirectX 8 could be missing or corrupt.","MILKDROP ERROR",MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
			return false;
        }

        // Get ordinal adapter # for the currently-selected Windowed Mode display adapter
        int ordinal_adapter = D3DADAPTER_DEFAULT;
        int nAdapters = m_vjd3d8->GetAdapterCount();
        for (int i=0; i<nAdapters; i++)
        {
            D3DADAPTER_IDENTIFIER8 temp;
            if ((m_vjd3d8->GetAdapterIdentifier(i, D3DENUM_NO_WHQL_LEVEL, &temp) == D3D_OK) &&
                (memcmp(&temp.DeviceIdentifier, &m_adapter_guid_windowed, sizeof(GUID))==0))
            {
                ordinal_adapter = i;
                break;
            }
        }

        // Get current display mode for windowed-mode adapter:
        D3DDISPLAYMODE dm;
        if (D3D_OK != m_vjd3d8->GetAdapterDisplayMode(ordinal_adapter, &dm))
        {
			MessageBox(NULL,"VJ mode init error: error determining color format\rfor currently-selected Windowed Mode display adapter.","MILKDROP ERROR",MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
			return false;
        }

        // And get the upper-left corner of the monitor for it:
        HMONITOR hMon = m_vjd3d8->GetAdapterMonitor(ordinal_adapter);
        if (hMon)
        {
            MONITORINFO mi;
            mi.cbSize = sizeof(mi);
            if (GetMonitorInfo(hMon, &mi))
            {
                upper_left_corner.x = mi.rcWork.left;
                upper_left_corner.y = mi.rcWork.top;
            }
        }

        // CREATE THE WINDOW

    	RECT rect;
        if (pClientRect)
        {
            rect = *pClientRect;
            AdjustWindowRect(&rect, dwStyle, 0); // convert client->wnd
        }
        else
        {
            SetRect(&rect, 0, 0, 384, 384);
            AdjustWindowRect(&rect, dwStyle, 0); // convert client->wnd

            rect.right  -= rect.left;
            rect.left   = 0;
            rect.bottom -= rect.top;
            rect.top    = 0;

            rect.top    += upper_left_corner.y+32;
            rect.left   += upper_left_corner.x+32;
            rect.right  += upper_left_corner.x+32;
            rect.bottom += upper_left_corner.y+32;
        }

		WNDCLASS wc;
		memset(&wc,0,sizeof(wc));
		wc.lpfnWndProc = VJModeWndProc;				// our window procedure
		wc.hInstance = GetInstance();	// hInstance of DLL
		wc.hIcon = LoadIcon( GetInstance(), MAKEINTRESOURCE(IDI_PLUGIN_ICON) );
		wc.lpszClassName = TEXT_WINDOW_CLASSNAME;			// our window class name
         wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; // CS_DBLCLKS lets the window receive WM_LBUTTONDBLCLK, for toggling fullscreen mode...
         wc.cbClsExtra = 0;
         wc.cbWndExtra = sizeof(DWORD);
         wc.hCursor = LoadCursor(NULL, IDC_ARROW);
         wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
         wc.lpszMenuName = NULL;

		if (!RegisterClass(&wc))
		{
			MessageBox(NULL,"Error registering window class for text window","MILKDROP ERROR",MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
			return false;
		}
		m_bTextWindowClassRegistered = true;

		//DWORD nThreadID;
		//CreateThread(NULL, 0, TextWindowThread, &rect, 0, &nThreadID);

		// Create the text window
		m_hTextWnd = CreateWindowEx(
			0,
			TEXT_WINDOW_CLASSNAME,				// our window class name
			TEXT_WINDOW_CLASSNAME,				// use description for a window title
			dwStyle,
			rect.left, rect.top,								// screen position (read from config)
			rect.right - rect.left, rect.bottom - rect.top,  // width & height of window (need to adjust client area later)
			NULL,								// parent window (winamp main window)
			NULL,								// no menu
			GetInstance(),						// hInstance of DLL
			NULL
        ); // no window creation data

		if (!m_hTextWnd)
		{
			MessageBox(NULL,"Error creating VJ window","MILKDROP ERROR",MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
			return false;
		}

        SetWindowLong(m_hTextWnd, GWL_USERDATA, (LONG)this);

        GetClientRect(m_hTextWnd, &rect);
        m_nTextWndWidth  = rect.right-rect.left;
        m_nTextWndHeight = rect.bottom-rect.top;


        // Create the device
        D3DPRESENT_PARAMETERS pres_param;
        pres_param.BackBufferCount = 1;
        pres_param.BackBufferFormat = dm.Format;
        pres_param.BackBufferWidth  = rect.right - rect.left;
        pres_param.BackBufferHeight = rect.bottom - rect.top;
        pres_param.hDeviceWindow = m_hTextWnd;
        pres_param.AutoDepthStencilFormat = D3DFMT_D16;
        pres_param.EnableAutoDepthStencil = FALSE;
        pres_param.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pres_param.MultiSampleType = D3DMULTISAMPLE_NONE;
        pres_param.Flags = 0;
        pres_param.FullScreen_RefreshRateInHz = 0;
        pres_param.FullScreen_PresentationInterval = 0;
        pres_param.Windowed = TRUE;

        HRESULT hr;
        if (D3D_OK != (hr = m_vjd3d8->CreateDevice(ordinal_adapter,//D3DADAPTER_DEFAULT,
                               D3DDEVTYPE_HAL,
                               m_hTextWnd,
                               D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                               &pres_param,
                               &m_vjd3d8_device)))
        {
            m_vjd3d8_device = NULL;
			MessageBox(m_lpDX->GetHwnd(),"Error creating D3D device for VJ mode","MILKDROP ERROR",MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
			return false;
        }

        if (!AllocateFonts(m_vjd3d8_device))
            return false;

        if (m_fix_slow_text)    // note that when not doing vj mode, m_lpDDSText is allocated in AllocateDX8Stuff
            AllocateTextSurface();

        m_text.Finish();
        m_text.Init(m_vjd3d8_device, m_lpDDSText, 0);
	}
#endif
    return true;
}

void CPluginShell::CleanUpVJStuff()
{
#if 0
    // ALWAYS set the textures to NULL before releasing textures,
    // otherwise they might still have a hanging reference!
    if (m_lpDX && m_lpDX->m_lpDevice)
    {
        m_lpDX->m_lpDevice->SetTexture(0, NULL);
        m_lpDX->m_lpDevice->SetTexture(1, NULL);
    }

    if (m_vjd3d8_device)
    {
        m_vjd3d8_device->SetTexture(0, NULL);
        m_vjd3d8_device->SetTexture(1, NULL);
    }

    // clean up VJ mode
    {
        if (m_vjd3d8_device)
        {
            CleanUpFonts();
            SafeRelease(m_lpDDSText);
        }

        SafeRelease(m_vjd3d8_device);
        SafeRelease(m_vjd3d8);

        if (m_hTextWnd)
	    {
		    //dumpmsg("Finish: destroying text window");
		    DestroyWindow(m_hTextWnd);
		    m_hTextWnd = NULL;
		    //dumpmsg("Finish: text window destroyed");
	    }

	    if (m_bTextWindowClassRegistered)
	    {
		    //dumpmsg("Finish: unregistering text window class");
		    UnregisterClass(TEXT_WINDOW_CLASSNAME,GetInstance()); // unregister window class
		    m_bTextWindowClassRegistered = false;
		    //dumpmsg("Finish: text window class unregistered");
	    }
    }
#endif
}

int CPluginShell::AllocateFonts(IDirect3DDevice9* pDevice)
{
#if 0
    // Create D3DX system font:
    for (int i=0; i<NUM_BASIC_FONTS + NUM_EXTRA_FONTS; i++)
        if (D3DXCreateFont(pDevice, m_font[i], &m_d3dx_font[i]) != D3D_OK)
        {
            MessageBox(m_lpDX ? m_lpDX->GetHwnd() : NULL, "Error creating D3DX fonts", "ERROR", MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
            return false;
        }

    // get actual font heights
    for (i=0; i<NUM_BASIC_FONTS + NUM_EXTRA_FONTS; i++)
    {
        RECT r;
        SetRect(&r, 0, 0, 1024, 1024);
        int h = m_d3dx_font[i]->DrawText("M", -1, &r, DT_CALCRECT, 0xFFFFFFFF);
        if (h>0) m_fontinfo[i].nSize = h;
    }
#endif
    return true;
}

void CPluginShell::CleanUpFonts()
{
#if 0
    for (int i=0; i<NUM_BASIC_FONTS + NUM_EXTRA_FONTS; i++)
        SafeRelease(m_d3dx_font[i]);
#endif
}

void CPluginShell::AllocateTextSurface()
{
#if 0
    IDirect3DDevice8 *pDevice = m_vjd3d8_device ? m_vjd3d8_device : GetDevice();
    int w = m_vjd3d8_device ? m_nTextWndWidth  : GetWidth() ;
    int h = m_vjd3d8_device ? m_nTextWndHeight : GetHeight();

    if (D3D_OK != D3DXCreateTexture(pDevice, w, h, 1, D3DUSAGE_RENDERTARGET, GetBackBufFormat(), D3DPOOL_DEFAULT, &m_lpDDSText))
        m_lpDDSText = NULL; // OK if there's not enough mem for it!
    else
    {
        // if m_lpDDSText doesn't cover enough of screen, cancel it.
        D3DSURFACE_DESC desc;
        if (D3D_OK == m_lpDDSText->GetLevelDesc(0, &desc))
        {
            if ((desc.Width  < 256 && w >= 256)  ||
                (desc.Height < 256 && h >= 256)  ||
                (desc.Width /(float)w < 0.74f) ||
                (desc.Height/(float)h < 0.74f)
               )
            {
                m_lpDDSText->Release();
                m_lpDDSText = NULL;
            }
        }
    }
#endif
}

int CPluginShell::AllocateDX8Stuff()
{
//    if (m_screenmode == DESKTOP)
//        if (!InitDesktopMode())
//            return false;

    if (!m_vjd3d8_device)    // otherwise it's done in InitVJStuff, and tied to the vj text wnd device
        if (!AllocateFonts(GetDevice()))
            return false;

    if (!AllocateMyDX8Stuff())
        return false;

    // OFFSCREEN TEXT SETUP:
    if (m_fix_slow_text && !m_vjd3d8_device)    // otherwise it's allocated in InitVJStuff
        AllocateTextSurface();

    if (!m_vjd3d8_device)                       // otherwise it's initialized in InitVJStuff
    {
//        m_text.Finish();
//        m_text.Init(GetDevice(), m_lpDDSText, 1);
    }

    // invalidate various 'caches' here:
    m_playlist_top_idx = -1;    // invalidating playlist cache forces recompute of playlist width
    //m_icon_list.clear();      // clear desktop mode icon list, so it has to read the bitmaps back in

    return true;
}

void CPluginShell::CleanUpDX8Stuff(int final_cleanup)
{
    // ALWAYS set the textures to NULL before releasing textures,
    // otherwise they might still have a hanging reference!
    if (m_lpDX && m_lpDX->m_lpDevice)
    {
        m_lpDX->m_lpDevice->SetTexture(0, NULL);
        m_lpDX->m_lpDevice->SetTexture(1, NULL);
    }

    if (m_vjd3d8_device)
    {
        m_vjd3d8_device->SetTexture(0, NULL);
        m_vjd3d8_device->SetTexture(1, NULL);
    }

    //----------------------//

//    m_text.Finish();

//    if (!m_vjd3d8_device)   // otherwise it happens in CleanUpVJStuff()
//        SafeRelease(m_lpDDSText);

    CleanUpMyDX8Stuff(final_cleanup);

    if (!m_vjd3d8_device)   // otherwise it happens in CleanUpVJStuff()
        CleanUpFonts();

//    if (m_screenmode == DESKTOP)
//        CleanUpDesktopMode();
}

void CPluginShell::OnUserResizeTextWindow()
{
#if 0
    // Update window properties
    RECT w, c;
    GetWindowRect( m_hTextWnd, &w );
    GetClientRect( m_hTextWnd, &c );

    WINDOWPLACEMENT wp;
    ZeroMemory(&wp, sizeof(wp));
    wp.length = sizeof(wp);
    GetWindowPlacement(m_hTextWnd, &wp);

    // convert client rect from client coords to screen coords:
    // (window rect is already in screen coords...)
    POINT p;
    p.x = c.left;
    p.y = c.top;
    if (ClientToScreen(m_hTextWnd, &p))
    {
        c.left += p.x;
        c.right += p.x;
        c.top += p.y;
        c.bottom += p.y;
    }

    if(wp.showCmd != SW_SHOWMINIMIZED)
    {
        if (m_nTextWndWidth  != c.right-c.left ||
            m_nTextWndHeight != c.bottom-c.top)
        {
            CleanUpVJStuff();
            if (!InitVJStuff(&c))
            {
                SuggestHowToFreeSomeMem();
                m_lpDX->m_ready = false;   // flag to exit
                return;
            }
        }

        // save the new window position:
        //if (wp.showCmd==SW_SHOWNORMAL)
        //    SaveTextWindowPos();
    }
#endif
}

void CPluginShell::OnUserResizeWindow()
{
#if 0
    // Update window properties
    RECT w, c;
    GetWindowRect( m_lpDX->GetHwnd(), &w );
    GetClientRect( m_lpDX->GetHwnd(), &c );

    WINDOWPLACEMENT wp;
    ZeroMemory(&wp, sizeof(wp));
    wp.length = sizeof(wp);
    GetWindowPlacement(m_lpDX->GetHwnd(), &wp);

    // convert client rect from client coords to screen coords:
    // (window rect is already in screen coords...)
    POINT p;
    p.x = c.left;
    p.y = c.top;
    if (ClientToScreen(m_lpDX->GetHwnd(), &p))
    {
        c.left += p.x;
        c.right += p.x;
        c.top += p.y;
        c.bottom += p.y;
    }

    if(wp.showCmd != SW_SHOWMINIMIZED)
    {
        if (m_lpDX->m_client_width  != c.right-c.left ||
            m_lpDX->m_client_height != c.bottom-c.top)
        {
            CleanUpDX8Stuff(0);
            if (!m_lpDX->OnUserResizeWindow(&w, &c))
            {
                // note: a basic warning messagebox will have already been given.
                // now suggest specific advice on how to regain more video memory:
                SuggestHowToFreeSomeMem();
                return;
            }
            if (!AllocateDX8Stuff())
            {
                m_lpDX->m_ready = false;   // flag to exit
                return;
            }
        }

        // save the new window position:
        if (wp.showCmd==SW_SHOWNORMAL)
            m_lpDX->SaveWindow();
    }
#endif
}

void CPluginShell::StuffParams(DXCONTEXT_PARAMS *pParams)
{
    pParams->screenmode   = m_screenmode;
    pParams->display_mode = m_disp_mode_fs;
    pParams->nbackbuf     = 1;
    pParams->m_dualhead_horz = m_dualhead_horz;
    pParams->m_dualhead_vert = m_dualhead_vert;
    pParams->m_skin = (m_screenmode==WINDOWED) ? m_skin : 0;
    switch(m_screenmode)
    {
    case WINDOWED:
        pParams->allow_page_tearing = m_allow_page_tearing_w;
        pParams->adapter_guid       = m_adapter_guid_windowed;
        pParams->multisamp          = m_multisample_windowed;
        break;
    case FULLSCREEN:
    case FAKE_FULLSCREEN:
        pParams->allow_page_tearing = m_allow_page_tearing_fs;
        pParams->adapter_guid       = m_adapter_guid_fullscreen;
        pParams->multisamp          = m_multisample_fullscreen;
        break;
    case DESKTOP:
        pParams->allow_page_tearing = m_allow_page_tearing_dm;
        pParams->adapter_guid       = m_adapter_guid_desktop;
        pParams->multisamp          = m_multisample_desktop;
        break;
    }
    pParams->parent_window = (m_screenmode==DESKTOP) ? m_hWndDesktopListView : NULL;
}

void CPluginShell::ToggleDesktop()
{
    CleanUpDX8Stuff(0);

    switch(m_screenmode)
    {
    case WINDOWED:
    case FULLSCREEN:
    case FAKE_FULLSCREEN:
        m_screenmode = DESKTOP;
        break;
    case DESKTOP:
        m_screenmode = WINDOWED;
        break;
    }

    DXCONTEXT_PARAMS params;
    StuffParams(&params);

    if (!m_lpDX->StartOrRestartDevice(&params))
    {
        // note: a basic warning messagebox will have already been given.
        if (m_lpDX->m_lastErr == DXC_ERR_CREATEDEV_PROBABLY_OUTOFVIDEOMEMORY)
            SuggestHowToFreeSomeMem();
        return;
    }

    if (!AllocateDX8Stuff())
    {
        m_lpDX->m_ready = false;   // flag to exit
        return;
    }

//    SetForegroundWindow(m_lpDX->GetHwnd());
//    SetActiveWindow(m_lpDX->GetHwnd());
//    SetFocus(m_lpDX->GetHwnd());
}

#define IPC_IS_PLAYING_VIDEO 501 // from wa_ipc.h
#define IPC_SET_VIS_FS_FLAG 631 // a vis should send this message with 1/as param to notify winamp that it has gone to or has come back from fullscreen mode

void CPluginShell::ToggleFullScreen()
{
#if 0
    CleanUpDX8Stuff(0);

    switch(m_screenmode)
    {
    case DESKTOP:
    case WINDOWED:
        m_screenmode = m_fake_fullscreen_mode ? FAKE_FULLSCREEN : FULLSCREEN;
        if (m_screenmode == FULLSCREEN && SendMessage(GetWinampWindow(),WM_WA_IPC,0,IPC_IS_PLAYING_VIDEO)>1)
        {
          m_screenmode=FAKE_FULLSCREEN;
        }
        SendMessage(GetWinampWindow(),WM_WA_IPC,1,IPC_SET_VIS_FS_FLAG);
        break;
    case FULLSCREEN:
    case FAKE_FULLSCREEN:
        m_screenmode = WINDOWED;
        SendMessage(GetWinampWindow(),WM_WA_IPC,0,IPC_SET_VIS_FS_FLAG);
        break;
    }

    DXCONTEXT_PARAMS params;
    StuffParams(&params);

    if (!m_lpDX->StartOrRestartDevice(&params))
    {
        // note: a basic warning messagebox will have already been given.
        if (m_lpDX->m_lastErr == DXC_ERR_CREATEDEV_PROBABLY_OUTOFVIDEOMEMORY)
            SuggestHowToFreeSomeMem();
        return;
    }

    if (!AllocateDX8Stuff())
    {
        m_lpDX->m_ready = false;   // flag to exit
        return;
    }

    SetForegroundWindow(m_lpDX->GetHwnd());
    SetActiveWindow(m_lpDX->GetHwnd());
    SetFocus(m_lpDX->GetHwnd());
#endif
}

void CPluginShell::ToggleHelp()
{
    m_show_help = 1-m_show_help;
//    int ret = CheckMenuItem( m_context_menu, ID_SHOWHELP, MF_BYCOMMAND | (m_show_help ? MF_CHECKED : MF_UNCHECKED) );
}

void CPluginShell::TogglePlaylist()
{
    m_show_playlist = 1-m_show_playlist;
    m_playlist_top_idx = -1;    // <- invalidates playlist cache
//    int ret = CheckMenuItem( m_context_menu, ID_SHOWPLAYLIST, MF_BYCOMMAND | (m_show_playlist ? MF_CHECKED : MF_UNCHECKED) );
}

int CPluginShell::InitDirectX()
{

//    m_lpDX = new DXContext(m_hWndWinamp,m_hInstance,CLASSNAME,WINDOWCAPTION,CPluginShell::WindowProc,(LONG)this, m_minimize_winamp, m_szConfigIniFile);
  m_lpDX = new DXContext(m_device, m_szConfigIniFile);
    if (!m_lpDX)
    {
//        MessageBox(NULL, "Unable to initialize DXContext;\rprobably out of memory.", "ERROR", MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
        return FALSE;
    }

    if (m_lpDX->m_lastErr != S_OK)
    {
        // warning messagebox will have already been given
        delete m_lpDX;
        return FALSE;
    }

    // initialize graphics
    DXCONTEXT_PARAMS params;
    StuffParams(&params);

    if (!m_lpDX->StartOrRestartDevice(&params))
    {
        // note: a basic warning messagebox will have already been given.

        if (m_lpDX->m_lastErr == DXC_ERR_CREATEDEV_PROBABLY_OUTOFVIDEOMEMORY)
        {
            // suggest specific advice on how to regain more video memory:
            SuggestHowToFreeSomeMem();
        }

        delete m_lpDX;
        m_lpDX = NULL;
        return FALSE;
    }

    return TRUE;
}

void CPluginShell::CleanUpDirectX()
{
    SafeDelete(m_lpDX);
}

int CPluginShell::PluginPreInitialize(HWND hWinampWnd, HINSTANCE hWinampInstance)
{

    // PROTECTED CONFIG PANEL SETTINGS (also see 'private' settings, below)
    m_start_fullscreen      = 0;
    m_start_desktop         = 0;
    m_fake_fullscreen_mode  = 0;
    m_max_fps_fs            = 30;
    m_max_fps_dm            = 30;
    m_max_fps_w             = 30;
    m_show_press_f1_msg     = 1;
    m_allow_page_tearing_w  = 1;
    m_allow_page_tearing_fs = 0;
    m_allow_page_tearing_dm = 0;
    m_minimize_winamp       = 1;
    m_desktop_show_icons    = 1;
    m_desktop_textlabel_boxes = 1;
    m_desktop_manual_icon_scoot = 0;
    m_desktop_555_fix       = 2;
    m_dualhead_horz         = 2;
    m_dualhead_vert         = 1;
    m_save_cpu              = 1;
    m_skin                  = 1;
    m_fix_slow_text         = 1;

    // initialize font settings:
    strcpy(m_fontinfo[SIMPLE_FONT    ].szFace,        SIMPLE_FONT_DEFAULT_FACE );
           m_fontinfo[SIMPLE_FONT    ].nSize        = SIMPLE_FONT_DEFAULT_SIZE ;
           m_fontinfo[SIMPLE_FONT    ].bBold        = SIMPLE_FONT_DEFAULT_BOLD ;
           m_fontinfo[SIMPLE_FONT    ].bItalic      = SIMPLE_FONT_DEFAULT_ITAL ;
           m_fontinfo[SIMPLE_FONT    ].bAntiAliased = SIMPLE_FONT_DEFAULT_AA   ;
    strcpy(m_fontinfo[DECORATIVE_FONT].szFace,        DECORATIVE_FONT_DEFAULT_FACE);
           m_fontinfo[DECORATIVE_FONT].nSize        = DECORATIVE_FONT_DEFAULT_SIZE;
           m_fontinfo[DECORATIVE_FONT].bBold        = DECORATIVE_FONT_DEFAULT_BOLD;
           m_fontinfo[DECORATIVE_FONT].bItalic      = DECORATIVE_FONT_DEFAULT_ITAL;
           m_fontinfo[DECORATIVE_FONT].bAntiAliased = DECORATIVE_FONT_DEFAULT_AA  ;
    strcpy(m_fontinfo[HELPSCREEN_FONT].szFace,        HELPSCREEN_FONT_DEFAULT_FACE);
           m_fontinfo[HELPSCREEN_FONT].nSize        = HELPSCREEN_FONT_DEFAULT_SIZE;
           m_fontinfo[HELPSCREEN_FONT].bBold        = HELPSCREEN_FONT_DEFAULT_BOLD;
           m_fontinfo[HELPSCREEN_FONT].bItalic      = HELPSCREEN_FONT_DEFAULT_ITAL;
           m_fontinfo[HELPSCREEN_FONT].bAntiAliased = HELPSCREEN_FONT_DEFAULT_AA  ;
    strcpy(m_fontinfo[PLAYLIST_FONT  ].szFace,        PLAYLIST_FONT_DEFAULT_FACE);
           m_fontinfo[PLAYLIST_FONT  ].nSize        = PLAYLIST_FONT_DEFAULT_SIZE;
           m_fontinfo[PLAYLIST_FONT  ].bBold        = PLAYLIST_FONT_DEFAULT_BOLD;
           m_fontinfo[PLAYLIST_FONT  ].bItalic      = PLAYLIST_FONT_DEFAULT_ITAL;
           m_fontinfo[PLAYLIST_FONT  ].bAntiAliased = PLAYLIST_FONT_DEFAULT_AA  ;

    #if (NUM_EXTRA_FONTS >= 1)
        strcpy(m_fontinfo[NUM_BASIC_FONTS + 0].szFace,        EXTRA_FONT_1_DEFAULT_FACE);
               m_fontinfo[NUM_BASIC_FONTS + 0].nSize        = EXTRA_FONT_1_DEFAULT_SIZE;
               m_fontinfo[NUM_BASIC_FONTS + 0].bBold        = EXTRA_FONT_1_DEFAULT_BOLD;
               m_fontinfo[NUM_BASIC_FONTS + 0].bItalic      = EXTRA_FONT_1_DEFAULT_ITAL;
               m_fontinfo[NUM_BASIC_FONTS + 0].bAntiAliased = EXTRA_FONT_1_DEFAULT_AA;
    #endif
    #if (NUM_EXTRA_FONTS >= 2)
        strcpy(m_fontinfo[NUM_BASIC_FONTS + 1].szFace,        EXTRA_FONT_2_DEFAULT_FACE);
               m_fontinfo[NUM_BASIC_FONTS + 1].nSize        = EXTRA_FONT_2_DEFAULT_SIZE;
               m_fontinfo[NUM_BASIC_FONTS + 1].bBold        = EXTRA_FONT_2_DEFAULT_BOLD;
               m_fontinfo[NUM_BASIC_FONTS + 1].bItalic      = EXTRA_FONT_2_DEFAULT_ITAL;
               m_fontinfo[NUM_BASIC_FONTS + 1].bAntiAliased = EXTRA_FONT_2_DEFAULT_AA;
    #endif
    #if (NUM_EXTRA_FONTS >= 3)
        strcpy(m_fontinfo[NUM_BASIC_FONTS + 2].szFace,        EXTRA_FONT_3_DEFAULT_FACE);
               m_fontinfo[NUM_BASIC_FONTS + 2].nSize        = EXTRA_FONT_3_DEFAULT_SIZE;
               m_fontinfo[NUM_BASIC_FONTS + 2].bBold        = EXTRA_FONT_3_DEFAULT_BOLD;
               m_fontinfo[NUM_BASIC_FONTS + 2].bItalic      = EXTRA_FONT_3_DEFAULT_ITAL;
               m_fontinfo[NUM_BASIC_FONTS + 2].bAntiAliased = EXTRA_FONT_3_DEFAULT_AA;
    #endif
    #if (NUM_EXTRA_FONTS >= 4)
        strcpy(m_fontinfo[NUM_BASIC_FONTS + 3].szFace,        EXTRA_FONT_4_DEFAULT_FACE);
               m_fontinfo[NUM_BASIC_FONTS + 3].nSize        = EXTRA_FONT_4_DEFAULT_SIZE;
               m_fontinfo[NUM_BASIC_FONTS + 3].bBold        = EXTRA_FONT_4_DEFAULT_BOLD;
               m_fontinfo[NUM_BASIC_FONTS + 3].bItalic      = EXTRA_FONT_4_DEFAULT_ITAL;
               m_fontinfo[NUM_BASIC_FONTS + 3].bAntiAliased = EXTRA_FONT_4_DEFAULT_AA;
    #endif
    #if (NUM_EXTRA_FONTS >= 5)
        strcpy(m_fontinfo[NUM_BASIC_FONTS + 4].szFace,        EXTRA_FONT_5_DEFAULT_FACE);
               m_fontinfo[NUM_BASIC_FONTS + 4].nSize        = EXTRA_FONT_5_DEFAULT_SIZE;
               m_fontinfo[NUM_BASIC_FONTS + 4].bBold        = EXTRA_FONT_5_DEFAULT_BOLD;
               m_fontinfo[NUM_BASIC_FONTS + 4].bItalic      = EXTRA_FONT_5_DEFAULT_ITAL;
               m_fontinfo[NUM_BASIC_FONTS + 4].bAntiAliased = EXTRA_FONT_5_DEFAULT_AA;
    #endif

    m_disp_mode_fs.Width = DEFAULT_FULLSCREEN_WIDTH;
    m_disp_mode_fs.Height = DEFAULT_FULLSCREEN_HEIGHT;
    m_disp_mode_fs.Format = D3DFMT_UNKNOWN;
    m_disp_mode_fs.RefreshRate = 60;

    // PROTECTED STRUCTURES/POINTERS
//    for (int i=0; i<NUM_BASIC_FONTS + NUM_EXTRA_FONTS; i++)
//        m_d3dx_font[i] = NULL;
//    m_d3dx_desktop_font = NULL;
    m_lpDDSText = NULL;
    ZeroMemory(&m_sound, sizeof(td_soundinfo));
    for (int ch=0; ch<2; ch++)
        for (int i=0; i<3; i++)
        {
            m_sound.infinite_avg[ch][i] = m_sound.avg[ch][i] = m_sound.med_avg[ch][i] = m_sound.long_avg[ch][i] = 1.0f;
        }

    // GENERAL PRIVATE STUFF
    //m_screenmode: set at end (derived setting)
    m_frame = 0;
    m_time  = 0;
    m_fps   = 30;
    m_hWndWinamp = hWinampWnd;
    m_hInstance  = hWinampInstance;
    m_lpDX       = NULL;
    m_szPluginsDirPath[0] = 0;  // will be set further down
    m_szConfigIniFile[0]  = 0;  // will be set further down
    // m_szPluginsDirPath:
    {
        // get path to INI file & read in prefs/settings right away, so DumpMsg works!
//        GetModuleFileName(m_hInstance,m_szPluginsDirPath,MAX_PATH);
//        char *p = m_szPluginsDirPath + strlen(m_szPluginsDirPath);
//        while (p >= m_szPluginsDirPath && *p != '\\') p--;
//        if (++p >= m_szPluginsDirPath) *p = 0;
		sprintf(m_szPluginsDirPath, "special://xbmc/addons/");
//		sprintf(m_szPluginsDirPath, "d:\\");
	}
    sprintf(m_szConfigIniFile, "%s%s", m_szPluginsDirPath, INIFILE);

    // PRIVATE CONFIG PANEL SETTINGS
    m_multisample_fullscreen      = D3DMULTISAMPLE_NONE;
    m_multisample_desktop         = D3DMULTISAMPLE_NONE;
    m_multisample_windowed        = D3DMULTISAMPLE_NONE;
    ZeroMemory(&m_adapter_guid_fullscreen, sizeof(GUID));
    ZeroMemory(&m_adapter_guid_desktop   , sizeof(GUID));
    ZeroMemory(&m_adapter_guid_windowed  , sizeof(GUID));

    // PRIVATE RUNTIME SETTINGS
    m_lost_focus = 0;
    m_hidden     = 0;
    m_resizing   = 0;
    m_show_help  = 0;
    m_show_playlist = 0;
    m_playlist_pos = 0;
    m_playlist_pageups = 0;
    m_playlist_top_idx = -1;
    m_playlist_btm_idx = -1;
    // m_playlist_width_pixels will be considered invalid whenever 'm_playlist_top_idx' is -1.
    // m_playlist[256][256] will be considered invalid whenever 'm_playlist_top_idx' is -1.
    m_exiting    = 0;
    m_upper_left_corner_y = 0;
    m_lower_left_corner_y = 0;
    m_upper_right_corner_y = 0;
    m_lower_right_corner_y = 0;
    m_left_edge = 0;
    m_right_edge = 0;
    m_force_accept_WM_WINDOWPOSCHANGING = 0;

    // PRIVATE - GDI STUFF
    m_main_menu     = NULL;
    m_context_menu  = NULL;
//    for (i=0; i<NUM_BASIC_FONTS + NUM_EXTRA_FONTS; i++)
//        m_font[i] = NULL;
//    m_font_desktop = NULL;

    // PRIVATE - DESKTOP MODE STUFF
//    m_icon_list.clear();
//    for (int i=0; i<MAX_ICON_TEXTURES; i++)
//        m_desktop_icons_texture[i] = NULL;
//    FindDesktopWindows(&m_hWndProgMan, &m_hWndDesktop, &m_hWndDesktopListView);
//    GetDesktopFolder(m_szDesktopFolder);
    m_desktop_icon_size       = 32;
    m_desktop_dragging        = 0;   // '1' when user is dragging icons around
    m_desktop_box             = 0;   // '1' when user is drawing a box
    m_desktop_wc_registered   = 0;
//    m_desktop_bk_color        = 0xFF000000 | BGR2RGB(::GetSysColor(COLOR_BACKGROUND));
//    m_desktop_text_color      = 0xFF000000 | BGR2RGB(SendMessage(m_hWndDesktopListView, LVM_GETTEXTCOLOR, 0, 0));
//    m_desktop_sel_color       = 0xFF000000 | BGR2RGB(::GetSysColor(COLOR_HIGHLIGHT));
//    m_desktop_sel_text_color  = 0xFF000000 | BGR2RGB(::GetSysColor(COLOR_HIGHLIGHTTEXT));
    m_desktop_icon_state      = 0;
    m_desktop_icon_count      = 0;
    m_desktop_icon_update_frame = 0;
    m_desktop_icons_disabled  = 0;
    m_vms_desktop_loaded      = 0;
    m_desktop_hook_set        = 0;

    // PRIVATE - MORE TIMEKEEPING
    m_last_raw_time = 0;
    memset(m_time_hist, 0, sizeof(m_time_hist));
    m_time_hist_pos = 0;
    if (!QueryPerformanceFrequency(&m_high_perf_timer_freq))
        m_high_perf_timer_freq.QuadPart = 0;
    m_prev_end_of_frame.QuadPart = 0;

    // PRIVATE AUDIO PROCESSING DATA
    //(m_fftobj needs no init)
    memset(m_oldwave[0], 0, sizeof(float)*512);
    memset(m_oldwave[1], 0, sizeof(float)*512);
    m_prev_align_offset[0] = 0;
    m_prev_align_offset[1] = 0;
    m_align_weights_ready = 0;

    // SEPARATE TEXT WINDOW (FOR VJ MODE)
	m_vj_mode       = 0;
    m_hidden_textwnd = 0;
    m_resizing_textwnd = 0;
	m_hTextWnd		= NULL;
	m_nTextWndWidth = 0;
	m_nTextWndHeight = 0;
	m_bTextWindowClassRegistered = false;
    m_vjd3d8        = NULL;
    m_vjd3d8_device = NULL;

    //-----

    m_screenmode = NOT_YET_KNOWN;

    OverrideDefaults();
    ReadConfig();

//    if (m_start_fullscreen)
    {
        m_screenmode = m_fake_fullscreen_mode ? FAKE_FULLSCREEN : FULLSCREEN;
//        if (m_screenmode == FULLSCREEN && SendMessage(GetWinampWindow(),WM_WA_IPC,0,IPC_IS_PLAYING_VIDEO)>1)
//        {
 //         m_screenmode=FAKE_FULLSCREEN;
 //       }
    }
 //   else if (m_start_desktop)
 //       m_screenmode = DESKTOP;
 //   else
 //       m_screenmode = WINDOWED;

    MyPreInitialize();
    MyReadConfig();

    //-----

    return TRUE;
}

int CPluginShell::PluginInitialize(LPDIRECT3DDEVICE9 device, int iPosX, int iPosY, int iWidth, int iHeight, float pixelRatio)
{
  // note: initialize GDI before DirectX.  Also separate them because
  // when we change windowed<->fullscreen, or lose the device and restore it,
  // we don't want to mess with any (persistent) GDI stuff.
  m_device = device;
  if (!InitDirectX())        return FALSE;  // gives its own error messages
  m_lpDX->m_client_width = iWidth;
  m_lpDX->m_client_height = iHeight;
  m_posX = iPosX;
  m_posY = iPosY;
  m_pixelRatio = pixelRatio;

  if (!InitNonDx8Stuff())    return FALSE;  // gives its own error messages
  if (!InitVJStuff())        return FALSE;
  if (!AllocateDX8Stuff())   return FALSE;  // gives its own error messages

  return TRUE;
}

void CPluginShell::PluginQuit()
{
    CleanUpDX8Stuff(1);
    CleanUpVJStuff();
    CleanUpNonDx8Stuff();
    CleanUpDirectX();
    CleanUpMyDX8Stuff(1);

//    SetFocus(m_hWndWinamp);
//    SetActiveWindow(m_hWndWinamp);
//    SetForegroundWindow(m_hWndWinamp);
}

void CPluginShell::ReadConfig()
{
#if 0
    int old_ver    = InternalGetPrivateProfileInt("settings","version"   ,-1,m_szConfigIniFile);
    int old_subver = InternalGetPrivateProfileInt("settings","subversion",-1,m_szConfigIniFile);

    // nuke old settings from prev. version:
    if (old_ver < INT_VERSION)
        return;
    else if (old_subver < INT_SUBVERSION)
        return;

    //D3DMULTISAMPLE_TYPE m_multisample_fullscreen;
    //D3DMULTISAMPLE_TYPE m_multisample_desktop;
    //D3DMULTISAMPLE_TYPE m_multisample_windowed;
    m_multisample_fullscreen      = (D3DMULTISAMPLE_TYPE)InternalGetPrivateProfileInt("settings","multisample_fullscreen",m_multisample_fullscreen,m_szConfigIniFile);
    m_multisample_desktop         = (D3DMULTISAMPLE_TYPE)InternalGetPrivateProfileInt("settings","multisample_desktop",m_multisample_desktop,m_szConfigIniFile);
    m_multisample_windowed        = (D3DMULTISAMPLE_TYPE)InternalGetPrivateProfileInt("settings","multisample_windowed"  ,m_multisample_windowed  ,m_szConfigIniFile);

    //GUID m_adapter_guid_fullscreen
    //GUID m_adapter_guid_desktop
    //GUID m_adapter_guid_windowed
    char str[256];
    InternalGetPrivateProfileString("settings","adapter_guid_fullscreen","",str,sizeof(str)-1,m_szConfigIniFile);
    TextToGuid(str, &m_adapter_guid_fullscreen);
    InternalGetPrivateProfileString("settings","adapter_guid_desktop","",str,sizeof(str)-1,m_szConfigIniFile);
    TextToGuid(str, &m_adapter_guid_desktop);
    InternalGetPrivateProfileString("settings","adapter_guid_windowed","",str,sizeof(str)-1,m_szConfigIniFile);
    TextToGuid(str, &m_adapter_guid_windowed);

	/*
    // FONTS
    #define READ_FONT(n) { \
        InternalGetPrivateProfileString("settings","szFontFace"#n,m_fontinfo[n].szFace,m_fontinfo[n].szFace,sizeof(m_fontinfo[n].szFace), m_szConfigIniFile); \
	    m_fontinfo[n].nSize   = InternalGetPrivateProfileInt("settings","nFontSize"#n  ,m_fontinfo[n].nSize  ,m_szConfigIniFile); \
	    m_fontinfo[n].bBold   = InternalGetPrivateProfileInt("settings","bFontBold"#n  ,m_fontinfo[n].bBold  ,m_szConfigIniFile); \
	    m_fontinfo[n].bItalic = InternalGetPrivateProfileInt("settings","bFontItalic"#n,m_fontinfo[n].bItalic,m_szConfigIniFile); \
	    m_fontinfo[n].bAntiAliased = InternalGetPrivateProfileInt("settings","bFontAA"#n,m_fontinfo[n].bItalic,m_szConfigIniFile); \
    }
    READ_FONT(0);
    READ_FONT(1);
    READ_FONT(2);
    READ_FONT(3);
    #if (NUM_EXTRA_FONTS >= 1)
        READ_FONT(4);
    #endif
    #if (NUM_EXTRA_FONTS >= 2)
        READ_FONT(5);
    #endif
    #if (NUM_EXTRA_FONTS >= 3)
        READ_FONT(6);
    #endif
    #if (NUM_EXTRA_FONTS >= 4)
        READ_FONT(7);
    #endif
    #if (NUM_EXTRA_FONTS >= 5)
        READ_FONT(8);
    #endif
*/
    m_start_fullscreen     = InternalGetPrivateProfileInt("settings","start_fullscreen",m_start_fullscreen,m_szConfigIniFile);
    m_start_desktop        = InternalGetPrivateProfileInt("settings","start_desktop"   ,m_start_desktop   ,m_szConfigIniFile);
    m_fake_fullscreen_mode = InternalGetPrivateProfileInt("settings","fake_fullscreen_mode",m_fake_fullscreen_mode,m_szConfigIniFile);
    m_max_fps_fs           = InternalGetPrivateProfileInt("settings","max_fps_fs",m_max_fps_fs,m_szConfigIniFile);
    m_max_fps_dm           = InternalGetPrivateProfileInt("settings","max_fps_dm",m_max_fps_dm,m_szConfigIniFile);
    m_max_fps_w            = InternalGetPrivateProfileInt("settings","max_fps_w" ,m_max_fps_w ,m_szConfigIniFile);
    m_show_press_f1_msg    = InternalGetPrivateProfileInt("settings","show_press_f1_msg",m_show_press_f1_msg,m_szConfigIniFile);
    m_allow_page_tearing_w = InternalGetPrivateProfileInt("settings","allow_page_tearing_w",m_allow_page_tearing_w,m_szConfigIniFile);
    m_allow_page_tearing_fs= InternalGetPrivateProfileInt("settings","allow_page_tearing_fs",m_allow_page_tearing_fs,m_szConfigIniFile);
    m_allow_page_tearing_dm= InternalGetPrivateProfileInt("settings","allow_page_tearing_dm",m_allow_page_tearing_dm,m_szConfigIniFile);
    m_minimize_winamp      = InternalGetPrivateProfileInt("settings","minimize_winamp",m_minimize_winamp,m_szConfigIniFile);
    m_desktop_show_icons   = InternalGetPrivateProfileInt("settings","desktop_show_icons",m_desktop_show_icons,m_szConfigIniFile);
    m_desktop_textlabel_boxes = InternalGetPrivateProfileInt("settings","desktop_textlabel_boxes",m_desktop_textlabel_boxes,m_szConfigIniFile);
    m_desktop_manual_icon_scoot = InternalGetPrivateProfileInt("settings","desktop_manual_icon_scoot",m_desktop_manual_icon_scoot,m_szConfigIniFile);
    m_desktop_555_fix      = InternalGetPrivateProfileInt("settings","desktop_555_fix",m_desktop_555_fix,m_szConfigIniFile);
    m_dualhead_horz        = InternalGetPrivateProfileInt("settings","dualhead_horz",m_dualhead_horz,m_szConfigIniFile);
    m_dualhead_vert        = InternalGetPrivateProfileInt("settings","dualhead_vert",m_dualhead_vert,m_szConfigIniFile);
    m_save_cpu             = InternalGetPrivateProfileInt("settings","save_cpu",m_save_cpu,m_szConfigIniFile);
    m_skin                 = InternalGetPrivateProfileInt("settings","skin",m_skin,m_szConfigIniFile);
    m_fix_slow_text        = InternalGetPrivateProfileInt("settings","fix_slow_text",m_fix_slow_text,m_szConfigIniFile);
	m_vj_mode              = GetPrivateProfileBool("settings","vj_mode",m_vj_mode,m_szConfigIniFile);

    //D3DDISPLAYMODE m_fs_disp_mode
    m_disp_mode_fs.Width           = InternalGetPrivateProfileInt("settings","disp_mode_fs_w",m_disp_mode_fs.Width          ,m_szConfigIniFile);
    m_disp_mode_fs.Height           = InternalGetPrivateProfileInt("settings","disp_mode_fs_h",m_disp_mode_fs.Height          ,m_szConfigIniFile);
    m_disp_mode_fs.RefreshRate = InternalGetPrivateProfileInt("settings","disp_mode_fs_r",m_disp_mode_fs.RefreshRate,m_szConfigIniFile);
    m_disp_mode_fs.Format      = (D3DFORMAT)InternalGetPrivateProfileInt("settings","disp_mode_fs_f",m_disp_mode_fs.Format     ,m_szConfigIniFile);

    // note: we don't call MyReadConfig() yet, because we
    // want to completely finish CPluginShell's preinit (and ReadConfig)
    // before calling CPlugin's preinit and ReadConfig.

#endif
}

void CPluginShell::WriteConfig()
{
#if 0
    //D3DMULTISAMPLE_TYPE m_multisample_fullscreen;
    //D3DMULTISAMPLE_TYPE m_multisample_desktop;
    //D3DMULTISAMPLE_TYPE m_multisample_windowed;
    WritePrivateProfileInt((int)m_multisample_fullscreen,"multisample_fullscreen",m_szConfigIniFile,"settings");
    WritePrivateProfileInt((int)m_multisample_desktop   ,"multisample_desktop"   ,m_szConfigIniFile,"settings");
    WritePrivateProfileInt((int)m_multisample_windowed  ,"multisample_windowed"  ,m_szConfigIniFile,"settings");

    //GUID m_adapter_guid_fullscreen
    //GUID m_adapter_guid_desktop
    //GUID m_adapter_guid_windowed
    char str[256];
    GuidToText(&m_adapter_guid_fullscreen, str, sizeof(str));
    WritePrivateProfileString("settings","adapter_guid_fullscreen",str,m_szConfigIniFile);
    GuidToText(&m_adapter_guid_desktop, str, sizeof(str));
    WritePrivateProfileString("settings","adapter_guid_desktop",str,m_szConfigIniFile);
    GuidToText(&m_adapter_guid_windowed,   str, sizeof(str));
    WritePrivateProfileString("settings","adapter_guid_windowed"  ,str,m_szConfigIniFile);

    // FONTS
    #define WRITE_FONT(n) { \
	    WritePrivateProfileString("settings","szFontFace"#n,m_fontinfo[n].szFace,m_szConfigIniFile); \
	    WritePrivateProfileInt(m_fontinfo[n].bBold,  "bFontBold"#n,   m_szConfigIniFile, "settings"); \
	    WritePrivateProfileInt(m_fontinfo[n].bItalic,"bFontItalic"#n, m_szConfigIniFile, "settings"); \
	    WritePrivateProfileInt(m_fontinfo[n].nSize,  "nFontSize"#n,   m_szConfigIniFile, "settings"); \
	    WritePrivateProfileInt(m_fontinfo[n].bAntiAliased, "bFontAA"#n,m_szConfigIniFile, "settings"); \
    }
    WRITE_FONT(0);
    WRITE_FONT(1);
    WRITE_FONT(2);
    WRITE_FONT(3);
    #if (NUM_EXTRA_FONTS >= 1)
        WRITE_FONT(4);
    #endif
    #if (NUM_EXTRA_FONTS >= 2)
        WRITE_FONT(5);
    #endif
    #if (NUM_EXTRA_FONTS >= 3)
        WRITE_FONT(6);
    #endif
    #if (NUM_EXTRA_FONTS >= 4)
        WRITE_FONT(7);
    #endif
    #if (NUM_EXTRA_FONTS >= 5)
        WRITE_FONT(8);
    #endif

    WritePrivateProfileInt(m_start_fullscreen,"start_fullscreen",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_start_desktop   ,"start_desktop"   ,m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_fake_fullscreen_mode,"fake_fullscreen_mode",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_max_fps_fs,"max_fps_fs",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_max_fps_dm,"max_fps_dm",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_max_fps_w ,"max_fps_w" ,m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_show_press_f1_msg,"show_press_f1_msg",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_allow_page_tearing_w,"allow_page_tearing_w",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_allow_page_tearing_fs,"allow_page_tearing_fs",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_allow_page_tearing_dm,"allow_page_tearing_dm",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_minimize_winamp,"minimize_winamp",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_desktop_show_icons,"desktop_show_icons",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_desktop_textlabel_boxes,"desktop_textlabel_boxes",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_desktop_manual_icon_scoot,"desktop_manual_icon_scoot",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_desktop_555_fix,"desktop_555_fix",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_dualhead_horz,"dualhead_horz",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_dualhead_vert,"dualhead_vert",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_save_cpu,"save_cpu",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_skin,"skin",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_fix_slow_text,"fix_slow_text",m_szConfigIniFile,"settings");
	WritePrivateProfileInt(m_vj_mode,"vj_mode",m_szConfigIniFile,"settings");

    //D3DDISPLAYMODE m_fs_disp_mode
    WritePrivateProfileInt(m_disp_mode_fs.Width          ,"disp_mode_fs_w",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_disp_mode_fs.Height          ,"disp_mode_fs_h",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_disp_mode_fs.RefreshRate,"disp_mode_fs_r",m_szConfigIniFile,"settings");
    WritePrivateProfileInt(m_disp_mode_fs.Format     ,"disp_mode_fs_f",m_szConfigIniFile,"settings");

    WritePrivateProfileInt(INT_VERSION            ,"version"    ,m_szConfigIniFile,"settings");
    WritePrivateProfileInt(INT_SUBVERSION         ,"subversion" ,m_szConfigIniFile,"settings");

    // finally, save the plugin's unique settings:
    MyWriteConfig();

#endif
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

int CPluginShell::PluginRender(unsigned char *pWaveL, unsigned char *pWaveR)//, unsigned char *pSpecL, unsigned char *pSpecR)
{
    // return FALSE here to tell Winamp to terminate the plugin

    if (!m_lpDX || !m_lpDX->m_ready)
    {
        // note: 'm_ready' will go false when a device reset fatally fails
        //       (for example, when user resizes window, or toggles fullscreen.)
        m_exiting = 1;
        return false;   // EXIT THE PLUGIN
    }

//    if (m_hTextWnd)
//        m_lost_focus = ((GetFocus() != GetPluginWindow()) && (GetFocus() != m_hTextWnd));
//    else
//        m_lost_focus = (GetFocus() != GetPluginWindow());

//    if ((m_screenmode==WINDOWED   && m_hidden) ||
//        //(m_screenmode==FULLSCREEN && m_lost_focus) ||
//        (m_screenmode==WINDOWED   && m_resizing)
//        )
//    {
//        Sleep(30);
//        return true;
//    }

    // test for lost device
    // (this happens when device is fullscreen & user alt-tabs away,
    //  or when monitor power-saving kicks in)
    //HRESULT hr = m_lpDX->m_lpDevice->TestCooperativeLevel();
    //if (hr == D3DERR_DEVICENOTRESET)
    //{
    //    // device WAS lost, and is now ready to be reset (and come back online):
    //    CleanUpDX8Stuff(0);
    //    if (m_lpDX->m_lpDevice->Reset(&m_lpDX->m_d3dpp) != D3D_OK)
    //    {
    //        // note: a basic warning messagebox will have already been given.
    //        // now suggest specific advice on how to regain more video memory:
    //        if (m_lpDX->m_lastErr == DXC_ERR_CREATEDEV_PROBABLY_OUTOFVIDEOMEMORY)
    //            SuggestHowToFreeSomeMem();
    //        return false;  // EXIT THE PLUGIN
    //    }
    //    if (!AllocateDX8Stuff())
    //        return false;  // EXIT THE PLUGIN
    //}
    //else if (hr != D3D_OK)
    //{
    //    // device is lost, and not yet ready to come back; sleep.
    //    Sleep(30);
    //    return true;
    //}

//    if (m_vjd3d8_device)
//    {
//        HRESULT hr = m_vjd3d8_device->TestCooperativeLevel();
//        if (hr == D3DERR_DEVICENOTRESET)
//        {
//            RECT c;
//            GetClientRect(m_hTextWnd, &c);
//
//            POINT p;
//            p.x = c.left;
//            p.y = c.top;
//            if (ClientToScreen(m_hTextWnd, &p))
//            {
//                c.left += p.x;
//                c.right += p.x;
//                c.top += p.y;
//                c.bottom += p.y;
//            }
//
//            CleanUpVJStuff();
//            if (!InitVJStuff(&c))
//                return false;  // EXIT THE PLUGIN
//        }
//    }
//
//    if (m_screenmode==DESKTOP)
//    {
//        PushWindowToJustBeforeDesktop(GetPluginWindow());
//    }

    DoTime();
    AnalyzeNewSound(pWaveL, pWaveR);
    AlignWaves();

    DrawAndDisplay(0);

    EnforceMaxFPS();

    m_frame++;

    return true;
}

void CPluginShell::PushWindowToJustBeforeDesktop(HWND h)
{
#if 0
    // if our window isn't already at the bottom of the Z order,
    // freshly send it to HWND_BOTTOM.

    // this usually gives us the Program Manager window:
    HWND hWndBottom = GetWindow(h, GW_HWNDLAST);

    // then, bottommost 'normal' window is usually the one just in front of it:
    if (hWndBottom == m_hWndProgMan)
        hWndBottom = GetWindow(hWndBottom, GW_HWNDPREV);

    if (hWndBottom != h)
    {
        m_force_accept_WM_WINDOWPOSCHANGING = 1;
        SetWindowPos(h, HWND_BOTTOM, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
        m_force_accept_WM_WINDOWPOSCHANGING = 0;
    }
#endif
}

void CPluginShell::DrawAndDisplay(int redraw)
{
    int cx = m_vjd3d8_device ? m_nTextWndWidth  : m_lpDX->m_client_width;
    int cy = m_vjd3d8_device ? m_nTextWndHeight : m_lpDX->m_client_height;
/*

	if (m_lpDDSText)
    {
        D3DSURFACE_DESC desc;
        if (D3D_OK == m_lpDDSText->GetLevelDesc(0, &desc))
        {
            cx = min(cx, desc.Width);
            cy = min(cy, desc.Height);
        }
    }
*/
    m_upper_left_corner_y  = TEXT_MARGIN;
    m_upper_right_corner_y = TEXT_MARGIN;
    m_lower_left_corner_y  = cy - TEXT_MARGIN;
    m_lower_right_corner_y = cy - TEXT_MARGIN;
    m_left_edge            = TEXT_MARGIN;
    m_right_edge           = cx - TEXT_MARGIN;

	/*
    if (m_screenmode == DESKTOP || m_screenmode == FAKE_FULLSCREEN)
    {
        // check if taskbar is above plugin window;
        // if so, scoot text & icons out of the way.
        //     [...should always be true for Desktop Mode,
        //         but it's like this for code simplicity.]
        int taskbar_is_above_plugin_window = 1;
        HWND h = FindWindow("Shell_TrayWnd", NULL);
        while (h) //(..shouldn't be very many windows to iterate through here)
        {
            h = GetWindow(h, GW_HWNDPREV);
            if (h == GetPluginWindow())
            {
                taskbar_is_above_plugin_window = 0;
                break;
            }
        }

        if (taskbar_is_above_plugin_window)
        {
            // respect the taskbar area; make sure the text, desktop icons, etc.
            // don't appear underneath it.
            //m_upper_left_corner_y  += m_lpDX->m_monitor_work_rect.top - m_lpDX->m_monitor_rect.top;
            //m_upper_right_corner_y += m_lpDX->m_monitor_work_rect.top - m_lpDX->m_monitor_rect.top;
            //m_lower_left_corner_y  -= m_lpDX->m_monitor_rect.bottom - m_lpDX->m_monitor_work_rect.bottom;
            //m_lower_right_corner_y -= m_lpDX->m_monitor_rect.bottom - m_lpDX->m_monitor_work_rect.bottom;
            //m_left_edge  += m_lpDX->m_monitor_work_rect.left - m_lpDX->m_monitor_rect.left;
            //m_right_edge -= m_lpDX->m_monitor_rect.right - m_lpDX->m_monitor_work_rect.right;
            m_lpDX->UpdateMonitorWorkRect();
            m_upper_left_corner_y  = max(m_upper_left_corner_y , m_lpDX->m_monitor_work_rect.top - m_lpDX->m_monitor_rect.top + TEXT_MARGIN);
            m_upper_right_corner_y = max(m_upper_right_corner_y, m_lpDX->m_monitor_work_rect.top - m_lpDX->m_monitor_rect.top + TEXT_MARGIN);
            m_lower_left_corner_y  = min(m_lower_left_corner_y , m_lpDX->m_client_height - (m_lpDX->m_monitor_rect.bottom - m_lpDX->m_monitor_work_rect.bottom) - TEXT_MARGIN);
            m_lower_right_corner_y = min(m_lower_right_corner_y, m_lpDX->m_client_height - (m_lpDX->m_monitor_rect.bottom - m_lpDX->m_monitor_work_rect.bottom) - TEXT_MARGIN);
            m_left_edge  = max(m_left_edge , m_lpDX->m_monitor_work_rect.left - m_lpDX->m_monitor_rect.left + TEXT_MARGIN );
            m_right_edge = min(m_right_edge, m_lpDX->m_client_width - (m_lpDX->m_monitor_rect.right - m_lpDX->m_monitor_work_rect.right) + TEXT_MARGIN);
        }
    }
*/
    //if (D3D_OK==m_lpDX->m_lpDevice->BeginScene())
    {
        MyRenderFn(redraw);

        //PrepareFor2DDrawing_B(GetDevice(), GetWidth(), GetHeight());

        //RenderDesktop();
        //RenderBuiltInTextMsgs();
        //MyRenderUI(&m_upper_left_corner_y, &m_upper_right_corner_y, &m_lower_left_corner_y, &m_lower_right_corner_y, m_left_edge, m_right_edge);
        //RenderPlaylist();

//        if (!m_vjd3d8_device)
//        {
//            D3DXMATRIX Ortho2D;
//            D3DXMatrixOrthoLH(&Ortho2D, 2.0f, -2.0f, 0.0f, 1.0f);
//            m_lpDX->m_lpDevice->SetTransform(D3DTS_PROJECTION, &Ortho2D);
//
//            m_text.DrawNow();
//        }

       // m_lpDX->m_lpDevice->EndScene();
    }
/*
    // VJ Mode:
    if (m_vjd3d8_device && !m_hidden_textwnd && D3D_OK==m_vjd3d8_device->BeginScene())
    {
        if (!m_lpDDSText)
            m_vjd3d8_device->Clear(0, 0, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
        PrepareFor2DDrawing_B(m_vjd3d8_device, m_nTextWndWidth, m_nTextWndHeight);

        D3DXMATRIX Ortho2D;
        D3DXMatrixOrthoLH(&Ortho2D, 2.0f, -2.0f, 0.0f, 1.0f);
        m_vjd3d8_device->SetTransform(D3DTS_PROJECTION, &Ortho2D);

        m_text.DrawNow();

        m_vjd3d8_device->EndScene();
    }

    if (m_screenmode == DESKTOP)
    {
        // window is hidden after creation, until 1st frame is ready to go;
        // now that it's ready, we show it.
        // see dxcontext::Internal_Init()'s call to SetWindowPos() for the DESKTOP case.
        if (!IsWindowVisible(GetPluginWindow()))
            ShowWindow(GetPluginWindow(), SW_SHOWNORMAL);
    }

    m_lpDX->m_lpDevice->Present(NULL,NULL,NULL,NULL);
    if (m_vjd3d8_device && !m_hidden_textwnd)
        m_vjd3d8_device->Present(NULL,NULL,NULL,NULL);
*/
}

void CPluginShell::EnforceMaxFPS()
{
    int max_fps;
    switch(m_screenmode)
    {
    case WINDOWED:        max_fps = m_max_fps_w;  break;
    case FULLSCREEN:      max_fps = m_max_fps_fs; break;
    case FAKE_FULLSCREEN: max_fps = m_max_fps_fs; break;
    case DESKTOP:         max_fps = m_max_fps_dm; break;
    }

    if (max_fps <= 0)
        return;

    float fps_lo = (float)max_fps;
    float fps_hi = (float)max_fps;

    if (m_save_cpu)
    {
        // Find the optimal lo/hi bounds for the fps
        // that will result in a maximum difference,
        // in the time for a single frame, of 0.002 seconds -
        // the assumed granularity for Sleep(1) -

        // Using this range of acceptable fps
        // will allow us to do (sloppy) fps limiting
        // using only Sleep(1), and never the
        // second half of it: Sleep(0) in a tight loop,
        // which sucks up the CPU (whereas Sleep(1)
        // leaves it idle).

        // The original equation:
        //   1/(max_fps*t1) = 1/(max*fps/t1) - 0.002
        // where:
        //   t1 > 0
        //   max_fps*t1 is the upper range for fps
        //   max_fps/t1 is the lower range for fps

        float a = 1;
        float b = -0.002f * max_fps;
        float c = -1.0f;
        float det = b*b - 4*a*c;
        if (det>0)
        {
            float t1 = (-b + sqrtf(det)) / (2*a);
            //float t2 = (-b - sqrtf(det)) / (2*a);

            if (t1 > 1.0f)
            {
                fps_lo = max_fps / t1;
                fps_hi = max_fps * t1;
                // verify: now [1.0f/fps_lo - 1.0f/fps_hi] should equal 0.002 seconds.
                // note: allowing tolerance to go beyond these values for
                // fps_lo and fps_hi would gain nothing.
            }
        }
    }

    if (m_high_perf_timer_freq.QuadPart > 0)
    {
        LARGE_INTEGER t;
        QueryPerformanceCounter(&t);

        if (m_prev_end_of_frame.QuadPart != 0)
        {
            int ticks_to_wait_lo = (int)((float)m_high_perf_timer_freq.QuadPart / (float)fps_hi);
            int ticks_to_wait_hi = (int)((float)m_high_perf_timer_freq.QuadPart / (float)fps_lo);
            int done = 0;
            do
            {
                QueryPerformanceCounter(&t);

                int ticks_passed = (int)(t.QuadPart - m_prev_end_of_frame.QuadPart);
                //int ticks_left = ticks_to_wait - ticks_passed;

                if (t.QuadPart < m_prev_end_of_frame.QuadPart)    // time wrap
                    done = 1;
                if (ticks_passed >= ticks_to_wait_lo)
                    done = 1;

                if (!done)
                {
                    // if > 0.01s left, do Sleep(1), which will actually sleep some
                    //   steady amount of up to 2 ms (depending on the OS),
                    //   and do so in a nice way (cpu meter drops; laptop battery spared).
                    // otherwise, do a few Sleep(0)'s, which just give up the timeslice,
                    //   but don't really save cpu or battery, but do pass a tiny
                    //   amount of time.

                    //if (ticks_left > (int)m_high_perf_timer_freq.QuadPart/500)
                    if (ticks_to_wait_hi - ticks_passed > (int)m_high_perf_timer_freq.QuadPart/100)
                        Sleep(5);
                    else if (ticks_to_wait_hi - ticks_passed > (int)m_high_perf_timer_freq.QuadPart/1000)
                        Sleep(1);
                    else
                        for (int i=0; i<10; i++)
                            Sleep(0);  // causes thread to give up its timeslice
                }
            }
            while (!done);
        }

        m_prev_end_of_frame = t;
    }
    else
    {
        Sleep(1000/max_fps);
    }
}

void CPluginShell::DoTime()
{
    if (m_frame==0)
    {
        m_fps = 30;
        m_time = 0;
        m_time_hist_pos = 0;
    }

    double new_raw_time;
    float elapsed;

    if (m_high_perf_timer_freq.QuadPart != 0)
    {
        // get high-precision time
        // precision: usually from 1..6 us (MICROseconds), depending on the cpu speed.
        // (higher cpu speeds tend to have better precision here)
        LARGE_INTEGER t;
        if (!QueryPerformanceCounter(&t))
        {
            m_high_perf_timer_freq.QuadPart = 0;   // something went wrong (exception thrown) -> revert to crappy timer
        }
        else
        {
            new_raw_time = (double)t.QuadPart;
            elapsed = (float) ((new_raw_time - m_last_raw_time)/(double)m_high_perf_timer_freq.QuadPart);
        }
    }

    if (m_high_perf_timer_freq.QuadPart == 0)
    {
        // get low-precision time
        // precision: usually 1 ms (MILLIsecond) for win98, and 10 ms for win2k.
        new_raw_time = (double)(GetTickCount()*0.001);
        elapsed = (float)(new_raw_time - m_last_raw_time);
    }

    m_last_raw_time = new_raw_time;
    int slots_to_look_back = (m_high_perf_timer_freq.QuadPart==0) ? TIME_HIST_SLOTS : TIME_HIST_SLOTS/2;

    m_time += 1.0f/m_fps;

    // timekeeping goals:
    //    1. keep 'm_time' increasing SMOOTHLY: (smooth animation depends on it)
    //          m_time += 1.0f/m_fps;     // where m_fps is a bit damped
    //    2. keep m_time_hist[] 100% accurate (except for filtering out pauses),
    //       so that when we look take the difference between two entries,
    //       we get the real amount of time that passed between those 2 frames.
    //          m_time_hist[i] = m_last_raw_time + elapsed_corrected;

    if (m_frame > TIME_HIST_SLOTS)
    {
        if (m_fps < 60.0f)
            slots_to_look_back = (int)(slots_to_look_back*(0.1f + 0.9f*(m_fps/60.0f)));

        if (elapsed > 5.0f/m_fps || elapsed > 1.0f || elapsed < 0)
            elapsed = 1.0f / 30.0f;

        float old_hist_time = m_time_hist[(m_time_hist_pos - slots_to_look_back + TIME_HIST_SLOTS) % TIME_HIST_SLOTS];
		float new_hist_time = m_time_hist[(m_time_hist_pos - 1 + TIME_HIST_SLOTS) % TIME_HIST_SLOTS]
								+ elapsed;

        m_time_hist[m_time_hist_pos] = new_hist_time;
        m_time_hist_pos = (m_time_hist_pos+1) % TIME_HIST_SLOTS;

        float new_fps = slots_to_look_back / (float)(new_hist_time - old_hist_time);
        float damping = (m_high_perf_timer_freq.QuadPart==0) ? 0.93f : 0.87f;

        // damp heavily, so that crappy timer precision doesn't make animation jerky
        if (fabsf(m_fps - new_fps) > 3.0f)
            m_fps = new_fps;
        else
            m_fps = damping*m_fps + (1-damping)*new_fps;
    }
    else
    {
        float damping = (m_high_perf_timer_freq.QuadPart==0) ? 0.8f : 0.6f;

        if (m_frame < 2)
            elapsed = 1.0f / 30.0f;
        else if (elapsed > 1.0f || elapsed < 0)
            elapsed = 1.0f / m_fps;

        float old_hist_time = m_time_hist[0];
		float new_hist_time = m_time_hist[(m_time_hist_pos - 1 + TIME_HIST_SLOTS) % TIME_HIST_SLOTS]
								+ elapsed;

		m_time_hist[m_time_hist_pos] = new_hist_time;
        m_time_hist_pos = (m_time_hist_pos+1) % TIME_HIST_SLOTS;

        if (m_frame > 0)
        {
            float new_fps = (m_frame) / (new_hist_time - old_hist_time);
            m_fps = damping*m_fps + (1-damping)*new_fps;
        }
    }

    // Synchronize the audio and video by telling Winamp how many milliseconds we want the audio data,
    // before it's actually audible.  If we set this to the amount of time it takes to display 1 frame
    // (1/fps), the video and audio should be perfectly synchronized.
//    if (m_fps < 2.0f)
//        mod1.latencyMs = 500;
//    else if (m_fps > 125.0f)
//        mod1.latencyMs = 8;
//    else
//        mod1.latencyMs = (int)(1000.0f/m_fps*m_lpDX->m_frame_delay + 0.5f);
}

void CPluginShell::AnalyzeNewSound(unsigned char *pWaveL, unsigned char *pWaveR)
{
    // we get 512 samples in from XBMC.
    // the output of the fft has 'num_frequencies' samples,
    //   and represents the frequency range 0 hz - 22,050 hz.
    // usually, plugins only use half of this output (the range 0 hz - 11,025 hz),
    //   since >10 khz doesn't usually contribute much.

    int i;

    float temp_wave[2][512];

    int old_i = 0;
    for (i=0; i<512; i++)
    {
        m_sound.fWaveform[0][i] = (float)((pWaveL[i] ^ 128) - 128);
        m_sound.fWaveform[1][i] = (float)((pWaveR[i] ^ 128) - 128);

        // simulating single frequencies from 200 to 11,025 Hz:
        //float freq = 1.0f + 11050*(GetFrame() % 100)*0.01f;
        //m_sound.fWaveform[0][i] = 10*sinf(i*freq*6.28f/44100.0f);

        // damp the input into the FFT a bit, to reduce high-frequency noise:
        temp_wave[0][i] = 0.5f*(m_sound.fWaveform[0][i] + m_sound.fWaveform[0][old_i]);
        temp_wave[1][i] = 0.5f*(m_sound.fWaveform[1][i] + m_sound.fWaveform[1][old_i]);
        old_i = i;
    }

    m_fftobj.time_to_frequency_domain(temp_wave[0], m_sound.fSpectrum[0]);
    m_fftobj.time_to_frequency_domain(temp_wave[1], m_sound.fSpectrum[1]);

    // sum (left channel) spectrum up into 3 bands
    // [note: the new ranges do it so that the 3 bands are equally spaced, pitch-wise]
    float min_freq = 200.0f;
    float max_freq = 11025.0f;
    float net_octaves = (logf(max_freq/min_freq) / logf(2.0f));     // 5.7846348455575205777914165223593
    float octaves_per_band = net_octaves / 3.0f;                    // 1.9282116151858401925971388407864
    float mult = powf(2.0f, octaves_per_band); // each band's highest freq. divided by its lowest freq.; 3.805831305510122517035102576162
    // [to verify: min_freq * mult * mult * mult should equal max_freq.]
    for (int ch=0; ch<2; ch++)
    {
        for (int i=0; i<3; i++)
        {
            // old guesswork code for this:
            //   float exp = 2.1f;
            //   int start = (int)(NUM_FREQUENCIES*0.5f*powf(i/3.0f, exp));
            //   int end   = (int)(NUM_FREQUENCIES*0.5f*powf((i+1)/3.0f, exp));
            // results:
            //          old range:      new range (ideal):
            //   bass:  0-1097          200-761
            //   mids:  1097-4705       761-2897
            //   treb:  4705-11025      2897-11025
            int start = (int)(NUM_FREQUENCIES * min_freq*powf(mult, i  )/11025.0f);
            int end   = (int)(NUM_FREQUENCIES * min_freq*powf(mult, i+1)/11025.0f);
            if (start < 0) start = 0;
            if (end > NUM_FREQUENCIES) end = NUM_FREQUENCIES;

            m_sound.imm[ch][i] = 0;
            for (int j=start; j<end; j++)
                m_sound.imm[ch][i] += m_sound.fSpectrum[ch][j];
            m_sound.imm[ch][i] /= (float)(end-start);
        }
    }

    // some code to find empirical long-term averages for imm[0..2]:
    /*{
        static float sum[3];
        static int count = 0;

        #define FRAMES_PER_SONG 300     // should be at least 200!

        if (m_frame < FRAMES_PER_SONG)
        {
            sum[0] = sum[1] = sum[2] = 0;
            count = 0;
        }
        else
        {
            if (m_frame%FRAMES_PER_SONG == 0)
            {
                char buf[256];
                sprintf(buf, "%.4f, %.4f, %.4f     (%d samples / ~%d songs)\n",
                    sum[0]/(float)(count),
                    sum[1]/(float)(count),
                    sum[2]/(float)(count),
                    count,
                    count/(FRAMES_PER_SONG-10)
                );
                OutputDebugString(buf);

                // skip to next song
                PostMessage(m_hWndWinamp,WM_COMMAND,40048,0);
            }
            else if (m_frame%FRAMES_PER_SONG == 5)
            {
                // then advance to 0-2 minutes into the song:
                PostMessage(m_hWndWinamp,WM_USER,(20 + (rand()%65) + (rand()%65))*1000,106);
            }
            else if (m_frame%FRAMES_PER_SONG >= 10)
            {
                sum[0] += m_sound.imm[0];
                sum[1] += m_sound.imm[1];
                sum[2] += m_sound.imm[2];
                count++;
            }
        }
    }*/

    // multiply by long-term, empirically-determined inverse averages:
    // (for a trial of 244 songs, 10 seconds each, somewhere in the 2nd or 3rd minute,
    //  the average levels were: 0.326781557	0.38087377	0.199888934
    for (int ch=0; ch<2; ch++)
    {
        m_sound.imm[ch][0] /= 0.326781557f;//0.270f;
        m_sound.imm[ch][1] /= 0.380873770f;//0.343f;
        m_sound.imm[ch][2] /= 0.199888934f;//0.295f;
    }

    // do temporal blending to create attenuated and super-attenuated versions
    for (int ch=0; ch<2; ch++)
    {
        for (i=0; i<3; i++)
        {
            // m_sound.avg[i]
            {
                float avg_mix;
                if (m_sound.imm[ch][i] > m_sound.avg[ch][i])
                    avg_mix = AdjustRateToFPS(0.2f, 14.0f, m_fps);
                else
                    avg_mix = AdjustRateToFPS(0.5f, 14.0f, m_fps);
                m_sound.avg[ch][i] = m_sound.avg[ch][i]*avg_mix + m_sound.imm[ch][i]*(1-avg_mix);
            }

            // m_sound.med_avg[i]
            // m_sound.long_avg[i]
            {
                float med_mix  = 0.91f;//0.800f + 0.11f*powf(t, 0.4f);    // primarily used for velocity_damping
                float long_mix = 0.96f;//0.800f + 0.16f*powf(t, 0.2f);    // primarily used for smoke plumes
                med_mix  = AdjustRateToFPS( med_mix, 14.0f, m_fps);
                long_mix = AdjustRateToFPS(long_mix, 14.0f, m_fps);
                m_sound.med_avg[ch][i]  =  m_sound.med_avg[ch][i]*(med_mix ) + m_sound.imm[ch][i]*(1-med_mix );
                m_sound.long_avg[ch][i] = m_sound.long_avg[ch][i]*(long_mix) + m_sound.imm[ch][i]*(1-long_mix);
            }
        }
    }
}

void CPluginShell::PrepareFor2DDrawing_B(IDirect3DDevice9 *pDevice, int w, int h)
{
    // New 2D drawing area will have x,y coords in the range <-1,-1> .. <1,1>
    //         +--------+ Y=-1
    //         |        |
    //         | screen |             Z=0: front of scene
    //         |        |             Z=1: back of scene
    //         +--------+ Y=1
    //       X=-1      X=1
    // NOTE: After calling this, be sure to then call (at least):
    //  1. SetVertexShader()
    //  2. SetTexture(), if you need it
    // before rendering primitives!
    // Also, be sure your sprites have a z coordinate of 0.

    pDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    pDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
    pDevice->SetRenderState( D3DRS_ZFUNC,     D3DCMP_LESSEQUAL );
    pDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
    pDevice->SetRenderState( D3DRS_FILLMODE,  D3DFILL_SOLID );
    pDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
    pDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
//    pDevice->SetRenderState( D3DRS_CLIPPING, TRUE );
    pDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
    pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    pDevice->SetRenderState( D3DRS_LOCALVIEWER, FALSE );
    pDevice->SetRenderState( D3DRS_COLORVERTEX, TRUE );

    pDevice->SetTexture(0, NULL);
    pDevice->SetTexture(1, NULL);
    pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);//D3DTEXF_LINEAR);
    pDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);//D3DTEXF_LINEAR);
    pDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    pDevice->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE );
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT );
    pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE );

    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
    pDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

    pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

    // set up for 2D drawing:
    {
        D3DXMATRIX Ortho2D;
        D3DXMATRIX Identity;

        D3DXMatrixOrthoLH(&Ortho2D, w, h, 0.0f, 1.0f);
        D3DXMatrixIdentity(&Identity);

        pDevice->SetTransform(D3DTS_PROJECTION, &Ortho2D);
        pDevice->SetTransform(D3DTS_WORLD, &Identity);
        pDevice->SetTransform(D3DTS_VIEW, &Identity);
    }
}

void CPluginShell::DrawDarkTranslucentBox(RECT* pr)
{
    // 'pr' is the rectangle that some text will occupy;
    // a black box will be drawn around it, plus a bit of extra margin space.
#if 0
    m_text.DrawDarkBox(pr);

    if (m_vjd3d8_device)
        return;

    LPDIRECT3DDEVICE8 lpDevice = GetDevice();
    int w = m_lpDX->m_client_width;
    int h = m_lpDX->m_client_height;

    lpDevice->SetVertexShader( SIMPLE_VERTEX_FORMAT );
    lpDevice->SetTexture(0, NULL);

    lpDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    lpDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    lpDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

	  SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	  SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
	  SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	  SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
    SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );

    // set up a quad
    SIMPLEVERTEX verts[4];
    for (int i=0; i<4; i++)
    {
        verts[i].x = (i%2==0) ? (float)(-w/2  + pr->left  )  :
                                (float)(-w/2  + pr->right );
        verts[i].y = (i/2==0) ? (float)-(-h/2 + pr->bottom)  :
                                (float)-(-h/2 + pr->top   );
        verts[i].z = 0;
        verts[i].Diffuse = 0xFF000000;// (m_screenmode==DESKTOP) ? 0xE0000000 : 0xD0000000;
    }

    lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(SIMPLEVERTEX));

    // undo unusual state changes:
    lpDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
    lpDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
#endif
}

void CPluginShell::RenderBuiltInTextMsgs()
{
#if 0
    int _show_press_f1_NOW = (m_show_press_f1_msg && m_time < PRESS_F1_DUR);

    {
        RECT r;

        if (m_show_help)
        {
            int y = m_upper_left_corner_y;

            SetRect(&r, 0, 0, GetWidth(), GetHeight());
            m_text.DrawText(m_d3dx_font[HELPSCREEN_FONT], g_szHelp, -1, &r, DT_CALCRECT, 0xFFFFFFFF);

            r.top += m_upper_left_corner_y;
            r.left += m_left_edge;
            r.right += m_left_edge + PLAYLIST_INNER_MARGIN*2;
            r.bottom += m_upper_left_corner_y + PLAYLIST_INNER_MARGIN*2;
            DrawDarkTranslucentBox(&r);

            r.top += PLAYLIST_INNER_MARGIN;
            r.left += PLAYLIST_INNER_MARGIN;
            r.right -= PLAYLIST_INNER_MARGIN;
            r.bottom -= PLAYLIST_INNER_MARGIN;
            m_text.DrawText(m_d3dx_font[HELPSCREEN_FONT], g_szHelp, -1, &r, 0, 0xFFFFFFFF);

            m_upper_left_corner_y += r.bottom-r.top + PLAYLIST_INNER_MARGIN*3;
        }

        // render 'Press F1 for Help' message in lower-right corner:
        if (_show_press_f1_NOW)
        {
            int dx = (int)(160.0f * powf(m_time/(float)(PRESS_F1_DUR), (float)(PRESS_F1_EXP)));
            SetRect(&r, m_left_edge, m_lower_right_corner_y - GetFontHeight(DECORATIVE_FONT), m_right_edge + dx, m_lower_right_corner_y);
            m_lower_right_corner_y -= m_text.DrawText(m_d3dx_font[DECORATIVE_FONT], PRESS_F1_MSG, &r, DT_RIGHT, 0xFFFFFFFF);
        }
    }
#endif
}

void CPluginShell::RenderPlaylist()
{
#if 0
    // draw playlist:
    if (m_show_playlist)
    {
        RECT r;
        int nSongs = SendMessage(m_hWndWinamp,WM_USER, 0, 124);
        int now_playing = SendMessage(m_hWndWinamp,WM_USER, 0, 125);

        if (nSongs <= 0)
        {
            m_show_playlist = 0;
        }
        else
        {
            int playlist_vert_pixels = m_lower_left_corner_y - m_upper_left_corner_y;
            int disp_lines  = min(MAX_SONGS_PER_PAGE, (playlist_vert_pixels - PLAYLIST_INNER_MARGIN*2) / GetFontHeight(PLAYLIST_FONT));
            int total_pages = (nSongs) / disp_lines;

            if (disp_lines<=0)
                return;

            // apply PgUp/PgDn keypresses since last time
            m_playlist_pos -= m_playlist_pageups * disp_lines;
            m_playlist_pageups = 0;

            if (m_playlist_pos < 0)
                m_playlist_pos = 0;
            if (m_playlist_pos >= nSongs)
                m_playlist_pos = nSongs-1;

            // NOTE: 'dwFlags' is used for both DDRAW and DX8
            DWORD dwFlags   = DT_NOPREFIX | DT_SINGLELINE | DT_WORD_ELLIPSIS;
            DWORD color;

            int cur_page    = (m_playlist_pos) / disp_lines;
            int cur_line    = (m_playlist_pos + disp_lines - 1) % disp_lines;
            int new_top_idx = cur_page * disp_lines;
            int new_btm_idx = new_top_idx + disp_lines;
            char buf[1024];

			// ask winamp for the song names, but DO IT BEFORE getting the DC,
			// otherwise vaio will crash (~DDRAW port).
            if (m_playlist_top_idx != new_top_idx ||
                m_playlist_btm_idx != new_btm_idx)
            {
				for (int i=0; i<disp_lines; i++)
				{
					int j = new_top_idx + i;
					if (j < nSongs)
					{
						// clip max len. of song name to 240 chars, to prevent overflows
						strcpy(buf, (char*)SendMessage(m_hWndWinamp, WM_USER, j, 212));
						buf[240] = 0;
						sprintf(m_playlist[i], "%d. %s ", j+1, buf);  // leave an extra space @ end, so italicized fonts don't get clipped
					}
				}
			}

            // update playlist cache, if necessary:
            if (m_playlist_top_idx != new_top_idx ||
                m_playlist_btm_idx != new_btm_idx)
            {
                m_playlist_top_idx = new_top_idx;
                m_playlist_btm_idx = new_btm_idx;
                m_playlist_width_pixels = 0;

                int max_w = min(m_right_edge - m_left_edge, m_lpDX->m_client_width - TEXT_MARGIN*2 - PLAYLIST_INNER_MARGIN*2);

                for (int i=0; i<disp_lines; i++)
                {
                    int j = new_top_idx + i;
                    if (j < nSongs)
                    {
                        // clip max len. of song name to 240 chars, to prevent overflows
                        //strcpy(buf, (char*)SendMessage(m_hWndWinamp, WM_USER, j, 212));
                        //buf[240] = 0;
                        //sprintf(m_playlist[i], "%d. %s ", j+1, buf);  // leave an extra space @ end, so italicized fonts don't get clipped

                        SetRect(&r, 0, 0, max_w, 1024);
                        m_text.DrawText(GetFont(PLAYLIST_FONT), m_playlist[i], -1, &r, dwFlags | DT_CALCRECT, 0xFFFFFFFF);
                        int w = r.right-r.left;
                        if (w>0)
                            m_playlist_width_pixels = max(m_playlist_width_pixels, w);
                    }
                    else
                    {
                        m_playlist[i][0] = 0;
                    }
                }

                if (m_playlist_width_pixels == 0 ||
                    m_playlist_width_pixels > max_w)
                    m_playlist_width_pixels = max_w;
            }

            int start = max(0,      (cur_page  )*disp_lines);
            int end   = min(nSongs, (cur_page+1)*disp_lines);

            // draw dark box around where the playlist will go:

            RECT r;
            r.top    = m_upper_left_corner_y;
            r.left   = m_left_edge;
            r.right  = m_left_edge + m_playlist_width_pixels + PLAYLIST_INNER_MARGIN*2;
            r.bottom = m_upper_left_corner_y + (end-start)*GetFontHeight(PLAYLIST_FONT) + PLAYLIST_INNER_MARGIN*2;
            DrawDarkTranslucentBox(&r);

            //m_d3dx_font[PLAYLIST_FONT]->Begin();

            // draw playlist text
            int y = m_upper_left_corner_y + PLAYLIST_INNER_MARGIN;
            for (int i=start; i<end; i++)
            {
                SetRect(&r, m_left_edge + PLAYLIST_INNER_MARGIN, y, m_left_edge + PLAYLIST_INNER_MARGIN + m_playlist_width_pixels, y + GetFontHeight(PLAYLIST_FONT));

                if (m_lpDX->GetBitDepth() == 8)
                    color = (i==m_playlist_pos) ?
                        (i==now_playing ? 0xFFFFFFFF : 0xFFFFFFFF) :
                        (i==now_playing ? 0xFFFFFFFF : 0xFF707070);
                else
                    color = (i==m_playlist_pos) ?
                        (i==now_playing ? PLAYLIST_COLOR_BOTH : PLAYLIST_COLOR_HILITE_TRACK) :
                        (i==now_playing ? PLAYLIST_COLOR_PLAYING_TRACK : PLAYLIST_COLOR_NORMAL);

                y += m_text.DrawText(GetFont(PLAYLIST_FONT), m_playlist[i-start], -1, &r, dwFlags, color);
            }

            //m_d3dx_font[PLAYLIST_FONT]->End();
        }
    }
#endif
}

void CPluginShell::SuggestHowToFreeSomeMem()
{
#if 0
    // This function is called when the plugin runs out of video memory;
    //   it lets you show a messagebox to the user so you can (intelligently)
    //   suggest how to free up some video memory, based on what settings
    //   they've chosen.

    char str[1024];

    if (m_lpDX->m_current_mode.multisamp != D3DMULTISAMPLE_NONE)
    {
        if (m_lpDX->m_current_mode.screenmode == WINDOWED)
            sprintf(str,
                "To free up some memory, please RESTART WINAMP, then return\r"
                "   to the plugin's config panel and try setting your\r"
                "   WINDOWED MODE MULTISAMPLING back to 'NONE.'\r"
                "\r"
                "Then try running the plugin again."
            );
        else if (m_lpDX->m_current_mode.screenmode == FAKE_FULLSCREEN)
            sprintf(str,
                "To free up some memory, please RESTART WINAMP, then return\r"
                "   to the plugin's config panel and try setting your\r"
                "   FAKE FULLSCREEN MODE MULTISAMPLING back to 'NONE.'\r"
                "\r"
                "Then try running the plugin again."
            );
        else
            sprintf(str,
                "To free up some memory, please RESTART WINAMP, then return\r"
                "   to the plugin's config panel and try setting your\r"
                "   FULLSCREEN MODE MULTISAMPLING back to 'NONE.'\r"
                "\r"
                "Then try running the plugin again."
            );
    }
    else
    if (m_lpDX->m_current_mode.screenmode == FULLSCREEN)  // true fullscreen
        sprintf(str,
            "To free up some video memory, try the following:\r"
            "\r"
            "1. Try closing all other applications that might be using video memory, especially:\r"
            "\r"
            "        * WINDOWS MEDIA PLAYER\r"
            "        * any video conferencing software, such as NETMEETING\r"
            "        * any DVD playback, TV tuner, or TV capture software\r"
            "        * any video editing software\r"
            "        * any software that uses Overlays, such as Drempels Desktop\r"
            "        * any audio dictation software, such as Dragon NaturallySpeaking\r"
            "        * any other 3D programs currently running\r"
            "\r"
            "2. Also try returning to the config panel (ALT+K) and selecting a display mode\r"
            "    that uses less video memory.  16-bit display modes use half as much memory\r"
            "    as 32-bit display modes, and lower-resolution display modes (such as 640 x 480)\r"
            "    use proportionally less video memory.\r"
            "\r"
            "After making these changes, please RESTART WINAMP before trying to run\r"
            "the plugin again.\r"
        );
    else    // windowed, desktop mode, or fake fullscreen
        sprintf(str,
            "To free up some video memory, try the following:\r"
            "\r"
            "1. Try closing all other applications that might be using video memory, especially:\r"
            "\r"
            "        * WINDOWS MEDIA PLAYER\r"
            "        * any video conferencing software, such as NETMEETING\r"
            "        * any DVD playback, TV tuner, or TV capture software\r"
            "        * any video editing software\r"
            "        * any software that uses Overlays, such as Drempels Desktop\r"
            "        * any audio dictation software, such as Dragon NaturallySpeaking\r"
            "        * any other 3D programs currently running\r"
            "\r"
            "2. Also try changing your Windows display mode to a lesser bit depth\r"
            "    (i.e. 16-bit color), or a smaller resolution.\r"
            "\r"
            "After making these changes, please RESTART WINAMP before trying to run\r"
            "the plugin again.\r"
        );

    MessageBox(m_lpDX->GetHwnd(), str, "SUGGESTION", MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
#endif
}

LRESULT CALLBACK CPluginShell::WindowProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
    CPluginShell* p = (CPluginShell*)GetWindowLong(hWnd,GWL_USERDATA);
    if (p)
        return p->PluginShellWindowProc(hWnd, uMsg, wParam, lParam);
    else
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
#endif
	return 0;
}


LRESULT CPluginShell::PluginShellWindowProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
    SHORT mask = 1 << (sizeof(SHORT)*8 - 1);
    //bool bShiftHeldDown = (GetKeyState(VK_SHIFT) & mask) != 0;
    bool bCtrlHeldDown  = (GetKeyState(VK_CONTROL) & mask) != 0;
    //bool bAltHeldDown: most keys come in under WM_SYSKEYDOWN when ALT is depressed.

    int i;

    #ifdef _DEBUG
        if (uMsg != WM_MOUSEMOVE &&
            uMsg != WM_NCHITTEST &&
            uMsg != WM_SETCURSOR &&
            uMsg != WM_COPYDATA &&
            uMsg != WM_USER)
        {
            char caption[256] = "WndProc: frame 0, ";
            if (m_frame > 0)
            {
		        float time = m_time;
		        int hours = (int)(time/3600);
		        time -= hours*3600;
		        int minutes = (int)(time/60);
		        time -= minutes*60;
		        int seconds = (int)time;
		        time -= seconds;
		        int dsec = (int)(time*100);
		        sprintf(caption, "WndProc: frame %d, t=%dh:%02dm:%02d.%02ds, ", m_frame, hours, minutes, seconds, dsec);
            }
            OutputDebugMessage(caption, hWnd, uMsg, wParam, lParam);
        }
    #endif

    switch (uMsg)
    {

    case WM_USER:
        if (m_screenmode == DESKTOP)
        {
            // this function resides in vms_desktop.dll;
            // its response will come later, via the WM_COPYDATA
            // message (See below).
            getItemData(wParam);
            return 0;
        }
        break;

    case WM_COPYDATA:
        if (m_screenmode == DESKTOP)
        {
            // this message is vms_desktop.dll's response to
            // our call to getItemData().
            PCOPYDATASTRUCT c = (PCOPYDATASTRUCT)lParam;
            if (c && (c->cbData % sizeof(icon_t) == 0))
            {
                icon_t *pNewIcons = (icon_t*)c->lpData;

                EnterCriticalSection(&m_desktop_cs);

                if (m_desktop_icon_state == 1 && (c->dwData & 0x80000000) ) // if doing a total refresh...
                {
                    // ...we build the list from zero
                    int len = c->dwData & 0xFFFF;
                    for (int i=0; i<len; i++)
                        m_icon_list.push_back(pNewIcons[i]);
                }
                else if (m_desktop_icon_state == 3 && !(c->dwData & 0x80000000) )
                {
                    // otherwise, we alter existing things in the list:
                    std::list<icon_t>::iterator p;
                    int start = c->dwData & 0xFFFF;
                    int len   = c->dwData >> 16;

                    int i = 0;
                    for (p = m_icon_list.begin(); p != m_icon_list.end() && i<start; p++)
                        i++;
                    for ( ; p != m_icon_list.end() && i<start+len; p++)
                    {
                        p->x = pNewIcons[i-start].x;
                        p->y = pNewIcons[i-start].y;
                        memcpy(p->name, pNewIcons[i-start].name, sizeof(p->name));
                        memcpy(p->pidl, pNewIcons[i-start].pidl, sizeof(p->pidl));
                        i++;
                    }

                    m_desktop_icon_state = 2;
                    m_desktop_icon_update_frame = GetFrame();
                }

                LeaveCriticalSection(&m_desktop_cs);
            }

            return 0;
        }
        break;

    case WM_ERASEBKGND:
        // Repaint window when song is paused and image needs to be repainted:
        if (SendMessage(m_hWndWinamp,WM_USER,0,104)!=1 && m_lpDX && m_lpDX->m_lpDevice)    // WM_USER/104 return codes: 1=playing, 3=paused, other=stopped
        {
            m_lpDX->m_lpDevice->Present(NULL,NULL,NULL,NULL);
            return 0;
        }
        break;

    case WM_WINDOWPOSCHANGING:
        if (
            m_screenmode == DESKTOP
            && (!m_force_accept_WM_WINDOWPOSCHANGING)
            && m_lpDX && m_lpDX->m_ready
           )
        {
            // unless we requested it ourselves or it's init time,
            // prevent the fake desktop window from moving around
            // in the Z order!  (i.e., keep it on the bottom)

            // without this code, when you click on the 'real' desktop
            // in a multimon setup, any windows that are overtop of the
            // 'fake' desktop will flash, since they'll be covered
            // up by the fake desktop window (but then shown again on
            // the next frame, when we detect that the fake desktop
            // window isn't on bottom & send it back to the bottom).

            LPWINDOWPOS pwp = (LPWINDOWPOS)lParam;
            if (pwp)
                pwp->flags |= SWP_NOOWNERZORDER | SWP_NOZORDER;
        }
        if (m_screenmode==WINDOWED && m_lpDX && m_lpDX->m_ready && m_lpDX->m_current_mode.m_skin)
            m_lpDX->SaveWindow();
        break;

    case WM_NCACTIVATE:
        // *Very Important Handler!*
        //    -Without this code, the app would not work properly when running in true
        //     fullscreen mode on multiple monitors; it would auto-minimize whenever the
        //     user clicked on a window in another display.
        if (wParam == 0 &&
            m_screenmode == FULLSCREEN &&
            m_frame > 0 &&
            !m_exiting &&
            m_lpDX &&
            m_lpDX->m_ready
            && m_lpDX->m_lpD3D &&
            m_lpDX->m_lpD3D->GetAdapterCount() > 1 &&
            m_hTextWnd==NULL    // important!; if text window exists on diff. monitor, DX8 knows there are multiple monitors, and we don't need to worry about the app auto-minimizing when somebody clicks on another monitor; but if there is no 2nd device (~text window), it does its auto-minimize thing.
            )
        {
            return 0;
        }
        break;

    case WM_DESTROY:
        // note: don't post quit message here if the window is being destroyed
        // and re-created on a switch between windowed & FAKE fullscreen modes.
        if (!m_lpDX->TempIgnoreDestroyMessages())
        {
            // this is a final exit, and not just destroy-then-recreate-the-window.
            // so, flag DXContext so it knows that someone else
            // will take care of destroying the window!
            m_lpDX->OnTrulyExiting();
            PostQuitMessage(0);
        }
        return FALSE;
        break;

    case WM_SIZE:
        // clear or set activity flag to reflect focus
        if (m_lpDX && m_lpDX->m_ready && m_screenmode==WINDOWED && !m_resizing)
        {
            m_hidden = (SIZE_MAXHIDE==wParam || SIZE_MINIMIZED==wParam) ? TRUE : FALSE;

            if (SIZE_MAXIMIZED==wParam || SIZE_RESTORED==wParam) // the window has been maximized or restored
                OnUserResizeWindow();
        }
        break;

    case WM_ENTERSIZEMOVE:
        m_resizing = 1;
        break;

    case WM_EXITSIZEMOVE:
        if( m_lpDX && m_lpDX->m_ready && m_screenmode==WINDOWED )
            OnUserResizeWindow();
        m_resizing = 0;
        break;

    case WM_GETMINMAXINFO:
        {
            // don't let the window get too small
            MINMAXINFO* p = (MINMAXINFO*)lParam;
            if (p->ptMinTrackSize.x < 64)
                p->ptMinTrackSize.x = 64;
            p->ptMinTrackSize.y = p->ptMinTrackSize.x*3/4;
        }
        return 0;

    case WM_MOUSEMOVE:
        if (m_screenmode==DESKTOP && (m_desktop_dragging==1 || m_desktop_box==1))
        {
            m_desktop_drag_curpos.x = LOWORD(lParam);
            m_desktop_drag_curpos.y = HIWORD(lParam);
            if (m_desktop_box==1)
            {
                // update selection based on box coords
                RECT box, temp;
                box.left   = min(m_desktop_drag_curpos.x, m_desktop_drag_startpos.x);
                box.right  = max(m_desktop_drag_curpos.x, m_desktop_drag_startpos.x);
                box.top    = min(m_desktop_drag_curpos.y, m_desktop_drag_startpos.y);
                box.bottom = max(m_desktop_drag_curpos.y, m_desktop_drag_startpos.y);

                std::list<icon_t>::iterator p;
                for (p = m_icon_list.begin(); p != m_icon_list.end(); p++)
                {
                    p->selected = 0;

                    if (IntersectRect(&temp, &box, &p->label_rect))
                        p->selected = 1;
                    else if (IntersectRect(&temp, &box, &p->icon_rect))
                        p->selected = 1;
                }
            }

            // repaint window manually, if winamp is paused
            if (SendMessage(m_hWndWinamp,WM_USER,0,104) != 1)
            {
                PushWindowToJustBeforeDesktop(GetPluginWindow());
                DrawAndDisplay(1);
            }

            //return 0;
        }
        break;

    case WM_LBUTTONUP:
        if (m_screenmode==DESKTOP)
        {
            if (m_desktop_dragging)
            {
                m_desktop_dragging = 0;

                // move selected item(s) to new cursor position
                int dx = LOWORD(lParam) - m_desktop_drag_startpos.x;
                int dy = HIWORD(lParam) - m_desktop_drag_startpos.y;

                if (dx!=0 || dy!=0)
                {
                    int idx=0;
                    std::list<icon_t>::iterator p;
                    for (p = m_icon_list.begin(); p != m_icon_list.end(); p++)
                    {
                        if (p->selected)
                        {
                            SendMessage(m_hWndDesktopListView, LVM_SETITEMPOSITION, idx, MAKELPARAM(p->x + dx, p->y + dy));
                            p->x += dx;
                            p->y += dy;
                        }
                        idx++;
                    }
                }

                // repaint window manually, if winamp is paused
                if (SendMessage(m_hWndWinamp,WM_USER,0,104) != 1)
                {
                    PushWindowToJustBeforeDesktop(GetPluginWindow());
                    DrawAndDisplay(1);
                }
            }

            if (m_desktop_box)
            {
                m_desktop_box = 0;

                // repaint window manually, if winamp is paused
                if (SendMessage(m_hWndWinamp,WM_USER,0,104) != 1)
                {
                    PushWindowToJustBeforeDesktop(GetPluginWindow());
                    DrawAndDisplay(1);
                }
            }

            //return 0;
        }
        break;


    case WM_USER+1666:
      if (wParam == 1 && lParam == 15)
      {
        if (m_screenmode == FULLSCREEN || m_screenmode == FAKE_FULLSCREEN)
          ToggleFullScreen();
      }
    return 0;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        // Toggle between Fullscreen and Windowed modes on double-click
        // note: this requires the 'CS_DBLCLKS' windowclass style!
        if (m_screenmode != DESKTOP)
        {
            SetFocus(hWnd);
            if (uMsg==WM_LBUTTONDBLCLK && m_frame>0)
            {
                ToggleFullScreen();
                return 0;
            }
        }
        else
        {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);

            int done = 0;

            for (int pass=0; pass<2 && !done; pass++)
            {
                std::list<icon_t>::iterator p;
                for (p = m_icon_list.begin(); p != m_icon_list.end(); p++)
                {
                    RECT *pr = (pass==0) ? &p->icon_rect : &p->label_rect;
                    int bottom_extend = (pass==0) ? 3 : 0; // accepts clicks in the 3-pixel gap between the icon and the text label.
                    if (pt.x >= pr->left &&
                        pt.x <= pr->right &&
                        pt.y >= pr->top &&
                        pt.y <= pr->bottom + bottom_extend)
                    {
                        switch(uMsg)
                        {
                        case WM_RBUTTONUP:
                            //pt.x += m_lpDX->m_monitor_rect.left;
                            //pt.y += m_lpDX->m_monitor_rect.top;
                            DoExplorerMenu(GetPluginWindow(), (LPITEMIDLIST)p->pidl, pt);
                            break;
                        case WM_LBUTTONDBLCLK:
                            {
                                char buf[MAX_PATH];
                                sprintf(buf, "%s\\%s", m_szDesktopFolder, p->name);
                                ExecutePidl((LPITEMIDLIST)p->pidl, buf, m_szDesktopFolder, GetPluginWindow());
                            }
                            break;
                        case WM_LBUTTONDOWN:
                            m_desktop_dragging = 1;
                            memcpy(m_desktop_drag_pidl, p->pidl, sizeof(m_desktop_drag_pidl));
                            m_desktop_drag_startpos.x = LOWORD(lParam);
                            m_desktop_drag_startpos.y = HIWORD(lParam);
                            m_desktop_drag_curpos.x = LOWORD(lParam);
                            m_desktop_drag_curpos.y = HIWORD(lParam);
                            if (!(wParam & MK_CONTROL)) // if CTRL not held down
                            {
                                if (!p->selected)
                                {
                                    DeselectDesktop();
                                    p->selected = 1;
                                }
                            }
                            else
                            {
                                p->selected = 1-p->selected;
                            }
                            break;
                        case WM_RBUTTONDOWN:
                            DeselectDesktop();
                            p->selected = 1;
                            break;
                        }

                        done = 1;
                        break;
                    }
                }
            }

            if (!done)
            {
                // deselect all, unless they're CTRL+clicking and missed an icon.
                if (uMsg!=WM_LBUTTONDOWN || !(wParam & MK_CONTROL))
                    DeselectDesktop();

                if (uMsg==WM_RBUTTONUP)// || uMsg==WM_RBUTTONDOWN)
                {
                    // note: can't use GetMenu and TrackPopupMenu here because the hwnd param to TrackPopupMenu must belong to current application.

                    // (before sending coords to desktop window, xform them into its client coords:)
                    POINT pt;
                    pt.x = LOWORD(lParam);
                    pt.y = HIWORD(lParam);
                    ScreenToClient(m_hWndDesktopListView, &pt);
                    lParam = MAKELPARAM(pt.x + m_lpDX->m_monitor_rect.left, pt.y + m_lpDX->m_monitor_rect.top);

                    PostMessage(m_hWndDesktopListView, uMsg, wParam, lParam);
                    //PostMessage(m_hWndDesktopListView, WM_CONTEXTMENU, (WPARAM)m_hWndDesktopListView, lParam);
                }
                else if (uMsg==WM_LBUTTONDOWN)
                {
                    m_desktop_box = 1;
                    m_desktop_drag_startpos.x = LOWORD(lParam);
                    m_desktop_drag_startpos.y = HIWORD(lParam);
                    m_desktop_drag_curpos.x = LOWORD(lParam);
                    m_desktop_drag_curpos.y = HIWORD(lParam);
                }
            }

            // repaint window manually, if winamp is paused
            if (SendMessage(m_hWndWinamp,WM_USER,0,104) != 1)
            {
                PushWindowToJustBeforeDesktop(GetPluginWindow());
                DrawAndDisplay(1);
            }

            //return 0;
        }
        break;

        /*
    case WM_SETFOCUS:
        // note: this msg never comes in when embedwnd is used, but that's ok, because that's only
        // in Windowed mode, and m_lost_focus only makes us sleep when fullscreen.
        m_lost_focus = 0;
        break;

    case WM_KILLFOCUS:
        // note: this msg never comes in when embedwnd is used, but that's ok, because that's only
        // in Windowed mode, and m_lost_focus only makes us sleep when fullscreen.
        m_lost_focus = 1;
        break;
        */

    case WM_SETCURSOR:
        if (
            (m_screenmode == FULLSCREEN) ||
            (m_screenmode == FAKE_FULLSCREEN && m_lpDX && m_lpDX->m_fake_fs_covers_all)
           )
        {
            // hide the cursor
            SetCursor(NULL);
            return TRUE; // prevent Windows from setting cursor to window class cursor
        }
        break;

    case WM_NCHITTEST:
        // Prevent the user from selecting the menu in fullscreen mode
        if (m_screenmode != WINDOWED)
            return HTCLIENT;
        break;

    case WM_SYSCOMMAND:
        // Prevent *moving/sizing* and *entering standby mode* when in fullscreen mode
        switch( wParam )
        {
            case SC_MOVE:
            case SC_SIZE:
            case SC_MAXIMIZE:
            case SC_KEYMENU:
                if (m_screenmode != WINDOWED)
                    return 1;
                break;
            case SC_MONITORPOWER:
                if (m_screenmode == FULLSCREEN || m_screenmode == FAKE_FULLSCREEN)
                    return 1;
                break;
        }
        break;

    case WM_CONTEXTMENU:
        // launch popup context menu.  see handler for WM_COMMAND also.
        if (m_screenmode == DESKTOP)
        {
            // note: execution should never reach this point,
            // because we don't pass WM_RBUTTONUP to DefWindowProc
            // when in desktop mode!
            return 0;
        }
        else if (m_screenmode == WINDOWED)    // context menus only allowed in ~windowed modes
        {
            TrackPopupMenuEx( m_context_menu, TPM_VERTICAL, LOWORD(lParam), HIWORD(lParam), hWnd, NULL );
            return 0;
        }
        break;

    case WM_COMMAND:
        // handle clicks on items on context menu.
        if (m_screenmode == WINDOWED)
        {
            switch( LOWORD(wParam) )
            {
                case ID_QUIT:
                    m_exiting = 1;
                    PostMessage(hWnd, WM_CLOSE, 0, 0);
                    return 0;
                case ID_GO_FS:
                    if (m_frame > 0)
                        ToggleFullScreen();
                    return 0;
                case ID_DESKTOP_MODE:
                    if (m_frame > 0)
                        ToggleDesktop();
                    return 0;
                case ID_SHOWHELP:
                    ToggleHelp();
                    return 0;
                case ID_SHOWPLAYLIST:
                    TogglePlaylist();
                    return 0;
            }
            // then allow the plugin to override any command:
            if (MyWindowProc(hWnd, uMsg, wParam, lParam) == 0)
                return 0;
        }
        break;

    /*
    KEY HANDLING: the basic idea:
        -in all cases, handle or capture:
            -ZXCVBRS, zxcvbrs
                -also make sure it's case-insensitive!  (lowercase come through only as WM_CHAR; uppercase come in as both)
            -(ALT+ENTER)
            -(F1, ESC, UP, DN, Left, Right, SHIFT+l/r)
            -(P for playlist)
                -when playlist showing: steal J, HOME, END, PGUP, PGDN, UP, DOWN, ESC
            -(BLOCK J, L)
        -when integrated with winamp (using embedwnd), also handle these keys:
            -j, l, L, CTRL+L [windowed mode only!]
            -CTRL+P, CTRL+D
            -CTRL+TAB
            -ALT-E
            -ALT+F (main menu)
            -ALT+3 (id3)
    */

    case WM_SYSKEYDOWN:
        if (wParam==VK_RETURN && m_frame > 0)
        {
            ToggleFullScreen();
            return 0;
        }
        // if in embedded mode (using winamp skin), pass ALT+ keys on to winamp
        // ex: ALT+E, ALT+F, ALT+3...
        if (m_screenmode==WINDOWED && m_lpDX->m_current_mode.m_skin)
            return PostMessage(m_hWndWinamp, uMsg, wParam, lParam); // force-pass to winamp; required for embedwnd
        break;

    case WM_SYSKEYUP:
        if (m_screenmode==WINDOWED && m_lpDX->m_current_mode.m_skin)
            return PostMessage(m_hWndWinamp, uMsg, wParam, lParam); // force-pass to winamp; required for embedwnd
        break;

    case WM_SYSCHAR:
        if ((wParam=='d' || wParam=='D') && m_frame > 0)
        {
            ToggleDesktop();
            return 0;
        }
        break;

    case WM_CHAR:
        // if playlist is showing, steal p/j keys from the plugin:
        if (m_show_playlist)
        {
            switch(wParam)
            {
            /*
            case 'p':
            case 'P':
                TogglePlaylist();
                return 0;*/
            case 'j':
            case 'J':
                m_playlist_pos = SendMessage(m_hWndWinamp,WM_USER, 0, 125);
                return 0;
            default:
                {
                    int nSongs = SendMessage(m_hWndWinamp,WM_USER, 0, 124);
                    int found = 0;
                    int orig_pos = m_playlist_pos;
                    int inc = (wParam>='A' && wParam<='Z') ? -1 : 1;
                    while (1)
                    {
                        if (inc==1 && m_playlist_pos >= nSongs-1)
                            break;
                        if (inc==-1 && m_playlist_pos <= 0)
                            break;

                        m_playlist_pos += inc;

                        char buf[32];
						strncpy(buf, (char*)SendMessage(m_hWndWinamp, WM_USER, m_playlist_pos, 212), 31);
                        buf[31] = 0;

	                    // remove song # and period from beginning
	                    char *p = buf;
	                    while (*p >= '0' && *p <= '9') p++;
	                    if (*p == '.' && *(p+1) == ' ')
	                    {
		                    p += 2;
		                    int pos = 0;
		                    while (*p != 0)
		                    {
			                    buf[pos++] = *p;
			                    p++;
		                    }
		                    buf[pos++] = 0;
	                    }

                        int wParam2 = (wParam>='A' && wParam<='Z') ? (wParam + 'a'-'A') : (wParam + 'A'-'a');
                        if (buf[0]==wParam || buf[0]==wParam2)
                        {
                            found = 1;
                            break;
                        }
                    }

                    if (!found)
                        m_playlist_pos = orig_pos;
                }
                return 0;
            }
        }

        // then allow the plugin to override any keys:
        if (MyWindowProc(hWnd, uMsg, wParam, lParam) == 0)
            return 0;

        // finally, default key actions:
        switch(wParam)
        {
        // WINAMP PLAYBACK CONTROL KEYS:
        case 'z':
        case 'Z':
            PostMessage(m_hWndWinamp,WM_COMMAND,40044,0);
            return 0;
        case 'x':
        case 'X':
            PostMessage(m_hWndWinamp,WM_COMMAND,40045,0);
            return 0;
        case 'c':
        case 'C':
            PostMessage(m_hWndWinamp,WM_COMMAND,40046,0);
            return 0;
        case 'v':
        case 'V':
            PostMessage(m_hWndWinamp,WM_COMMAND,40047,0);
            return 0;
        case 'b':
        case 'B':
            PostMessage(m_hWndWinamp,WM_COMMAND,40048,0);
            return 0;
        case 's':
        case 'S':
            //if (SendMessage(m_hWndWinamp,WM_USER,0,250))
            //    sprintf(m_szUserMessage, "shuffle is now OFF");    // shuffle was on
            //else
            //    sprintf(m_szUserMessage, "shuffle is now ON");    // shuffle was off

            // toggle shuffle
            PostMessage(m_hWndWinamp,WM_COMMAND,40023,0);
            return 0;
        case 'r':
        case 'R':
            // toggle repeat
            PostMessage(m_hWndWinamp,WM_COMMAND,40022,0);
            return 0;
        case 'p':
        case 'P':
            TogglePlaylist();
            return 0;
        case 'l':
            // note that this is actually correct; when you hit 'l' from the
            // MAIN winamp window, you get an "open files" dialog; when you hit
            // 'l' from the playlist editor, you get an "add files to playlist" dialog.
            // (that sends IDC_PLAYLIST_ADDMP3==1032 to the playlist, which we can't
            //  do from here.)
            PostMessage(m_hWndWinamp,WM_COMMAND,40029,0);
            return 0;
        case 'L':
            PostMessage(m_hWndWinamp,WM_COMMAND,40187,0);
            return 0;
        case 'j':
            PostMessage(m_hWndWinamp,WM_COMMAND,40194,0);
            return 0;
        }

        return 0;//DefWindowProc(hWnd,uMsg,wParam,lParam);
        break;  // end case WM_CHAR

    case WM_KEYUP:

        // allow the plugin to override any keys:
        if (MyWindowProc(hWnd, uMsg, wParam, lParam) == 0)
            return 0;

        /*
        switch(wParam)
        {
        case VK_SOMETHING:
            ...
            break;
        }
        */

        return 0;
        break;

    case WM_KEYDOWN:
        if (m_show_playlist)
        {
            switch(wParam)
            {
            case VK_ESCAPE:
                m_show_playlist = 0;
                return 0;

            case VK_UP:
                {
                    int nRepeat = lParam & 0xFFFF;
                    if (GetKeyState(VK_SHIFT) & mask)
                        m_playlist_pos -= 10*nRepeat;
                    else
                        m_playlist_pos -= nRepeat;
                }
                return 0;

            case VK_DOWN:
                {
                    int nRepeat = lParam & 0xFFFF;
                    if (GetKeyState(VK_SHIFT) & mask)
                        m_playlist_pos += 10*nRepeat;
                    else
                        m_playlist_pos += nRepeat;
                }
                return 0;

            case VK_HOME:
                m_playlist_pos = 0;
                return 0;

            case VK_END:
                m_playlist_pos = SendMessage(m_hWndWinamp,WM_USER, 0, 124) - 1;
                return 0;

            case VK_PRIOR:
                if (GetKeyState(VK_SHIFT) & mask)
                    m_playlist_pageups += 10;
                else
                    m_playlist_pageups++;
                return 0;

            case VK_NEXT:
                if (GetKeyState(VK_SHIFT) & mask)
                    m_playlist_pageups -= 10;
                else
                    m_playlist_pageups--;
                return 0;

            case VK_RETURN:
				SendMessage(m_hWndWinamp,WM_USER, m_playlist_pos, 121);	// set sel
				SendMessage(m_hWndWinamp,WM_COMMAND, 40045, 0);	// play it
                return 0;
            }
        }

        // allow the plugin to override any keys:
        if (MyWindowProc(hWnd, uMsg, wParam, lParam) == 0)
            return 0;

        switch(wParam)
        {
        case VK_F1:
            m_show_press_f1_msg = 0;
            ToggleHelp();
            return 0;

        case VK_ESCAPE:
            if (m_show_help)
                ToggleHelp();
            else
            {
              if (m_screenmode == FAKE_FULLSCREEN || m_screenmode == FULLSCREEN)
              {
                ToggleFullScreen();
              }
              else if (m_screenmode == DESKTOP)
              {
                ToggleDesktop();
              }
                // exit the program on escape
                //m_exiting = 1;
                //PostMessage(hWnd, WM_CLOSE, 0, 0);
            }
            return 0;

        case VK_UP:
            // increase volume
            {
                int nRepeat = lParam & 0xFFFF;
                for (i=0; i<nRepeat*2; i++) PostMessage(m_hWndWinamp,WM_COMMAND,40058,0);
            }
            return 0;

        case VK_DOWN:
            // decrease volume
            {
                int nRepeat = lParam & 0xFFFF;
                for (i=0; i<nRepeat*2; i++) PostMessage(m_hWndWinamp,WM_COMMAND,40059,0);
            }
            return 0;

        case VK_LEFT:
        case VK_RIGHT:
            {
                bool bShiftHeldDown = (GetKeyState(VK_SHIFT) & mask) != 0;
                int cmd = (wParam == VK_LEFT) ? 40144 : 40148;
                int nRepeat = lParam & 0xFFFF;
                int reps = (bShiftHeldDown) ? 6*nRepeat : 1*nRepeat;

                for (int i=0; i<reps; i++)
                    PostMessage(m_hWndWinamp,WM_COMMAND,cmd,0);
            }
            return 0;
        default:
            // pass CTRL+A thru CTRL+Z, and also CTRL+TAB, to winamp, *if we're in windowed mode* and using an embedded window.
            // be careful though; uppercase chars come both here AND to WM_CHAR handler,
            //   so we have to eat some of them here, to avoid them from acting twice.
            if (m_screenmode==WINDOWED && m_lpDX && m_lpDX->m_current_mode.m_skin)
            {
                if ( bCtrlHeldDown && ((wParam >= 'A' && wParam <= 'Z') || wParam==VK_TAB) )
                {
                    PostMessage(m_hWndWinamp, uMsg, wParam, lParam);
                    return 0;
                }
            }
            return 0;
        }

        return 0;
        break;
    }

    return MyWindowProc(hWnd, uMsg, wParam, lParam);//DefWindowProc(hWnd, uMsg, wParam, lParam);
    //return 0L;
#endif
	return 0;
}

LRESULT CALLBACK CPluginShell::DesktopWndProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
    CPluginShell* p = (CPluginShell*)GetWindowLong(hWnd,GWL_USERDATA);
    if (p)
        return p->PluginShellDesktopWndProc(hWnd, uMsg, wParam, lParam);
    else
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
#endif
	return 0;
}

LRESULT CPluginShell::PluginShellDesktopWndProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
    //#ifdef _DEBUG
    //    OutputDebugMessage("kbfocus", hWnd, uMsg, wParam, lParam);
    //#endif

    switch (uMsg)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
    case WM_SYSCHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        //PostMessage(GetPluginWindow(), uMsg, wParam, lParam);
        PluginShellWindowProc(GetPluginWindow(), uMsg, wParam, lParam);
        return 0;
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
#endif
	return 0;
}

void CPluginShell::AlignWaves()
{
    // align waves, using recursive (mipmap-style) least-error matching
    // note: NUM_WAVEFORM_SAMPLES must be between 32 and 512.

    int align_offset[2] = { 0, 0 };

#if (NUM_WAVEFORM_SAMPLES < 512) // [don't let this code bloat our DLL size if it's not going to be used]

    int nSamples = NUM_WAVEFORM_SAMPLES;

    #define MAX_OCTAVES 10

    int octaves = floorf(logf(512-nSamples)/logf(2.0f));
    if (octaves < 4)
        return;
    if (octaves > MAX_OCTAVES)
        octaves = MAX_OCTAVES;

    for (int ch=0; ch<2; ch++)
    {
        // only worry about matching the lower 'nSamples' samples
        float temp_new[MAX_OCTAVES][512];
        float temp_old[MAX_OCTAVES][512];
        static float temp_weight[MAX_OCTAVES][512];
        static int   first_nonzero_weight[MAX_OCTAVES];
        static int   last_nonzero_weight[MAX_OCTAVES];
        int spls[MAX_OCTAVES];
        int space[MAX_OCTAVES];

        memcpy(temp_new[0], m_sound.fWaveform[ch], sizeof(float)*512);
        memcpy(temp_old[0], &m_oldwave[ch][m_prev_align_offset[ch]], sizeof(float)*nSamples);
        spls[0] = 512;
        space[0] = 512 - nSamples;

        // potential optimization: could reuse (instead of recompute) mip levels for m_oldwave[2][]?
        for (int octave=1; octave<octaves; octave++)
        {
            spls[octave] = spls[octave-1]/2;
            space[octave] = space[octave-1]/2;
            for (int n=0; n<spls[octave]; n++)
            {
                temp_new[octave][n] = 0.5f*(temp_new[octave-1][n*2] + temp_new[octave-1][n*2+1]);
                temp_old[octave][n] = 0.5f*(temp_old[octave-1][n*2] + temp_old[octave-1][n*2+1]);
            }
        }

        if (!m_align_weights_ready)
        {
            m_align_weights_ready = 1;
            for (int octave=0; octave<octaves; octave++)
            {
                int compare_samples = spls[octave] - space[octave];
                for (int n=0; n<compare_samples; n++)
                {
                    // start with pyramid-shaped pdf, from 0..1..0
                    if (n < compare_samples/2)
                        temp_weight[octave][n] = n*2/(float)compare_samples;
                    else
                        temp_weight[octave][n] = (compare_samples-1 - n)*2/(float)compare_samples;

                    // TWEAK how much the center matters, vs. the edges:
                    temp_weight[octave][n] = (temp_weight[octave][n] - 0.8f)*5.0f + 0.8f;

                    // clip:
                    if (temp_weight[octave][n]>1) temp_weight[octave][n] = 1;
                    if (temp_weight[octave][n]<0) temp_weight[octave][n] = 0;
                }

                int n = 0;
                while (temp_weight[octave][n] == 0 && n < compare_samples)
                    n++;
                first_nonzero_weight[octave] = n;

                n = compare_samples-1;
                while (temp_weight[octave][n] == 0 && n >= 0)
                    n--;
                last_nonzero_weight[octave] = n;
            }
        }

        int n1 = 0;
        int n2 = space[octaves-1];
        for (int octave = octaves-1; octave>=0; octave--)
        {
            // for example:
            //  space[octave] == 4
            //  spls[octave] == 36
            //  (so we test 32 samples, w/4 offsets)
            int compare_samples = spls[octave]-space[octave];

            int lowest_err_offset = -1;
            float lowest_err_amount = 0;
            for (int n=n1; n<n2; n++)
            {
                float err_sum = 0;
                //for (int i=0; i<compare_samples; i++)
                for (int i=first_nonzero_weight[octave]; i<=last_nonzero_weight[octave]; i++)
                {
                    float x = (temp_new[octave][i+n] - temp_old[octave][i]) * temp_weight[octave][i];
                    if (x>0)
                        err_sum += x;
                    else
                        err_sum -= x;
                }

                if (lowest_err_offset == -1 || err_sum < lowest_err_amount)
                {
                    lowest_err_offset = n;
                    lowest_err_amount = err_sum;
                }
            }

            // now use 'lowest_err_offset' to guide bounds of search in next octave:
            //  space[octave] == 8
            //  spls[octave] == 72
            //     -say 'lowest_err_offset' was 2
            //     -that corresponds to samples 4 & 5 of the next octave
            //     -also, expand about this by 2 samples?  YES.
            //  (so we'd test 64 samples, w/8->4 offsets)
            if (octave > 0)
            {
                n1 = lowest_err_offset*2  -1;
                n2 = lowest_err_offset*2+2+1;
                if (n1 < 0) n1=0;
                if (n2 > space[octave-1]) n2 = space[octave-1];
            }
            else
                align_offset[ch] = lowest_err_offset;
        }
    }
#endif
    memcpy(m_oldwave[0], m_sound.fWaveform[0], sizeof(float)*512);
    memcpy(m_oldwave[1], m_sound.fWaveform[1], sizeof(float)*512);
    m_prev_align_offset[0] = align_offset[0];
    m_prev_align_offset[1] = align_offset[1];

    // finally, apply the results: modify m_sound.fWaveform[2][0..512]
    // by scooting the aligned samples so that they start at m_sound.fWaveform[2][0].
    for (int ch=0; ch<2; ch++)
        if (align_offset[ch]>0)
        {
            for (int i=0; i<nSamples; i++)
                m_sound.fWaveform[ch][i] = m_sound.fWaveform[ch][i+align_offset[ch]];
            // zero the rest out, so it's visually evident that these samples are now bogus:
            memset(&m_sound.fWaveform[ch][nSamples], 0, (512-nSamples)*sizeof(float));
        }
}

LRESULT CALLBACK CPluginShell::VJModeWndProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
    CPluginShell* p = (CPluginShell*)GetWindowLong(hWnd,GWL_USERDATA);
    if (p)
        return p->PluginShellVJModeWndProc(hWnd, uMsg, wParam, lParam);
    else
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
#endif
	return 0;
}

LRESULT CPluginShell::PluginShellVJModeWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#if 0
    #ifdef _DEBUG
        if (message != WM_MOUSEMOVE &&
            message != WM_NCHITTEST &&
            message != WM_SETCURSOR &&
            message != WM_COPYDATA &&
            message != WM_USER)
        {
            char caption[256] = "VJWndProc: frame 0, ";
            if (m_frame > 0)
            {
		        float time = m_time;
		        int hours = (int)(time/3600);
		        time -= hours*3600;
		        int minutes = (int)(time/60);
		        time -= minutes*60;
		        int seconds = (int)time;
		        time -= seconds;
		        int dsec = (int)(time*100);
		        sprintf(caption, "VJWndProc: frame %d, t=%dh:%02dm:%02d.%02ds, ", m_frame, hours, minutes, seconds, dsec);
            }
            OutputDebugMessage(caption, hwnd, message, wParam, lParam);
        }
    #endif

    switch(message)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSCHAR:
        // pass keystrokes on to plugin!
        return PluginShellWindowProc(GetPluginWindow(),message,wParam,lParam);

    case WM_ERASEBKGND:
        // Repaint window when song is paused and image needs to be repainted:
        if (SendMessage(m_hWndWinamp,WM_USER,0,104)!=1 && m_vjd3d8_device)    // WM_USER/104 return codes: 1=playing, 3=paused, other=stopped
        {
            m_vjd3d8_device->Present(NULL,NULL,NULL,NULL);
            return 0;
        }
        break;

        /*
    case WM_WINDOWPOSCHANGING:
        if (m_screenmode == DESKTOP)
        {
            LPWINDOWPOS pwp = (LPWINDOWPOS)lParam;
            if (pwp)
                pwp->flags |= SWP_NOOWNERZORDER | SWP_NOZORDER;
        }
        break;

    case WM_ACTIVATEAPP:
        // *Very Important Handler!*
        //    -Without this code, the app would not work properly when running in true
        //     fullscreen mode on multiple monitors; it would auto-minimize whenever the
        //     user clicked on a window in another display.
        if (wParam == 1 &&
            m_screenmode == DESKTOP &&
            m_frame > 0 &&
            !m_exiting
           )
        {
            return 0;
        }
        break;

    /*
    case WM_NCACTIVATE:
        // *Very Important Handler!*
        //    -Without this code, the app would not work properly when running in true
        //     fullscreen mode on multiple monitors; it would auto-minimize whenever the
        //     user clicked on a window in another display.
        // (NOTE: main window also handles this message this way)
        if (wParam == 0 &&
            m_screenmode == FULLSCREEN &&
            m_frame > 0 &&
            !m_exiting &&
            m_lpDX &&
            m_lpDX->m_ready
            && m_lpDX->m_lpD3D &&
            m_lpDX->m_lpD3D->GetAdapterCount() > 1
            )
        {
            return 0;
        }
        break;
        */

        /*
    case WM_ACTIVATEAPP:
        if (wParam == 1 &&
            m_screenmode == DESKTOP &&
            m_frame > 0 &&
            !m_exiting &&
            m_vjd3d8_device
           )
        {
            return 0;
        }
        break;
        */

        /*
    case WM_WINDOWPOSCHANGING:
        if (
            m_screenmode == DESKTOP
            && (!m_force_accept_WM_WINDOWPOSCHANGING)
            && m_lpDX && m_lpDX->m_ready
           )
        {
            // unless we requested it ourselves or it's init time,
            // prevent the fake desktop window from moving around
            // in the Z order!  (i.e., keep it on the bottom)

            // without this code, when you click on the 'real' desktop
            // in a multimon setup, any windows that are overtop of the
            // 'fake' desktop will flash, since they'll be covered
            // up by the fake desktop window (but then shown again on
            // the next frame, when we detect that the fake desktop
            // window isn't on bottom & send it back to the bottom).

            LPWINDOWPOS pwp = (LPWINDOWPOS)lParam;
            if (pwp)
                pwp->flags |= SWP_NOOWNERZORDER | SWP_NOZORDER;
        }
        break;
        */

    case WM_CLOSE:
        // if they close the VJ window (by some means other than ESC key),
        // this will make the graphics window close, too.
        m_exiting = 1;
        if (GetPluginWindow())
            PostMessage(GetPluginWindow(), WM_CLOSE, 0, 0);
        break;

    case WM_GETMINMAXINFO:
        {
            // don't let the window get too small
            MINMAXINFO* p = (MINMAXINFO*)lParam;
            if (p->ptMinTrackSize.x < 64)
                p->ptMinTrackSize.x = 64;
            p->ptMinTrackSize.y = p->ptMinTrackSize.x*3/4;
        }
        return 0;

    case WM_SIZE:
        // clear or set activity flag to reflect focus
        if (m_vjd3d8_device && !m_resizing_textwnd)
        {
            m_hidden_textwnd = (SIZE_MAXHIDE==wParam || SIZE_MINIMIZED==wParam) ? TRUE : FALSE;

            if (SIZE_MAXIMIZED==wParam || SIZE_RESTORED==wParam) // the window has been maximized or restored
                OnUserResizeTextWindow();
        }
        break;

    case WM_ENTERSIZEMOVE:
        m_resizing_textwnd = 1;
        break;

    case WM_EXITSIZEMOVE:
        if( m_vjd3d8_device )
            OnUserResizeTextWindow();
        m_resizing_textwnd = 0;
        break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
#endif
	return 0;
}
