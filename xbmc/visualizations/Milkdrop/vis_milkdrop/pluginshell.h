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

#ifndef __NULLSOFT_DX8_PLUGIN_SHELL_H__
#define __NULLSOFT_DX8_PLUGIN_SHELL_H__ 1

#include "shell_defines.h"
#include "dxcontext.h"
#include "fft.h"
#include "defines.h"
//#include "textmgr.h"

//#include "icon_t.h"
//#include <list>
//using std::list;

//extern "C" void SetTextureStageState( int x, DWORD dwY, DWORD dwZ);
//extern "C" void d3dSetSamplerState( int x, DWORD dwY, DWORD dwZ);
//extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);

#define TIME_HIST_SLOTS 128     // # of slots used if fps > 60.  half this many if fps==30.
#define MAX_SONGS_PER_PAGE 40

typedef struct
{
    char szFace[256];
    int nSize;  // size requested @ font creation time
    int bBold;
    int bItalic;
    int bAntiAliased;
} td_fontinfo;

typedef struct
{
    float   imm[2][3];                // bass, mids, treble, no damping, for each channel (long-term average is 1)
    float    avg[2][3];               // bass, mids, treble, some damping, for each channel (long-term average is 1)
    float     med_avg[2][3];          // bass, mids, treble, more damping, for each channel (long-term average is 1)
    float      long_avg[2][3];        // bass, mids, treble, heavy damping, for each channel (long-term average is 1)
    float       infinite_avg[2][3];   // bass, mids, treble: winamp's average output levels. (1)
    float   fWaveform[2][576];             // Not all 576 are valid! - only NUM_WAVEFORM_SAMPLES samples are valid for each channel (note: NUM_WAVEFORM_SAMPLES is declared in shell_defines.h)
    float   fSpectrum[2][NUM_FREQUENCIES]; // NUM_FREQUENCIES samples for each channel (note: NUM_FREQUENCIES is declared in shell_defines.h)
} td_soundinfo;                    // ...range is 0 Hz to 22050 Hz, evenly spaced.

class CPluginShell
{
protected:
    
    // GET METHODS
    // ------------------------------------------------------------
public:
    int       GetFrame();          // returns current frame # (starts at zero)
    float     GetTime();           // returns current animation time (in seconds) (starts at zero) (updated once per frame)
    float     GetFps();            // returns current estimate of framerate (frames per second)
protected:
    eScrMode  GetScreenMode();     // returns WINDOWED, FULLSCREEN, FAKE_FULLSCREEN, DESKTOP, or NOT_YET_KNOWN (if called before or during OverrideDefaults()).
    HWND      GetWinampWindow();   // returns handle to Winamp main window
    HINSTANCE GetInstance();       // returns handle to the plugin DLL module; used for things like loading resources (dialogs, bitmaps, icons...) that are built into the plugin.
    char*     GetPluginsDirPath(); // usually returns 'c:\\program files\\winamp\\plugins\\'
    char*     GetConfigIniFile();  // usually returns 'c:\\program files\\winamp\\plugins\\something.ini' - filename is determined from identifiers in 'defines.h'

    // GET METHODS THAT ONLY WORK ONCE DIRECTX IS READY
    // ------------------------------------------------------------
    //  The following 'Get' methods are only available after DirectX has been initialized.
    //  If you call these from OverrideDefaults, MyPreInitialize, or MyReadConfig, 
    //    they will return NULL (zero).
    // ------------------------------------------------------------
    HWND         GetPluginWindow();      // returns handle to the plugin window.  NOT persistent; can change!  
    int          GetWidth();             // returns width of plugin window interior, in pixels.
    int          GetHeight();            // returns height of plugin window interior, in pixels.
    int          GetBitDepth();          // returns 8, 16, 24 (rare), or 32
    LPDIRECT3DDEVICE9  GetDevice();      // returns a pointer to the DirectX 8 Device.  NOT persistent; can change!
    D3DCAPS9*    GetCaps();              // returns a pointer to the D3DCAPS8 structer for the device.  NOT persistent; can change.
    D3DFORMAT    GetBackBufFormat();     // returns the pixelformat of the back buffer (probably D3DFMT_R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_R5G6B5, D3DFMT_X1R5G5B5, D3DFMT_A1R5G5B5, D3DFMT_A4R4G4B4, D3DFMT_R3G3B2, D3DFMT_A8R3G3B2, D3DFMT_X4R4G4B4, or D3DFMT_UNKNOWN)
    D3DFORMAT    GetBackBufZFormat();    // returns the pixelformat of the back buffer's Z buffer (probably D3DFMT_D16_LOCKABLE, D3DFMT_D32, D3DFMT_D15S1, D3DFMT_D24S8, D3DFMT_D16, D3DFMT_D24X8, D3DFMT_D24X4S4, or D3DFMT_UNKNOWN)
    char*        GetDriverFilename();    // returns a text string with the filename of the current display adapter driver, such as "nv4_disp.dll"
    char*        GetDriverDescription(); // returns a text string describing the current display adapter, such as "NVIDIA GeForce4 Ti 4200"

