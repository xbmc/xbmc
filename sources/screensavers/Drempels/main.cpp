/* 
 *  This source file is part of Drempels, a program that falls
 *  under the Gnu Public License (GPL) open-source license.  
 *  Use and distribution of this code is regulated by law under 
 *  this license.  For license details, visit:
 *    http://www.gnu.org/copyleft/gpl.html
 * 
 *  The Drempels open-source project is accessible at 
 *  sourceforge.net; the direct URL is:
 *    http://sourceforge.net/projects/drempels/
 *  
 *  Drempels was originally created by Ryan M. Geiss in 2001
 *  and was open-sourced in February 2005.  The original
 *  Drempels homepage is available here:
 *    http://www.geisswerks.com/drempels/
 *
 */

 /*
    DREMPELS, IN BRIEF
        Drempels is an application that replaces your desktop background 
        (or "wallpaper") with smoothly-animated psychedelic graphics.  
        It renders the graphics on the CPU at a low resolution, and then 
        uses a hardware overlay to stretch the image to fit on the entire screen.  
        At video scan time, any color that is the same as the "overlay key color" 
        (15,0,15 by default) will be replaced by the Drempels image, so we paint 
        the wallpaper with the overlay key color to ensure that the Drempels image 
        shows up where your wallpaper would normally be.  

        Drempels can also be run as a screensaver.  The separate project, 
        drempels_scr.dsw, will build a tiny screensaver that simply calls the 
        main drempels.exe when it's time to run the screensaver.

        Run drempels.exe with no arguments to start it in desktop mode.  Run 
        "drempels /s" to run it fullscreen (screensaver mode), and run 
        "drempels /c" to configure it.
    
        Drempels is currently ported only to the Windows platform.  
        It requires an MMX processor, DirectX 5.0 or later, and hardware 
        overlay support (which is almost universal, since many video players 
        use it).

    SOURCE FILE OVERVIEW
        main.cpp     - the bulk of the code
        gpoly.cpp/h  - assembly code for the rasterization of the main image.
        video.h      - assembly code for blitting image to the screen.  
                         includes stretch factors of 1X, 2X, and 4X.
        yuv.cpp/h    - assembly code for conversion from RGB to YUV colorspace
                         (for blitting to overlay surfaces).
        sysstuff.h   - code for reading/writing the registry, checking for MMX, etc.
        texmgr.cpp/h - class that loads and manages the source bitmaps

    
	RELEASING A NEW VERSION: checklist:
		-update version # identifier and on the 2 dialogs (config + about)
		-update version # in the DREMPELS.NSI (installer) file (2 occurences)
		-put release date in DREMPELS.TXT (x2)
		-compile in RELEASE mode
		-post 2 zips and 1 txt on web site
		-backup frozen source

	NOTES ON INVOKING DREMPELS:
	    -startup folder:  'drempels.exe'    ( -> runs in app mode, where g_bRunningAsSaver is true)
	    -drempels folder: 'drempels.exe'    ( -> runs in app mode, where g_bRunningAsSaver is true)
	    -screensaver:     'drempels.scr [/s|/c]'   ( -> runs drempels.exe [/s|/c])

    -------------------------------------------------------------------------------

	to do:
        -figure out how to get notification when monitor turns back on 
            after powering off (and call Update_Overlay() then?) 
		-screensaver seems to not always kick in on XP systems 
            (and I've heard that maybe it just doesn't work on some XP systems). 
        -a feature to automatically turn off 'active desktop' would be cool. 
        -port to non-Windows platforms 
        -make it multiple-monitors friendly (...for a good reference on how 
            to make it multiple-monitor friendly, check out the VMS sample 
            Winamp plug-in, which is 100% multimon-happy: http://www.nullsoft.com/free/vms/ ).
        -
        -
        -
  */

// VS6 project settings:
// ---------------------
//  libs:  comctl32.lib ddraw.lib dsound.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib dxguid.lib winmm.lib LIBCMT.LIB 
//  settings:  C/C++ -> code generation -> calling convention is __stdcall.




#define CURRENT_VERSION 150                 // ex: "140" means v1.40

// size of the main grid for computation of motion vectors:
// (motion vectors are then bicubically interpolated, per-pixel, within grid cells)
#define UVCELLSX 36 //18
#define UVCELLSY 28 //14

#define WM_DREMPELS_SYSTRAY_MSG			WM_USER + 407
#define IDC_DREMPELS_SYSTRAY_ICON			555
#define ID_DREMPELS_SYSTRAY_CLOSE			556
#define ID_DREMPELS_SYSTRAY_HIDE_CTRL_WND	557
#define ID_DREMPELS_SYSTRAY_SHOW_CTRL_WND	558
#define ID_DREMPELS_SYSTRAY_RESUME			559
#define ID_DREMPELS_SYSTRAY_SUSPEND			560
#define ID_DREMPELS_SYSTRAY_HOTKEYS			561


//#include <windows.h>
#include <xtl.h>
//#include <regstr.h>
#include <stdlib.h>
//#include <shlobj.h>  // for BROWSEINFO struct (for SHBrowseForFolder)

extern LPDIRECT3DDEVICE8       g_pd3dDevice;

extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);

// PARAMETERS---------------

// hidden:
float warp_factor = 0.22f;			// 0.05 = no warping, 0.25 = good warping, 0.75 = chaos
float rotational_speed = 0.05f;
float mode_focus_exponent = 2.9f;	// 1 = simple linear combo (very blendy) / 4 = fairly focused / 16 = 99% singular
int limit_fps = 30;
int tex_scale = 10;			// goes from 0 to 20; 10 is normal; 0 is down 4X; 20 is up 4X
int speed_scale = 10;		// goes from 0 to 20; 10 is normal; 0 is down 8X; 20 is up 8X

// exposed:
bool  g_bResize = true;
bool  g_bAutoBlend = false;
int   g_iBlendPercent = 7;     

