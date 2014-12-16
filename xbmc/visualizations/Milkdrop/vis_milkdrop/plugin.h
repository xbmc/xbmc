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

#ifndef __NULLSOFT_DX8_EXAMPLE_PLUGIN_H__
#define __NULLSOFT_DX8_EXAMPLE_PLUGIN_H__ 1

#include "pluginshell.h"
#include "md_defines.h"
//#include "menu.h"
#include "support.h"
//#include "texmgr.h"
#include "state.h"

typedef enum { UI_REGULAR, UI_MENU, UI_LOAD, UI_LOAD_DEL, UI_LOAD_RENAME, UI_SAVEAS, UI_SAVE_OVERWRITE, UI_EDIT_MENU_STRING, UI_CHANGEDIR, UI_IMPORT_WAVE, UI_EXPORT_WAVE, UI_IMPORT_SHAPE, UI_EXPORT_SHAPE } ui_mode;
typedef struct { float rad; float ang; float a; float c;  } td_vertinfo; // blending: mix = max(0,min(1,a*t + c));
typedef char* CHARPTR;
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#define MY_FFT_SAMPLES 512     // for old [pre-vms] milkdrop sound analysis
typedef struct 
{
	float   imm[3];			// bass, mids, treble (absolute)
	float	imm_rel[3];		// bass, mids, treble (relative to song; 1=avg, 0.9~below, 1.1~above)
	float	avg[3];			// bass, mids, treble (absolute)
	float	avg_rel[3];		// bass, mids, treble (relative to song; 1=avg, 0.9~below, 1.1~above)
	float	long_avg[3];	// bass, mids, treble (absolute)
    float   fWave[2][576];
    float   fSpecLeft[MY_FFT_SAMPLES];
} td_mysounddata;

typedef struct
{
	int 	bActive;
	int 	bFilterBadChars;	// if true, it will filter out any characters that don't belong in a filename, plus the & symbol (because it doesn't display properly with DrawText)
	int 	bDisplayAsCode;		// if true, semicolons will be followed by a newline, for display
	int		nMaxLen;			// can't be more than 511
	int		nCursorPos;
	int		nSelAnchorPos;		// -1 if no selection made
	int 	bOvertypeMode;
	char	szText[8192];
	char	szPrompt[512];
	char	szToolTip[512];
	char	szClipboard[8192];
} td_waitstr;

typedef struct
{
	int 	bBold;
	int 	bItal;
	char	szFace[128];
	int		nColorR;    // 0..255
	int		nColorG;    // 0..255
	int		nColorB;    // 0..255
} 
td_custom_msg_font;

typedef struct
{
    char szFace[256];
    int nSize;
    int bBold;
    int bItalic;
} td_titlefontinfo;

typedef struct
{
	int		nFont;
	float	fSize;	// 0..100
	float	x;
	float	y;
	float	randx;
	float   randy;
	float	growth;
	float	fTime;	// total time to display the message, in seconds
	float	fFade;	// % (0..1) of the time that is spent fading in
	
	// overrides
	int     bOverrideBold;
	int     bOverrideItal;
	int     bOverrideFace;
	int     bOverrideColorR;
	int     bOverrideColorG;
	int     bOverrideColorB;
	int		nColorR;    // 0..255
	int		nColorG;    // 0..255
	int		nColorB;    // 0..255
	int  	nRandR;
	int     nRandG;
	int  	nRandB;
	int     bBold;
	int     bItal;
	char    szFace[128];

	char	szText[256];
} 
td_custom_msg;

typedef struct
{
	int 	bRedrawSuperText;	// true if it needs redraw
	int 	bIsSongTitle;		// false for custom message, true for song title
	char	szText[256];
	char	nFontFace[128];
	int 	bBold;
	int 	bItal;
	float	fX;
	float   fY;
	float	fFontSize;			// [0..100] for custom messages, [0..4] for song titles
	float   fGrowth;			// applies to custom messages only
	int		nFontSizeUsed;		// height IN PIXELS
	float	fStartTime;
	float	fDuration;
	float	fFadeTime;			// applies to custom messages only; song title fade times are handled specially
	int  	nColorR;
	int     nColorG;
	int  	nColorB;
}
td_supertext;