    // FONTS & TEXT
    // ------------------------------------------------------------
public:
//    LPD3DXFONT   GetFont(eFontIndex idx);       // returns a D3DX font handle for drawing text; see shell_defines.h for the definition of the 'eFontIndex' enum.
//    int          GetFontHeight(eFontIndex idx); // returns the height of the font, in pixels; see shell_defines.h for the definition of the 'eFontIndex' enum.
//    CTextManager m_text;
protected:

    // MISC
    // ------------------------------------------------------------
    td_soundinfo m_sound;                   // a structure always containing the most recent sound analysis information; defined in pluginshell.h.
    void         SuggestHowToFreeSomeMem(); // gives the user a 'smart' messagebox that suggests how they can free up some video memory.

    // CONFIG PANEL SETTINGS
    // ------------------------------------------------------------
    // *** only read/write these values during CPlugin::OverrideDefaults! ***
    int          m_start_fullscreen;        // 0 or 1
    int          m_start_desktop;           // 0 or 1
    int          m_fake_fullscreen_mode;    // 0 or 1
    // JM HACK
public:
  int          m_max_fps_fs;              // 1-120, or 0 for 'unlimited'
protected:
  // JM HACK
    int          m_max_fps_dm;              // 1-120, or 0 for 'unlimited'
    int          m_max_fps_w;               // 1-120, or 0 for 'unlimited'
    int          m_show_press_f1_msg;       // 0 or 1
    int          m_allow_page_tearing_w;    // 0 or 1
    int          m_allow_page_tearing_fs;   // 0 or 1
    int          m_allow_page_tearing_dm;   // 0 or 1
    int          m_minimize_winamp;         // 0 or 1
    int          m_desktop_show_icons;      // 0 or 1
    int          m_desktop_textlabel_boxes; // 0 or 1
    int          m_desktop_manual_icon_scoot; // 0 or 1
    int          m_desktop_555_fix;         // 0 = 555, 1 = 565, 2 = 888
    int          m_dualhead_horz;           // 0 = both, 1 = left, 2 = right
    int          m_dualhead_vert;           // 0 = both, 1 = top, 2 = bottom
    int          m_save_cpu;                // 0 or 1
    int          m_skin;                    // 0 or 1
    int          m_fix_slow_text;           // 0 or 1
    td_fontinfo  m_fontinfo[NUM_BASIC_FONTS + NUM_EXTRA_FONTS];
    D3DDISPLAYMODE m_disp_mode_fs;          // a D3DDISPLAYMODE struct that specifies the width, height, refresh rate, and color format to use when the plugin goes fullscreen.
	int			m_posX;
	int			m_posY;
	int			m_backBufferWidth;
	int			m_backBufferHeight;
  float   m_pixelRatio;

    // PURE VIRTUAL FUNCTIONS (...must be implemented by derived classes)
    // ------------------------------------------------------------
	virtual void OverrideDefaults()		 {};
	virtual void MyPreInitialize()       {};
	virtual void MyReadConfig()          {};
	virtual void MyWriteConfig()         {};
	virtual int  AllocateMyNonDx8Stuff() { return 0; };
	virtual void  CleanUpMyNonDx8Stuff() {};
	virtual int  AllocateMyDX8Stuff()    { return 0; };
	virtual void  CleanUpMyDX8Stuff(int final_cleanup) {};
	virtual void MyRenderFn(int redraw)  {};
	virtual void MyRenderUI(int *upper_left_corner_y, int *upper_right_corner_y, int *lower_left_corner_y, int *lower_right_corner_y, int xL, int xR) {};
	virtual LRESULT MyWindowProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam) { return 0; };
	virtual BOOL MyConfigTabProc(int nPage, HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam) { return true;};

//=====================================================================================================================
private:

    // GENERAL PRIVATE STUFF
    eScrMode     m_screenmode;      // // WINDOWED, FULLSCREEN, or FAKE_FULLSCREEN (i.e. running in a full-screen-sized window)
    int          m_frame;           // current frame #, starting at zero
    float        m_time;            // current animation time in seconds; starts at zero.
    float        m_fps;             // current estimate of frames per second
    HWND         m_hWndWinamp;      // handle to Winamp window 
    HINSTANCE    m_hInstance;       // handle to application instance
    DXContext*   m_lpDX;            // pointer to DXContext object
    char         m_szPluginsDirPath[MAX_PATH];  // usually 'c:\\program files\\winamp\\plugins\\'
    char         m_szConfigIniFile[MAX_PATH];   // usually 'c:\\program files\\winamp\\plugins\\something.ini' - filename is determined from identifiers in 'defines.h'
    LPDIRECT3DDEVICE9   m_device;
    // FONTS
	IDirect3DTexture9* m_lpDDSText;