float time_between_textures = 20.0f;
float texture_fade_time = 5.0f;
char  szTexPath[MAX_PATH] = "q:\\screensavers\\Drempels";
//char  szTexPathCurrent[MAX_PATH] = "c:\\program files\\drempels";
int   g_nTimeBetweenSubdirs = 120;
int   g_subdirIndices[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
int   g_nSubdirs = 9;
bool  g_bAutoSwitchSubdirs = false;
bool  g_bAutoSwitchRandom = false;
float g_fTimeTilSubdirSwitch = 120;
int   g_nSubdirIndex = -1;
float anim_speed = 1.0f;
float master_zoom = 1.0f;	// 2 = zoomed in 2x, 0.5 = zoomed OUT 2x.
float mode_switch_speed_multiplier = 1.0f;		// 1 = normal speed, 2 = double speed, 0.5 = half speed
int   motion_blur = 7;		// goes from 0 to 10
int   g_bExitOnMouseMove = true;
int   g_iMouseMoves = 0;
int   g_iMouseMovesThisFrame = 0;
int   g_bAnimFrozen = 0;
int   g_bPausedViaMinimize = 0;
int   g_bForceRepaintOnce = 0;
//int   g_bGeissMode  = 0;
int   g_bSuspended = 0;		// only available in Desktop mode; means old wallpaper is back & drempels is suspended.
int   g_bTrackingPopupMenu = false;

DWORD mainproc_start_time;


bool g_bIsRunningAsSaver = false;	// set to TRUE if /s flag is present
bool g_bStartupMode = false;			// set to TRUE if /y flag is present
bool g_bRunDrempelsAtStartup = false;

// procsize: 0=full, 1=half, 2=quarter
int  procsize_as_app   = 2;
int  procsize_as_saver = 0;
int  procsize = 1;			    

// rendermode:
// 0 fullscreen, 
// 1 windowed, 
// 2 background mode
// 3 desktop mode
int   rendermode_as_app   = 3;
int   rendermode_as_saver = 0;
int   rendermode = 0;		

bool  high_quality = true;


//char szInstallDir[512] = "c:\\program files\\drempels";
//char szKeyWallpaper[512] = "drempels.dat";
//char szOldBackgroundColor[512] = "0 0 0";
//char szOldWallpaper[512] = "";
DWORD dwOldBackgroundColor = 0;
bool bOldWallpaperKnown = false;
bool bWallpaperSet = false;
unsigned char key_R = 15;	// christophe uses 0,5,8
unsigned char key_G = 0;	// 15,0,15 works on voodoo3 in 16-bit color!  
unsigned char key_B = 15;	//   (but 15,15,15 doesn't - dither problems)

extern int g_width;
extern int g_height;


bool  g_bCoeffsFrozen = false;
bool  g_bToggleCoeffFreeze = false;
float g_fCoeffFreezeTime;
float g_fCoeffFreezeDebt = 0;

// coords of the window that accomodates the FXWxFXH drawing area,
// if the drawing area upper-left corner is located at 0x0.
RECT g_clientRect;

//COLORREF g_FgTextColor = RGB(255,255,255);
//COLORREF g_BkgTextColor = RGB(0,0,0); 

float g_fBlurAmount = 0.88f;	// goes from 0 to 0.97

bool bWarpmodePaused = false;


BOOL MungeFPCW( WORD *pwOldCW )
{
    BOOL ret = FALSE;
    WORD wTemp, wSave;
 
    __asm fstcw wSave
    if (wSave & 0x300 ||            // Not single mode
        0x3f != (wSave & 0x3f) ||   // Exceptions enabled
        wSave & 0xC00)              // Not round to nearest mode
    {
        __asm
        {
            mov ax, wSave
            and ax, not 300h    ;; single mode
            or  ax, 3fh         ;; disable all exceptions
            and ax, not 0xC00   ;; round to nearest mode
            mov wTemp, ax
            fldcw   wTemp
        }
        ret = TRUE;
    }
    *pwOldCW = wSave;
    return ret;
}
 

void RestoreFPCW(WORD wSave)
{
    __asm fldcw wSave
}



#define WIN32_LEAN_AND_MEAN

#pragma hdrstop
//#include "resource.h"
#include <stdio.h>		// for sprintf() for fps display

//#define _MT             // for multithreading
//#include <process.h>    // for multithreading


#define NAME "Drempels"
#define TITLE "Drempels"

//#include <windowsx.h>
//#include <ddraw.h>
#include <time.h>
#include <math.h>
//#include <mmreg.h>
//#include <commctrl.h>
//#include <shellapi.h>

//#include "yuv.h"

//#define APPREGPATH "SOFTWARE\\drempels"

//----------------------------------------------------

//bool CheckMMXTechnology();

//void SetWallpaper();
//void RestoreWallpaper();
//void Suspend(bool bSuspend);
//void ToggleHelpPanel();
//void ToggleControlWindow();

BOOL FX_Init();
void FX_Fini();
//bool TryLockSurface(LPDIRECTDRAWSURFACE lpDDsurf, LPDDSURFACEDESC pDDSD);
//void Put_Helpmsg_To_Backbuffer (COLORREF *fg, COLORREF *bg);
//void Put_FPS_To_Backbuffer     (COLORREF *fg, COLORREF *bg, int ypos, bool bRightJustify = false);
//void Put_Trackmsg_To_Backbuffer(COLORREF *fg, COLORREF *bg, int ypos, bool bRightJustify = false);
//void Put_Msg_To_Backbuffer(char *str, COLORREF *fg, COLORREF *bg, int ypos, bool bRightJustify = false);

//BOOL CALLBACK ConfigDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
//BOOL CALLBACK HotkeyDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
//BOOL CALLBACK DisclaimerDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
//BOOL CALLBACK AboutDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

void DrempelsExit();

BOOL doInit();
//void finiObjects( void );
//long FAR PASCAL WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

//void FX_Screenshot(int fr);
//void FX_Loadimage(char *s);
//void ReadConfigRegistry();
//void WriteConfigRegistry();
//void dumpmsg(char *s);

//----------------------------------------------------

HWND hSaverMainWindow;
HWND g_hWndHotkeys = NULL;
bool g_bSystrayReady = false;
LPGUID g_lpGuid;
float debug_param = 1.0;

//----------------------------------------------------

int  VidMode = 0;
BOOL VidModeAutoPicked = false;
unsigned int  iNumVidModes = 0;  // set by DDraw's enumeration
unsigned int  iDispBits=24;
long FXW = 640;         // to set default pick, go to EnumModesCallback()
long FXH = 480;         // to set default pick, go to EnumModesCallback()
long FXW2 = FXW;
long FXH2 = FXH;



typedef struct
{
    int  iDispBits;
    int  FXW;
    int  FXH;
    BOOL VIDEO_CARD_555;
    char name[64];
} VidModeInfo;
#define MAX_VID_MODES 512
VidModeInfo VidList[MAX_VID_MODES];



    int  ThreadNr = 0;
    BOOL g_QuitASAP = false;
    BOOL g_MainProcFinished = false;
    BOOL g_bFirstRun = false;
    BOOL g_bDumpFileCleared = false;
    BOOL g_bDebugMode = false;
    BOOL g_bSuppressHelpMsg = false;
    BOOL g_bSuppressAllMsg = false;
    BOOL g_DisclaimerAgreed = true;
    BOOL g_ConfigAccepted = false;
    BOOL g_Capturing = false;
    BOOL g_bTakeScreenshot = false;
    BOOL g_bLostFocus = false;
    BOOL g_bUserAltTabbedOut = false;

    /*-------------------------------------------------*/

    bool  bMMX;
    bool  bExitOnMouse = FALSE;
    int   last_mouse_x = -1;
    int   last_mouse_y = -1;

	// fps info
	#define FPS_FRAMES 32
	float fps = 0;			// fps over the last FPS_FRAMES frames, **factoring out the time spent blending textures**
	float honest_fps = 0;	// honest fps over the last 32 frames (for report to user)
	float tex_fade_debt[FPS_FRAMES];
	int   tex_fade_debt_pos	= 0;
	float time_rec[FPS_FRAMES];
	int   time_rec_pos = 0;
	float sleep_amount = 0;		// adjusted to maintain limit_fps
	float avg_texfade_time = 0;

	void RandomizeStartValues();
	float fRandStart1;
	float fRandStart2;
	float fRandStart3;
	float fRandStart4;
	float warp_w[4];
	float warp_uscale[4];
	float warp_vscale[4];
	float warp_phase[4];

    char szDEBUG[512];
	char szMCM[] = " [click mouse button to exit - press h for help] ";
	char szTEXNAME[512];
	char szTEXLOADFAIL[512];
	char szBehLocked[] = " - behavior is LOCKED - ";
	char szBehUnlocked[] = " - behavior is unlocked - ";
	char szLocked[] = " - texture is LOCKED - ";
	char szUnlocked[] = " - texture is unlocked - ";
    char szTrack[] = " - stopped at track xxx/yyy -              ";
	
    char szH1[]  = " SPACE: load random texture ";
	char szH2[]  = " R: randomize texture & behavior ";
	char szH3[]  = " T/B: lock/unlock cur. texture/behavior "; //"z x c v b:  << play pause stop >> (for CD) ";
	char szH4[]  = " Q: toggle texture quality (hi/lo) "; 
	char szH5[]  = " +/-: adjust motion blur ";
	char szH6[]  = " J/K: adjust zoom, U/I: adjust speed ";
	char szH7[]  = " H/F: display help/fps ";
	char szH8[]  = " P: [un]pause animation ";
	char szH9[]  = " M: toggle messages on/off ";//" @: save screenshot to C:\\ ";
	char szH10[] = " ESC: quit, N: minimize ";
	char szH11[] = "";//" F5: refresh texture file listing ";
    char szCurrentCD[128];
    char szNewCD[128];

//    fourcc_enum             g_eOverlayFormat;
//	DDCAPS                  g_ddcaps;
//	DDCOLORKEY              g_ddck;
//	LPDIRECTDRAW            lpDD = NULL;           // DirectDraw object
	//LPDIRECTDRAW2           lpDD2 = NULL;          // DirectDraw object
//	LPDIRECTDRAWSURFACE     lpDDSPrimary = NULL;   
//	LPDIRECTDRAWSURFACE     lpDDSBack    = NULL;      
//	LPDIRECTDRAWSURFACE     lpDDSMsg     = NULL;      
//	LPDIRECTDRAWSURFACE     lpDDSOverlay = NULL;   
//	LPDIRECTDRAWSURFACE     lpDDSVS[2] = { NULL, NULL };      // DirectDraw system memory surface #1
    LPDIRECT3DTEXTURE8 lpD3DVS[2] = {NULL, NULL};

//	bool                    bFrontShowing = true;
//	bool                    g_bDisplayModeChanged = false;
    
//    void                    Destroy_Overlay();
//    bool                    Init_Overlay(bool bShowMsgBoxes);
//    bool                    Update_Overlay();

	//BOOL                    bActive;        // is application active?
	BOOL					bUserQuit = FALSE;

	int gXC, gYC;

    char winpath[512];

	long  intframe=0;
	unsigned char VIDEO_CARD_555=0;		// ~ VIDEO_CARD_IS_555
    unsigned char SHOW_DEBUG          = 0;
	unsigned char SHOW_MOUSECLICK_MSG = 30;
    unsigned char SHOW_TRACK_MSG      = 0;
	unsigned char SHOW_MODEPREFS_MSG  = 0;
	unsigned char SHOW_LOCKED         = 0;
	unsigned char SHOW_UNLOCKED       = 0;
	unsigned char SHOW_BEH_LOCKED     = 0;
	unsigned char SHOW_BEH_UNLOCKED   = 0;
	unsigned char SHOW_TEXLOADFAIL    = 0;
	unsigned char SHOW_TEXNAME        = 0;
	bool          SHOW_FPS            = 0;
	bool          SHOW_HELP_MSG       = 0;


    extern HINSTANCE hMainInstance; /* screen saver instance handle  */ 
    HINSTANCE g_hSaverMainInstance = NULL;

    enum TScrMode {smNone,smConfig,smPassword,smPreview,smSaver};
    TScrMode ScrMode=smNone;

//#include "video.h"
//#include "sysstuff.h"
#include "gpoly.h"
#include "texmgr.h"

texmgr TEX;
int    g_bRandomizeTextures = true;
float  g_fTexStartTime = 0.0f;
int    g_bTexFading = false;
int    g_bTexLocked = false;
td_cellcornerinfo cell[UVCELLSX][UVCELLSY];

HINSTANCE hInstance=NULL;
HWND hScrWindow=NULL;

/*----------------------------------------------------------------------------------*/

//BOOL DlgItemIsChecked(HWND hDlg, int nIDDlgItem) {
//    return ((SendDlgItemMessage(hDlg, nIDDlgItem, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0) == BST_CHECKED) ? TRUE : FALSE); 
//}


/*
HRESULT WINAPI EnumModesCallback(
  LPDDSURFACEDESC lpDDSurfaceDesc,  
  LPVOID lpContext                  
)
{
    char modemsg[256];
    sprintf(modemsg, "video mode: %dx%dx%d, linearsize=%d, Rmask=%x, Gmask=%x, Bmask=%x, RGBAmask=%x ", 
        lpDDSurfaceDesc->dwWidth, 
        lpDDSurfaceDesc->dwHeight, 
        lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount, 
        lpDDSurfaceDesc->dwLinearSize,
        lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask,
        lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask,
        lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask,
        lpDDSurfaceDesc->ddpfPixelFormat.dwRGBAlphaBitMask
    );

    int w = lpDDSurfaceDesc->dwWidth;
    int h = lpDDSurfaceDesc->dwHeight;
    int bits = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;

	if ((lpDDSurfaceDesc->dwFlags | DDSD_PITCH) &&
        lpDDSurfaceDesc->dwLinearSize > 0 &&
        (bits==16 || bits==24 || bits==32) && 
	        (w==320 || w==400 || w==480 || w==512 || w==640 || w==800 || w==1024 || w==1280 || w==1600)
	   )
    {

        // determine # of green bits for 16-bit color modes
        int gmask, gbits=0;
        gmask = lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask;
        while (gmask>0) { gbits += gmask & 1; gmask >>= 1; }
    

        if (g_bFirstRun &&             // if no reg. settings were found
            w == 640 && 
            h == 480 && 
            bits > 8 && 
            !VidModeAutoPicked)
        {
            VidMode = iNumVidModes;
            VidModeAutoPicked = true;
        }


	  // if we're enumerating for config panel, if this video mode is the one they have picked, we need
	  // to remember that so it shows up as selected in the combobox.
        if (!g_bFirstRun &&
            w == FXW &&
		    h == FXH &&
            bits == iDispBits &&
            (gbits == 5) == VIDEO_CARD_555)
        {
            VidMode = iNumVidModes;
        }

       
        // collect mode info in VidList[]

        VidList[iNumVidModes].VIDEO_CARD_555 = (gbits == 5);   // regular = 5/6/5, fucked = (1)/5/5/5
		VidList[iNumVidModes].FXW = w;
		VidList[iNumVidModes].FXH = h;
		VidList[iNumVidModes].iDispBits = bits;
		sprintf(VidList[iNumVidModes].name, " %d x %d   %d-bit", 
				lpDDSurfaceDesc->dwWidth, 
				lpDDSurfaceDesc->dwHeight,
				lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount
			);
        if (VidList[iNumVidModes].iDispBits==8) strcat(VidList[iNumVidModes].name, "       [recommended]");

        if (iNumVidModes < MAX_VID_MODES)     
        {
            iNumVidModes++;
        }
        
        if (iNumVidModes >= MAX_VID_MODES)
        {
            dumpmsg("**********TOO MANY VIDEO MODES*************!!!!!!");
        }

        strcat(modemsg, " - okay");
	}

    dumpmsg(modemsg);

    return DDENUMRET_OK;

}
*/
struct TEXVERTEX
{
	FLOAT x, y, z, w;	// Position
	DWORD colour;		// Colour
	float u, v;			// Texture coords
};

#define	D3DFVF_TEXVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)



void RandomizeStartValues()
{
	if (g_bCoeffsFrozen) return;

	fRandStart1 = 6.28f*(rand() % 4096)/4096.0f;
	fRandStart2 = 6.28f*(rand() % 4096)/4096.0f;
	fRandStart3 = 6.28f*(rand() % 4096)/4096.0f;
	fRandStart4 = 6.28f*(rand() % 4096)/4096.0f;

	// randomize rotational direction
	if (rand()%2) rotational_speed *= -1.0f;

	// randomize warping parameters
	for (int i=0; i<4; i++)
	{
		warp_w[i]      = 0.02f + 0.015f*(rand()%4096)/4096.0f; 
		warp_uscale[i] = 0.23f + 0.120f*(rand()%4096)/4096.0f; 
		warp_vscale[i] = 0.23f + 0.120f*(rand()%4096)/4096.0f; 
		warp_phase[i]  = 6.28f*(rand()%4096)/4096.0f;
		if (rand() % 2) warp_w[i] *= -1;
		if (rand() % 2) warp_uscale[i] *= -1;
		if (rand() % 2) warp_vscale[i] *= -1;
		if (rand() % 2) warp_phase[i] *= -1;
	}
}

/*

void ComputeTextColor(unsigned char *texture)
{
	int r=0, g=0, b=0;
	// find avg. level of this image
	for (int i=0; i<256; i++)
	{
		int offset = (rand() & 0xFFFF) * 4;
		r += texture[offset+2];
		g += texture[offset+1];
		b += texture[offset];
	}
	r /= 256;
	g /= 256;
	b /= 256;

	if ((r+g+b)/3 > 170)  // biased so white text is more common
	{
		g_FgTextColor  = RGB(0, 0, 0);
		g_BkgTextColor = RGB(127, 127, 127);
	}
	else
	{
		g_FgTextColor  = RGB(255, 255, 255);
		g_BkgTextColor = RGB(0, 0, 0);
	}
}
*/

//void __cdecl MainProc( void *p )
void DrempelsRender()
{    
	int iStep = 1;

	// remember start time so we can run animation by real time values
//	srand((unsigned int)(mainproc_start_time + (rand()%256)));

//	RandomizeStartValues();

//  while (!g_QuitASAP)
  {
    // freeze animation, if requested
/*
    if ((g_bAnimFrozen || g_bPausedViaMinimize) && !g_bForceRepaintOnce)
    {
      DWORD dwFreezeTime = GetTickCount();
      while ((g_bAnimFrozen || g_bPausedViaMinimize) && 
        !g_QuitASAP && 
        !g_bForceRepaintOnce) 
      {
        Sleep(30);
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
      }
      mainproc_start_time += GetTickCount() - dwFreezeTime; 
    }
*/

    // skip animation if we're forcing a repaint or the animation is frozen
//    if (!g_bForceRepaintOnce && !g_bAnimFrozen)
    {
      // animate (image will move)

      // regular timekeeping stuff
      intframe++;

      // mouse movement
/*
            if (rendermode==0)
            {
              g_iMouseMoves += (g_iMouseMovesThisFrame > 0) ? 1 : -1;
              g_iMouseMovesThisFrame = 0;
              if (g_iMouseMoves > 2)
              {
                dumpmsg("WM_MOUSEMOVE - exiting");
                bUserQuit = TRUE;   // only used for plugin
                g_QuitASAP = true;
              }
              if (g_iMouseMoves < 0)
              {
                g_iMouseMoves = 0;
              }
            }
*/
      

      static float fOldTime; 
      static float fTime = 0;
      if (fTime == 0)
        fTime = (GetTickCount() - mainproc_start_time) * 0.001f / 0.92f;
      else
        fTime = fTime*0.92f + 0.08f*(GetTickCount() - mainproc_start_time) * 0.001f;

      float fDeltaT = fTime - fOldTime;

      fOldTime = fTime;

      // target-fps stuff
      float prev_time_rec = time_rec[time_rec_pos];
      time_rec[time_rec_pos++] = fTime;
      time_rec_pos %= FPS_FRAMES;

      tex_fade_debt_pos = (tex_fade_debt_pos+1) % FPS_FRAMES;
      if (g_bTexFading && !g_bTexLocked)
        tex_fade_debt[tex_fade_debt_pos] = avg_texfade_time*0.001f;
      else
        tex_fade_debt[tex_fade_debt_pos] = 0;

      float tex_fade_debt_sum = 0;
      for (int i=0; i<FPS_FRAMES; i++) tex_fade_debt_sum += tex_fade_debt[i];

      if (intframe > FPS_FRAMES*3) 
      {
        // damp it just a little bit
        fps        = (float)FPS_FRAMES / (fTime - prev_time_rec - tex_fade_debt_sum);	
        honest_fps = (float)FPS_FRAMES / (fTime - prev_time_rec);	

        // adjust sleepy time
        float spf_now = 1.0f/fps;
        float spf_des = 1.0f/(float)limit_fps;
        sleep_amount += 0.05f/fps*(spf_des - spf_now)*1000.0f;	// it will adjust by up to 0.1x the difference, every *second* of animation.
      }
      else if (intframe == FPS_FRAMES*2)
      {
        fps        = (float)FPS_FRAMES / (fTime - prev_time_rec - tex_fade_debt_sum);
        honest_fps = (float)FPS_FRAMES / (fTime - prev_time_rec);	

        // initially set sleepy time.,,
        float spf_now = 1.0f/fps;
        float spf_des = 1.0f/(float)limit_fps;
        sleep_amount = (spf_des - spf_now)*1000.0f;

        fps        = 0;	// so it still reports 'calibrating' instead of the initial high-speed fps.
        honest_fps = 0;
      }
      else 
      {
        fps        = 0;
        honest_fps = 0;
      }

/*
      if (limit_fps < 60)
      {
        // sleepy time!
        if (sleep_amount >= 1.0f)
        {
          if (intframe > FPS_FRAMES*3 && 
            g_bTexFading && 
            !g_bTexLocked)
          {
            if (sleep_amount > avg_texfade_time)
            {
              Sleep((int)(sleep_amount - avg_texfade_time));
            }
          }
          else
          {
            Sleep((int)(sleep_amount));
          }
        }
      }
*/

      
      if (g_bRandomizeTextures)
      {
        g_bRandomizeTextures = false;
        g_bTexLocked = false;
        g_bTexFading = false;
        g_fTexStartTime = fTime;

        //char s[64];
        //sprintf(s, "c:\\tile256\\img%02d.tga", rand() % 30);	
        char *s = TEX.GetRandomFilename();
        if (s==NULL)
        {
          // load up the built-in texture
          //TEX.LoadBuiltInTex256(0);
        }
        else if (!TEX.LoadTex256(s, 0, g_bResize, g_bAutoBlend, g_iBlendPercent))
        {
          char *s2 = strrchr(s, '\\');
          if (s2 == NULL) 
            s2 = s; 
          else
            s2++;
          sprintf(szTEXLOADFAIL, "BAD IMAGE: %s", s2);
          SHOW_TEXLOADFAIL = 30;
        }
/*
        else
        {
          char *s2 = strrchr(s, '\\');
          if (s2 == NULL) 
            s2 = s; 
          else
            s2++;
          //sprintf(szTEXNAME, " %s ", s2);    
          if (g_nSubdirIndex != -1)
            sprintf(szTEXNAME, " %d\\%s ", g_subdirIndices[g_nSubdirIndex], s2);
          else 
            sprintf(szTEXNAME, " %s ", s2);               

          //SHOW_TEXLOADFAIL = 30;            

          ComputeTextColor(TEX.tex[0]);
        }
*/

        /*
        for (i=0; i<4; i++)
        {
        sprintf(s, "c:\\z\\img%02d.tga", rand() % 10);
        TGA_to_buffer(s, mix[i], FXW, FXH);
        }
        */
      }


//      if (g_bTexLocked)
//      {
//        g_fTexStartTime += fDeltaT;
//      }

/*
      if (g_bAutoSwitchSubdirs && g_nSubdirs >= 1)
      {
        g_fTimeTilSubdirSwitch -= fDeltaT;
        if (g_fTimeTilSubdirSwitch < 0) {
          g_fTimeTilSubdirSwitch = (float)g_nTimeBetweenSubdirs;
          // TO DO: SWITCH TO NEXT (OR RANDOM) SUBDIR
          if (g_bAutoSwitchRandom && g_nSubdirs > 2)
            g_nSubdirIndex = (g_nSubdirIndex + 1 + (rand() % (g_nSubdirs-2)) ) % g_nSubdirs;
          else
            g_nSubdirIndex = (g_nSubdirIndex+1) % g_nSubdirs;
          sprintf(szTexPathCurrent, "%s\\%d", szTexPath, g_subdirIndices[g_nSubdirIndex]);
          if (!TEX.EnumTgaAndBmpFiles(szTexPathCurrent))
          {
            sprintf(szTEXLOADFAIL, " [ no images: %s ] ", szTexPathCurrent);
            SHOW_TEXLOADFAIL = 30;
            strcpy(szTexPathCurrent, szTexPath);
            TEX.EnumTgaAndBmpFiles(szTexPathCurrent);
          }
        }
      }
*/

      if (fTime - g_fTexStartTime > time_between_textures)
      {
        if (fTime - g_fTexStartTime < time_between_textures + texture_fade_time)
        {
          // fade
          if (!g_bTexFading)
          {
            // start fading
            g_bTexFading = true;
            TEX.SwapTex(0, 2);

            //char s[64];
            //sprintf(s, "c:\\tile256\\img%02d.tga", rand() % 30);	
            char *s = TEX.GetRandomFilename();
            if (!TEX.LoadTex256(s, 1, g_bResize, g_bAutoBlend, g_iBlendPercent))
            {
              char *s2 = strrchr(s, '\\');
              if (s2 == NULL) 
                s2 = s; 
              else
                s2++;
              //sprintf(szTEXNAME, " %s ", s2);    
              if (g_nSubdirIndex != -1)
                sprintf(szTEXNAME, " %d\\%s ", g_subdirIndices[g_nSubdirIndex], s2);
              else 
                sprintf(szTEXNAME, " %s ", s2);               

              //SHOW_TEXLOADFAIL = 30;
            }
//            else
//            {
//              ComputeTextColor(TEX.tex[1]);
//            }
          }

          // continue fading
          if (!g_bTexLocked)
          {
            static int sampleframes = 1;
            DWORD a = GetTickCount();
            TEX.BlendTex(2, 1, 0, (fTime - g_fTexStartTime - time_between_textures)/texture_fade_time, bMMX);
            DWORD b = GetTickCount();

            if (sampleframes == 1)
            {
              avg_texfade_time = b - a + 1;
            }
            else if (sampleframes < 50)
            {
              avg_texfade_time *= (float)(sampleframes-1) / (float)sampleframes;
              avg_texfade_time += (float)(b - a + 1) / (float)sampleframes;
            }
            else
            {
              avg_texfade_time = avg_texfade_time*0.98f + 0.02f*(b - a + 1);
            }

            sampleframes++;
          }
        }
        else
        {
          // done fading
          g_bTexFading = false;
          g_fTexStartTime = fTime;
          TEX.SwapTex(0, 1);
        }
      }


      static float fAnimTime = 0;
      float fmult = anim_speed;
      fmult *= 0.75f;
      fmult *= powf(8.0f, 1.0f - speed_scale*0.1f);
      if (rendermode == 3)
        fmult *= 0.8f;
      fAnimTime += fDeltaT * fmult;

      if (TEX.tex[0] != NULL)//TEX.texW[0] == 256 && TEX.texH[0] == 256)
      {
        float intframe2 = fAnimTime*22.5f;
        float scale = 0.45f + 0.1f*sinf(intframe2*0.01f);

        float rot = fAnimTime*rotational_speed*6.28f;

        float eye_x = 0.5f + 0.4f*sinf(fAnimTime*0.054f) + 0.4f*sinf(fAnimTime*0.0054f);
        float eye_y = 0.5f + 0.4f*cosf(fAnimTime*0.047f) + 0.4f*cosf(fAnimTime*0.0084f);

        float ut, vt;
        int i, j;

        memset(cell, 0, sizeof(td_cellcornerinfo)*(UVCELLSX)*(UVCELLSY));



       WORD wOldCW;
        BOOL bChangedFPCW = MungeFPCW( &wOldCW );

#define NUM_MODES 7

        // this executes if they pressed 'b':
        /*
        if (g_bToggleCoeffFreeze)
                {
                  g_bToggleCoeffFreeze = false;
                  if (g_bCoeffsFrozen)
                  {
                    // unfreeze
                    g_fCoeffFreezeDebt = fAnimTime - g_fCoeffFreezeTime;
                    SHOW_BEH_LOCKED = 0;
                    SHOW_BEH_UNLOCKED = 20;
                    SHOW_LOCKED = 0;
                    SHOW_UNLOCKED = 0;
                  }
                  else
                  {
                    // freeze
                    g_fCoeffFreezeTime = fAnimTime - g_fCoeffFreezeDebt;
                    SHOW_BEH_LOCKED = 20;
                    SHOW_BEH_UNLOCKED = 0;
                    SHOW_LOCKED = 0;
                    SHOW_UNLOCKED = 0;
                  }
                  g_bCoeffsFrozen = !g_bCoeffsFrozen;
                }*/
        

        float fCoeffTime;
        if (g_bCoeffsFrozen)
          fCoeffTime = g_fCoeffFreezeTime;
        else
          fCoeffTime = fAnimTime - g_fCoeffFreezeDebt;

        float t[NUM_MODES];
        t[0] = powf(0.50f + 0.50f*sinf(fCoeffTime*mode_switch_speed_multiplier * 0.1216f + fRandStart1), 1.0f);
        t[1] = powf(0.48f + 0.48f*sinf(fCoeffTime*mode_switch_speed_multiplier * 0.0625f + fRandStart2), 2.0f);
        t[2] = powf(0.45f + 0.45f*sinf(fCoeffTime*mode_switch_speed_multiplier * 0.0253f + fRandStart3), 12.0f);
        t[3] = powf(0.50f + 0.50f*sinf(fCoeffTime*mode_switch_speed_multiplier * 0.0916f + fRandStart4), 2.0f);
        t[4] = powf(0.50f + 0.50f*sinf(fCoeffTime*mode_switch_speed_multiplier * 0.0625f + fRandStart1), 2.0f);
        t[5] = powf(0.70f + 0.50f*sinf(fCoeffTime*mode_switch_speed_multiplier * 0.0466f + fRandStart2), 1.0f);
        t[6] = powf(0.50f + 0.50f*sinf(fCoeffTime*mode_switch_speed_multiplier * 0.0587f + fRandStart3), 2.0f);
        //t[(intframe/120) % NUM_MODES] += 20.0f;

        // normalize
        float sum = 0.0f;
        for (i=0; i<NUM_MODES; i++) sum += t[i]*t[i];
        float mag = 1.0f/sqrtf(sum);
        for (i=0; i<NUM_MODES; i++) t[i] *= mag;

        // keep top dog at 1.0, and scale all others down by raising to some exponent
        for (i=0; i<NUM_MODES; i++) t[i] = powf(t[i], mode_focus_exponent);

        // bias t[1] by bass (stomach)
        //t[1] += max(0, (bass - 1.1f)*2.5f);

        // bias t[2] by treble (crazy)
        //t[2] += max(0, (treb - 1.1f)*1.5f);

        // give bias to original drempels effect
        t[0] += 0.2f;

        // re-normalize
        sum = 0.0f;
        for (i=0; i<NUM_MODES; i++) sum += t[i]*t[i];
        mag = 1.0f/sqrtf(sum);
//        if (g_bGeissMode)
//          mag *= 0.03f;
        for (i=0; i<NUM_MODES; i++) t[i] *= mag;



        // orig: 1.0-4.5... now: 1.0 + 1.15*[0.0...3.0]
        float fscale1 = 1.0f + 1.15f*(powf(2.0f, 1.0f + 0.5f*sinf(fAnimTime*0.892f) + 0.5f*sinf(fAnimTime*0.624f)) - 1.0f);
        float fscale2 = 4.0f + 1.0f*sinf(fRandStart3 + fAnimTime*0.517f) + 1.0f*sinf(fRandStart4 + fAnimTime*0.976f);
        float fscale3 = 4.0f + 1.0f*sinf(fRandStart1 + fAnimTime*0.654f) + 1.0f*sinf(fRandStart1 + fAnimTime*1.044f);
        float fscale4 = 4.0f + 1.0f*sinf(fRandStart2 + fAnimTime*0.517f) + 1.0f*sinf(fRandStart3 + fAnimTime*0.976f);
        float fscale5 = 4.0f + 1.0f*sinf(fRandStart4 + fAnimTime*0.654f) + 1.0f*sinf(fRandStart2 + fAnimTime*1.044f);

        float t3_uc = 0.3f*sinf(0.217f*(fAnimTime+fRandStart1)) + 0.2f*sinf(0.185f*(fAnimTime+fRandStart2));
        float t3_vc = 0.3f*cosf(0.249f*(fAnimTime+fRandStart3)) + 0.2f*cosf(0.153f*(fAnimTime+fRandStart4));
        float t3_rot = 3.3f*cosf(0.1290f*(fAnimTime+fRandStart2)) + 2.2f*cosf(0.1039f*(fAnimTime+fRandStart3));
        float cosf_t3_rot = cosf(t3_rot);
        float sinf_t3_rot = sinf(t3_rot);
        float t4_uc = 0.2f*sinf(0.207f*(fAnimTime+fRandStart2)) + 0.2f*sinf(0.145f*(fAnimTime+fRandStart4));
        float t4_vc = 0.2f*cosf(0.219f*(fAnimTime+fRandStart1)) + 0.2f*cosf(0.163f*(fAnimTime+fRandStart3));
        float t4_rot = 0.61f*cosf(0.1230f*(fAnimTime+fRandStart4)) + 0.43f*cosf(0.1009f*(fAnimTime+fRandStart1));
        float cosf_t4_rot = cosf(t4_rot);
        float sinf_t4_rot = sinf(t4_rot);

        float u_delta = 0.05f;//1.0f/((UVCELLSX-4)/2) * 0.05f; //0.01f;
        float v_delta = 0.05f;//1.0f/((UVCELLSY-4)/2) * 0.05f; //0.01f;

        float u_offset = 0.5f;// + gXC/(float)FXW*4.0f;
        float v_offset = 0.5f;// + gYC/(float)FXH*4.0f;

        for (i=0; i<UVCELLSX; i++)
          for (j=0; j<UVCELLSY; j++)
          {
            float base_u = (i/2*2)/(float)(UVCELLSX-2) - u_offset;
            float base_v = (j/2*2)/(float)(UVCELLSY-2) - v_offset;
            if (i & 1) base_u += u_delta;
            if (j & 1) base_v += v_delta;
            base_v *= -1.0f;

//            if (g_bGeissMode)
//            {
//              cell[i][j].u = base_u;
//              cell[i][j].v = base_v;
//            }
//            else
            {
              cell[i][j].u = 0;//fTime*0.4f + base_u;//base_u; 
              cell[i][j].v = 0;//base_v; 
            }

            // correct for aspect ratio:
            base_u *= 1.333f;


            //------------------------------ v1.0 code
            {
              float u = base_u; 
              float v = base_v; 
              u += warp_factor*0.65f*sinf(intframe2*warp_w[0] + (base_u*warp_uscale[0] + base_v*warp_vscale[0])*6.28f + warp_phase[0]); 
              v += warp_factor*0.65f*sinf(intframe2*warp_w[1] + (base_u*warp_uscale[1] - base_v*warp_vscale[1])*6.28f + warp_phase[1]); 
              u += warp_factor*0.35f*sinf(intframe2*warp_w[2] + (base_u*warp_uscale[2] - base_v*warp_vscale[2])*6.28f + warp_phase[2]); 
              v += warp_factor*0.35f*sinf(intframe2*warp_w[3] + (base_u*warp_uscale[3] + base_v*warp_vscale[3])*6.28f + warp_phase[3]); 
              u /= scale;
              v /= scale;

              ut = u;
              vt = v;
              u = ut*cosf(rot) - vt*sinf(rot);
              v = ut*sinf(rot) + vt*cosf(rot);

              // NOTE: THIS MULTIPLIER WAS --2.7-- IN THE ORIGINAL DREMPELS 1.0!!!
              u += 2.0f*sinf(intframe2*0.00613f);
              v += 2.0f*cosf(intframe2*0.0138f);

              cell[i][j].u += u * t[0]; 
              cell[i][j].v += v * t[0]; 
            }
            //------------------------------ v1.0 code

            {
              // stomach
              float u = base_u; 
              float v = base_v; 

              float rad = sqrtf(u*u + v*v);
              float ang = atan2f(u, v);

              rad *= 1.0f + 0.3f*sinf(fAnimTime * 0.53f + ang*1.0f + fRandStart2);
              ang += 0.9f*sinf(fAnimTime * 0.45f + rad*4.2f + fRandStart3);

              u = rad*cosf(ang)*1.7f;
              v = rad*sinf(ang)*1.7f;

              cell[i][j].u += u * t[1]; 
              cell[i][j].v += v * t[1]; 
            }						


            {
              // crazy
              float u = base_u; 
              float v = base_v; 

              float rad = sqrtf(u*u + v*v);
              float ang = atan2f(u, v);

              rad *= 1.0f + 0.3f*sinf(fAnimTime * 1.59f + ang*20.4f + fRandStart3);
              ang += 1.8f*sinf(fAnimTime * 1.35f + rad*22.1f + fRandStart4);

              u = rad*cosf(ang);
              v = rad*sinf(ang);

              cell[i][j].u += u * t[2]; 
              cell[i][j].v += v * t[2]; 
            }

            {
              // rotation
              //float u = (i/(float)UVCELLSX)*1.6f - 0.5f - t3_uc;  
              //float v = (j/(float)UVCELLSY)*1.6f - 0.5f - t3_vc; 
              float u = base_u*1.6f - t3_uc; 
              float v = base_v*1.6f - t3_vc; 
              float u2 = u*cosf_t3_rot - v*sinf_t3_rot + t3_uc;
              float v2 = u*sinf_t3_rot + v*cosf_t3_rot + t3_vc;

              cell[i][j].u += u2 * t[3]; 
              cell[i][j].v += v2 * t[3]; 
            }

            {
              // zoom out & minor rotate (to keep it interesting, if isolated)
              //float u = i/(float)UVCELLSX - 0.5f - t4_uc; 
              //float v = j/(float)UVCELLSY - 0.5f - t4_vc; 
              float u = base_u - t4_uc; 
              float v = base_v - t4_vc; 

              u = u*fscale1 + t4_uc - t3_uc;
              v = v*fscale1 + t4_vc - t3_uc;

              float u2 = u*cosf_t4_rot - v*sinf_t4_rot + t3_uc;
              float v2 = u*sinf_t4_rot + v*cosf_t4_rot + t3_vc;

              cell[i][j].u += u2 * t[4]; 
              cell[i][j].v += v2 * t[4]; 
            }

            {
              // SWIRLIES!
              float u = base_u*1.4f;
              float v = base_v*1.4f;
              float offset = 0;//((u+2.0f)*(v-2.0f) + u*u + v*v)*50.0f;

              float u2 = u + 0.03f*sinf(u*(fscale2 + 2.0f) + v*(fscale3 + 2.0f) + fRandStart4 + fAnimTime*1.13f + 3.0f + offset);
              float v2 = v + 0.03f*cosf(u*(fscale4 + 2.0f) - v*(fscale5 + 2.0f) + fRandStart2 + fAnimTime*1.03f - 7.0f + offset);
              u2 += 0.024f*sinf(u*(fscale3*-0.1f) + v*(fscale5*0.9f) + fRandStart3 + fAnimTime*0.53f - 3.0f);
              v2 += 0.024f*cosf(u*(fscale2*0.9f) + v*(fscale4*-0.1f) + fRandStart1 + fAnimTime*0.58f + 2.0f);

              cell[i][j].u += u2*1.25f * t[5]; 
              cell[i][j].v += v2*1.25f * t[5]; 
            }						


            {
              // tunnel
              float u = base_u*1.4f - t4_vc;
              float v = base_v*1.4f - t4_uc;

              float rad = sqrtf(u*u + v*v);
              float ang = atan2f(u, v);

              u = rad + 3.0f*sinf(fAnimTime*0.133f + fRandStart1) + t4_vc;
              v = rad*0.5f * 0.1f*cosf(ang + fAnimTime*0.079f + fRandStart4) + t4_uc;

              cell[i][j].u += u * t[6]; 
              cell[i][j].v += v * t[6]; 
            }

          }

          float inv_master_zoom = 1.0f / (master_zoom * 1.8f);
          inv_master_zoom *= powf(4.0f, 1.0f - tex_scale*0.1f);
          float int_scalar = 256.0f*(INTFACTOR);
          for (j=0; j<UVCELLSY; j++)
            for (i=0; i<UVCELLSX; i++)
            {
              cell[i][j].u *= inv_master_zoom;
              cell[i][j].v *= inv_master_zoom;
              cell[i][j].u += 0.5f; 
              cell[i][j].v += 0.5f; 
              cell[i][j].u *= int_scalar;
              cell[i][j].v *= int_scalar;
            }

            for (j=0; j<UVCELLSY; j++)
              for (i=0; i<UVCELLSX-1; i+=2)
              {
                cell[i][j].r = (cell[i+1][j].u - cell[i][j].u) / (u_delta*FXW2);
                cell[i][j].s = (cell[i+1][j].v - cell[i][j].v) / (v_delta*FXW2);
              }

              for (j=0; j<UVCELLSY-1; j+=2)
                for (i=0; i<UVCELLSX; i+=2)
                {
                  cell[i][j].dudy = (cell[i][j+1].u - cell[i][j].u) / (u_delta*FXH2);
                  cell[i][j].dvdy = (cell[i][j+1].v - cell[i][j].v) / (v_delta*FXH2);
                  cell[i][j].drdy = (cell[i][j+1].r - cell[i][j].r) / (u_delta*FXH2);
                  cell[i][j].dsdy = (cell[i][j+1].s - cell[i][j].s) / (v_delta*FXH2);
                }

                g_fBlurAmount = 0.97f*powf(motion_blur*0.1f, 0.27f);
                float src_scale = (1.0f-g_fBlurAmount)*255.0f;
                float dst_scale = g_fBlurAmount*255.0f;

                if ( bChangedFPCW )
                  RestoreFPCW( wOldCW );

//                if (rendermode==0 && (g_bUserAltTabbedOut || !TEX.tex[0]))
//                {
//                  Sleep(15);
//                }
//                else
                {
//                  DDSURFACEDESC ddsd1;
//                  ZeroMemory(&ddsd1, sizeof(ddsd1));
//                  ddsd1.dwSize = sizeof(ddsd1);
//
//                  DDSURFACEDESC ddsd2;
//                  ZeroMemory(&ddsd2, sizeof(ddsd2));
//                  ddsd2.dwSize = sizeof(ddsd2);

                  IDirect3DSurface8* surface1, *surface2;
                  D3DLOCKED_RECT lock1, lock2;

                  lpD3DVS[intframe%2]->GetSurfaceLevel(0, &surface1);
                  surface1->LockRect(&lock1, NULL, 0);
                  lpD3DVS[(intframe+1)%2]->GetSurfaceLevel(0, &surface2);
                  surface2->LockRect(&lock2, NULL, 0);

//                  if (TryLockSurface(lpDDSVS[intframe%2], &ddsd1))
                  {
//                    if (TryLockSurface(lpDDSVS[(intframe+1)%2], &ddsd2))
                    {
 /*
                      if (g_bGeissMode)
                       {
                         for (i=0; i<FXW2; i++)
                         {
                           int h = FXH2*(0.5 + 0.3f*sinf((fAnimTime*0.1f + i/(float)FXW2)*2.0f*6.28f));
                           int offset = (i*4) + h*lock1.Pitch;
                           unsigned char c = 192 + (rand() % 64);
                           ((unsigned char *)lock1.pBits)[offset] = c;
                           ((unsigned char *)lock2.pBits)[offset] = c;
                           ((unsigned char *)lock1.pBits)[offset+1] = c;
                           ((unsigned char *)lock2.pBits)[offset+1] = c;
                           ((unsigned char *)lock1.pBits)[offset+2] = c;
                           ((unsigned char *)lock2.pBits)[offset+2] = c;
                           ((unsigned char *)lock1.pBits)[offset+3] = c;
                           ((unsigned char *)lock2.pBits)[offset+3] = c;
                         }
 
                         for (i=0; i<10; i++)
                           for (j=0; j<10; j++)
                           {
                             int offset = ((i+0.5f)*320 /10*4) + ((j+0.5f)*240/10)*lock1.Pitch;
                             unsigned char c = 192 + (rand() % 64);
                             ((unsigned char *)lock1.pBits)[offset] = c;
                             ((unsigned char *)lock2.pBits)[offset] = c;
                             ((unsigned char *)lock1.pBits)[offset+1] = c;
                             ((unsigned char *)lock2.pBits)[offset+1] = c;
                             ((unsigned char *)lock1.pBits)[offset+2] = c;
                             ((unsigned char *)lock2.pBits)[offset+2] = c;
                             ((unsigned char *)lock1.pBits)[offset+3] = c;
                             ((unsigned char *)lock2.pBits)[offset+3] = c;
                           }
                       }
 
  */
                      /*
                      if (g_bGeissMode)
                      {

                        for (j=0; j<UVCELLSY-2; j+=2)
                          for (i=0; i<UVCELLSX-2; i+=2) 
                            BlitWarpNon256AndMix(cell[i][j], cell[i+2][j], cell[i][j+2], cell[i+2][j+2],
                            (i)*FXW2/(UVCELLSX-2), 
                            (j)*FXH2/(UVCELLSY-2), 
                            (i+2)*FXW2/(UVCELLSX-2), 
                            (j+2)*FXH2/(UVCELLSY-2), 
                            (unsigned char *)lock1.pBits,
                            (int)dst_scale, 
                            (int)src_scale,
                            (unsigned char *)lock2.pBits, 
                            FXW2, 
                            FXH2,	
                            (unsigned char *)lock1.pBits,
                            FXW2,
                            FXH2,
                            high_quality);
                      }
                      else */
                      if (TEX.texW[0]==256 && TEX.texH[0]==256)
                      {
                        for (j=0; j<UVCELLSY-2; j+=2)
                          for (i=0; i<UVCELLSX-2; i+=2) 
                            BlitWarp256AndMix(cell[i][j], cell[i+2][j], cell[i][j+2], cell[i+2][j+2],
                            (i)*FXW2/(UVCELLSX-2), 
                            (j)*FXH2/(UVCELLSY-2), 
                            (i+2)*FXW2/(UVCELLSX-2), 
                            (j+2)*FXH2/(UVCELLSY-2), 
                            (unsigned char *)lock1.pBits,
                            (int)dst_scale, 
                            (int)src_scale,
                            (unsigned char *)lock2.pBits, 
                            FXW2, 
                            FXH2,	
                            TEX.tex[0], 
                            high_quality);
                      }
/*
                      else
                        for (j=0; j<UVCELLSY-2; j+=2)
                          for (i=0; i<UVCELLSX-2; i+=2) 
                            BlitWarpNon256AndMix(cell[i][j], cell[i+2][j], cell[i][j+2], cell[i+2][j+2],
                            (i)*FXW2/(UVCELLSX-2), 
                            (j)*FXH2/(UVCELLSY-2), 
                            (i+2)*FXW2/(UVCELLSX-2), 
                            (j+2)*FXH2/(UVCELLSY-2), 
                            (unsigned char *)ddsd1.lpSurface,
                            (int)dst_scale, 
                            (int)src_scale,
                            (unsigned char *)ddsd2.lpSurface, 
                            FXW2, 
                            FXH2,	
                            TEX.tex[0], 
                            TEX.texW[0],
                            TEX.texH[0],
                            high_quality);
*/
//                      lpDDSVS[(intframe+1)%2]->Unlock(NULL);
                    }

 //                   lpDDSVS[intframe%2]->Unlock(NULL);
                      surface2->UnlockRect();
                      surface2->Release();
                      surface1->UnlockRect();
                      surface1->Release(); 
                 }
               }
      }
    } // end if (!g_bForceRepaintOnce)

    g_bForceRepaintOnce = false;


//    if (g_bTakeScreenshot)
//    {
//      g_bTakeScreenshot = false;
//      FX_Screenshot(intframe);
//    }


//    if (rendermode != 0 || !g_bUserAltTabbedOut || !bFrontShowing)
    {
//      if (rendermode==0) // fullscreen
      {
        g_pd3dDevice->SetTexture(0, lpD3DVS[(intframe+1)%2]);

        float x = 0;
        float y = 0;
        float sizeX = g_width;
        float sizeY = g_height;
        int col = 0xffffffff;

	      TEXVERTEX	v[4];

	      v[0].x = x;
	      v[0].y = y;
	      v[0].z = 0;
	      v[0].w = 1.0f;
	      v[0].colour = col;
	      v[0].u = 0;
	      v[0].v = 0;

	      v[1].x = x + sizeX;
	      v[1].y = y;
	      v[1].z = 0;
	      v[1].w = 1.0f;
	      v[1].colour = col;
	      v[1].u = FXW2;
	      v[1].v = 0;

	      v[2].x = x + sizeX;
	      v[2].y = y + sizeY;
	      v[2].z = 0;
	      v[2].w = 1.0f;
	      v[2].colour = col;
	      v[2].u = FXW2;
	      v[2].v = FXH2;

	      v[3].x = x;
	      v[3].y = y + sizeY;
	      v[3].z = 0;
	      v[3].w = 1.0f;
	      v[3].colour = col;
	      v[3].u = 0;
	      v[3].v = FXH2;

        d3dSetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
        d3dSetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
	      d3dSetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	      d3dSetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	      g_pd3dDevice->SetVertexShader(D3DFVF_TEXVERTEX);
	      g_pd3dDevice->DrawPrimitiveUP(D3DPT_QUADLIST, 1, v, sizeof(TEXVERTEX));


       /*
         RECT srcrect;
                srcrect.left = 0;
                srcrect.top = 0;
                srcrect.right = FXW2;
                srcrect.bottom = FXH2;
                RECT dstrect;
                dstrect.left = 0;
                dstrect.top = 0;
                dstrect.right = FXW;
                dstrect.bottom = FXH;
        
                if (intframe < 3)
                {
                  char buf[64];
                  sprintf(buf, "begin manual blit, frame %d", intframe);
                  dumpmsg(buf);
                }
        
                Merge_All_VS_To_Backbuffer(lpDDSVS[(intframe+1)%2], lpDDSBack);
        
                if (intframe < 3)
                {
                  char buf[64];
                  sprintf(buf, "end manual blit, frame %d", intframe);
                  dumpmsg(buf);
                }*/
        
      }
      /*
      else if (rendermode==1 || rendermode==2) // windowed
            {
              RECT srcrect;
              srcrect.left = 0;
              srcrect.top = 0;
              srcrect.right = FXW2;
              srcrect.bottom = FXH2;
              RECT dstrect;
              dstrect.left = 0;
              dstrect.top = 0;
              dstrect.right = FXW;    // changed in 1.3 (was FXW2)
              dstrect.bottom = FXH;   // changed in 1.3 (was FXH2)
      
              if (intframe < 3)
              {
                char buf[64];
                sprintf(buf, "begin manual blit, frame %d", intframe);
                dumpmsg(buf);
              }
      
              Merge_All_VS_To_Backbuffer(lpDDSVS[(intframe+1)%2], lpDDSBack);
      
              if (intframe < 3)
              {
                char buf[64];
                sprintf(buf, "end manual blit, frame %d", intframe);
                dumpmsg(buf);
              }
            }
            else //if (rendermode==3)
            {
              // do nothing yet - have to wait until text
              // is drawn into lpDDSVS[] before we copy it
              // to lpDDSBack, since you can't write text
              // directly to an overlay surface.
            }*/
      


      /*
// figure out which surface we should write text to
      // (have to get a DC for it - won't work with overlays!)
      if (rendermode==3)
      {
        lpDDSMsg = lpDDSVS[(intframe+1)%2];
      }
      else
      {
        lpDDSMsg = lpDDSBack;
      }

      int ypos = 0;	// NOTE: y_inc is #defined in video.h!!!


      // user-solicited text messages:
      if (SHOW_FPS && !g_QuitASAP)
      {
        Put_FPS_To_Backbuffer(&g_FgTextColor, &g_BkgTextColor, ypos, true);
        ypos += y_inc;
      }

      if (SHOW_TEXNAME && !g_QuitASAP)
      {
        Put_Msg_To_Backbuffer(szTEXNAME, &g_FgTextColor, &g_BkgTextColor, ypos, true);
        ypos += y_inc;
      }

      if (SHOW_HELP_MSG && !g_QuitASAP)
      {
        Put_Helpmsg_To_Backbuffer(&g_FgTextColor, &g_BkgTextColor);
      }

      if (!g_bSuppressAllMsg)
      {
        // automated text messages:
        if (SHOW_TEXLOADFAIL && !g_QuitASAP)
        {
          SHOW_TEXLOADFAIL--;
          Put_Msg_To_Backbuffer(szTEXLOADFAIL, &g_FgTextColor, &g_BkgTextColor, ypos, true);
          ypos += y_inc;
        }


        // these 4 are mutually exclusive
        if (SHOW_LOCKED && !g_QuitASAP)
        {
          SHOW_LOCKED--;
          Put_Msg_To_Backbuffer(szLocked, &g_FgTextColor, &g_BkgTextColor, ypos, true);
          ypos += y_inc;
        }
        else if (SHOW_UNLOCKED && !g_QuitASAP)
        {
          SHOW_UNLOCKED--;
          Put_Msg_To_Backbuffer(szUnlocked, &g_FgTextColor, &g_BkgTextColor, ypos, true);
          ypos += y_inc;
        }
        else if (SHOW_BEH_LOCKED && !g_QuitASAP)
        {
          SHOW_BEH_LOCKED--;
          Put_Msg_To_Backbuffer(szBehLocked, &g_FgTextColor, &g_BkgTextColor, ypos, true);
          ypos += y_inc;
        }
        else if (SHOW_BEH_UNLOCKED && !g_QuitASAP)
        {
          SHOW_BEH_UNLOCKED--;
          Put_Msg_To_Backbuffer(szBehUnlocked, &g_FgTextColor, &g_BkgTextColor, ypos, true);
          ypos += y_inc;
        }

        if (SHOW_MOUSECLICK_MSG && !g_bSuppressHelpMsg && !g_QuitASAP)
        {
          SHOW_MOUSECLICK_MSG--;
          if (rendermode != 3)
          {
            Put_Msg_To_Backbuffer(szMCM, &g_FgTextColor, &g_BkgTextColor, ypos, true);
            ypos += y_inc;
          }
        }

        if (SHOW_DEBUG > 0 && !g_QuitASAP)
        {
          Put_Msg_To_Backbuffer(szDEBUG, &g_FgTextColor, &g_BkgTextColor, ypos, true);
          ypos += y_inc;
        }
      }
*/

      //---------------------------------------------------------
      // PAGE FLIP (or blit, if windowed)
      //---------------------------------------------------------
/*
      if (rendermode==0)
      {

        HRESULT ddrval = lpDDSPrimary->Flip( NULL, DDFLIP_WAIT );

        //flip_clock_time += clock() - temp_clock;
        if (ddrval == DD_OK)
        {
          bFrontShowing = !bFrontShowing;
        }
        else if (ddrval == DDERR_SURFACELOST)
        {
          lpDDSPrimary->Restore();
        }
        else
        {
          dumpmsg("lpDDSPrimary->Flip() failed.");
          switch(ddrval)
          {
          case DDERR_GENERIC:         dumpmsg("  [DDERR_GENERIC]");         break;
          case DDERR_INVALIDOBJECT:   dumpmsg("  [DDERR_INVALIDOBJECT]");   break;
          case DDERR_INVALIDPARAMS:   dumpmsg("  [DDERR_INVALIDPARAMS]");   break;
          case DDERR_NOFLIPHW:        dumpmsg("  [DDERR_NOFLIPHW]");        break;
          case DDERR_NOTFLIPPABLE:    dumpmsg("  [DDERR_NOTFLIPPABLE]");    break;
          case DDERR_SURFACEBUSY:     dumpmsg("  [DDERR_SURFACEBUSY]");     break;
          case DDERR_SURFACELOST:     dumpmsg("  [DDERR_SURFACELOST]");     break;
          case DDERR_UNSUPPORTED:     dumpmsg("  [DDERR_UNSUPPORTED]");     break;
          case DDERR_WASSTILLDRAWING: dumpmsg("  [DDERR_WASSTILLDRAWING]"); break;
          case DD_OK:                 dumpmsg("  [OK]");                    break;
          default:                    dumpmsg("  [UNKNOWN]");               break;
          }
        }
      }
      */
      /*
        else if (rendermode==1 || rendermode==2)
              {
                RECT windrect;
                ::GetWindowRect(hSaverMainWindow, &windrect);
        
                RECT srcrect;
                srcrect.left = 0;
                srcrect.top = 0;
                srcrect.right = FXW;        // changed in 1.3 (was FXW2)
                srcrect.bottom = FXH;       // changed in 1.3 (was FXH2)
                RECT dstrect;
                dstrect.left = -g_clientRect.left + windrect.left;
                dstrect.top = -g_clientRect.top + windrect.top;
                dstrect.right = dstrect.left + FXW;
                dstrect.bottom = dstrect.top + FXH;
        
                HRESULT hr;
        
                if (intframe < 3)
                {
                  char buf[64];
                  sprintf(buf, "begin primary surface Blt, frame %d", intframe);
                  dumpmsg(buf);
                }
        
                hr = lpDDSPrimary->Blt(&dstrect, lpDDSBack, &srcrect, DDBLT_WAIT, NULL);
        
                if (intframe < 3)
                {
                  char buf[64];
                  sprintf(buf, "end primary surface Blt, frame %d", intframe);
                  dumpmsg(buf);
                }
        
                if (hr == DDERR_SURFACELOST)
                {
                  lpDDSPrimary->Restore();
                }
                else if (hr != DD_OK)
                {
                  char buf[64];
                  sprintf(buf, "pagecopy/Blt: error %d (%08x)", hr, hr);
                  dumpmsg(buf);
        
                  //Merge_All_VS_To_Backbuffer(lpDDSVS[(intframe+1)%2], lpDDSBack);
        
                  if (hr==DDERR_GENERIC) 
                    dumpmsg("DDERR_GENERIC  ");
                  else if (hr==DDERR_INVALIDCLIPLIST) 
                    dumpmsg("DDERR_INVALIDCLIPLIST ");
                  else if (hr==DDERR_INVALIDOBJECT) 
                    dumpmsg("DDERR_INVALIDOBJECT  ");
                  else if (hr==DDERR_INVALIDPARAMS) 
                    dumpmsg("DDERR_INVALIDPARAMS  ");
                  else if (hr==DDERR_INVALIDRECT) 
                    dumpmsg("DDERR_INVALIDRECT  ");
                  else if (hr==DDERR_NOALPHAHW) 
                    dumpmsg("DDERR_NOALPHAHW  ");
                  else if (hr==DDERR_NOBLTHW) 
                    dumpmsg("DDERR_NOBLTHW  ");
                  else if (hr==DDERR_NOCLIPLIST) 
                    dumpmsg("DDERR_NOCLIPLIST  ");
                  else if (hr==DDERR_NODDROPSHW) 
                    dumpmsg("DDERR_NODDROPSHW  ");
                  else if (hr==DDERR_NOMIRRORHW) 
                    dumpmsg("DDERR_NOMIRRORHW  ");
                  else if (hr==DDERR_NORASTEROPHW) 
                    dumpmsg("DDERR_NORASTEROPHW  ");
                  else if (hr==DDERR_NOROTATIONHW) 
                    dumpmsg("DDERR_NOROTATIONHW  ");
                  else if (hr==DDERR_NOSTRETCHHW) 
                    dumpmsg("DDERR_NOSTRETCHHW  ");
                  else if (hr==DDERR_NOZBUFFERHW) 
                    dumpmsg("DDERR_NOZBUFFERHW  ");
                  else if (hr==DDERR_SURFACEBUSY) 
                    dumpmsg("DDERR_SURFACEBUSY  ");
                  else if (hr==DDERR_SURFACELOST) 
                    dumpmsg("DDERR_SURFACELOST  ");
                  else if (hr==DDERR_UNSUPPORTED) 
                    dumpmsg("DDERR_UNSUPPORTED  ");
                  else if (hr==DDERR_WASSTILLDRAWING) 
                    dumpmsg("DDERR_WASSTILLDRAWIN ");
                }
              }
              else if (rendermode == 3)// desktop mode
              {
                // copy lpDDSVS (w/text on it) to lpDDSBack,
                // then do page flip on overlay
        
                RECT srcrect;
                srcrect.left = 0;
                srcrect.top = 0;
                srcrect.right = FXW2;
                srcrect.bottom = FXH2;
                RECT dstrect;
                dstrect.left = 0;
                dstrect.top = 0;
                dstrect.right = FXW;
                dstrect.bottom = FXH;
        
                HRESULT hr;
        
                if (g_eOverlayFormat == UYVY || g_eOverlayFormat == YUY2) // using YUV & manual blit
                {
                  DDSURFACEDESC ddsd_src, ddsd_dest;
                  ZeroMemory(&ddsd_src, sizeof(DDSURFACEDESC));
                  ZeroMemory(&ddsd_dest, sizeof(DDSURFACEDESC));
                  ddsd_src.dwSize = sizeof(DDSURFACEDESC);
                  ddsd_dest.dwSize = sizeof(DDSURFACEDESC);
                  hr = lpDDSVS[(intframe+1)%2]->Lock(NULL, &ddsd_src, DDLOCK_WAIT, NULL);
                  if (hr == DD_OK)
                  {
                    hr = lpDDSBack->Lock(NULL, &ddsd_dest, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_NOSYSLOCK, NULL);
                    if (hr == DD_OK)
                    { 
                      CopyRGBSurfaceToYUVSurfaceMMX(&ddsd_src, &ddsd_dest, g_eOverlayFormat);
                      lpDDSBack->Unlock(NULL);
                    }
                    else if (hr == DDERR_SURFACELOST)
                    {
                      // this catches when another app goes fullscreen
                      // and you return.  However, it does NOT catch
                      // it when the monitor turns off to save power!
                      dumpmsg("overlay surface lost!");
        
                      if (lpDDSPrimary->IsLost())
                      {
                        hr = lpDDSPrimary->Restore();
                        if (hr != DD_OK) 
                          dumpmsg("fatal: couldn't restore primary after overlay lost");
                      }
        
                      lpDDSOverlay->Restore();
                      if (hr != DD_OK) 
                        dumpmsg("fatal: couldn't restore overlay after lost");
                      else
                        Update_Overlay();
                    }
        
                    lpDDSVS[(intframe+1)%2]->Unlock(NULL);
                  }
                  else if (hr == DDERR_SURFACELOST)
                  {
                    lpDDSVS[(intframe+1)%2]->Restore();
                  }
                }
                else // using RGB overlay or a funky fourcc mode; use Blt function and cross fingers
                {
                  DDBLTFX fx = { sizeof(DDBLTFX) };
        
                  // no stretch, since VS is small and back buffer of overlay is too
                  // no colorkeying involved here since we're just flat blitting to the overlay surface!
                  hr = lpDDSBack->Blt(NULL, lpDDSVS[(intframe+1)%2], NULL, DDBLT_WAIT, &fx);
                  //hr = lpDDSBack->Blt(NULL, lpDDSVS[(intframe+1)%2], NULL, DDBLT_WAIT, NULL);
                  //hr = lpDDSBack->BltFast(0, 0, lpDDSVS[(intframe+1)%2], &srcrect, DDBLTFAST_WAIT);
        
                  //if (procsize==0)	// no stretch
                  //hr = lpDDSBack->BltFast(0, 0, lpDDSVS[(intframe+1)%2], &srcrect, DDBLTFAST_WAIT);
                  //else
                  //hr = lpDDSBack->Blt(&dstrect, lpDDSVS[(intframe+1)%2], &srcrect, DDBLT_WAIT, NULL);
        
                  if (hr == DDERR_SURFACELOST)
                  {
                    lpDDSBack->Restore();
                  }
                  else if (hr != DD_OK)
                  {
                    char buf[64];
                    sprintf(buf, "Blt: error %d (%08x)", hr, hr);
                    dumpmsg(buf);
        
                    if (hr==DDERR_EXCEPTION) 
                      dumpmsg("DDERR_EXCEPTION");
                    else if (hr==DDERR_GENERIC) 
                      dumpmsg("DDERR_GENERIC  ");
                    else if (hr==DDERR_INVALIDCLIPLIST) 
                      dumpmsg("DDERR_INVALIDCLIPLIST ");
                    else if (hr==DDERR_INVALIDOBJECT) 
                      dumpmsg("DDERR_INVALIDOBJECT  ");
                    else if (hr==DDERR_INVALIDPARAMS) 
                      dumpmsg("DDERR_INVALIDPARAMS  ");
                    else if (hr==DDERR_INVALIDRECT) 
                      dumpmsg("DDERR_INVALIDRECT  ");
                    else if (hr==DDERR_NOALPHAHW) 
                      dumpmsg("DDERR_NOALPHAHW  ");
                    else if (hr==DDERR_NOBLTHW) 
                      dumpmsg("DDERR_NOBLTHW  ");
                    else if (hr==DDERR_NOCLIPLIST) 
                      dumpmsg("DDERR_NOCLIPLIST  ");
                    else if (hr==DDERR_NODDROPSHW) 
                      dumpmsg("DDERR_NODDROPSHW  ");
                    else if (hr==DDERR_NOMIRRORHW) 
                      dumpmsg("DDERR_NOMIRRORHW  ");
                    else if (hr==DDERR_NORASTEROPHW) 
                      dumpmsg("DDERR_NORASTEROPHW  ");
                    else if (hr==DDERR_NOROTATIONHW) 
                      dumpmsg("DDERR_NOROTATIONHW  ");
                    else if (hr==DDERR_NOSTRETCHHW) 
                      dumpmsg("DDERR_NOSTRETCHHW  ");
                    else if (hr==DDERR_NOZBUFFERHW) 
                      dumpmsg("DDERR_NOZBUFFERHW  ");
                    else if (hr==DDERR_SURFACEBUSY) 
                      dumpmsg("DDERR_SURFACEBUSY  ");
                    else if (hr==DDERR_SURFACELOST) 
                      dumpmsg("DDERR_SURFACELOST  ");
                    else if (hr==DDERR_UNSUPPORTED) 
                      dumpmsg("DDERR_UNSUPPORTED  ");
                    else if (hr==DDERR_WASSTILLDRAWING) 
                      dumpmsg("DDERR_WASSTILLDRAWIN ");
                  }
                }*/
        

/*
        // desktop-preservation mode: do nothing 
        // (overlay is automatic!)
        if (lpDDSOverlay)
        {
          lpDDSOverlay->Flip(NULL, DDFLIP_WAIT);
        }

        if (!bWallpaperSet)
        {
          SetWallpaper();
        }
      }
      */
    }

/*
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
*/
  }

//  dumpmsg("Normal program termination (end of crunchy thread).");
//  g_MainProcFinished = true;
  //PostQuitMessage(0);
  //DestroyWindow(hMainWnd);

//  return;   // THE THREAD ENDS HERE.

}


/*
BOOL DoSaver(HWND hparwnd, HINSTANCE hInstance)
{ 
	if( !doInit( hInstance, SW_MAXIMIZE ) )
	{
		return false;
	}
	
	// ---------------- FOR THE SCREENSAVER ONLY -----------------------
	
	dumpmsg("starting MainProc thread...");
	
	MainProc(0);
	
	dumpmsg("MainProc ended.  Calling finiObjects.");
	finiObjects();
	dumpmsg("FiniObjects complete - ending program.");
	
	return true;
}
*/

/*

BOOL CALLBACK HotkeyDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			char buf[32768];

			SendMessage( GetDlgItem( hwnd, IDC_HOTKEYS_EDIT ), EM_SETLIMITTEXT, 32768, 0 );

  			wsprintf(buf,
					"You can control Drempels via the keyboard when the \"Dr\" icon\r\n"
					"in the system tray is highlighted in GREEN.  When it is BLACK\r\n"
					"(not highlighted), however, the keyboard will have no effect.\r\n"
					"\r\n"
					"If the icon is highlighted in green and you click on another window,\r\n"
					"it will go dark, and Drempels will relinquish keyboard control.\r\n"
					"Just click on the dark \"Dr\" icon again to highlight it and\r\n"
					"return keyboard control to Drempels.\r\n"
					"\r\n"
					"Hotkeys you can use while Drempels is running AND THE ICON IS GREEN:\r\n"
					"\r\n"
					"\tSPACE:\t load a random texture \r\n"
					"\tR:  \t randomize texture & behavior \r\n"
					"\tT:  \t lock/unlock current texture \r\n"
					"\tB:  \t lock/unlock current behavior \r\n"
					"\tQ:  \t toggle hi/lo texture quality \r\n"
					"\t+/-:\t adjust motion blur \r\n"
					"\tJ/K:\t adjust zoom \r\n"
					"\tU/I:\t adjust animation speed \r\n"
					"\tP:  \t [un]pause animation \r\n"
					"\tS:  \t [un]suspend Drempels (desktop mode only) \r\n"
					"\tESC:\t quit \r\n"
					"\tH:  \t display help \r\n"
					"\tF:  \t display fps (frames per second) \r\n"
					"\tF5: \t refresh texture list from disk \r\n"
					"\tF6: \t display current texture filename \r\n"
					"\t@:  \t save a screenshot to C:\\  (TGA format) \r\n"
					);
						
			SendMessage( GetDlgItem( hwnd, IDC_HOTKEYS_EDIT ), WM_SETTEXT, 0, (LPARAM)buf );
	
			return true;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			g_hWndHotkeys = NULL;
			EndDialog(hwnd,LOWORD(wParam));
			break;
		}
	}

	return FALSE;
}

*/

/*
BOOL CALLBACK ConfigDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	char buf[2048];
	
	switch (msg)
	{ 
	case WM_INITDIALOG:
		{ 
			

			HRESULT hr;
			
			g_lpGuid = NULL;

			//-------------- DirectDraw video mode detection ---------------------
			
			LPDIRECTDRAW   lpDD = NULL; 		  // DirectDraw object
			//LPDIRECTDRAW2  lpDD2 = NULL;			// DirectDraw object
			HRESULT 	   ddrval;
			dumpmsg("calling DirectDrawCreate() for config panel...");
			ddrval = DirectDrawCreate( NULL, &lpDD, NULL );
			if (ddrval != DD_OK)
			{
				wsprintf(buf, "Direct Draw Init Failed for config (DirectDrawCreate) - you probably don't have DirectX installed! (%08lx)\n", ddrval );
				MessageBox( hwnd, buf, "ERROR", MB_OK );
				dumpmsg(buf); 
				//finiObjects(); //3.40
				
				int id=LOWORD(wParam);
				EndDialog(hwnd,id);
				//exit(66);
			}
			
			dumpmsg("calling SetCooperativeLevel() for config panel...");
			ddrval = lpDD->SetCooperativeLevel( hwnd, DDSCL_NORMAL );//EXCLUSIVE | DDSCL_FULLSCREEN );
			if (ddrval != DD_OK)
			{
				wsprintf(buf, "Direct Draw Init Failed for config (SetCooperativeLevel) - you probably don't have DirectX installed! (%08lx)\n", ddrval );
				MessageBox( hwnd, buf, "ERROR", MB_OK );
				dumpmsg(buf); 
				//finiObjects(); //3.40
				
				int id=LOWORD(wParam);
				EndDialog(hwnd,id);
				//exit(66);
			}
			
			dumpmsg("calling EnumDisplayModes() for config panel...");
			g_lpGuid = NULL;
			hr = lpDD->EnumDisplayModes(0, NULL, NULL, &EnumModesCallback);
			if (hr == DD_OK && iNumVidModes>0) 
			{
				for (int i=0; i<iNumVidModes; i++)
				{
					SendMessage( GetDlgItem( hwnd, IDC_BITDEPTHCOMBO ), CB_ADDSTRING, 0, (LPARAM)VidList[i].name);
					SendMessage( GetDlgItem( hwnd, IDC_BITDEPTHCOMBO ), CB_SETITEMDATA, i, i);
				}
				SendMessage( GetDlgItem( hwnd, IDC_BITDEPTHCOMBO ), CB_SETCURSEL, VidMode, 0);
			}
			else
			{
				SendMessage( GetDlgItem( hwnd, IDC_BITDEPTHCOMBO ), CB_ADDSTRING, 0, (LPARAM)"(enum failed!)");
				SendMessage( GetDlgItem( hwnd, IDC_BITDEPTHCOMBO ), CB_SETITEMDATA, 0, -1);
				SendMessage( GetDlgItem( hwnd, IDC_BITDEPTHCOMBO ), CB_SETCURSEL, 0, 0);
			}
			
			if( lpDD != NULL )
			{
				lpDD->Release();
				lpDD = NULL;
			}
			
			for (int i=0; i<=50; i++)
			{
				char s[16];
				sprintf(s, "%d", i + 10);
				SendMessage( GetDlgItem( hwnd, IDC_FPS_COMBO ), CB_ADDSTRING, 0, (LPARAM)s);
				SendMessage( GetDlgItem( hwnd, IDC_FPS_COMBO ), CB_SETITEMDATA, i, i);
			}
			SendMessage( GetDlgItem( hwnd, IDC_FPS_COMBO ), CB_SETCURSEL, limit_fps - 10, 0);
			
			//------render mode combobox for SAVER-------

			SendMessage( GetDlgItem( hwnd, IDC_RENDERMODE_COMBO ), CB_ADDSTRING, 0, (LPARAM)" fullscreen, low detail");
			SendMessage( GetDlgItem( hwnd, IDC_RENDERMODE_COMBO ), CB_ADDSTRING, 0, (LPARAM)" fullscreen, med. detail");
			SendMessage( GetDlgItem( hwnd, IDC_RENDERMODE_COMBO ), CB_ADDSTRING, 0, (LPARAM)" fullscreen, high detail *");
			//SendMessage( GetDlgItem( hwnd, IDC_RENDERMODE_COMBO ), CB_ADDSTRING, 0, (LPARAM)" windowed, low detail");
			//SendMessage( GetDlgItem( hwnd, IDC_RENDERMODE_COMBO ), CB_ADDSTRING, 0, (LPARAM)" windowed, med. detail *");
			//SendMessage( GetDlgItem( hwnd, IDC_RENDERMODE_COMBO ), CB_ADDSTRING, 0, (LPARAM)" windowed, high detail ");
            for (i=0; i<3; i++)
			    SendMessage( GetDlgItem( hwnd, IDC_RENDERMODE_COMBO ), CB_SETITEMDATA, i, i);

            int cursel;
			/ *
            if (rendermode_as_saver == 0)
                cursel = 0;
            else 
                cursel = 3;
            cursel += 2 - procsize_as_saver;
			* /
			cursel = 2 - procsize_as_saver;
            SendMessage( GetDlgItem( hwnd, IDC_RENDERMODE_COMBO ), CB_SETCURSEL, cursel, 0);
	
			//------render mode combobox for APP-------

			SendMessage( GetDlgItem( hwnd, IDC_DESKTOP_RENDERMODE_COMBO ), CB_ADDSTRING, 0, (LPARAM)" desktop wallpaper mode, low detail *");
			SendMessage( GetDlgItem( hwnd, IDC_DESKTOP_RENDERMODE_COMBO ), CB_ADDSTRING, 0, (LPARAM)" desktop wallpaper mode, med. detail - slow");
			SendMessage( GetDlgItem( hwnd, IDC_DESKTOP_RENDERMODE_COMBO ), CB_ADDSTRING, 0, (LPARAM)" desktop wallpaper mode, high detail - SLOW");
            for (i=0; i<3; i++)
			    SendMessage( GetDlgItem( hwnd, IDC_DESKTOP_RENDERMODE_COMBO ), CB_SETITEMDATA, i, i);

			cursel = 2 - procsize_as_app;
            SendMessage( GetDlgItem( hwnd, IDC_DESKTOP_RENDERMODE_COMBO ), CB_SETCURSEL, cursel, 0);
			
			//---------------------------------------------------------------
			
			
			dumpmsg("Setting initial states of controls for config panel...");
			
			SendMessage( GetDlgItem( hwnd, IDC_TIMEBETWEENTEXTURES), TBM_SETRANGEMIN,
				FALSE, (LPARAM)(0.1f*1000) );
			SendMessage( GetDlgItem( hwnd, IDC_TIMEBETWEENTEXTURES), TBM_SETRANGEMAX,
				FALSE, (LPARAM)(60*1000) );
			SendMessage( GetDlgItem( hwnd, IDC_TIMEBETWEENTEXTURES), TBM_SETPOS,
				TRUE, (LPARAM)(time_between_textures*1000) );
			
			SendMessage( GetDlgItem( hwnd, IDC_TEXTUREFADETIME), TBM_SETRANGEMIN,
				FALSE, (LPARAM)(0.1f*1000) );
			SendMessage( GetDlgItem( hwnd, IDC_TEXTUREFADETIME), TBM_SETRANGEMAX,
				FALSE, (LPARAM)(60*1000) );
			SendMessage( GetDlgItem( hwnd, IDC_TEXTUREFADETIME), TBM_SETPOS,
				TRUE, (LPARAM)(texture_fade_time*1000) );
			
			// go from range [0.25...4.0] to t=[0..1]
			float t_zoom = logf(master_zoom)/logf(2.0f)*0.25f + 0.5f;
			
			// go from range [0.25...4.0] to t=[0..1]
			float t_switch = logf(mode_switch_speed_multiplier)/logf(2.0f)*0.25f + 0.5f;
			
			// go from range [0.125...8.0] to t=[0..1]
			float t_anim = logf(anim_speed)/logf(2.0f)*0.3333f*0.5f + 0.5f;
			
			SendMessage( GetDlgItem( hwnd, IDC_MASTERZOOM), TBM_SETRANGEMIN,
				FALSE, (LPARAM)(0) );
			SendMessage( GetDlgItem( hwnd, IDC_MASTERZOOM), TBM_SETRANGEMAX,
				FALSE, (LPARAM)(1000) );
			SendMessage( GetDlgItem( hwnd, IDC_MASTERZOOM), TBM_SETPOS,
				TRUE, (LPARAM)(   t_zoom*1000	));
			
			SendMessage( GetDlgItem( hwnd, IDC_MODESWITCHSPEEDMULTIPLIER), TBM_SETRANGEMIN,
				FALSE, (LPARAM)(0) );
			SendMessage( GetDlgItem( hwnd, IDC_MODESWITCHSPEEDMULTIPLIER), TBM_SETRANGEMAX,
				FALSE, (LPARAM)(1000) );
			SendMessage( GetDlgItem( hwnd, IDC_MODESWITCHSPEEDMULTIPLIER), TBM_SETPOS,
				TRUE, (LPARAM)(   t_switch*1000   ));
			
			SendMessage( GetDlgItem( hwnd, IDC_ANIMATIONSPEED), TBM_SETRANGEMIN,
				FALSE, (LPARAM)(0) );
			SendMessage( GetDlgItem( hwnd, IDC_ANIMATIONSPEED), TBM_SETRANGEMAX,
				FALSE, (LPARAM)(1000) );
			SendMessage( GetDlgItem( hwnd, IDC_ANIMATIONSPEED), TBM_SETPOS,
				TRUE, (LPARAM)(   t_anim*1000	));
			
			SendMessage( GetDlgItem( hwnd, IDC_MOTIONBLUR), TBM_SETRANGEMIN,
				FALSE, (LPARAM)(0) );
			SendMessage( GetDlgItem( hwnd, IDC_MOTIONBLUR), TBM_SETRANGEMAX,
				FALSE, (LPARAM)(10) );
			SendMessage( GetDlgItem( hwnd, IDC_MOTIONBLUR), TBM_SETPOS,
				TRUE, (LPARAM)(   motion_blur	));
						
			CheckDlgButton(hwnd, IDC_SUPPRESSALLMSG, g_bSuppressAllMsg);
			CheckDlgButton(hwnd, IDC_DEBUGINFO, g_bDebugMode);
			CheckDlgButton(hwnd, IDC_HELPMSG, g_bSuppressHelpMsg);
			CheckDlgButton(hwnd, IDC_EXITONMOUSEMOVE, g_bExitOnMouseMove);
			CheckDlgButton(hwnd, IDC_HIGHQUALITY, high_quality);
			CheckDlgButton(hwnd, IDC_CB_STARTUP,  g_bRunDrempelsAtStartup);
			//CheckDlgButton(hwnd, IDC_DESKTOPMODE, desktop_mode);
			//CheckDlgButton(hwnd, IDC_FULLSCREEN, fullscreen);
			
			SendMessage( GetDlgItem( hwnd, IDC_IMAGEDIR ), WM_SETTEXT, 0, (LPARAM)szTexPath );

            char buf[64];
            sprintf(buf, "%d", g_nTimeBetweenSubdirs);
			SendMessage( GetDlgItem( hwnd, IDC_SUBDIR_TIME ), WM_SETTEXT, 0, (LPARAM)buf );
            
			/ *
			SetWindowText( GetDlgItem( hwnd, IDC_TEXT1 ), "SOUND ENABLED will make this application react to music.  Play an audio CD, pipe music into your sound card, or speak into your microphone to see the effect." );
			SetWindowText( GetDlgItem( hwnd, IDC_TEXT2 ), "NOTE: This application was designed to respond best to sound in a certain volume range.  For best response, use Windows' built-in volume controls to set the input volumes (line-in, cd-audio, and microphone) to about 50%." );
			SetWindowText( GetDlgItem( hwnd, IDC_MAGIC ), "Disable CD-ROM drive controls" );
			SetWindowText( GetDlgItem( hwnd, IDC_MAGIC2), "Auto-start CD" );
			* /
			//SetWindowText( GetDlgItem( hwnd, IDC_SLIDERTEXT), " " );
			//SetWindowText( GetDlgItem( hwnd, IDC_SLIDERTEXT2), " " );
			//SetWindowText( GetDlgItem( hwnd, IDC_SLIDERTEXT3), " " );
			//::EnableWindow(GetDlgItem( hwnd, IDC_MAGICSLIDER), false );
			//::ShowWindow(GetDlgItem( hwnd, IDC_MAGICSLIDER), SW_HIDE );
			
			// initialize res-selection combo box
			SendMessage( GetDlgItem( hwnd, IDC_BITDEPTHCOMBO ), CB_INITSTORAGE,
				127, 127 );  // save space for 127 strings, each 127 bytes long. (respectively)
			SendMessage( GetDlgItem( hwnd, IDC_FPS_COMBO	 ), CB_INITSTORAGE,
				60, 60 );  // save space for 127 strings, each 127 bytes long. (respectively)
			//for (x=0; x<num_vid_modes; x++)
			//	  SendMessage( GetDlgItem( hwnd, IDC_BITDEPTHCOMBO ), CB_ADDSTRING, 
			//		  0, (LPARAM)szVidMode[x] );
			//SendMessage( GetDlgItem( hwnd, IDC_BITDEPTHCOMBO ), CB_SETCURSEL, 
			//		VidMode, 0 );
			
			
			return TRUE;
	}
	case WM_COMMAND:
		{ 
			int id=LOWORD(wParam);
			if (id==IDOK)
			{ 
				//if (DlgItemIsChecked(hwnd, IDC_VIDEOFIX))
				//	VIDEO_CARD_555 = 1;
				//else
				//	VIDEO_CARD_555 = 0;
				
				//dither//bDither			 = DlgItemIsChecked(hwnd, IDC_DITHER);
				//dither//bRandomDither 	 = DlgItemIsChecked(hwnd, IDC_RANDOMDITHER);
				g_bDebugMode	   = DlgItemIsChecked(hwnd, IDC_DEBUGINFO);
				g_bSuppressHelpMsg = DlgItemIsChecked(hwnd, IDC_HELPMSG);
				g_bRunDrempelsAtStartup = DlgItemIsChecked(hwnd, IDC_CB_STARTUP );
				g_bSuppressAllMsg  = DlgItemIsChecked(hwnd, IDC_SUPPRESSALLMSG);
				g_bExitOnMouseMove = DlgItemIsChecked(hwnd, IDC_EXITONMOUSEMOVE);
				high_quality	   = DlgItemIsChecked(hwnd, IDC_HIGHQUALITY);
				//desktop_mode		 = DlgItemIsChecked(hwnd, IDC_DESKTOPMODE);
				//fullscreen		 = DlgItemIsChecked(hwnd, IDC_FULLSCREEN);
				
				//AUTOMIN			 = DlgItemIsChecked(hwnd, IDC_AUTOMIN);
				VidMode 		= SendMessage( GetDlgItem( hwnd, IDC_BITDEPTHCOMBO ), CB_GETCURSEL, 0, 0);
				limit_fps		= 10 + SendMessage( GetDlgItem( hwnd, IDC_FPS_COMBO ), CB_GETCURSEL, 0, 0);
				//procsize		= SendMessage( GetDlgItem( hwnd, IDC_PROCSIZE_COMBO ), CB_GETCURSEL, 0, 0);
				//rendermode    = SendMessage( GetDlgItem( hwnd, IDC_RENDERMODE_COMBO ), CB_GETCURSEL, 0, 0);

				int cursel;

				// Rendermode/Procsize for saver-----------------------
                cursel				= SendMessage( GetDlgItem( hwnd, IDC_RENDERMODE_COMBO ), CB_GETCURSEL, 0, 0);
				procsize_as_saver	= 2 - (cursel - (cursel/3)*3);
                rendermode_as_saver = 0;

				// Rendermode/Procsize for app-------------------------
                cursel				= SendMessage( GetDlgItem( hwnd, IDC_DESKTOP_RENDERMODE_COMBO ), CB_GETCURSEL, 0, 0);
                procsize_as_app		= 2 - (cursel - (cursel/3)*3);
                rendermode_as_app	= 3;


    			SendMessage( GetDlgItem( hwnd, IDC_IMAGEDIR ), WM_GETTEXT, MAX_PATH-1, (LPARAM)szTexPath );


                char buf[64];
    			SendMessage( GetDlgItem( hwnd, IDC_SUBDIR_TIME ), WM_GETTEXT, MAX_PATH-1, (LPARAM)buf);
                sscanf(buf, "%d", &g_nTimeBetweenSubdirs);
                if (g_nTimeBetweenSubdirs < 1) g_nTimeBetweenSubdirs = 1;
                if (g_nTimeBetweenSubdirs > 99999) g_nTimeBetweenSubdirs = 99999;
                
				time_between_textures	= 0.001f*SendMessage( GetDlgItem( hwnd, IDC_TIMEBETWEENTEXTURES ), TBM_GETPOS, FALSE, 0 );
				texture_fade_time		= 0.001f*SendMessage( GetDlgItem( hwnd, IDC_TEXTUREFADETIME 	), TBM_GETPOS, FALSE, 0 );
				motion_blur 			= SendMessage( GetDlgItem( hwnd, IDC_MOTIONBLUR ), TBM_GETPOS, FALSE, 0 );
				float t_zoom		   = 0.001f*SendMessage( GetDlgItem( hwnd, IDC_MASTERZOOM		   ), TBM_GETPOS, FALSE, 0 );
				float t_switch		   = 0.001f*SendMessage( GetDlgItem( hwnd, IDC_MODESWITCHSPEEDMULTIPLIER	 ), TBM_GETPOS, FALSE, 0 );
				float t_anim		   = 0.001f*SendMessage( GetDlgItem( hwnd, IDC_ANIMATIONSPEED	   ), TBM_GETPOS, FALSE, 0 );
				master_zoom 				 = powf(2.0f, 4.0f*(t_zoom	 - 0.5f));
				mode_switch_speed_multiplier = powf(2.0f, 4.0f*(t_switch - 0.5f));
				anim_speed					 = powf(2.0f, 6.0f*(t_anim	 - 0.5f));
				
				FXW = VidList[VidMode].FXW;
				FXH = VidList[VidMode].FXH;
				iDispBits = VidList[VidMode].iDispBits;
				VIDEO_CARD_555 = VidList[VidMode].VIDEO_CARD_555;
				
				WriteConfigRegistry();
			}
			if (id==IDOK || id==IDCANCEL) 
			{
				g_ConfigAccepted = id==IDOK;
				EndDialog(hwnd,id);
				//exit(0); //beely
			}
			if (id==IDC_COLORKEY)
			{
				static COLORREF acrCustClr[16]; 

				CHOOSECOLOR cc;
				ZeroMemory(&cc, sizeof(CHOOSECOLOR));
				cc.lStructSize = sizeof(CHOOSECOLOR);
				cc.hwndOwner = NULL;//hSaverMainWindow;
				cc.Flags = CC_RGBINIT | CC_FULLOPEN;
				cc.rgbResult = RGB(key_R, key_G, key_B);
				cc.lpCustColors = (LPDWORD)acrCustClr;
				if (ChooseColor(&cc))
				{
					key_R = GetRValue(cc.rgbResult);
					key_G = GetGValue(cc.rgbResult);
					key_B = GetBValue(cc.rgbResult);
				}
			}
			if (id==IDABOUT)
			{
				DialogBox( hInstance, MAKEINTRESOURCE(IDD_ABOUT),
					hwnd, (DLGPROC)AboutDialogProc );
			}
			/ *if (id==IDREG)
			{
				dumpmsg("showing the disclaimer");
				DialogBox( hInstance, MAKEINTRESOURCE(IDD_DISC), hwnd, (DLGPROC)DisclaimerDialogProc ); 
				dumpmsg("done with disclaimer");
			}* /
			if (id==IDC_IMAGEDIRBUTTON)
			{
				dumpmsg("directory chooser");
				
				char szTitle[] = "Please choose the folder that houses your collection of 24-bit TGA/BMP/JPG image files (preferably 256x256 in size!).";
				
				BROWSEINFO bi;
				bi.hwndOwner = hwnd;
				bi.pidlRoot = NULL;
				bi.pszDisplayName = szTexPath;
				bi.lpszTitle = szTitle;
				bi.ulFlags = BIF_RETURNONLYFSDIRS;
				bi.lpfn = NULL;
				bi.lParam = NULL;
				bi.iImage = NULL;		//???
				
				ITEMIDLIST *id;
				id = ::SHBrowseForFolder(&bi);
				if (id)
				{
					SHGetPathFromIDList(id, szTexPath);
					SendMessage( GetDlgItem( hwnd, IDC_IMAGEDIR ), WM_SETTEXT, 0, (LPARAM)szTexPath );
					//delete id;
				}
				
				dumpmsg("done with directory chooser");
			}
	} 
	break;

	default:
		{
            int cursel      = SendMessage( GetDlgItem( hwnd, IDC_RENDERMODE_COMBO ), CB_GETCURSEL, 0, 0);
			::EnableWindow(GetDlgItem( hwnd, IDC_BITDEPTHCOMBO ), cursel/3 != 2 );
		}

  }
  return FALSE;
}
*/
/*
BOOL CALLBACK DisclaimerDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{

  switch (msg)
  { 
    case WM_INITDIALOG:
	{ 
		char buf[12000];

		SendMessage( GetDlgItem( hwnd, IDC_DISC ), EM_SETLIMITTEXT, 16000, 0 );

  	    wsprintf(buf,
				"Drempels is copyright (c) 2000 by Ryan M. Geiss.\r\n"
				"\r\n"
                );
		wsprintf((char *)(buf+strlen(buf)), 
                "TERMS OF USE / COPYRIGHT INFORMATION / PUBLIC DISPLAY\r\n"
                "---------------------------------\r\n"
                "Drempels is absolutely FREE for personal use.  However, if you\r\n"
                "wish to use it for any commercial purpose, you must first acquire\r\n"
                "permission from the author.  You can go about this by e-mailing\r\n"
                "ryan@geisswerks.com \r\n\r\n"
                );
		wsprintf((char *)(buf+strlen(buf)), 
				"DISTRIBUTION\r\n"
				"------------\r\n"
				"You may personally use and copy this program freely.  However, you may NOT charge\r\n"
				"money for distributing it.  Don't try to sell this program - it's free.\r\n"
                "\r\n"
                );
		wsprintf((char *)(buf+strlen(buf)), 
				"DISCLAIMER\r\n"
				"----------\r\n"
				"The author is not responsible for any damages or loss of data that\r\n"
				"occur to your system for using this program.  This program is distributed freely, and\r\n"
				"if you choose to use it, you take all risk on yourself.  If your\r\n"
				"system appears to have problems when this program is run, you should immediately\r\n"
				"discontinue using it.  If you have a sensitive medical condition such as epilepsy\r\n"
				"or a heart condition, you should not use this program.\r\n"
                "\r\n"
			);
		SendMessage( GetDlgItem( hwnd, IDC_DISC ), WM_SETTEXT, 0, (LPARAM)buf );
		

		return TRUE;
    }
    case WM_COMMAND:
    { 
	  int id=LOWORD(wParam);
      if (id==IDCANCEL)
      { 
		  EndDialog(hwnd,id);
          return FALSE;
		  //exit(0);
      }
      else if (id==IDOK)
	  {
		  EndDialog(hwnd,id);
          g_DisclaimerAgreed = true;
		  return TRUE;
	  }
    } 
	break;
  }
  return FALSE;
}

*/
/*
BOOL CALLBACK AboutDialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{

  switch (msg)
  { 
    case WM_INITDIALOG:
	{ 
		SendMessage( GetDlgItem( hwnd, IDC_EDIT1 ), WM_SETTEXT, 0, (LPARAM)"drempels@geisswerks.com" );
		SendMessage( GetDlgItem( hwnd, IDC_EDIT2 ), WM_SETTEXT, 0, (LPARAM)"http://www.geisswerks.com/drempels/" );
		return TRUE;
    }
	break;
    case WM_COMMAND:
    { 
	  int id=LOWORD(wParam);
      if (id==IDOK)
	  {
		  EndDialog(hwnd,id);
		  return TRUE;
	  }
	}
	break;
  }
  return FALSE;
}
*/



// This routine is for using ScrPrev. It's so that you can start the saver
// with the command line /p scrprev and it runs itself in a preview window.
// You must first copy ScrPrev somewhere in your search path
/*
HWND CheckForScrprev()
{ HWND hwnd=FindWindow("Scrprev",NULL); // looks for the Scrprev class
if (hwnd==NULL) // try to load it
{ STARTUPINFO si; PROCESS_INFORMATION pi; ZeroMemory(&si,sizeof(si)); ZeroMemory(&pi,sizeof(pi));
si.cb=sizeof(si);
si.lpReserved=NULL; si.lpTitle=NULL; si.dwFlags=0; si.cbReserved2=0; si.lpReserved2=0; si.lpDesktop=0;
BOOL cres=CreateProcess(NULL,"Scrprev",0,0,FALSE,CREATE_NEW_PROCESS_GROUP | CREATE_DEFAULT_ERROR_MODE,
						0,0,&si,&pi);
if (!cres) { dumpmsg("Error creating scrprev process"); return NULL;}
DWORD wres=WaitForInputIdle(pi.hProcess,2000);
if (wres==WAIT_TIMEOUT) {dumpmsg("Scrprev never becomes idle"); return NULL;}
if (wres==0xFFFFFFFF) {dumpmsg("ScrPrev, misc error after ScrPrev execution");return NULL;}
hwnd=FindWindow("Scrprev",NULL);
}
if (hwnd==NULL) {dumpmsg("Unable to find Scrprev window"); return NULL;}
::SetForegroundWindow(hwnd);
hwnd=GetWindow(hwnd,GW_CHILD);
if (hwnd==NULL) {dumpmsg("Couldn't find Scrprev child"); return NULL;}
return hwnd;
}
*/


void RefreshFileListings()
{
    // also check on existence of the 9 subdirs
    g_nSubdirs = 0;
     g_nSubdirIndex = -1;
    for (int i=1; i<=9; i++) {
        char buf[MAX_PATH];
        sprintf(buf, "%s\\%d", szTexPath, i);
        if (GetFileAttributes(buf) != -1)
            g_subdirIndices[g_nSubdirs++] = i;
    }
}


//int WINAPI WinMain(HINSTANCE h,HINSTANCE,LPSTR,int)
void DrempelsInit()
{ 
//	hInstance=h;
//	g_hSaverMainInstance = h;
//	HWND hwnd=NULL;

	// don't allow 2 copies to run at once (can happen w/screensavers)
//	if (FindWindow(NAME, TITLE))
//	{
//		return false;
//	}
			
//	char *c=GetCommandLine();
	
/*
	ScrMode = smNone;
	
	if (*c=='\"') {c++; while (*c!=0 && *c!='\"') c++;} else {while (*c!=0 && *c!=' ') c++;}
	if (*c!=0) c++;
	while (*c==' ') c++;
	//if (*c==0) {ScrMode=smConfig; hwnd=NULL;}
	if (*c==0) {ScrMode=smSaver; hwnd=NULL;}
	else
	{ 
		if (*c=='-' || *c=='/') c++;
		if (*c=='p' || *c=='P' || *c=='l' || *c=='L')
		{ 
			c++; while (*c==' ' || *c==':') c++;
			if ((strcmp(c,"scrprev")==0) || (strcmp(c,"ScrPrev")==0) || (strcmp(c,"SCRPREV")==0)) hwnd=CheckForScrprev();
			else hwnd=(HWND)atoi(c);
			ScrMode=smPreview;
		}
		else if (*c=='x' || *c=='X')
		{
			ScrMode=smSaver;
		}
		else if (*c=='y' || *c=='Y')
		{
			g_bStartupMode = true;
			ScrMode=smSaver;
		}
		else if (*c=='s' || *c=='S') 
        {
			g_bIsRunningAsSaver = true;
            ScrMode=smSaver; 
        }
		else if (*c=='c' || *c=='C') 
        {
            c++; 
            
            while (*c==' ' || *c==':') c++; 
            
            if (*c==0) 
                hwnd=GetForegroundWindow(); 
            else 
                hwnd=(HWND)atoi(c); 
            
            ScrMode=smConfig;
        }
		else if (*c=='a' || *c=='A') 
        {
            c++; 
            while (*c==' ' || *c==':') c++; 
            
            hwnd=(HWND)atoi(c); 
            ScrMode=smPassword;
        }
	}
	
	if (ScrMode != smConfig && ScrMode != smSaver)
	{
		exit(0);
	}
*/

  ScrMode = smSaver;

	/*
	if (g_bStartupMode == true)
		{
			LONG res; 
			HKEY skey; 
			DWORD valtype, valsize, val;
			
			res=RegOpenKeyEx(HKEY_CURRENT_USER,APPREGPATH,0,KEY_ALL_ACCESS,&skey);
			if (res==ERROR_SUCCESS)
			{
				res=RegQueryValueEx(skey,"launch_at_startup",0,&valtype,(LPBYTE)&val,&valsize);	 
				if (res==ERROR_SUCCESS) g_bRunDrempelsAtStartup=val; 
			
				RegCloseKey(skey);
			}
	
			if (!g_bRunDrempelsAtStartup)
			{
				exit(0);
			}
		}*/
	
	
	//---------- if they've never run Geiss before, or if they just upgraded,
	//			 show them the Settings screen -------------
	/*
	LONG res, res2; 
		HKEY skey; 
		DWORD valtype, valsize, val;
		int version = -1;
		valsize=sizeof(val); 
		char buf[128];
		
		res=RegOpenKeyEx(HKEY_CURRENT_USER,APPREGPATH,0,KEY_ALL_ACCESS,&skey);
		
		if (res==ERROR_SUCCESS)
		{
			res2=RegQueryValueEx(skey,"Version",0,&valtype,(LPBYTE)&val,&valsize);	 
			if (res2==ERROR_SUCCESS) version=val;
		}
		
		RegCloseKey(skey);
		
		if (res!=ERROR_SUCCESS)	 // give them the disclaimer
		{
			InitCommonControls(); // loads common controls DLL 
			ScrMode = smConfig;
			dumpmsg("showing the disclaimer");
	        / *
			g_DisclaimerAgreed = false;
			DialogBox( h, MAKEINTRESOURCE(IDD_DISC), hwnd, (DLGPROC)DisclaimerDialogProc ); 
			dumpmsg("done with disclaimer");
			g_bFirstRun = true;
			if (!g_DisclaimerAgreed)
			{
				return 1;
			}
	        * /
	        g_bFirstRun = true;
	        g_DisclaimerAgreed = true; // skip it
		}
		
		if (version != CURRENT_VERSION)		  // bring up new config dialog
		{
			ScrMode = smConfig;
			g_bFirstRun = true;	// so it will auto-pick 640x480x8.
		}*/
	
	
	/*
	// if this guy has problems w/reading the registry, (like FXW or FXH or iDispBits is 
		//  screwy) he'll set scrMode to smConfig.
		dumpmsg("Reading registry");
		
		if (ScrMode==smConfig) dumpmsg(" mode before ReadConfigRegistry() is config");
		if (ScrMode==smSaver) dumpmsg(" mode before ReadConfigRegistry() is normal");
		//-------------------
		ReadConfigRegistry();
		//-------------------
		
		sprintf(buf, "Drempels debug file, version %d", CURRENT_VERSION);
		dumpmsg(buf);
	
		sprintf(buf, "Command line: %s", GetCommandLine());
		dumpmsg(buf);
		
		if (ScrMode==smConfig)        dumpmsg("Mode of Operation: running config panel");
		else if (ScrMode==smSaver)    dumpmsg("Mode of Operation: running screensaver");
		else if (ScrMode==smNone)     dumpmsg("Mode of Operation: ? (none)");
		else if (ScrMode==smPreview)  dumpmsg("Mode of Operation: running preview");
		else if (ScrMode==smPassword) dumpmsg("Mode of Operation: running password dialog");
		
		*/
	
/*
	
		if (ScrMode==smConfig) //DialogBox(hInstance,MAKEINTRESOURCE(DLG_CONFIG),hwnd,ConfigDialogProc);
		{
			//make sure the sliderbar, etc. controls are loaded:
			dumpmsg("InitCommonControls()...");
			InitCommonControls(); // loads common control’s DLL 
			
			dumpmsg("Bringing up Config Dialog...");
			DialogBox( h, MAKEINTRESOURCE(IDD_CONFIG),
				hwnd, (DLGPROC)ConfigDialogProc );
			
			return 1;
		}
		*/
	
	if (ScrMode==smSaver) 
	{
		// first get TGA file listing!
		if (!TEX.EnumTgaAndBmpFiles(szTexPath))
		{
//      dumpmsg("Could not find any textures\n");
//			char s[2048];
//			sprintf(s, "Could not find any TGA, BMP, or JPG image files in the directory you specified, or the directory does not exist! [%s]  At least one .TGA or .BMP file must be there so the program has a texture to use.  (Also, the images must be 24-bit Targe (TGA), Bitmap (BMP), or Jpeg (JPG) files, preferably 256x256 in size.)", szTexPath);
//			MessageBox(NULL, s, "Error - failed to find textures", MB_OK);
//			exit(1);
		}

        RefreshFileListings();
		
		// also ensure MMX is present
//		bMMX = CheckMMXTechnology();    
//		if (!bMMX)
//		{
//			MessageBox(NULL, "This program requires your computer to support the MMX instruction set.  Sorry!", "MMX instruction set unavailable", MB_OK);
//			exit(1);
//		}
//		
//		DoSaver(hwnd, h);
	}
  doInit();

  mainproc_start_time = GetTickCount();

 	RandomizeStartValues();


//	return 0;
}

//--------------------------------------------------------------
//--------------------------------------------------------------

BOOL FX_Init()
{
//	dumpmsg("Init: begin FX_Init");

	memset(tex_fade_debt, 0, sizeof(float)*FPS_FRAMES);
	memset(time_rec, 0, sizeof(float)*FPS_FRAMES);

	/*
	TGA_to_buffer("c:\\z\\img00.tga", mix[0], FXW, FXH);
	TGA_to_buffer("c:\\z\\img01.tga", mix[1], FXW, FXH);
	TGA_to_buffer("c:\\z\\img02.tga", mix[2], FXW, FXH);
	TGA_to_buffer("c:\\z\\img03.tga", mix[3], FXW, FXH);
	*/
	srand(time(NULL) + (rand()%256));
	
	gXC = FXW/2;
	gYC = FXH/2;

//	dumpmsg("Init: end FX_Init"); 
	return true;
}

void FX_Fini()
{
	// collect garbage here
}


/*

bool GetDDCaps()
{
    HRESULT ddrval;
 
    // Get driver capabilities to determine Overlay support.
    ZeroMemory(&g_ddcaps, sizeof(g_ddcaps));
    g_ddcaps.dwSize = sizeof(g_ddcaps);
 
    ddrval = lpDD->GetCaps(&g_ddcaps, NULL);
    if (FAILED(ddrval))
        return false;
  
    return true;
}

*/

/*
bool TryCreateOverlayRGB(DDSURFACEDESC *pddsd)
{
	HRESULT ddrval;
	#define MAX_CC_CODES 64
	DWORD iNumCodes = MAX_CC_CODES;
	DWORD codes[MAX_CC_CODES];

	dumpmsg("Init: creating overlay surface");

    // stab at 16-bit YUV formats

    g_eOverlayFormat = UYVY;
   
	ZeroMemory(&pddsd->ddpfPixelFormat, sizeof(DDPIXELFORMAT));
    pddsd->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    pddsd->ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    pddsd->ddpfPixelFormat.dwFourCC = mmioFOURCC('U','Y','V','Y');
    pddsd->ddpfPixelFormat.dwYUVBitCount = 16;
	ddrval = lpDD->CreateSurface( pddsd, &lpDDSOverlay, NULL );
	if( ddrval == DD_OK ) return true;

    g_eOverlayFormat = YUY2;
    
	ZeroMemory(&pddsd->ddpfPixelFormat, sizeof(DDPIXELFORMAT));
    pddsd->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    pddsd->ddpfPixelFormat.dwFlags = DDPF_FOURCC | DDPF_YUV;
    pddsd->ddpfPixelFormat.dwFourCC = mmioFOURCC('Y','U','Y','2');
    pddsd->ddpfPixelFormat.dwYUVBitCount = 16;
	ddrval = lpDD->CreateSurface( pddsd, &lpDDSOverlay, NULL );
	if( ddrval == DD_OK ) return true;

    g_eOverlayFormat = RGB_OVERLAY;

    // stab at 16/32-bit uncompressed RGB formats (ideal!)
    ZeroMemory(&pddsd->ddpfPixelFormat, sizeof(DDPIXELFORMAT));
    pddsd->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    pddsd->ddpfPixelFormat.dwFourCC = BI_RGB;
    pddsd->ddpfPixelFormat.dwRGBBitCount = 16;
    pddsd->ddpfPixelFormat.dwFlags = DDPF_RGB;
	ddrval = lpDD->CreateSurface( pddsd, &lpDDSOverlay, NULL );
	if( ddrval == DD_OK ) return true;

    ZeroMemory(&pddsd->ddpfPixelFormat, sizeof(DDPIXELFORMAT));
    pddsd->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    pddsd->ddpfPixelFormat.dwFourCC = BI_RGB;
    pddsd->ddpfPixelFormat.dwRGBBitCount = 32;
    pddsd->ddpfPixelFormat.dwFlags = DDPF_RGB;
	ddrval = lpDD->CreateSurface( pddsd, &lpDDSOverlay, NULL );
	if( ddrval == DD_OK ) return true;

	// this one works on the ATI Rage 128.
    ZeroMemory(&pddsd->ddpfPixelFormat, sizeof(DDPIXELFORMAT));
    pddsd->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    pddsd->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
	pddsd->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
	pddsd->ddpfPixelFormat.dwBBitMask = 0x000000FF;
	pddsd->ddpfPixelFormat.dwRGBBitCount = 32;
	pddsd->ddpfPixelFormat.dwFlags = DDPF_RGB;
	ddrval = lpDD->CreateSurface( pddsd, &lpDDSOverlay, NULL );
	if( ddrval == DD_OK ) return true;

    ZeroMemory(&pddsd->ddpfPixelFormat, sizeof(DDPIXELFORMAT));
    pddsd->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    pddsd->ddpfPixelFormat.dwRBitMask = 0x000000FF;
	pddsd->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
	pddsd->ddpfPixelFormat.dwBBitMask = 0x00FF0000;
	pddsd->ddpfPixelFormat.dwRGBBitCount = 32;
	pddsd->ddpfPixelFormat.dwFlags = DDPF_RGB;
	ddrval = lpDD->CreateSurface( pddsd, &lpDDSOverlay, NULL );
	if( ddrval == DD_OK ) return true;

    ZeroMemory(&pddsd->ddpfPixelFormat, sizeof(DDPIXELFORMAT));
    pddsd->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    pddsd->ddpfPixelFormat.dwRBitMask = 0xFF000000;
	pddsd->ddpfPixelFormat.dwGBitMask = 0x00FF0000;
	pddsd->ddpfPixelFormat.dwBBitMask = 0x0000FF00;
	pddsd->ddpfPixelFormat.dwRGBBitCount = 32;
	pddsd->ddpfPixelFormat.dwFlags = DDPF_RGB;
	ddrval = lpDD->CreateSurface( pddsd, &lpDDSOverlay, NULL );
	if( ddrval == DD_OK ) return true;

    ZeroMemory(&pddsd->ddpfPixelFormat, sizeof(DDPIXELFORMAT));
    pddsd->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    pddsd->ddpfPixelFormat.dwRBitMask = 0x0000FF00;
	pddsd->ddpfPixelFormat.dwGBitMask = 0x00FF0000;
	pddsd->ddpfPixelFormat.dwBBitMask = 0xFF000000;
	pddsd->ddpfPixelFormat.dwRGBBitCount = 32;
	pddsd->ddpfPixelFormat.dwFlags = DDPF_RGB;
	ddrval = lpDD->CreateSurface( pddsd, &lpDDSOverlay, NULL );
	if( ddrval == DD_OK ) return true;

    g_eOverlayFormat = MISC;

	// try not specifying
    ZeroMemory(&pddsd->ddpfPixelFormat, sizeof(DDPIXELFORMAT));
    pddsd->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddrval = lpDD->CreateSurface( pddsd, &lpDDSOverlay, NULL );
	if( ddrval == DD_OK ) return true;

    g_eOverlayFormat = NATIVE;

	// try a native four-character-code

    dumpmsg("Init: attempting to use fourcc overlay");

    if (!(g_ddcaps.dwCaps & DDCAPS_OVERLAYFOURCC))
    {
		MessageBox( NULL, "failed to create overlay (no four-cc codes!).", "ERROR", MB_OK );
		finiObjects();
		return FALSE;
    }
        
	if (lpDD->GetFourCCCodes(&iNumCodes, codes) != DD_OK)
	{
   		MessageBox( NULL, "failed to create overlay (GetFourCCCodes failed).", "ERROR", MB_OK );
		finiObjects();
		return FALSE;
	}

	int iCodeUsed = 0;
	do
	{
	    ZeroMemory(&pddsd->ddpfPixelFormat, sizeof(DDPIXELFORMAT));
        pddsd->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		pddsd->ddpfPixelFormat.dwFlags = DDPF_FOURCC;
		pddsd->ddpfPixelFormat.dwFourCC = codes[iCodeUsed];
		ddrval = lpDD->CreateSurface( pddsd, &lpDDSBack, NULL );
		iCodeUsed++;
	} while (ddrval != DD_OK && iCodeUsed < iNumCodes && iCodeUsed < MAX_CC_CODES);
	iCodeUsed--;

    if (ddrval != DD_OK)
    {
   		MessageBox( NULL, "failed to create overlay (no fourcc codes worked).", "ERROR", MB_OK );
		finiObjects();
		return FALSE;
	}

	return false;
}

*/

/*

void Destroy_Overlay()
{
    if (lpDDSOverlay)
    {
        lpDDSOverlay->Release();
        lpDDSOverlay = NULL;
    }
}

*/

/*
bool Init_Overlay(bool bShowMsgBoxes)
{
    // create an overlay surface (lpDDSOverlay) with front & back.
    // (lpDDSBack) will point to its back buffer.
    DDSURFACEDESC ddsd;

	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_BACKBUFFERCOUNT; 
	ddsd.ddsCaps.dwCaps =   DDSCAPS_OVERLAY | 
                            DDSCAPS_FLIP |
                            DDSCAPS_COMPLEX | 
                            DDSCAPS_VIDEOMEMORY;
    ddsd.dwBackBufferCount = 1;
	ddsd.dwWidth  = FXW2;
	ddsd.dwHeight = FXH2;

	if (!TryCreateOverlayRGB(&ddsd))
	{
		// (failure messagebox will be given in TryCreateOverlay,
		//  and finiObjects called as well)
		return false;
	}

    switch(g_eOverlayFormat)
    {
    case RGB_OVERLAY:
        dumpmsg("Init: overlay creation successful; format is RGB"); break;
    case UYVY:
        dumpmsg("Init: overlay creation successful; format is U-Y-V-Y"); break;
    case YUY2:
        dumpmsg("Init: overlay creation successful; format is Y-U-Y-2"); break;
    case MISC:
        dumpmsg("Init: overlay creation successful; format is MISC (unspecified pf)"); break;
    case NATIVE:
        dumpmsg("Init: overlay creation successful; format is NATIVE (fourcc)"); break;
    default:
        dumpmsg("Init: overlay creation successful; format is ???"); break;
    }

	dumpmsg("Init: getting pointer to overlay back buffer");

    DDSCAPS caps;
    caps.dwCaps = DDSCAPS_BACKBUFFER;
    if (lpDDSOverlay->GetAttachedSurface(&caps, &lpDDSBack) != DD_OK)
    {
		MessageBox( NULL, "Couldn't get back buffer for overlay.", "ERROR", MB_OK );
		finiObjects();
		return FALSE;
    }

	//----------------------------------------------------
	// color-keying and overlay update 
	//----------------------------------------------------

	dumpmsg("Init: colorkey");

	//SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, "c:\\windows\\drempels.bmp", true);
    DDSURFACEDESC ddsd_system;
    ZeroMemory(&ddsd_system, sizeof(DDSURFACEDESC));
    ddsd_system.dwSize = sizeof(DDSURFACEDESC);
    if (lpDD->GetDisplayMode(&ddsd_system) != DD_OK ||
        !(ddsd_system.dwFlags & DDSD_PIXELFORMAT)   ||
        ddsd_system.ddpfPixelFormat.dwRBitMask == 0 ||
        ddsd_system.ddpfPixelFormat.dwGBitMask == 0 ||
        ddsd_system.ddpfPixelFormat.dwBBitMask == 0)
    {
        // couldn't get display mode; guess @ 32 bits.
		dumpmsg("WARNING: couldn't get display mode pixel format -> guessing 32 bits for color key!");
		g_ddck.dwColorSpaceLowValue  = RGB(key_R, key_G, key_B);
		g_ddck.dwColorSpaceHighValue = RGB(key_R, key_G, key_B);
    	// NOTE: the RGB macro is actually B-G-R (on TNT2) (and ATI Rage 128) (and on Voodoo3)
    }
    else
    {
        // use R/G/B bit masks to get color key color

        int nMaskBits[3] = { 0,0,0 };
		int nStartBit[3] = { -1,-1,-1 };

		for (int i=0; i<32; i++)
		{
			if (ddsd_system.ddpfPixelFormat.dwRBitMask & (1 << i)) 
			{
				nMaskBits[0]++;
				if (nStartBit[0] == -1) 
					nStartBit[0] = i;
			}
			if (ddsd_system.ddpfPixelFormat.dwGBitMask & (1 << i))
			{
				nMaskBits[1]++;
				if (nStartBit[1] == -1) 
					nStartBit[1] = i;
			}
			if (ddsd_system.ddpfPixelFormat.dwBBitMask & (1 << i)) 
			{
				nMaskBits[2]++;
				if (nStartBit[2] == -1) 
					nStartBit[2] = i;
			}
		}

		//g_ddck.dwColorSpaceLowValue = ((key_R>>3) << 11) | ((key_G>>2) << 5) | (key_B>>3);
        g_ddck.dwColorSpaceLowValue  = 0;
		g_ddck.dwColorSpaceLowValue |= (key_R >> (8 - nMaskBits[0])) << nStartBit[0];
		g_ddck.dwColorSpaceLowValue |= (key_G >> (8 - nMaskBits[1])) << nStartBit[1];
		g_ddck.dwColorSpaceLowValue |= (key_B >> (8 - nMaskBits[2])) << nStartBit[2];
        g_ddck.dwColorSpaceHighValue = g_ddck.dwColorSpaceLowValue;

		char buf[64];
		sprintf(buf, "Color key = %d %d %d, iDispBits = %d, hex key = %08x", key_R, key_G, key_B, iDispBits, g_ddck.dwColorSpaceLowValue);
		dumpmsg(buf);

		/ *
        g_ddck.dwColorSpaceLowValue  = 0;

        if (g_bColorKeyR)
            g_ddck.dwColorSpaceLowValue  |= ddsd_system.ddpfPixelFormat.dwRBitMask;
        if (g_bColorKeyG)
            g_ddck.dwColorSpaceLowValue  |= ddsd_system.ddpfPixelFormat.dwGBitMask;
        if (g_bColorKeyB)
            g_ddck.dwColorSpaceLowValue  |= ddsd_system.ddpfPixelFormat.dwBBitMask;

        g_ddck.dwColorSpaceHighValue = g_ddck.dwColorSpaceLowValue;
        * /

		//g_ddck.dwColorSpaceLowValue  = RGB(255,0,255);
		//g_ddck.dwColorSpaceHighValue = RGB(255,0,255);
		//g_ddck.dwColorSpaceLowValue  = 0xf81F;
		//g_ddck.dwColorSpaceHighValue = 0xf81F;
    	//NOTE: the RGB macro is actually B-G-R (on TNT2) (and ATI Rage 128) (and on Voodoo3)
    }

	char buf[256];
	sprintf(buf, "Init: key is %08x", g_ddck.dwColorSpaceLowValue);
	dumpmsg(buf);
	dumpmsg("Init: calling UpdateOverlay for first time");
    
    if (!Update_Overlay())
    {
        if (bShowMsgBoxes)
    		MessageBox( NULL, "UpdateOverlay failed.", "ERROR", MB_OK );
		finiObjects();
		return FALSE;
    }

	dumpmsg("Init: overlay beta coefficient is 1.48; ready to go.");

    return true;
}


*/
/*
bool Update_Overlay()
{
    HRESULT hr;

    if (!lpDDSPrimary)
	{
		dumpmsg("ERROR: Update_Overlay: lpDDSPrimary is NULL");
        return false;
	}
	
    if (lpDDSPrimary->IsLost())
        if (lpDDSPrimary->Restore() != DD_OK)
		{
			dumpmsg("ERROR: Update_Overlay: couldn't restore lpDDSPrimary");
            return false;
		}

	DDOVERLAYFX ovfx;
	ZeroMemory(&ovfx, sizeof(ovfx));
	ovfx.dwSize = sizeof(ovfx);
	//TRANSPAR:
    ovfx.dckDestColorkey.dwColorSpaceLowValue = g_ddck.dwColorSpaceLowValue; 
	ovfx.dckDestColorkey.dwColorSpaceHighValue = g_ddck.dwColorSpaceHighValue;
    //ovfx.dwAlphaSrcConst = 128;
    //ovfx.dwAlphaSrcConstBitDepth = 8;
    
    //TRANSPAR:
    DWORD dwUpdateFlags = DDOVER_SHOW | DDOVER_KEYDESTOVERRIDE;
                //DDOVER_SHOW | / *DDOVER_ALPHASRCCONSTOVERRIDE | * /DDOVER_ALPHASRC;

    RECT rd;
	rd.top = 0;
	rd.left = 0;
	rd.right = FXW; //TRANSPAR
	rd.bottom = FXH; //TRANSPAR

	// Call UpdateOverlay() to display the overlay on the 
	// screen.
	hr = lpDDSOverlay->UpdateOverlay(NULL, lpDDSPrimary, &rd, dwUpdateFlags, &ovfx);
    if (hr != DD_OK) 
    {
        dumpmsg("ERROR: Update_Overlay: failed!");
        return false;
    }
    
    return true;
}


*/
BOOL doInit()
{
//    WNDCLASS            wc;
//    DDSURFACEDESC       ddsd;
//    DDSCAPS             ddscaps;
//    HRESULT             ddrval;
//    char                buf[256];

//	ZeroMemory(&wc, sizeof(WNDCLASS));
//	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
//	ZeroMemory(&ddscaps, sizeof(DDSCAPS));

//    HWND                hMainWnd = NULL;   // for the plugin, hMainWnd is a GLOBAL!

//	dumpmsg("");
//	dumpmsg("Init: begin");

/*
    // protect vs. 8-bit
    if (rendermode != 0)
    {
        DEVMODE dm;
        ZeroMemory(&dm, sizeof(DEVMODE));
        dm.dmSize = sizeof(DEVMODE);
        if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
        {
            if (dm.dmBitsPerPel == 8)
            {
                MessageBox(NULL, "Drempels can not run in Windowed or Desktop mode when your color depth is 8 bits!  Please either change your Color Depth to 16, 24, or 32 bits (from Display Settings under the Control Panel); or you can always run Drempels in Fullscreen mode.", "ERROR (can't run in 8-bit color)", MB_OK);
                return false;
            }
        }
    }

*/
//	switch(rendermode)
//	{
//	case 0:
//		dumpmsg("Init: mode is FULLSCREEN"); break;
//	case 1:
//		dumpmsg("Init: mode is WINDOWED"); break;
//	case 2:
//		dumpmsg("Init: mode is BACKGROUND MODE (fake desktop mode)"); break;
//	case 3:
//		dumpmsg("Init: mode is DESKTOP MODE"); break;
//	};

    // windowed mode: clip window size to screen size
/*
    if (rendermode==1)
    {
		int cx = GetSystemMetrics(SM_CXSCREEN);
		int cy = GetSystemMetrics(SM_CYSCREEN);
        if (cx < FXW || cy < FXH)
        {
            char buf[64];
            sprintf(buf, "Init: window is too big... downsizing from %dx%d to %dx%d", FXW, FXH, cx, cy);
            dumpmsg(buf);

            FXW = cx;
            FXH = cy;
	        switch(procsize)
	        {
	        case 0:
		        FXW2 = FXW;
		        FXH2 = FXH;
		        break;
	        case 1:
		        FXW2 = FXW/2;
		        FXH2 = FXH/2;
		        break;
	        case 2:
		        FXW2 = FXW/4;
		        FXH2 = FXH/4;
		        break;
	        default:
		        procsize = 0;
		        FXW2 = FXW;
		        FXH2 = FXH;
		        break;
	        }
        }
    }
*/

    // desktop mode: get FXW, FXH
/*
	if (rendermode==2 || rendermode==3)
	{
		FXW = GetSystemMetrics(SM_CXSCREEN);
		FXH = GetSystemMetrics(SM_CYSCREEN);
    }

*/
    // --- at this point, FXW and FXH are set in stone!

    // now determine FXW2, FXH2
	switch(procsize)
	{
	case 0:
		FXW2 = FXW;
		FXH2 = FXH;
		break;
	case 1:
		FXW2 = FXW/2;
		FXH2 = FXH/2;
		break;
	case 2:
		FXW2 = FXW/4;
		FXH2 = FXH/4;
		break;
	default:
		procsize = 0;
		FXW2 = FXW;
		FXH2 = FXH;
		break;
	}

//	sprintf(buf, "Init: width=%d, height=%d, bits=%d", FXW2, FXH2, iDispBits);
//	dumpmsg(buf);
//	sprintf(buf, "Init: final width=%d, final height=%d", FXW, FXH);
//	dumpmsg(buf);
//
//	if (iDispBits==16)
//	{
//		if (VIDEO_CARD_555) 
//			dumpmsg("Init: 16-bit pixel format is 5-5-5");
//		else
//			dumpmsg("Init: 16-bit pixel format is 5-6-5");
//	}
	
/*
	dumpmsg("Init: registering window class");

	/ *
     * set up and register window class
     * /
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;					// prcessess messages
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;  // must be 'this_mod->hDllInstance' for plugin!  (okay)
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_LET_OP) );
    wc.hCursor = NULL;//cursor//LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NAME;
    wc.lpszClassName = NAME;
	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, "RegisterClass() failed.", "ERROR", MB_OK);
		return false;
	}
*/

/*
	// determine necessary window size (for windowed mode)
	// this will be > FXW,FXH because of the borders and the menu bar.
	g_clientRect.top = 0;
	g_clientRect.left = 0;
	g_clientRect.right = FXW;
	g_clientRect.bottom = FXH;

*/

    /*
     * create a window
     */
/*
	dumpmsg("Init: creating main window");

    // must be hMainWnd for plugin
	
	if (rendermode==0)
	{
		hMainWnd = CreateWindowEx(
			WS_EX_TOPMOST,
			NAME,
			TITLE,
			WS_POPUP | WS_VISIBLE,  // WS_VISIBLE added in 3.40/beta 4
			0, 0,
			FXW,//100, //GetSystemMetrics( SM_CXSCREEN ),     // beely beely!!!! was 640x80!!!
			FXH,//100, //GetSystemMetrics( SM_CYSCREEN ),     // (changed 12/22/98)
			NULL, 
			NULL,
			hInstance,
			NULL 
		);
	}
	else if (rendermode==1) // window
	{
		if (!AdjustWindowRectEx(&g_clientRect, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) ^ (WS_THICKFRAME), false, 0))
		{
			MessageBox(NULL, "AdjustWindowRectEx() failed.", "ERROR", MB_OK);
			return false;
		}
		
		hMainWnd = CreateWindowEx(
			0,
			NAME,
			TITLE,
            (WS_OVERLAPPEDWINDOW | WS_VISIBLE) ^ (WS_THICKFRAME | WS_MAXIMIZEBOX),	// removing WS_THICKFRAME makes the window border un-resizeable.
			0, 
			0,
			g_clientRect.right - g_clientRect.left,//GetSystemMetrics(SM_CXSCREEN),
			g_clientRect.bottom - g_clientRect.top,//GetSystemMetrics(SM_CYSCREEN),
			NULL, 
			NULL,
			hInstance,
			NULL 
		);
	}
	else if (rendermode==2) // background mode, preserving icons
	{
		DWORD ws = (WS_OVERLAPPEDWINDOW | WS_VISIBLE)^ (WS_MAXIMIZEBOX | WS_THICKFRAME);// ^ (WS_THICKFRAME | WS_BORDER);
		DWORD wsex = 0;
        
		if (!AdjustWindowRectEx(&g_clientRect, ws, false, wsex))
		{
			MessageBox(NULL, "AdjustWindowRectEx() failed.", "ERROR", MB_OK);
			return false;
		}

		hMainWnd = CreateWindowEx(
			wsex, // 'toolwindow' makes it borderless; 'appwindow' gives it a taskbar button.
			NAME,
			TITLE,
            ws,
			g_clientRect.left, 
			g_clientRect.top,
			g_clientRect.right - g_clientRect.left,//GetSystemMetrics(SM_CXSCREEN),
			g_clientRect.bottom - g_clientRect.top,//GetSystemMetrics(SM_CYSCREEN),
			GetDesktopWindow(),//NULL, 
			NULL,
			hInstance,
			NULL 
		);
	}
    else if (rendermode==3) // desktop mode, preserving the icons
	{
		if (!AdjustWindowRectEx(&g_clientRect, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) ^ (WS_THICKFRAME), false, 0))
		{
			MessageBox(NULL, "AdjustWindowRectEx() failed.", "ERROR", MB_OK);
			return false;
		}

		hMainWnd = CreateWindowEx(
			WS_EX_TOOLWINDOW,//|WS_EX_TRANSPARENT,      //TRANSPAR
			NAME,
			TITLE,
            //(WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_MINIMIZEBOX) ^ (WS_THICKFRAME | WS_MAXIMIZEBOX),	// removing WS_THICKFRAME makes the window border un-resizeable.
			(WS_VISIBLE),// | WS_SYSMENU),
			-200,	// x pos
			0,//FXH - 200,  // y pos
			120,   
			80, 
			NULL, 
			NULL,
			hInstance,
			NULL 
		);

		ShowWindow(hMainWnd, SW_HIDE);//SW_MINIMIZE);

		// create an icon in the taskbar
		NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
		nid.hWnd = hMainWnd;
		nid.uID = IDC_DREMPELS_SYSTRAY_ICON;
		nid.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_SYSTRAY) );
		nid.uCallbackMessage = WM_DREMPELS_SYSTRAY_MSG;
		strcpy(nid.szTip, "Drempels");
		nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
		Shell_NotifyIcon(NIM_ADD, &nid);
		
		g_bSystrayReady = true;

	}
	
    if( !hMainWnd )
    {
		MessageBox(NULL, "CreateWindowEx() failed.", "ERROR", MB_OK);
		return false;
    }

	hSaverMainWindow = hMainWnd;

	dumpmsg("Init: ShowWindow, UpdateWindow, and SetCursor(NULL)");
	if (rendermode==0)
	{
		ShowWindow( hMainWnd, SW_MAXIMIZE / *nCmdShow* / );
		UpdateWindow( hMainWnd );               //beely2: wasn't in plugin
		SetCursor(NULL);
	}
*/

    /*
     * create the main DirectDraw object
     */

/*
		dumpmsg("Init: creating main DirectDraw object");
	
	    ddrval = DirectDrawCreate( NULL, &lpDD, NULL );
	    if( ddrval != DD_OK )
	    {
			wsprintf(buf, "Direct Draw Init Failed - you probably don't have DirectX installed! (%08lx)\n", ddrval );
			MessageBox( NULL, buf, "ERROR", MB_OK );
			dumpmsg(buf); 
			finiObjects();
			return FALSE;
		}*/
	

/*
	dumpmsg("Init: setting cooperative level");

    if (rendermode == 0)// || rendermode == 3)
		ddrval = lpDD->SetCooperativeLevel( hMainWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN );
	else
		ddrval = lpDD->SetCooperativeLevel( NULL, DDSCL_NORMAL );

    if(ddrval != DD_OK )
	{
		wsprintf(buf, "DirectX couldn't set EXCLUSIVE and FULLSCREEN mode (ddrval=%08lx).", ddrval );
		MessageBox( NULL, buf, "ERROR INITIALIZING", MB_OK );
		dumpmsg(buf); 
		finiObjects();
		return FALSE;
	}
*/

/*
	if (rendermode==0)
	{
		dumpmsg("Init: SetDisplayMode()");
		ddrval = lpDD->SetDisplayMode( FXW, FXH, iDispBits );
		if ( ddrval != DD_OK )
		{
			wsprintf(buf, "Sorry!  Your video card doesn't support the %dx%dx%d video mode.\n", FXW, FXH, iDispBits, ddrval );
			MessageBox( NULL, buf, "ERROR", MB_OK );
			finiObjects();
			return FALSE;
		}
		g_bDisplayModeChanged = true;
	}
*/

    // do this here, while monitor changes display mode
    if (!FX_Init())
    {
		DrempelsExit();
		return false;
    }

	//----------------------------------------------------
	// primary surface
	//----------------------------------------------------

//	dumpmsg("Init: creating primary surface");

/*
	// Create the primary surface with 1 back buffer
	if (rendermode==0)
	{
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
		ddsd.dwSize = sizeof( ddsd );
		ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;// | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM; 
		ddsd.dwBackBufferCount = 1;
	}
	else
	{
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
		ddsd.dwSize = sizeof( ddsd );
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;// | DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
	}

	ddrval = lpDD->CreateSurface( &ddsd, &lpDDSPrimary, NULL );
	if( ddrval != DD_OK )
	{
		char buf[32];
		sprintf(buf, "error #%08x", ddrval);
		MessageBox( NULL, "failed to create primary surface.", buf, MB_OK );
		dumpmsg(buf); 
		finiObjects();
		return FALSE;
	}

*/
    
    // get caps
/*
	dumpmsg("Init: getting DDraw caps");
    if (!GetDDCaps()) // sets g_ddcaps
    {
		MessageBox( NULL, "could not get DD caps", buf, MB_OK );
		finiObjects();
		return FALSE;
    }


    int uStretchFactor1000;
    DWORD dwOverlayXPositionAlignment;
    bool bOverlay = g_ddcaps.dwCaps & DDCAPS_OVERLAY;
    bool bOverlayCantClip = g_ddcaps.dwCaps & DDCAPS_OVERLAYCANTCLIP;
    bool bOverlayFourCC = g_ddcaps.dwCaps & DDCAPS_OVERLAYFOURCC;
    bool bOverlayStretch = g_ddcaps.dwCaps & DDCAPS_OVERLAYSTRETCH;
    bool bDestKey = g_ddcaps.dwCKeyCaps & DDCKEYCAPS_DESTOVERLAY;

    // voodoo3: dwMinOverlayStretch = .1, dwMaxOverlayStretch = 10
    if(g_ddcaps.dwCaps & DDCAPS_OVERLAYSTRETCH)
        uStretchFactor1000 = (g_ddcaps.dwMinOverlayStretch>1000) ? g_ddcaps.dwMinOverlayStretch : 1000;
    else
        uStretchFactor1000 = 1000;

    // Grab any alignment restrictions and set the local 
    // variables accordingly.
    int uSrcSizeAlign = (g_ddcaps.dwCaps & DDCAPS_ALIGNSIZESRC) ? g_ddcaps.dwAlignSizeSrc : 0;
    int uDestSizeAlign= (g_ddcaps.dwCaps & DDCAPS_ALIGNSIZEDEST) ? g_ddcaps.dwAlignSizeDest : 0;


    // Set the "destination position alignment" global so we 
    // won't have to keep calling GetCaps() every time we move 
    // the overlay surface.
    if (g_ddcaps.dwCaps & DDCAPS_ALIGNBOUNDARYDEST)
        dwOverlayXPositionAlignment = g_ddcaps.dwAlignBoundaryDest;
    else
        dwOverlayXPositionAlignment = 0;

*/

	/*
	// test if overlays are supported - 2 checks:
	if (!AreOverlaysSupported())	// only call after primary surface is created!
	{
		...
	}
	if (!(g_ddcaps.dwCKeyCaps & DDCKEYCAPS_DESTOVERLAY))
	{
		...
	}
	*/



	//----------------------------------------------------
	// clipper
	//----------------------------------------------------

/*
    if (rendermode != 0)// && rendermode != 3)
    {
		dumpmsg("Init: creating clipper");

        // create the clipper
	    LPDIRECTDRAWCLIPPER pClipper;
        HRESULT hr;

	    hr = lpDD->CreateClipper(0, &pClipper, NULL);
	    if (FAILED (hr)) {
            dumpmsg("CreateClipper failed");
            finiObjects();
            return false;
	    }

	    // assign the clipper to the window
		hr = pClipper->SetHWnd(0, hSaverMainWindow);
	    if (FAILED (hr)) {
            dumpmsg("SetHWnd for Clipper failed");
    	    pClipper->Release();
            finiObjects();
            return false;
	    }
	    hr = lpDDSPrimary->SetClipper(pClipper);
	    if (FAILED (hr)) {
            dumpmsg("SetClipper failed");
    	    pClipper->Release();
            finiObjects();
		    return false;
	    }
	    pClipper->Release();
    }

*/

	//----------------------------------------------------
	// create back buffer (/overlay)
	//----------------------------------------------------

/*
	dumpmsg("Init: creating backbuffer");

	if (rendermode==0)
	{
		// Get a pointer to the back buffer
		ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
		dumpmsg("getattachedsurface()...");  
		ddrval = lpDDSPrimary->GetAttachedSurface(&ddscaps, 
						  &lpDDSBack);
		if( ddrval != DD_OK )
		{
			MessageBox( NULL, "lpDDSPrimary->GetAttachedSurface() failed.", "ERROR", MB_OK );
			dumpmsg(buf); 
			finiObjects();
			return FALSE;
		}
	}
	else if (rendermode==1 || rendermode==2)
	{
		// manually create a back buffer (an "off-screen surface")
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
		ddsd.dwSize = sizeof( ddsd );
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH; 
        // ---NOTE: vaio fails if dwCaps has LOCALVIDMEM or VIDEOMEMORY flags set.
        //    instead, leave it alone; if there is space it will put it in video memory automatically.
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;// | DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY; 
		ddsd.dwWidth  = FXW;     // changed in 1.3 (was FXW2)
		ddsd.dwHeight = FXH;     // changed in 1.3 (was FXH2)

		// needs to be in system memory or it might exceed the 
		// largest surface size that the video card can handle!
		//ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;

        // needs to be 32 bits so that in windowed mode,
        // Merge_All() has a 32-bit surface as the destination.
        // ...but then it'll often break if you try to blit from
        //    one bit depth to another!!!  (bad driver support)

        / *
		ddsd.dwFlags |= DDSD_PIXELFORMAT;
        ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
		ddsd.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
		ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FF;
		ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
		ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
        * /

		ddrval = lpDD->CreateSurface( &ddsd, &lpDDSBack, NULL );
		if( ddrval != DD_OK )
		{
			char buf[32];
			sprintf(buf, "error #%08x", ddrval);
			MessageBox( NULL, "failed to create backbuffer.", buf, MB_OK );
			dumpmsg(buf); 
			finiObjects();
			return FALSE;
		}
        
        ddsd.dwSize = sizeof(ddsd);
        if (lpDDSBack->GetSurfaceDesc(&ddsd) == DD_OK)
        {
			if ((ddsd.ddpfPixelFormat.dwRGBBitCount > 8) && 
				(ddsd.ddpfPixelFormat.dwRGBBitCount % 8 == 0) &&
				(ddsd.ddpfPixelFormat.dwRGBBitCount != iDispBits))
			{
	            iDispBits = ddsd.ddpfPixelFormat.dwRGBBitCount;

				sprintf(buf, "Init: windowed mode: changing iDispBits to match current Display Mode (%d bits).", iDispBits);
				dumpmsg(buf);
				
				if (iDispBits == 16) 
				{
					if (ddsd.ddpfPixelFormat.dwFlags & DDPF_RGB)
					{
						int gmask, gbits=0;
						gmask = ddsd.ddpfPixelFormat.dwGBitMask;
						while (gmask>0) { gbits += gmask & 1; gmask >>= 1; }
						VIDEO_CARD_555 = (gbits == 5);
					}
					else
					{
						dumpmsg("Init: windowed mode: WARNING: Windows bit depth is 16, but couldn't get the pixelformat!!!");
					}

					if (VIDEO_CARD_555) 
						dumpmsg("Init: 16-bit pixel format is 5-5-5");
					else
						dumpmsg("Init: 16-bit pixel format is 5-6-5");
				}
			}
        }
        else
        {
            dumpmsg("Init: windowed mode: WARNING: couldn't get the pixelformat!!!");
        }
	}
	else if (rendermode==3)// desktop-preservation mode; back buffer is an OVERLAY surface
	{
        if (!bOverlay)
        {
			MessageBox( NULL, "Either your video card does not support overlays or your current driver doesn't.", "ERROR", MB_OK );
			finiObjects();
			return FALSE;
        }

        if (!bDestKey)
        {
			MessageBox( NULL, "Either your video card does not support destination color keying, or your current driver doesn't support it.", "ERROR", MB_OK );
			finiObjects();
			return FALSE;
        }

        if (!Init_Overlay(true))
        {
            return FALSE;
        }
	}

*/
	//----------------------------------------------------
	// VS1 and VS2
	//----------------------------------------------------

//	dumpmsg("Init: creating offscreen surfaces");

	// create VS1 and VS2...
/*
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	ddsd.dwWidth = FXW2;
	ddsd.dwHeight = FXH2;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

	ddsd.dwFlags |= DDSD_PIXELFORMAT;
	ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
	ddsd.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
	ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FF;
	ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
	ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
	
	if (lpDD->CreateSurface( &ddsd, &lpDDSVS[0], NULL) != DD_OK)
	{
		MessageBox( NULL, "CreateSurface(VS1) failed.", "ERROR", MB_OK );
		dumpmsg(buf); 
		finiObjects();
		return FALSE;
	}

	if (lpDD->CreateSurface( &ddsd, &lpDDSVS[1], NULL) != DD_OK)
	{
		MessageBox( NULL, "CreateSurface(VS2) failed.", "ERROR", MB_OK );
		dumpmsg(buf); 
		finiObjects();
		return FALSE;
	}
*/

  g_pd3dDevice->CreateTexture(FXW2, FXH2, 1, D3DUSAGE_RENDERTARGET, D3DFMT_LIN_A8R8G8B8, 0, &lpD3DVS[0]);
  g_pd3dDevice->CreateTexture(FXW2, FXH2, 1, D3DUSAGE_RENDERTARGET, D3DFMT_LIN_A8R8G8B8, 0, &lpD3DVS[1]);

  IDirect3DSurface8* surface1, *surface2;
  D3DLOCKED_RECT lock1, lock2;
  lpD3DVS[0]->GetSurfaceLevel(0, &surface1);
  surface1->LockRect(&lock1, NULL, 0);
  lpD3DVS[1]->GetSurfaceLevel(0, &surface2);
  surface2->LockRect(&lock2, NULL, 0);

  memset(lock1.pBits, 0, FXW2 * FXH2 * 4);
  memset(lock2.pBits, 0, FXW2 * FXH2 * 4);
  surface2->UnlockRect();
  surface2->Release();
  surface1->UnlockRect();
  surface1->Release();

/*
	// Toggle Control Window
	dumpmsg("ToggleControlWindow();");
	ToggleControlWindow();
*/

	// READY TO GO!!!!!!
//    dumpmsg("Init: complete");
	return TRUE;  // all systems go.
}






void DrempelsExit()
{
  if (lpD3DVS[0] != NULL)
      lpD3DVS[0]->Release();

  if (lpD3DVS[1] != NULL)
      lpD3DVS[1]->Release();
}

/*
void finiObjects( void )
{
	dumpmsg("  finiObjects starts...");



/ *
    if (rendermode == 3)
    {
		RestoreWallpaper();
        Destroy_Overlay();
    }
* /


    if( lpDD != NULL )
    {
		if (g_bDisplayModeChanged)
		{
	        lpDD->RestoreDisplayMode();
		}
		if( lpDDSVS[0] != NULL )
		{
			lpDDSVS[0]->Release();
			lpDDSVS[0] = NULL;
		}
		if( lpDDSVS[1] != NULL )
		{
			lpDDSVS[1]->Release();
			lpDDSVS[1] = NULL;
		}
		if( lpDDSPrimary != NULL )
		{
			lpDDSPrimary->Release();
			lpDDSPrimary = NULL;
		}
        if (lpDD != NULL)
        {
		    lpDD->Release();
		    lpDD = NULL;
        }
    }

	FX_Fini();

	// remove icon from taskbar
	if (rendermode == 3 && g_bSystrayReady)
	{
		NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
		nid.hWnd = hSaverMainWindow;
		nid.uID = IDC_DREMPELS_SYSTRAY_ICON;
		Shell_NotifyIcon(NIM_DELETE, &nid);

		g_bSystrayReady = false;
	}	

	dumpmsg("  finiObjects completes.");
    

} / * finiObjects * /
*/

/*

void ToggleControlWindow()
{
	if (rendermode != 3) return;

	bool bShow = false;

	if (!::IsWindowVisible(hSaverMainWindow)) bShow = true;
	if (::IsIconic(hSaverMainWindow)) bShow = true;

	if (bShow)
	{
		if (g_bSuspended)
		{
			Suspend(false);
			return;
		}

		ShowWindow(hSaverMainWindow, SW_SHOW);
		if (::IsIconic(hSaverMainWindow))
		{
			ShowWindow(hSaverMainWindow, SW_RESTORE);
		}
		//SetFocus(hSaverMainWindow);
		//::SetActiveWindow(hSaverMainWindow);
		SetForegroundWindow(hSaverMainWindow);

    	SetWindowPos(hSaverMainWindow, NULL, -200, 0, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
	}
	else
	{
		ShowWindow(hSaverMainWindow, SW_HIDE);
	}
}

*/

/*

long FAR PASCAL WindowProc( HWND hWnd, UINT message, 
			    WPARAM wParam, LPARAM lParam )
{
    //PAINTSTRUCT ps;
    //RECT        rc;
    //SIZE        size;
    //BYTE phase = 0;
	HRESULT		hr;

    char buf[256];  // pwr mgmt
    sprintf(buf, "[MESSAGE] frame %d: msg=0x%x, wParam=0x%x", intframe, message, wParam);
    dumpmsg(buf);

    switch( message )
    {
		case WM_COMMAND:
			if (wParam == ID_DREMPELS_SYSTRAY_CLOSE)
			{
				dumpmsg("Closing from systray icon...");
				bUserQuit = TRUE;   // only used for plugin
				g_QuitASAP = true;
			}
			/ *
			else if (wParam == ID_DREMPELS_SYSTRAY_TOGGLE)
			{
				ToggleControlWindow();
			}
			* /
			else if (wParam == ID_DREMPELS_SYSTRAY_SHOW_CTRL_WND)
			{
				ShowWindow(hSaverMainWindow, SW_SHOW);
				if (::IsIconic(hSaverMainWindow))
				{
					ShowWindow(hSaverMainWindow, SW_RESTORE);
				}
				//SetFocus(hSaverMainWindow);
				//::SetActiveWindow(hSaverMainWindow);
				SetForegroundWindow(hSaverMainWindow);
			}
			else if (wParam == ID_DREMPELS_SYSTRAY_HIDE_CTRL_WND)
			{
				ShowWindow(hSaverMainWindow, SW_HIDE);
			}
			else if (wParam == ID_DREMPELS_SYSTRAY_RESUME)
			{
				Suspend(false);
			}
			else if (wParam == ID_DREMPELS_SYSTRAY_SUSPEND)
			{
				Suspend(true);
			}
			else if (wParam == ID_DREMPELS_SYSTRAY_HOTKEYS)
			{
				ToggleHelpPanel();
			}
			break;

		case WM_DREMPELS_SYSTRAY_MSG:
			if (wParam == IDC_DREMPELS_SYSTRAY_ICON)
			{
				bool bForcePopup = false;

				if (lParam == WM_LBUTTONDOWN)// || lParam == WM_LBUTTONUP)
				{
					if (g_bLostFocus)
						ToggleControlWindow();
					else
						bForcePopup = true;
				}
				
				if (lParam == WM_RBUTTONUP || bForcePopup)
				{
					//HMENU hMenu = LoadMenu(g_hSaverMainInstance, MAKEINTRESOURCE(IDM_DREMPELS_SYSTRAY_MENU));
					HMENU hMenu = ::CreatePopupMenu();
					POINT pt;
					GetCursorPos(&pt);
					::InsertMenu(hMenu, ID_DREMPELS_SYSTRAY_CLOSE,  MF_BYCOMMAND|MF_STRING, ID_DREMPELS_SYSTRAY_CLOSE,  "Close Drempels");

					/ *
					if (::IsWindowVisible(hSaverMainWindow) && ::GetFocus()==hSaverMainWindow && ::GetActiveWindow()==hSaverMainWindow)
					{
						// do nothing; focus goes to systray, which automatically closes the control window! =)
					}
					else
					{
					* /
						//::InsertMenu(hMenu, ID_DREMPELS_SYSTRAY_TOGGLE, MF_BYCOMMAND|MF_STRING, ID_DREMPELS_SYSTRAY_TOGGLE, "Show/Hide Control Window");
						//::InsertMenu(hMenu, ID_DREMPELS_SYSTRAY_HIDE_CTRL_WND, MF_BYCOMMAND|MF_STRING, ID_DREMPELS_SYSTRAY_HIDE_CTRL_WND, "Hide Control Window");
						if (g_bSuspended)
						{
							::InsertMenu(hMenu, ID_DREMPELS_SYSTRAY_RESUME, MF_BYCOMMAND|MF_STRING, ID_DREMPELS_SYSTRAY_RESUME, "Resume");
						}
						else
						{
							//::InsertMenu(hMenu, ID_DREMPELS_SYSTRAY_SHOW_CTRL_WND, MF_BYCOMMAND|MF_STRING, ID_DREMPELS_SYSTRAY_SHOW_CTRL_WND, "Show Control Window");
							::InsertMenu(hMenu, ID_DREMPELS_SYSTRAY_SUSPEND, MF_BYCOMMAND|MF_STRING, ID_DREMPELS_SYSTRAY_SUSPEND, "Suspend");
						}

						::InsertMenu(hMenu, ID_DREMPELS_SYSTRAY_HOTKEYS, MF_BYCOMMAND|MF_STRING, ID_DREMPELS_SYSTRAY_HOTKEYS, "Help w/Hotkeys");

						//g_bTrackingPopupMenu = true;
						::SetForegroundWindow(hSaverMainWindow);
						::TrackPopupMenu	(hMenu, TPM_RIGHTALIGN|TPM_BOTTOMALIGN|TPM_LEFTBUTTON, pt.x, pt.y, 0, hSaverMainWindow, NULL);
						//g_bTrackingPopupMenu = false;

					//}
				}
			}
			break;
        / *
        case WM_WINDOWPOSCHANGED:
        //case WM_MOVE:
            if (desktop_mode)
                SetWindowPos(hSaverMainWindow, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            break;
        * /

        case WM_ACTIVATEAPP:
            dumpmsg("WM_ACTIVATEAPP");
			break;

		case WM_ACTIVATE:
            dumpmsg("WM_ACTIVATE");

            if (rendermode==3 && intframe > 5)
            {
                // this only gets called when you switch to Drempels.
                // after the monitor shuts off (powersave), we need to call
                // Update_Overlay(), but we get NO message saying the monitor
                // ever comes back on, so we have to wait for the user to 
                // click on Drempels to do that.  
                if (lpDDSPrimary->IsLost())
                {
                    hr = lpDDSPrimary->Restore();
                    if (hr != DD_OK) 
                        dumpmsg("fatal: couldn't restore primary after overlay lost");
                }

                if (lpDDSOverlay->IsLost())
                {
                    lpDDSOverlay->Restore();
                    if (hr != DD_OK) 
                        dumpmsg("fatal: couldn't restore overlay after lost");
                }

                // this is the key part:
                if (!Update_Overlay())
                    dumpmsg("fatal: Update_Overlay failed after overlay lost");
            }

    		break;

        case WM_SIZE:
            dumpmsg("WM_SIZE");

            if (rendermode==0 || rendermode==1)
            {
                if (wParam == SIZE_MINIMIZED)
                    g_bPausedViaMinimize = true;
                else if (wParam == SIZE_RESTORED)
                    g_bPausedViaMinimize = false;
            }
            break;

		case WM_KILLFOCUS:
			dumpmsg("WM_KILLFOCUS");
			g_bLostFocus = true;

			if (rendermode==3 && g_bSystrayReady && !g_bTrackingPopupMenu)
			{
				ShowWindow(hSaverMainWindow, SW_HIDE);

				// update the icon in the taskbar
				NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
				nid.hWnd = hSaverMainWindow;
				nid.uID = IDC_DREMPELS_SYSTRAY_ICON;
				nid.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_SYSTRAY) );
				nid.uCallbackMessage = WM_DREMPELS_SYSTRAY_MSG;
				strcpy(nid.szTip, "Click here to control Drempels via Keyboard!");
				nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
				Shell_NotifyIcon(NIM_MODIFY, &nid);
			}
			break;

		case WM_SETFOCUS:
			dumpmsg("WM_SETFOCUS");
			g_bLostFocus = false;
			if (rendermode==3 && g_bSystrayReady)
			{
				// update the icon in the taskbar
				NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
				nid.hWnd = hSaverMainWindow;
				nid.uID = IDC_DREMPELS_SYSTRAY_ICON;
				nid.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_SYSTRAY_FOCUS) );
				nid.uCallbackMessage = WM_DREMPELS_SYSTRAY_MSG;
				strcpy(nid.szTip, "Drempels");
				nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
				Shell_NotifyIcon(NIM_MODIFY, &nid);
			}
			if (rendermode==0 && g_bAnimFrozen)
			{
				g_bForceRepaintOnce = true;
			}
			break;

		case WM_CLOSE:
			dumpmsg("WM_CLOSE");
			g_QuitASAP = true;
			break;

		case WM_DESTROY:
			dumpmsg("WM_DESTROY");
			g_QuitASAP = true;
			PostQuitMessage(0); 
			break;

        case WM_PAINT:
            dumpmsg("WM_PAINT");
            g_bForceRepaintOnce = true;
            break;

		case WM_SETCURSOR:      // called when mouse enters our client area
            dumpmsg("WM_SETCURSOR");
            if (!g_QuitASAP)
            {
                if (rendermode==0)
                    SetCursor(NULL);
			    else 
				    SetCursor(LoadCursor(NULL, IDC_ARROW));
				    //SetCursor(LoadCursor(g_hSaverMainInstance, MAKEINTRESOURCE(IDC_CURSOR1)));
            }
            break;

		case WM_POWERBROADCAST:
			dumpmsg("WM_POWERBROADCAST");
			break;
            
		case WM_LBUTTONDOWN: 
		case WM_MBUTTONDOWN: 
		case WM_RBUTTONDOWN: 
			if (rendermode==0)
			{
				dumpmsg("closing on WM_XBUTTONDOWN");
				bUserQuit = TRUE;  // only used for plugin
				//PostMessage(hWnd, WM_CLOSE, 0, 0);  // don't just quit... trigger password dialog
				g_QuitASAP = true;
			}
			/ *
			else
			{
				g_MouseDownX = gXC;
				g_MouseDownY = gYC;
			}
			* /
			break;

		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			/ *
			g_MouseUpX = gXC;
			g_MouseUpY = gYC;
			g_MouseDist = sqrtf((g_MouseUpY-g_MouseDownY)*(g_MouseUpY-g_MouseDownY) + (g_MouseUpX-g_MouseDownX)*(g_MouseUpX-g_MouseDownX));
			g_bMouseVector = true;
			* /
			break;

		case WM_MOUSEMOVE:
			//SHOW_MOUSECLICK_MSG = 30;
            if (rendermode==0 && !g_QuitASAP)// && !g_PassDialogReq)       // condition new in v3.54 ... to fix Pwr Mgmt bug? 
            {
                SetCursor(NULL);
            }
			
			if (intframe > 5)
			{
				if (rendermode==0)
                {
                    if (g_bExitOnMouseMove)
				    {
                        g_iMouseMovesThisFrame++;
				    }
                    else
                    {
                        SHOW_MOUSECLICK_MSG = 30;
                    }
                }

				int xPos;
				int yPos;
				xPos = LOWORD(lParam);  // horizontal position of cursor 
				yPos = HIWORD(lParam);  // vertical position of cursor 
				gXC = xPos;
				gYC = yPos;
				if (gXC < 0)    gXC = 0;
				if (gXC >= FXW) gXC = FXW-1;
				if (gYC < 0)    gYC = 0;
				if (gYC >= FXH) gYC = FXH-1;
			}
			
			break;

        case WM_SYSKEYDOWN:				// stops anim. on the ALT in ALT+TAB so they can see tab through their apps
			g_bUserAltTabbedOut = true;
			break;

        case WM_SYSKEYUP:				// resumes anim. if they just hit ALT and let go (never hit TAB)
			g_bUserAltTabbedOut = false;
			break;

		case WM_KEYUP:					// resumes anim. if TAB is released and it was on Drempels
			g_bUserAltTabbedOut = false;
			break;

        case WM_KEYDOWN:
			switch( wParam )
			{
			case VK_F5:
				if (!TEX.EnumTgaAndBmpFiles(szTexPathCurrent))
				{
					sprintf(szTEXLOADFAIL, " [ no images!!! ] ");
					SHOW_TEXLOADFAIL = 40;
				}
                // also update subdirs
                RefreshFileListings();
				break;
			
			case VK_F6:
				SHOW_TEXNAME = !SHOW_TEXNAME;
				break;
			}
			break;

        case WM_CHAR:
			switch( wParam )
			{
                case VK_TAB:
                    dumpmsg("TAB pressed!");
                    return 0;
                    break;

				case 'H':
				case 'h':
				case '?':
					if (rendermode != 3)
						SHOW_HELP_MSG = !SHOW_HELP_MSG;
					else
						ToggleHelpPanel();
					break;

                case 'r':
                case 'R':
					RandomizeStartValues();
					if (!g_bTexLocked) 
						g_bRandomizeTextures = true;
                    break;

				case 'b':
				case 'B':
					g_bToggleCoeffFreeze = true;
					break;

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    g_nSubdirIndex = -1;
                    if (wParam > '0')
                        for (int i=0; i<g_nSubdirs; i++)
                            if (g_subdirIndices[i] == wParam - '0')
                                g_nSubdirIndex = i;
                    if (g_nSubdirIndex == -1)
                        strcpy(szTexPathCurrent, szTexPath);
                    else
                        sprintf(szTexPathCurrent, "%s\\%d", szTexPath, g_subdirIndices[g_nSubdirIndex]);
				    if (!TEX.EnumTgaAndBmpFiles(szTexPathCurrent))
				    {
					    sprintf(szTEXLOADFAIL, " [ no images: %s ] ", szTexPathCurrent);
					    SHOW_TEXLOADFAIL = 30;
                        strcpy(szTexPathCurrent, szTexPath);
                        TEX.EnumTgaAndBmpFiles(szTexPathCurrent);
				    }
				    break;

                case 'A':
                    g_bAutoSwitchRandom = true;
                    g_bAutoSwitchSubdirs = !g_bAutoSwitchSubdirs;
                    break;
                case 'a':
                    g_bAutoSwitchRandom = false;
                    g_bAutoSwitchSubdirs = !g_bAutoSwitchSubdirs;
                    break;

                case 'm':
                case 'M':
                    g_bSuppressAllMsg = !g_bSuppressAllMsg;
					SHOW_TEXLOADFAIL = 0;
					SHOW_LOCKED = 0;
					SHOW_UNLOCKED = 0;
					SHOW_BEH_LOCKED = 0;
					SHOW_BEH_UNLOCKED = 0;
					SHOW_MOUSECLICK_MSG = 0;
                    break;

				case '@':
					g_bTakeScreenshot = true;
                    g_bForceRepaintOnce = true;
                    break;

                    //g_Capturing = !g_Capturing;
                    //break;
                ///AMDTEST
                ///#if AMDTEST
                ///case 'y':
                ///case 'Y':
                    ///procmode = (procmode+1) % 4;
                    ///intframe = 0;
                    ///core_clock_time = 0;
                    ///break;
                ///#endif
                ///AMDTEST

				case 's':
				case 'S':
					if (rendermode==3)
					{
						Suspend(!g_bSuspended);
					}
					break;

				case 'p':
				case 'P':
					g_bAnimFrozen = !g_bAnimFrozen;
                    break;

				case 'f':
				case 'F':
					SHOW_FPS = !SHOW_FPS;
                    //SHOW_TITLE = -1;
					break;

				case 't':
				case 'T':
					if (!g_bTexLocked)
					{
						SHOW_LOCKED = 20;
						SHOW_UNLOCKED = 0;
						SHOW_BEH_LOCKED = 0;
						SHOW_BEH_UNLOCKED = 0;
					}	
					else
					{
						SHOW_UNLOCKED = 20;
						SHOW_LOCKED = 0;
						SHOW_BEH_LOCKED = 0;
						SHOW_BEH_UNLOCKED = 0;
					}
					g_bTexLocked = !g_bTexLocked;
					break;

				/ *				
                case 'k':
                case 'K':
                    if (volpos < 20)
                    {
                        volpos++;
                        volscale *= 1.25;
                    }
                    break;

                case 'J':
                case 'j':
                    if (volpos > 0)
                    {
                        volpos--;
                        volscale *= 0.8;
                    }
                    break;
				* /
				case 'q':
				case 'Q':
					high_quality = !high_quality;
					break;

                case VK_PAUSE:
                    bWarpmodePaused = !bWarpmodePaused;
                    break;

				case '-':
					motion_blur--;
					if (motion_blur < 0) motion_blur = 0;
					if (motion_blur > 10) motion_blur = 10;
					break;

				case '+':
					motion_blur++;
					if (motion_blur < 0) motion_blur = 0;
					if (motion_blur > 10) motion_blur = 10;
					break;

				case 'j':
				case 'J':
					tex_scale--;
					if (tex_scale < 0) tex_scale = 0;
					if (tex_scale > 20) tex_scale = 20;
					break;

				case 'k':
				case 'K':
					tex_scale++;
					if (tex_scale < 0) tex_scale = 0;
					if (tex_scale > 20) tex_scale = 20;
					break;

				case 'i':
				case 'I':
					speed_scale--;
					if (speed_scale < 0) speed_scale = 0;
					if (speed_scale > 20) speed_scale = 20;
					break;

				case 'u':
				case 'U':
					speed_scale++;
					if (speed_scale < 0) speed_scale = 0;
					if (speed_scale > 20) speed_scale = 20;
					break;

				case 'n':
				case 'N':
					//if (rendermode==3)
					//	ShowWindow(hSaverMainWindow, SW_HIDE);
					//else
					//	ShowWindow(hSaverMainWindow, SW_MINIMIZE);
					if (rendermode != 3)
						ShowWindow(hSaverMainWindow, SW_MINIMIZE);
					break;

				case ' ':
					g_bRandomizeTextures = true;
					break;

				/ *
				case 'x':
				case 'X':
					if (rendermode == 3)
					{
						bUserQuit = TRUE;   
						g_QuitASAP = true;
					}
					break;
				* /

				case VK_ESCAPE:
					dumpmsg("WM_KEYDOWN/VK_ESCAPE");
					{
						bUserQuit = TRUE;   
						g_QuitASAP = true;
					}
					break;
            }
            break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);

} / * WindowProc * /

*/



/*



BOOL WINAPI RegisterDialogClasses(HANDLE hInst) 
{ 
    return TRUE; 
} 

*/




/*

void dumpmsg(char *s)
{
//  OutputDebugString(s);
//  OutputDebugString("\n");
/ *
    if (g_bDebugMode)
    {
        if (!g_bDumpFileCleared)
        {
            g_bDumpFileCleared = true;
	        FILE *infile = fopen("c:\\d_debug.txt", "w");
            if (infile)
            {
	            fprintf(infile, "[Drempels debug file]\n");
	            fclose(infile);
            }
        }
    
        FILE *infile2;
        infile2 = fopen("c:\\d_debug.txt", "a");
        if (infile2)
        {
	        fprintf(infile2, "%s\n", s);
            OutputDebugString(s);
            OutputDebugString("\n");
	        fclose(infile2);
        }
    }
* /
}
*/

/*
void RestoreWallpaper()
{
    if (bWallpaperSet)
    {
		// restore wallpaper
		if (bOldWallpaperKnown)
		{
			dumpmsg("restoring wallpaper");
			bool b = SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, szOldWallpaper, SPIF_SENDCHANGE);
			if (!b) dumpmsg("error resetting wallpaper!");
		}

		// restore desktop bkg color
		int nColorID = COLOR_BACKGROUND;
		::SetSysColors(1, &nColorID, &dwOldBackgroundColor);

		bWallpaperSet = false;
    }
}
*/
/*
void SetWallpaper()
{
	if (!bWallpaperSet)
	{
		// set wallpaper to HOT PINK!
		HKEY hKey;
		LONG res;
		bool b;
		unsigned long iLen;
		DWORD dwType = REG_SZ;

		res = RegOpenKeyEx(HKEY_CURRENT_USER, "Control Panel\\desktop", 0, KEY_WRITE|KEY_READ, &hKey);
		if (res == ERROR_SUCCESS)
		{
			// remember old wallpaper in szOldWallpaper
			iLen = 511;
			res = RegQueryValueEx(hKey, "Wallpaper", 0, &dwType, (unsigned char *)&szOldWallpaper[0], &iLen);
			if (res!=ERROR_SUCCESS) 
				dumpmsg("error getting old wallpaper string from registry!");
			else
				bOldWallpaperKnown = true;
		}
		else dumpmsg("error opening control panel registry key to backup wallpaper info!");

		char szKeyWallpaper[512];

		// set new desktop bkg color to colorkey
		dwOldBackgroundColor = ::GetSysColor(COLOR_BACKGROUND);
		int nColorID = COLOR_BACKGROUND;
		COLORREF colorkey = RGB(key_R, key_G, key_B);
		::SetSysColors(1, &nColorID, &colorkey);

		// set new wallpaper to ""
		dumpmsg("setting wallpaper");
		sprintf(szKeyWallpaper, "");
		b = SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, szKeyWallpaper, SPIF_SENDCHANGE);
		if (!b) dumpmsg("error setting wallpaper!");

		bWallpaperSet = true;
	}
}
*/
/*

void Suspend(bool bSuspend)
{
	if (bSuspend)
	{
		if (!g_bLostFocus)
			ToggleControlWindow();
		RestoreWallpaper();
		g_bSuspended = true;
		g_bAnimFrozen = true;

		/ *		
		RECT rect;
		ZeroMemory(&rect, sizeof(RECT));
		rect.right = 0;
		rect.bottom = 0;
		lpDDSOverlay->UpdateOverlay(&rect, lpDDSPrimary, &rect, 0, NULL);
		* /
	}
	else 
	{
		SetWallpaper();
		g_bSuspended = false;
		g_bAnimFrozen = false;

		if (g_bLostFocus)
			ToggleControlWindow();
		/ *
		Update_Overlay();
		* /
	}
}
*/


/*
void ToggleHelpPanel()
{
	if (g_hWndHotkeys == NULL)
	{
		g_hWndHotkeys = ::CreateDialog(g_hSaverMainInstance, MAKEINTRESOURCE(IDD_HOTKEYS), hSaverMainWindow, HotkeyDialogProc);
		RECT rect;
		GetWindowRect(g_hWndHotkeys, &rect);
		SetWindowPos(g_hWndHotkeys, 
			NULL, 
			FXW/2 - (rect.right-rect.left)/2, 
			FXH/2 - (rect.bottom-rect.top)/2,
			0, 0, SWP_NOZORDER | SWP_NOSIZE);
		ShowWindow(g_hWndHotkeys, SW_SHOW);
		SetFocus(g_hWndHotkeys);
	}
	else
	{
		PostMessage(g_hWndHotkeys, WM_COMMAND, IDOK, 0);
	}
}


*/