class CPlugin : public CPluginShell
{
public:
    
    //====[ 1. members added to create this specific example plugin: ]================================================

        /// CONFIG PANEL SETTINGS THAT WE'VE ADDED (TAB #2)
        bool		m_bFirstRun;
        float		m_fBlendTimeAuto;		// blend time when preset auto-switches
        float		m_fBlendTimeUser;		// blend time when user loads a new preset
        float		m_fTimeBetweenPresets;		// <- this is in addition to m_fBlendTimeAuto
        float		m_fTimeBetweenPresetsRand;	// <- this is in addition to m_fTimeBetweenPresets
        bool        m_bSequentialPresetOrder;
        bool        m_bHoldPreset;
        bool		m_bHardCutsDisabled;
        float		m_fHardCutLoudnessThresh;
        float		m_fHardCutHalflife;
        float		m_fHardCutThresh;
        //int			m_nWidth;
        //int			m_nHeight;
        //int			m_nDispBits;
        int			m_nTexSize;
        int			m_nGridX;
        int			m_nGridY;

        bool		m_bShowPressF1ForHelp;
        //char		m_szMonitorName[256];
        bool		m_bShowMenuToolTips;
        int			m_n16BitGamma;
        bool		m_bAutoGamma;
        //int		m_nFpsLimit;
        int			m_cLeftEye3DColor[3];
        int			m_cRightEye3DColor[3];
        bool		m_bEnableRating;
        bool        m_bInstaScan;
        bool		m_bSongTitleAnims;
        float		m_fSongTitleAnimDuration;
        float		m_fTimeBetweenRandomSongTitles;
        float		m_fTimeBetweenRandomCustomMsgs;
        int			m_nSongTitlesSpawned;
        int			m_nCustMsgsSpawned;

        bool		m_bAlways3D;
        float       m_fStereoSep;
        //bool		m_bAlwaysOnTop;
        //bool		m_bFixSlowText;
        bool		m_bWarningsDisabled;		// messageboxes
        bool		m_bWarningsDisabled2;		// warnings/errors in upper-right corner (m_szUserMessage)
        bool        m_bAnisotropicFiltering;
        bool        m_bPresetLockOnAtStartup;

        /*
        char		m_szFontFace[NUM_FONTS][128];
        int			m_nFontSize[NUM_FONTS];
        bool		m_bFontBold[NUM_FONTS];
        bool		m_bFontItalic[NUM_FONTS];
        char		 m_szTitleFontFace[128];
        int			 m_nTitleFontSize;			// percentage of screen width (0..100)
        bool		 m_bTitleFontBold;
        bool		 m_bTitleFontItalic;
        */
//        HFONT       m_gdi_title_font_doublesize;
//        LPD3DXFONT  m_d3dx_title_font_doublesize;

        // RUNTIME SETTINGS THAT WE'VE ADDED
        float       m_prev_time;
        bool		m_bTexSizeWasAuto;
        bool		m_bPresetLockedByUser;
        bool		m_bPresetLockedByCode;
        float		m_fAnimTime;
        float		m_fStartTime;
        float		m_fPresetStartTime;
        float		m_fNextPresetTime;
        CState		*m_pState;				// points to current CState
        CState		*m_pOldState;			// points to previous CState
        CState		m_state_DO_NOT_USE[2];	// do not use; use pState and pOldState instead.
        ui_mode		m_UI_mode;				// can be UI_REGULAR, UI_LOAD, UI_SAVEHOW, or UI_SAVEAS 
        //td_playlist_entry *m_szPlaylist;	// array of 128-char strings
        //int		m_nPlaylistCurPos;
        //int		m_nPlaylistLength;
        //int		m_nTrackPlaying;
        //int		m_nSongPosMS;
        //int		m_nSongLenMS;
        bool		m_bUserPagedUp;
        bool		m_bUserPagedDown;
        float		m_fMotionVectorsTempDx;
        float		m_fMotionVectorsTempDy;