//    LPD3DXFONT   m_d3dx_font[NUM_BASIC_FONTS + NUM_EXTRA_FONTS];
//    LPD3DXFONT   m_d3dx_desktop_font;
//    HFONT        m_font[NUM_BASIC_FONTS + NUM_EXTRA_FONTS];
//    HFONT        m_font_desktop;
    
    // PRIVATE CONFIG PANEL SETTINGS
    D3DMULTISAMPLE_TYPE m_multisample_fullscreen;
    D3DMULTISAMPLE_TYPE m_multisample_desktop;
    D3DMULTISAMPLE_TYPE m_multisample_windowed;
    GUID m_adapter_guid_fullscreen;
    GUID m_adapter_guid_desktop;
public:
    GUID m_adapter_guid_windowed;
private:

    // PRIVATE RUNTIME SETTINGS
    int m_lost_focus;     // ~mostly for fullscreen mode
    int m_hidden;         // ~mostly for windowed mode
    int m_resizing;       // ~mostly for windowed mode
    int m_show_help;
    int m_show_playlist;
    int  m_playlist_pos;            // current selection on (plugin's) playlist menu
    int  m_playlist_pageups;        // can be + or -
    int  m_playlist_top_idx;        // used to track when our little playlist cache (m_playlist) needs updated.
    int  m_playlist_btm_idx;        // used to track when our little playlist cache (m_playlist) needs updated.
    int  m_playlist_width_pixels;   // considered invalid whenever 'm_playlist_top_idx' is -1.
    char m_playlist[MAX_SONGS_PER_PAGE][256];   // considered invalid whenever 'm_playlist_top_idx' is -1.
    int m_exiting;
    int m_upper_left_corner_y;
    int m_lower_left_corner_y;
    int m_upper_right_corner_y;
    int m_lower_right_corner_y;
    int m_left_edge;
    int m_right_edge;
    int m_force_accept_WM_WINDOWPOSCHANGING;

    // PRIVATE - GDI STUFF
    HMENU               m_main_menu;
    HMENU               m_context_menu;

    // PRIVATE - DESKTOP MODE STUFF
//    list<icon_t>        m_icon_list;
    IDirect3DTexture9*  m_desktop_icons_texture[MAX_ICON_TEXTURES];
    HWND                m_hWndProgMan;
    HWND                m_hWndDesktop;
    HWND                m_hWndDesktopListView;
    char                m_szDesktopFolder[MAX_PATH];   // *without* the final backslash
    int                 m_desktop_icon_size;
    int                 m_desktop_dragging;  // '1' when user is dragging icons around
    int                 m_desktop_box;       // '1' when user is drawing a box
    BYTE                m_desktop_drag_pidl[1024]; // cast this to ITEMIDLIST
    POINT               m_desktop_drag_startpos; // applies to dragging or box-drawing
    POINT               m_desktop_drag_curpos;   // applies to dragging or box-drawing
    int                 m_desktop_wc_registered;
    DWORD               m_desktop_bk_color;
    DWORD               m_desktop_text_color;
    DWORD               m_desktop_sel_color;
    DWORD               m_desktop_sel_text_color;
    int                 m_desktop_icon_state;   // 0=uninit, 1=total refresh in progress, 2=ready, 3=update in progress
    int                 m_desktop_icon_count;
    int                 m_desktop_icon_update_frame;
//    CRITICAL_SECTION    m_desktop_cs;
    int                 m_desktop_icons_disabled;
    int                 m_vms_desktop_loaded;
    int                 m_desktop_hook_set;

    // PRIVATE - MORE TIMEKEEPING
    double m_last_raw_time;
    float  m_time_hist[TIME_HIST_SLOTS];		// cumulative
    int    m_time_hist_pos;
    LARGE_INTEGER m_high_perf_timer_freq;  // 0 if high-precision timer not available
    LARGE_INTEGER m_prev_end_of_frame;

    // PRIVATE AUDIO PROCESSING DATA
    FFT   m_fftobj;
    float m_oldwave[2][576];        // for wave alignment
    int   m_prev_align_offset[2];   // for wave alignment
    int   m_align_weights_ready;