        td_waitstr  m_waitstring;
        void		WaitString_NukeSelection();
        void		WaitString_Cut();
        void		WaitString_Copy();
        void		WaitString_Paste();
        void		WaitString_SeekLeftWord();
        void		WaitString_SeekRightWord();
        int			WaitString_GetCursorColumn();
        int			WaitString_GetLineLength();
        void		WaitString_SeekUpOneLine();
        void		WaitString_SeekDownOneLine();

        int			m_nPresets;			// the # of entries in the file listing.  Includes directories and then files, sorted alphabetically.
        int			m_nDirs;			// the # of presets that are actually directories.  Always between 0 and m_nPresets.
        int			m_nPresetListCurPos;// Index (in m_pPresetAddr[]) of the currently-HIGHLIGHTED preset (the user must press Enter on it to select it).
        int			m_nCurrentPreset;	// Index (in m_pPresetAddr[]) of the currently-RUNNING preset.  
								        //   Note that this is NOT the same as the currently-highlighted preset! (that's m_nPresetListCurPos)
								        //   Be careful - this can be -1 if the user changed dir. & a new preset hasn't been loaded yet.
        char		m_szCurrentPresetFile[512];	// this is always valid (unless no presets were found)
        CHARPTR		*m_pPresetAddr;		// slot n of this array is a pointer to the nth string in m_szpresets.  (optimization; allows for constant-time access.)  Each entry is just a filename + extension (with no path - the path is m_szPresetDir for all of them).
        float		*m_pfPresetRating;				// not-cumulative version
        char        *m_szpresets;		// 1 giant dynamically-allocated string; actually a string-of-strings, containing 'm_nPresets' null-terminated strings one after another; and the last string is followed with two NULL characters instead of just one.  
        int			m_nSizeOfPresetList; // the # of bytes currently allocated for 'm_szpresets'
        int         m_nRatingReadProgress;  // equals 'm_nPresets' if all ratings are read in & ready to go; -1 if uninitialized; otherwise, it's still reading them in, and range is: [0 .. m_nPresets-1]
        
        FFT            myfft;
        td_mysounddata mysound;
        
        // stuff for displaying text to user:
        //int			m_nTextHeightPixels;	// this is for the menu/detail font; NOT the "fancy font"
        //int			m_nTextHeightPixels_Fancy;
        bool		m_bShowFPS;
        bool		m_bShowRating;
        bool		m_bShowPresetInfo;
        bool		m_bShowDebugInfo;
        bool		m_bShowSongTitle;
        bool		m_bShowSongTime;
        bool		m_bShowSongLen;
        float		m_fShowUserMessageUntilThisTime;
        float		m_fShowRatingUntilThisTime;
        char		m_szUserMessage[512];
        char		m_szDebugMessage[512];
        char        m_szSongTitle    [512];
        char		m_szSongTitlePrev[512];
        //HFONT		m_hfont[3];	// 0=fancy font (for song titles, preset name)
						        // 1=legible font (the main font)
						        // 2=tooltip font (for tooltips in the menu system)
        //HFONT       m_htitlefont[NUM_TITLE_FONTS]; // ~25 different sizes
        // stuff for menu system:
/*        CMilkMenu	*m_pCurMenu;	// should always be valid!
        CMilkMenu	 m_menuPreset;
        CMilkMenu	  m_menuWave;
        CMilkMenu	  m_menuAugment;
        CMilkMenu	  m_menuCustomWave;
        CMilkMenu	  m_menuCustomShape;
        CMilkMenu	  m_menuMotion;
        CMilkMenu	  m_menuPost;
        CMilkMenu    m_menuWavecode[MAX_CUSTOM_WAVES];
        CMilkMenu    m_menuShapecode[MAX_CUSTOM_SHAPES];
*/
	char		m_szWinampPluginsPath[MAX_PATH];		// ends in a backslash
	char		m_szMsgIniFile[MAX_PATH];
	char        m_szImgIniFile[MAX_PATH];
	char		m_szPresetDir[MAX_PATH];
	float		m_fRandStart[4];

  // DIRECTX 8:
	IDirect3DTexture9 *m_lpVS[2];
  IDirect3DSurface9 *m_pZBuffer;
//	IDirect3DTexture8 *m_lpDDSTitle;    // CAREFUL: MIGHT BE NULL (if not enough mem)!
	int               m_nTitleTexSizeX, m_nTitleTexSizeY;
	SPRITEVERTEX      *m_verts;
	SPRITEVERTEX      *m_verts_temp;
	td_vertinfo       *m_vertinfo;
	WORD              *m_indices;