public:
    CPluginShell();
    ~CPluginShell();
    
    // called by vis.cpp, on behalf of Winamp:
    int  PluginPreInitialize(HWND hWinampWnd, HINSTANCE hWinampInstance);    
    int  PluginInitialize(LPDIRECT3DDEVICE9 device, int iPosX, int iPosY, int iWidth, int iHeight, float pixelRatio);                                                
    int  PluginRender(unsigned char *pWaveL, unsigned char *pWaveR);
    void PluginQuit();
    int  AllocateDX8Stuff();
    void CleanUpDX8Stuff(int final_cleanup);

    void ToggleHelp();
    void TogglePlaylist();
    
    // config panel / windows messaging processes:
    static LRESULT CALLBACK WindowProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK DesktopWndProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK VJModeWndProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);
    static BOOL    CALLBACK ConfigDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
    static BOOL    CALLBACK TabCtrlProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
    static BOOL    CALLBACK FontDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
    static BOOL    CALLBACK DesktopOptionsDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
    static BOOL    CALLBACK DualheadDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

private:
    void PushWindowToJustBeforeDesktop(HWND h);
    void DrawAndDisplay(int redraw);
    void ReadConfig();
    void WriteConfig();
    void DoTime();
    void AnalyzeNewSound(unsigned char *pWaveL, unsigned char *pWaveR);
    void AlignWaves();
    int  InitDirectX();
    void CleanUpDirectX();
    int  InitGDIStuff();
    void CleanUpGDIStuff();
    int  InitNonDx8Stuff();
    void CleanUpNonDx8Stuff();
    int  InitVJStuff(RECT* pClientRect=NULL);
    void CleanUpVJStuff();
    int  AllocateFonts(IDirect3DDevice9 *pDevice);
    void CleanUpFonts();
    void AllocateTextSurface();
    void ToggleDesktop();
    void OnUserResizeWindow();
    void OnUserResizeTextWindow();
    void PrepareFor2DDrawing_B(IDirect3DDevice9 *pDevice, int w, int h);
    void RenderBuiltInTextMsgs();
public:
    void ToggleFullScreen();
    void DrawDarkTranslucentBox(RECT* pr);
private:
    void RenderPlaylist();
    void StuffParams(DXCONTEXT_PARAMS *pParams);
    void EnforceMaxFPS();

    // DESKTOP MODE FUNCTIONS (found in desktop_mode.cpp)
    int  InitDesktopMode();
    void CleanUpDesktopMode();
    int  CreateDesktopIconTexture(IDirect3DTexture9** ppTex);
    void DeselectDesktop();
    void UpdateDesktopBitmaps();
    int  StuffIconBitmaps(int iStartIconIdx, int iTexNum, int *show_msgs);
    void RenderDesktop();

    // SEPARATE TEXT WINDOW (FOR VJ MODE)
	  int 		m_vj_mode;
      int       m_hidden_textwnd;
      int       m_resizing_textwnd;
      protected:
	   HWND		m_hTextWnd;
      private:
	  int		m_nTextWndWidth;
	  int		m_nTextWndHeight;
	  bool		m_bTextWindowClassRegistered;
      LPDIRECT3D9 m_vjd3d8;
      LPDIRECT3DDEVICE9 m_vjd3d8_device;
	  //HDC		m_memDC;		// memory device context
	  //HBITMAP m_memBM, m_oldBM;
	  //HBRUSH  m_hBlackBrush;

    // WINDOWPROC FUNCTIONS
    LRESULT PluginShellWindowProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);   // in windowproc.cpp
    LRESULT PluginShellDesktopWndProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT PluginShellVJModeWndProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);

    // CONFIG PANEL FUNCTIONS:
    BOOL    PluginShellConfigDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
    BOOL    PluginShellConfigTab1Proc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
    BOOL    PluginShellFontDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
    BOOL    PluginShellDesktopOptionsDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
    BOOL    PluginShellDualheadDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
    bool    InitConfig(HWND hDialogWnd);
    void    EndConfig();
    void    UpdateAdapters(int screenmode);
    void    UpdateFSAdapterDispModes();   // (fullscreen only)
    void    UpdateDispModeMultiSampling(int screenmode);
    void    UpdateMaxFps(int screenmode);
    int     GetCurrentlySelectedAdapter(int screenmode);
    void    SaveDisplayMode();
    void    SaveMultiSamp(int screenmode);
    void    SaveAdapter(int screenmode);
    void    SaveMaxFps(int screenmode);
    void    OnTabChanged(int nNewTab);
    LPDIRECT3DDEVICE9 GetTextDevice() { return (m_vjd3d8_device) ? m_vjd3d8_device : m_lpDX->m_lpDevice; }
};



























#endif