	bool		m_bMMX;
	//bool		m_bSSE;
    bool        m_bHasFocus;
    bool        m_bHadFocus;
	bool		m_bOrigScrollLockState;
    bool        m_bMilkdropScrollLockState;  // saved when focus is lost; restored when focus is regained

	int         m_nNumericInputMode;	// NUMERIC_INPUT_MODE_CUST_MSG, NUMERIC_INPUT_MODE_SPRITE
	int         m_nNumericInputNum;
	int			m_nNumericInputDigits;
	td_custom_msg_font   m_CustomMessageFont[MAX_CUSTOM_MESSAGE_FONTS];
	td_custom_msg        m_CustomMessage[MAX_CUSTOM_MESSAGES];

//	texmgr      m_texmgr;		// for user sprites

	td_supertext m_supertext;	// **contains info about current Song Title or Custom Message.**

    IDirect3DTexture9 *m_tracer_tex;

    //====[ 2. methods added: ]=====================================================================================
        
        void RefreshTab2(HWND hwnd);
        void RenderFrame(int bRedraw);
        void AlignWave(int nSamples);

        void        DrawTooltip(char* str, int xR, int yB);
        void        RandomizeBlendPattern();
         void        GenPlasma(int x0, int x1, int y0, int y1, float dt);
        void        LoadPerFrameEvallibVars(CState* pState);
        void        LoadCustomWavePerFrameEvallibVars(CState* pState, int i);
        void        LoadCustomShapePerFrameEvallibVars(CState* pState, int i);
    	void		WriteRealtimeConfig();	// called on Finish()
	    void		dumpmsg(char *s);
	    void		Randomize();
	    void		LoadRandomPreset(float fBlendTime);
	    void		LoadNextPreset(float fBlendTime);
	    void		LoadPreviousPreset(float fBlendTime);
	    void		LoadPreset(char *szPresetFilename, float fBlendTime);
	    void		UpdatePresetList();
	    void		UpdatePresetRatings();
	    //char*		GetConfigIniFile() { return m_szConfigIniFile; };
	    char*		GetMsgIniFile()    { return m_szMsgIniFile; };
	    char*		GetPresetDir()     { return m_szPresetDir; };
	    void		SavePresetAs(char *szNewFile);		// overwrites the file if it was already there.
	    void		DeletePresetFile(char *szDelFile);	
	    void		RenamePresetFile(char *szOldFile, char *szNewFile);
	    void		SetCurrentPresetRating(float fNewRating);
	    void		SeekToPreset(char cStartChar);
	    bool		ReversePropagatePoint(float fx, float fy, float *fx2, float *fy2);
	    int 		HandleRegularKey(WPARAM wParam);
	    bool		OnResizeGraphicsWindow();
	    bool		OnResizeTextWindow();
	    //bool		InitFont();
	    //void		ToggleControlWindow();	// for Desktop Mode only
	    //void		DrawUI();
	    void		ClearGraphicsWindow();	// for windowed mode only
      //bool    Update_Overlay();
	    //void		UpdatePlaylist();
	    void		LaunchCustomMessage(int nMsgNum);
	    void		ReadCustomMessages();
	    void		LaunchSongTitleAnim();

	    bool		RenderStringToTitleTexture();
	    void		ShowSongTitleAnim(/*IDirect3DTexture8* lpRenderTarget,*/ int w, int h, float fProgress);
	    void		DrawWave(float *fL, float *fR);
        void        DrawCustomWaves();
        void        DrawCustomShapes();
	    void		DrawSprites();
	    void		WarpedBlitFromVS0ToVS1();
	    void		RunPerFrameEquations();
	    void		ShowToUser(int bRedraw);
	    void		DrawUserSprites();
	    void		MergeSortPresets(int left, int right);
	    void		BuildMenus();
	    //void  ResetWindowSizeOnDisk();
	    bool		LaunchSprite(int nSpriteNum, int nSlot);
	    void		KillSprite(int iSlot);
        void        DoCustomSoundAnalysis();
        void        DrawMotionVectors();

    //====[ 3. virtual functions: ]===========================================================================

        virtual void OverrideDefaults();
        virtual void MyPreInitialize();
        virtual void MyReadConfig();
        virtual void MyWriteConfig();
        virtual int  AllocateMyNonDx8Stuff();
        virtual void  CleanUpMyNonDx8Stuff();
        virtual int  AllocateMyDX8Stuff();
        virtual void  CleanUpMyDX8Stuff(int final_cleanup);
        virtual void MyRenderFn(int redraw);
        virtual void MyRenderUI(int *upper_left_corner_y, int *upper_right_corner_y, int *lower_left_corner_y, int *lower_right_corner_y, int xL, int xR);
        virtual LRESULT MyWindowProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);
        virtual BOOL    MyConfigTabProc(int nPage, HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

    //====[ 4. methods from base class: ]===========================================================================
    /*
        // 'GET' METHODS
        // ------------------------------------------------------------
        int     GetFrame();            // returns current frame # (starts at zero)
        float   GetTime();             // returns current animation time (in seconds) (starts at zero) (updated once per frame)
        float   GetFps();              // returns current estimate of framerate (frames per second)
        eScrMode GetScreenMode();      // returns WINDOWED, FULLSCREEN, FAKE_FULLSCREEN, or NOT_YET_KNOWN (if called before or during OverrideDefaults()).
        HWND    GetWinampWindow();     // returns handle to Winamp main window
        HINSTANCE GetInstance();       // returns handle to the plugin DLL module; used for things like loading resources (dialogs, bitmaps, icons...) that are built into the plugin.
        char*   GetPluginsDirPath();   // usually returns 'c:\\program files\\winamp\\plugins\\'
        char*   GetConfigIniFile();    // usually returns 'c:\\program files\\winamp\\plugins\\something.ini' - filename is determined from identifiers in 'defines.h'

        // GET METHODS THAT ONLY WORK ONCE DIRECTX IS READY
        // ------------------------------------------------------------
        //  The following 'Get' methods are only available after DirectX has been initialized.
        //  If you call these from OverrideDefaults, MyPreInitialize, or MyReadConfig, 
        //    they will fail and return NULL (zero).
        // ------------------------------------------------------------
        HWND         GetPluginWindow();    // returns handle to the plugin window.  NOT persistent; can change.  
        int          GetWidth();           // returns width of plugin window interior, in pixels.
        int          GetHeight();          // returns height of plugin window interior, in pixels.
        D3DFORMAT    GetBackBufFormat();   // returns the pixelformat of the back buffer (probably D3DFMT_R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_R5G6B5, D3DFMT_X1R5G5B5, D3DFMT_A1R5G5B5, D3DFMT_A4R4G4B4, D3DFMT_R3G3B2, D3DFMT_A8R3G3B2, D3DFMT_X4R4G4B4, or D3DFMT_UNKNOWN)
        D3DFORMAT    GetBackBufZFormat();  // returns the pixelformat of the back buffer's Z buffer (probably D3DFMT_D16_LOCKABLE, D3DFMT_D32, D3DFMT_D15S1, D3DFMT_D24S8, D3DFMT_D16, D3DFMT_D24X8, D3DFMT_D24X4S4, or D3DFMT_UNKNOWN)
        D3DCAPS8*    GetCaps();            // returns a pointer to the D3DCAPS8 structer for the device.  NOT persistent; can change.
        LPDIRECT3DDEVICE8 GetDevice();     // returns a pointer to the DirectX 8 Device.  NOT persistent; can change.

        // FONTS & TEXT
        // ------------------------------------------------------------
        LPD3DXFONT   GetFont(eFontIndex idx);        // returns a handle to a D3DX font you can use to draw text on the screen
        int          GetFontHeight(eFontIndex idx);  // returns the height of the font, in pixels

        // MISC
        // ------------------------------------------------------------
        td_soundinfo m_sound;                   // a structure always containing the most recent sound analysis information; defined in pluginshell.h.
        void         SuggestHowToFreeSomeMem(); // gives the user a 'smart' messagebox that suggests how they can free up some video memory.
    */
    //=====================================================================================================================

};





#endif