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
#include "plugin.h"
//#include "resource.h"
#include "support.h"
#include "evallib\eval.h"		// for math. expr. eval - thanks Francis! (in SourceOffSite, it's the 'vis_avs\evallib' project.)
#include "evallib\compiler.h"
#include "utility.h"

#include <stdlib.h>
#include <stdio.h>
//#include <ddraw.h>
//#include <d3dcaps.h>
#include <assert.h>
#include <math.h>
//#include <shellapi.h>



#define D3DCOLOR_RGBA_01(r,g,b,a) D3DCOLOR_RGBA(((int)(r*255)),((int)(g*255)),((int)(b*255)),((int)(a*255)))
#define FRAND ((rand() % 7381)/7380.0f)
//#define D3D_OVERLOADS

#define VERT_CLIP 0.75f		// warning: top/bottom can get clipped if you go < 0.65!

//extern CPlugin* g_plugin;		// declared in main.cpp
//extern bool g_bDebugOutput;		// declared in support.cpp

int g_title_font_sizes[] =  
{ 
    // NOTE: DO NOT EXCEED 64 FONTS HERE.
	6,  8,  10, 12, 14, 16,      
	20, 26, 32, 38, 44, 50, 56,
	64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 
	160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 
	480, 512	/**/
};	

//#define COMPILE_MULTIMON_STUBS 1
//#include <multimon.h>

// This function evaluates whether the floating-point
// control Word is set to single precision/round to nearest/
// exceptions disabled. If not, the
// function changes the control Word to set them and returns
// TRUE, putting the old control Word value in the passback
// location pointed to by pwOldCW.
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
    if (pwOldCW) *pwOldCW = wSave;
    return ret;
}
 

void RestoreFPCW(WORD wSave)
{
    __asm fldcw wSave
}

int GetNumToSpawn(float fTime, float fDeltaT, float fRate, float fRegularity, int iNumSpawnedSoFar)
{
    // PARAMETERS
    // ------------
    // fTime:          sum of all fDeltaT's so far (excluding this one)
    // fDeltaT:        time window for this frame
    // fRate:          avg. rate (spawns per second) of generation
    // fRegularity:    regularity of generation 
	//					0.0: totally chaotic
	//					0.2: getting chaotic / very jittered
	//					0.4: nicely jittered
	//					0.6: slightly jittered
	//					0.8: almost perfectly regular
	//					1.0: perfectly regular
    // iNumSpawnedSoFar: the total number of spawnings so far
    //
    // RETURN VALUE
    // ------------
    // The number to spawn for this frame (add this to your net count!).
    //
	// COMMENTS
	// ------------
	// The spawn values returned will, over time, match
	// (within 1%) the theoretical totals expected based on the
	// amount of time passed and the average generation rate.
	//
	// UNRESOLVED ISSUES
	// -----------------
	// actual results of mixed gen. (0 < reg < 1) are about 1% too low
    // in the long run (vs. analytical expectations).  Decided not
	// to bother fixing it since it's only 1% (and VERY consistent).
    
    
    float fNumToSpawnReg;
    float fNumToSpawnIrreg;
    float fNumToSpawn;
    
    // compute # spawned based on regular generation
    fNumToSpawnReg = ((fTime + fDeltaT) * fRate) - iNumSpawnedSoFar;
    
    // compute # spawned based on irregular (random) generation
    if (fDeltaT <= 1.0f / fRate)
    {
        // case 1: avg. less than 1 spawn per frame
        if ((rand() % 16384)/16384.0f < fDeltaT * fRate)
            fNumToSpawnIrreg = 1.0f;
        else
            fNumToSpawnIrreg = 0.0f;
    }
    else
    {
        // case 2: avg. more than 1 spawn per frame
        fNumToSpawnIrreg = fDeltaT * fRate;
        fNumToSpawnIrreg *= 2.0f*(rand() % 16384)/16384.0f;
    }
    
    // get linear combo. of regular & irregular
    fNumToSpawn = fNumToSpawnReg*fRegularity + fNumToSpawnIrreg*(1.0f - fRegularity);

	// round to nearest integer for result
    return (int)(fNumToSpawn + 0.49f);
}


/*
char szHelp[] = 
{
	// note: this is a string-of-null-terminated-strings; the whole thing ends with a double null termination.
	// each substring will be drawn on its own line, and the strings are drawn in BOTTOM-UP order..
	// (this is just an effort to keep the file size low)
	"ESC: exit\0"

    " \0"
	"F9: toggle stereo 3D mode\0"
	"F8: change directory/drive\0"
	"F7: refresh milk_msg.ini\0"
	"F6: show preset rating\0"
	"F5: show fps\0"
	"F4: show preset name\0"
	"F2,F3: show song title,length\0"
	"F1: show help\0"

    " \0"
	"N: show per-frame variable moNitor\0"
	"S: save preset\0"
	"M: (preset editing) menu\0"

    " \0"
	"scroll lock: locks current preset\0"
	"+/-: rate current preset\0"
	"L: load specific preset\0"
	"R: toggle random(/sequential) preset order\0"
	"H: instant Hard cut (to next preset)\0"
	"spacebar: transition to next preset\0"

    " \0"
	"left/right arrows: seek 5 sec. [+SHIFT=seek 30]\0"
	"up/down arrows: adjust volume\0"
	"P: playlist\0"
	"U: toggle shuffle\0"
	"z/x/c/v/b: prev/play/pause/stop/next\0"

    " \0"
    "Y/K: enter custom message/sprite mode [see docs!]\0"
	"##: show custom message/sprite (##=00-99)\0"
	"T: launch song title animation\0"

    "\0\0"
};
*/


bool CPlugin::OnResizeTextWindow()
{
	/*
    if (!m_hTextWnd)
		return false;

	RECT rect;
	GetClientRect(m_hTextWnd, &rect);

	if (rect.right - rect.left != m_nTextWndWidth ||
		rect.bottom - rect.top != m_nTextWndHeight)
	{
		m_nTextWndWidth = rect.right - rect.left;
		m_nTextWndHeight = rect.bottom - rect.top;

		// first, resize fonts if necessary
		//if (!InitFont())
			//return false;

		// then resize the memory bitmap used for double buffering
		if (m_memDC)
		{
			SelectObject(m_memDC, m_oldBM);	// delete our doublebuffer
			DeleteObject(m_memDC);
			DeleteObject(m_memBM);	
			m_memDC = NULL;
			m_memBM = NULL;
			m_oldBM = NULL;
		}
		
		HDC hdc = GetDC(m_hTextWnd);
		if (!hdc) return false;

		m_memDC = CreateCompatibleDC(hdc);
		m_memBM = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
		m_oldBM = (HBITMAP)SelectObject(m_memDC,m_memBM);
		
		ReleaseDC(m_hTextWnd, hdc);

		// save new window pos
		WriteRealtimeConfig();
	}*/

	return true;
}


void CPlugin::ClearGraphicsWindow()
{
	// clear the window contents, to avoid a 1-pixel-thick border of noise that sometimes sticks around
    /*
	RECT rect;
	GetClientRect(GetPluginWindow(), &rect);

	HDC hdc = GetDC(GetPluginWindow());
	FillRect(hdc, &rect, m_hBlackBrush);
	ReleaseDC(GetPluginWindow(), hdc);
    */
}

/*
bool CPlugin::OnResizeGraphicsWindow()
{
    // NO LONGER NEEDED, SINCE PLUGIN SHELL CREATES A NEW DIRECTX
    // OBJECT WHENEVER WINDOW IS RESIZED.
}
*/

bool CPlugin::RenderStringToTitleTexture()	// m_szSongMessage
{
#if 0
    if (!m_lpDDSTitle)  // this *can* be NULL, if not much video mem!
        return false;

	if (m_supertext.szText[0]==0)
		return false;

    LPDIRECT3DDEVICE8 lpDevice = GetDevice();
    if (!lpDevice)
        return false;

	char szTextToDraw[512];
	sprintf(szTextToDraw, " %s ", m_supertext.szText);  //add a space @ end for italicized fonts; and at start, too, because it's centered!
	
    // Remember the original backbuffer and zbuffer
    LPDIRECT3DSURFACE8 pBackBuffer, pZBuffer;
    lpDevice->GetRenderTarget( &pBackBuffer );
//    lpDevice->GetDepthStencilSurface( &pZBuffer );

    // set render target to m_lpDDSTitle
    {
	    lpDevice->SetTexture(0, NULL);

        IDirect3DSurface8* pNewTarget = NULL;
        if (m_lpDDSTitle->GetSurfaceLevel(0, &pNewTarget) != D3D_OK) 
        {
            SafeRelease(pBackBuffer);
//            SafeRelease(pZBuffer);
            return false;
        }
        lpDevice->SetRenderTarget(pNewTarget, NULL);
        pNewTarget->Release();

	    lpDevice->SetTexture(0, NULL);
    }

	// clear the texture to black
    {
        lpDevice->SetVertexShader( WFVERTEX_FORMAT );
        lpDevice->SetTexture(0, NULL);

        lpDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

        // set up a quad
        WFVERTEX verts[4];
        for (int i=0; i<4; i++)
        {
            verts[i].x = (i%2==0) ? -1 : 1;
            verts[i].y = (i/2==0) ? -1 : 1;
            verts[i].z = 0;
            verts[i].Diffuse = 0xFF000000;
        }

        lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(WFVERTEX));
    }

    /*// 1. clip title if too many chars
	if (m_supertext.bIsSongTitle)
	{
		// truncate song title if too long; don't clip custom messages, though!
		int clip_chars = 32;
        int user_title_size = GetFontHeight(SONGTITLE_FONT);

        #define MIN_CHARS 8         // max clip_chars *for BIG FONTS*
        #define MAX_CHARS 64        // max clip chars *for tiny fonts*
        float t = (user_title_size-10)/(float)(128-10);
        t = min(1,max(0,t));
        clip_chars = (int)(MAX_CHARS - (MAX_CHARS-MIN_CHARS)*t);

		if ((int)strlen(szTextToDraw) > clip_chars+3)
			lstrcpy(&szTextToDraw[clip_chars], "...");
	}*/

    bool ret = true;

	// use 2 lines; must leave room for bottom of 'g' characters and such!
	RECT rect;
	rect.left   = 0;
	rect.right  = m_nTitleTexSizeX;
	rect.top    = m_nTitleTexSizeY* 1/21;  // otherwise, top of '%' could be cut off (1/21 seems safe)
	rect.bottom = m_nTitleTexSizeY*17/21;  // otherwise, bottom of 'g' could be cut off (18/21 seems safe, but we want some leeway)

    if (!m_supertext.bIsSongTitle)
    {
        // custom msg -> pick font to use that will best fill the texture

        HFONT gdi_font = NULL;
        LPD3DXFONT d3dx_font = NULL;

        int lo = 0;
	    int hi = sizeof(g_title_font_sizes)/sizeof(int) - 1;
    
        // limit the size of the font used:

        //int user_title_size = GetFontHeight(SONGTITLE_FONT);
        //while (g_title_font_sizes[hi] > user_title_size*2 && hi>4)
        //    hi--;

        RECT temp;
	    while (1)//(lo < hi-1)
	    {
		    int mid = (lo+hi)/2;

		    // create new gdi font at 'mid' size:
            gdi_font = CreateFont(g_title_font_sizes[mid], 0, 0, 0, m_supertext.bBold ? 900 : 400, m_supertext.bItal, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, m_fontinfo[SONGTITLE_FONT].bAntiAliased ? ANTIALIASED_QUALITY : DEFAULT_QUALITY, DEFAULT_PITCH, m_supertext.nFontFace);
            if (gdi_font) 
            {
                // create new d3dx font at 'mid' size:
                if (D3DXCreateFont(lpDevice, gdi_font, &d3dx_font) == D3D_OK)
                {
                    if (lo == hi-1)
                        break;      // DONE; but the 'lo'-size font is ready for use!

                    // compute size of text if drawn w/font of THIS size:
		            temp = rect;
		            int h = d3dx_font->DrawText(szTextToDraw, -1, &temp, DT_SINGLELINE | DT_CALCRECT | DT_NOPREFIX, 0xFFFFFFFF);

                    // adjust & prepare to reiterate:
		            if (temp.right >= rect.right || h > rect.bottom-rect.top)
			            hi = mid;
		            else
			            lo = mid;

                    SafeRelease(d3dx_font);
                }
        
                DeleteObject(gdi_font); gdi_font=NULL;
            }
	    }

        if (gdi_font && d3dx_font)
        {
	        // do actual drawing + set m_supertext.nFontSizeUsed; use 'lo' size
            int h = d3dx_font->DrawText(szTextToDraw, -1, &temp, DT_SINGLELINE | DT_CALCRECT | DT_NOPREFIX | DT_CENTER, 0xFFFFFFFF);
	        temp.left   = 0;
	        temp.right  = m_nTitleTexSizeX;  // now allow text to go all the way over, since we're actually drawing!
            temp.top    = m_nTitleTexSizeY/2 - h/2;
            temp.bottom = m_nTitleTexSizeY/2 + h/2;
	        m_supertext.nFontSizeUsed = d3dx_font->DrawText(szTextToDraw, -1, &temp, DT_SINGLELINE | DT_NOPREFIX | DT_CENTER, 0xFFFFFFFF);
	        
            ret = true;
        }
        else
        {
            ret = false;
        }

        // clean up font:
        SafeRelease(d3dx_font);
        if (gdi_font) DeleteObject(gdi_font); gdi_font=NULL;
    }
    else // song title
    {
        RECT temp = rect;

	    // do actual drawing + set m_supertext.nFontSizeUsed; use 'lo' size
        int h = m_d3dx_title_font_doublesize->DrawText(szTextToDraw, -1, &temp, DT_SINGLELINE | DT_CALCRECT | DT_NOPREFIX | DT_CENTER | DT_END_ELLIPSIS, 0xFFFFFFFF);
	    temp.left   = 0;
	    temp.right  = m_nTitleTexSizeX;  // now allow text to go all the way over, since we're actually drawing!
        temp.top    = m_nTitleTexSizeY/2 - h/2;
        temp.bottom = m_nTitleTexSizeY/2 + h/2;
	    m_supertext.nFontSizeUsed = m_d3dx_title_font_doublesize->DrawText(szTextToDraw, -1, &temp, DT_SINGLELINE | DT_NOPREFIX | DT_CENTER | DT_END_ELLIPSIS, 0xFFFFFFFF);
    }

    // Change the rendertarget back to the original setup
    lpDevice->SetTexture(0, NULL);
    lpDevice->SetRenderTarget( pBackBuffer, NULL );
    SafeRelease(pBackBuffer);
    SafeRelease(pZBuffer);

	return ret;
#endif
	return false;
}

void CPlugin::LoadPerFrameEvallibVars(CState* pState)
{
    // load the 'var_pf_*' variables in this CState object with the correct values.
    // for vars that affect pixel motion, that means evaluating them at time==-1,
    //    (i.e. no blending w/blendto value); the blending of the file dx/dy 
    //    will be done *after* execution of the per-vertex code.
    // for vars that do NOT affect pixel motion, evaluate them at the current time,
    //    so that if they're blending, both states see the blended value.

    // 1. vars that affect pixel motion: (eval at time==-1)
	*pState->var_pf_zoom		= (double)pState->m_fZoom.eval(-1);//GetTime()); 
	*pState->var_pf_zoomexp		= (double)pState->m_fZoomExponent.eval(-1);//GetTime());
	*pState->var_pf_rot			= (double)pState->m_fRot.eval(-1);//GetTime());  
	*pState->var_pf_warp		= (double)pState->m_fWarpAmount.eval(-1);//GetTime()); 
	*pState->var_pf_cx			= (double)pState->m_fRotCX.eval(-1);//GetTime());	
	*pState->var_pf_cy			= (double)pState->m_fRotCY.eval(-1);//GetTime()); 
	*pState->var_pf_dx			= (double)pState->m_fXPush.eval(-1);//GetTime());
	*pState->var_pf_dy			= (double)pState->m_fYPush.eval(-1);//GetTime());
	*pState->var_pf_sx			= (double)pState->m_fStretchX.eval(-1);//GetTime());
	*pState->var_pf_sy			= (double)pState->m_fStretchY.eval(-1);//GetTime());
	// read-only:
	*pState->var_pf_time		= (double)(GetTime() - m_fStartTime);
	*pState->var_pf_fps         = (double)GetFps();
	*pState->var_pf_bass		= (double)mysound.imm_rel[0];
	*pState->var_pf_mid			= (double)mysound.imm_rel[1];
	*pState->var_pf_treb		= (double)mysound.imm_rel[2];
	*pState->var_pf_bass_att	= (double)mysound.avg_rel[0];
	*pState->var_pf_mid_att		= (double)mysound.avg_rel[1];
	*pState->var_pf_treb_att	= (double)mysound.avg_rel[2];
	*pState->var_pf_frame		= (double)GetFrame();
	//*pState->var_pf_monitor     = 0;   -leave this as it was set in the per-frame INIT code!
	*pState->var_pf_q1          = pState->q_values_after_init_code[0];//0.0f;
	*pState->var_pf_q2          = pState->q_values_after_init_code[1];//0.0f;
	*pState->var_pf_q3          = pState->q_values_after_init_code[2];//0.0f;
	*pState->var_pf_q4          = pState->q_values_after_init_code[3];//0.0f;
	*pState->var_pf_q5          = pState->q_values_after_init_code[4];//0.0f;
	*pState->var_pf_q6          = pState->q_values_after_init_code[5];//0.0f;
	*pState->var_pf_q7          = pState->q_values_after_init_code[6];//0.0f;
	*pState->var_pf_q8          = pState->q_values_after_init_code[7];//0.0f;
    *pState->var_pf_monitor     = pState->monitor_after_init_code;
	*pState->var_pf_progress    = (GetTime() - m_fPresetStartTime) / (m_fNextPresetTime - m_fPresetStartTime);

	// 2. vars that do NOT affect pixel motion: (eval at time==now)
	*pState->var_pf_decay		= (double)pState->m_fDecay.eval(GetTime());
	*pState->var_pf_wave_a		= (double)pState->m_fWaveAlpha.eval(GetTime());
	*pState->var_pf_wave_r		= (double)pState->m_fWaveR.eval(GetTime());
	*pState->var_pf_wave_g		= (double)pState->m_fWaveG.eval(GetTime());
	*pState->var_pf_wave_b		= (double)pState->m_fWaveB.eval(GetTime());
	*pState->var_pf_wave_x		= (double)pState->m_fWaveX.eval(GetTime());
	*pState->var_pf_wave_y		= (double)pState->m_fWaveY.eval(GetTime());
	*pState->var_pf_wave_mystery= (double)pState->m_fWaveParam.eval(GetTime());
	*pState->var_pf_wave_mode   = (double)pState->m_nWaveMode;	//?!?! -why won't it work if set to pState->m_nWaveMode???
	*pState->var_pf_ob_size		= (double)pState->m_fOuterBorderSize.eval(GetTime());
	*pState->var_pf_ob_r		= (double)pState->m_fOuterBorderR.eval(GetTime());
	*pState->var_pf_ob_g		= (double)pState->m_fOuterBorderG.eval(GetTime());
	*pState->var_pf_ob_b		= (double)pState->m_fOuterBorderB.eval(GetTime());
	*pState->var_pf_ob_a		= (double)pState->m_fOuterBorderA.eval(GetTime());
	*pState->var_pf_ib_size		= (double)pState->m_fInnerBorderSize.eval(GetTime());
	*pState->var_pf_ib_r		= (double)pState->m_fInnerBorderR.eval(GetTime());
	*pState->var_pf_ib_g		= (double)pState->m_fInnerBorderG.eval(GetTime());
	*pState->var_pf_ib_b		= (double)pState->m_fInnerBorderB.eval(GetTime());
	*pState->var_pf_ib_a		= (double)pState->m_fInnerBorderA.eval(GetTime());
	*pState->var_pf_mv_x        = (double)pState->m_fMvX.eval(GetTime());
	*pState->var_pf_mv_y        = (double)pState->m_fMvY.eval(GetTime());
	*pState->var_pf_mv_dx       = (double)pState->m_fMvDX.eval(GetTime());
	*pState->var_pf_mv_dy       = (double)pState->m_fMvDY.eval(GetTime());
	*pState->var_pf_mv_l        = (double)pState->m_fMvL.eval(GetTime());
	*pState->var_pf_mv_r        = (double)pState->m_fMvR.eval(GetTime());
	*pState->var_pf_mv_g        = (double)pState->m_fMvG.eval(GetTime());
	*pState->var_pf_mv_b        = (double)pState->m_fMvB.eval(GetTime());
	*pState->var_pf_mv_a        = (double)pState->m_fMvA.eval(GetTime());
	*pState->var_pf_echo_zoom   = (double)pState->m_fVideoEchoZoom.eval(GetTime());
	*pState->var_pf_echo_alpha  = (double)pState->m_fVideoEchoAlpha.eval(GetTime());
	*pState->var_pf_echo_orient = (double)pState->m_nVideoEchoOrientation;
    // new in v1.04:
    *pState->var_pf_wave_usedots  = (double)pState->m_bWaveDots;
    *pState->var_pf_wave_thick    = (double)pState->m_bWaveThick;
    *pState->var_pf_wave_additive = (double)pState->m_bAdditiveWaves;
    *pState->var_pf_wave_brighten = (double)pState->m_bMaximizeWaveColor;
    *pState->var_pf_darken_center = (double)pState->m_bDarkenCenter;
    *pState->var_pf_gamma         = (double)pState->m_fGammaAdj.eval(GetTime());
    *pState->var_pf_wrap          = (double)pState->m_bTexWrap;
    *pState->var_pf_invert        = (double)pState->m_bInvert;
    *pState->var_pf_brighten      = (double)pState->m_bBrighten;
    *pState->var_pf_darken        = (double)pState->m_bDarken;
    *pState->var_pf_solarize      = (double)pState->m_bSolarize;
    *pState->var_pf_meshx         = (double)m_nGridX;
    *pState->var_pf_meshy         = (double)m_nGridY;
}

void CPlugin::RunPerFrameEquations()
{
	// run per-frame calculations

	int num_reps = (m_pState->m_bBlending) ? 2 : 1;
	for (int rep=0; rep<num_reps; rep++)
	{
		CState *pState;

		if (rep==0)
			pState = m_pState;
		else
			pState = m_pOldState;

		// values that will affect the pixel motion (and will be automatically blended 
		//	LATER, when the results of 2 sets of these params creates 2 different U/V
		//  meshes that get blended together.)
        LoadPerFrameEvallibVars(pState);

		// also do just a once-per-frame init for the *per-**VERTEX*** *READ-ONLY* variables
		// (the non-read-only ones will be reset/restored at the start of each vertex)
		*pState->var_pv_time		= *pState->var_pf_time;	
		*pState->var_pv_fps         = *pState->var_pf_fps;
		*pState->var_pv_frame		= *pState->var_pf_frame;
		*pState->var_pv_progress    = *pState->var_pf_progress;
		*pState->var_pv_bass		= *pState->var_pf_bass;	
		*pState->var_pv_mid			= *pState->var_pf_mid;		
		*pState->var_pv_treb		= *pState->var_pf_treb;	
		*pState->var_pv_bass_att	= *pState->var_pf_bass_att;
		*pState->var_pv_mid_att		= *pState->var_pf_mid_att;	
		*pState->var_pv_treb_att	= *pState->var_pf_treb_att;
        *pState->var_pv_meshx       = (double)m_nGridX;
        *pState->var_pv_meshy       = (double)m_nGridY;
        //*pState->var_pv_monitor     = *pState->var_pf_monitor;

		// execute once-per-frame expressions:
#ifndef _NO_EXPR_
		if (pState->m_pf_codehandle)
		{
			resetVars(pState->m_pf_vars);
			if (pState->m_pf_codehandle)
			{
				executeCode(pState->m_pf_codehandle);
			}
			resetVars(NULL);
		}
#endif

        // save some things for next frame:
        pState->monitor_after_init_code = *pState->var_pf_monitor;

        // save some things for per-vertex code:
        *pState->var_pv_q1		    = *pState->var_pf_q1;
		*pState->var_pv_q2		    = *pState->var_pf_q2;
		*pState->var_pv_q3		    = *pState->var_pf_q3;
		*pState->var_pv_q4		    = *pState->var_pf_q4;
		*pState->var_pv_q5		    = *pState->var_pf_q5;
		*pState->var_pv_q6		    = *pState->var_pf_q6;
		*pState->var_pv_q7		    = *pState->var_pf_q7;
		*pState->var_pv_q8		    = *pState->var_pf_q8;

        // (a few range checks:)
        *pState->var_pf_gamma     = max(0    , min(    8, *pState->var_pf_gamma    ));
        *pState->var_pf_echo_zoom = max(0.001, min( 1000, *pState->var_pf_echo_zoom));

		if (m_pState->m_bRedBlueStereo || m_bAlways3D)
		{
			// override wave colors
			*pState->var_pf_wave_r = 0.35f*(*pState->var_pf_wave_r) + 0.65f;
			*pState->var_pf_wave_g = 0.35f*(*pState->var_pf_wave_g) + 0.65f;
			*pState->var_pf_wave_b = 0.35f*(*pState->var_pf_wave_b) + 0.65f;
		}
	}

	if (m_pState->m_bBlending)
	{
		// For all variables that do NOT affect pixel motion, blend them NOW,
        // so later the user can just access m_pState->m_pf_whatever.  
		double mix  = (double)CosineInterp(m_pState->m_fBlendProgress);
		double mix2 = 1.0 - mix;
        *m_pState->var_pf_decay        = mix*(*m_pState->var_pf_decay       ) + mix2*(*m_pOldState->var_pf_decay       );
        *m_pState->var_pf_wave_a       = mix*(*m_pState->var_pf_wave_a      ) + mix2*(*m_pOldState->var_pf_wave_a      );
        *m_pState->var_pf_wave_r       = mix*(*m_pState->var_pf_wave_r      ) + mix2*(*m_pOldState->var_pf_wave_r      );
        *m_pState->var_pf_wave_g       = mix*(*m_pState->var_pf_wave_g      ) + mix2*(*m_pOldState->var_pf_wave_g      );
        *m_pState->var_pf_wave_b       = mix*(*m_pState->var_pf_wave_b      ) + mix2*(*m_pOldState->var_pf_wave_b      );
        *m_pState->var_pf_wave_x       = mix*(*m_pState->var_pf_wave_x      ) + mix2*(*m_pOldState->var_pf_wave_x      );
        *m_pState->var_pf_wave_y       = mix*(*m_pState->var_pf_wave_y      ) + mix2*(*m_pOldState->var_pf_wave_y      );
        *m_pState->var_pf_wave_mystery = mix*(*m_pState->var_pf_wave_mystery) + mix2*(*m_pOldState->var_pf_wave_mystery);
		// wave_mode: exempt (integer)
        *m_pState->var_pf_ob_size      = mix*(*m_pState->var_pf_ob_size     ) + mix2*(*m_pOldState->var_pf_ob_size     ); 
        *m_pState->var_pf_ob_r         = mix*(*m_pState->var_pf_ob_r        ) + mix2*(*m_pOldState->var_pf_ob_r        );
        *m_pState->var_pf_ob_g         = mix*(*m_pState->var_pf_ob_g        ) + mix2*(*m_pOldState->var_pf_ob_g        );
        *m_pState->var_pf_ob_b         = mix*(*m_pState->var_pf_ob_b        ) + mix2*(*m_pOldState->var_pf_ob_b        );
        *m_pState->var_pf_ob_a         = mix*(*m_pState->var_pf_ob_a        ) + mix2*(*m_pOldState->var_pf_ob_a        );
        *m_pState->var_pf_ib_size      = mix*(*m_pState->var_pf_ib_size     ) + mix2*(*m_pOldState->var_pf_ib_size     );
        *m_pState->var_pf_ib_r         = mix*(*m_pState->var_pf_ib_r        ) + mix2*(*m_pOldState->var_pf_ib_r        );
        *m_pState->var_pf_ib_g         = mix*(*m_pState->var_pf_ib_g        ) + mix2*(*m_pOldState->var_pf_ib_g        );
        *m_pState->var_pf_ib_b         = mix*(*m_pState->var_pf_ib_b        ) + mix2*(*m_pOldState->var_pf_ib_b        );
        *m_pState->var_pf_ib_a         = mix*(*m_pState->var_pf_ib_a        ) + mix2*(*m_pOldState->var_pf_ib_a        );
        *m_pState->var_pf_mv_x         = mix*(*m_pState->var_pf_mv_x        ) + mix2*(*m_pOldState->var_pf_mv_x        );
        *m_pState->var_pf_mv_y         = mix*(*m_pState->var_pf_mv_y        ) + mix2*(*m_pOldState->var_pf_mv_y        );
        *m_pState->var_pf_mv_dx        = mix*(*m_pState->var_pf_mv_dx       ) + mix2*(*m_pOldState->var_pf_mv_dx       );
        *m_pState->var_pf_mv_dy        = mix*(*m_pState->var_pf_mv_dy       ) + mix2*(*m_pOldState->var_pf_mv_dy       );
        *m_pState->var_pf_mv_l         = mix*(*m_pState->var_pf_mv_l        ) + mix2*(*m_pOldState->var_pf_mv_l        );
        *m_pState->var_pf_mv_r         = mix*(*m_pState->var_pf_mv_r        ) + mix2*(*m_pOldState->var_pf_mv_r        );
        *m_pState->var_pf_mv_g         = mix*(*m_pState->var_pf_mv_g        ) + mix2*(*m_pOldState->var_pf_mv_g        );
        *m_pState->var_pf_mv_b         = mix*(*m_pState->var_pf_mv_b        ) + mix2*(*m_pOldState->var_pf_mv_b        );
        *m_pState->var_pf_mv_a         = mix*(*m_pState->var_pf_mv_a        ) + mix2*(*m_pOldState->var_pf_mv_a        );
		*m_pState->var_pf_echo_zoom    = mix*(*m_pState->var_pf_echo_zoom   ) + mix2*(*m_pOldState->var_pf_echo_zoom   );
		*m_pState->var_pf_echo_alpha   = mix*(*m_pState->var_pf_echo_alpha  ) + mix2*(*m_pOldState->var_pf_echo_alpha  );
        *m_pState->var_pf_echo_orient  = (mix < 0.5f) ? *m_pOldState->var_pf_echo_orient : *m_pState->var_pf_echo_orient;
        // added in v1.04:
        *m_pState->var_pf_wave_usedots = (mix < 0.5f) ? *m_pOldState->var_pf_wave_usedots  : *m_pState->var_pf_wave_usedots ;
        *m_pState->var_pf_wave_thick   = (mix < 0.5f) ? *m_pOldState->var_pf_wave_thick    : *m_pState->var_pf_wave_thick   ;
        *m_pState->var_pf_wave_additive= (mix < 0.5f) ? *m_pOldState->var_pf_wave_additive : *m_pState->var_pf_wave_additive;
        *m_pState->var_pf_wave_brighten= (mix < 0.5f) ? *m_pOldState->var_pf_wave_brighten : *m_pState->var_pf_wave_brighten;
        *m_pState->var_pf_darken_center= (mix < 0.5f) ? *m_pOldState->var_pf_darken_center : *m_pState->var_pf_darken_center;
        *m_pState->var_pf_gamma        = mix*(*m_pState->var_pf_gamma       ) + mix2*(*m_pOldState->var_pf_gamma       );
        *m_pState->var_pf_wrap         = (mix < 0.5f) ? *m_pOldState->var_pf_wrap          : *m_pState->var_pf_wrap         ;
        *m_pState->var_pf_invert       = (mix < 0.5f) ? *m_pOldState->var_pf_invert        : *m_pState->var_pf_invert       ;
        *m_pState->var_pf_brighten     = (mix < 0.5f) ? *m_pOldState->var_pf_brighten      : *m_pState->var_pf_brighten     ;
        *m_pState->var_pf_darken       = (mix < 0.5f) ? *m_pOldState->var_pf_darken        : *m_pState->var_pf_darken       ;
        *m_pState->var_pf_solarize     = (mix < 0.5f) ? *m_pOldState->var_pf_solarize      : *m_pState->var_pf_solarize     ;
    }
}

void CPlugin::RenderFrame(int bRedraw)
{
	int i;

    float fDeltaT = 1.0f/GetFps();

	// update time
    /*
    float fDeltaT = (GetFrame()==0) ? 1.0f/30.0f : GetTime() - m_prev_time;
	DWORD dwTime		= GetTickCount();
	float fDeltaT		= (dwTime - m_dwPrevTickCount)*0.001f;
	if (GetFrame() > 64)
	{
		fDeltaT = (fDeltaT)*0.2f + 0.8f*(1.0f/m_fps);
		if (fDeltaT > 2.0f/m_fps)
		{
			char buf[64];
			sprintf(buf, "fixing time gap of %5.3f seconds", fDeltaT);
			dumpmsg(buf);

			fDeltaT = 1.0f/m_fps;
		}
	}
	m_dwPrevTickCount	= dwTime;
	GetTime()			+= fDeltaT;
    */

	if (GetFrame()==0) 
	{
		m_fStartTime = GetTime();
		m_fPresetStartTime = GetTime();
	}

	if (m_fNextPresetTime < 0)
	{
		float dt = m_fTimeBetweenPresetsRand * (rand()%1000)*0.001f;
		m_fNextPresetTime = GetTime() + m_fBlendTimeAuto + m_fTimeBetweenPresets + dt;
	}

	/*
    if (m_bPresetLockedByUser || m_bPresetLockedByCode)
	{
		// if the user has the preset LOCKED, or if they're in the middle of 
		// saving it, then keep extending the time at which the auto-switch will occur
		// (by the length of this frame).

		m_fPresetStartTime += fDeltaT;
		m_fNextPresetTime += fDeltaT;
	}*/

	// update fps
	/*
	if (GetFrame() < 4)
	{
		m_fps = 0.0f;
	}
	else if (GetFrame() <= 64)
	{
		m_fps = GetFrame() / (float)(GetTime() - m_fTimeHistory[0]);
	}
	else
	{
		m_fps = 64.0f / (float)(GetTime() - m_fTimeHistory[m_nTimeHistoryPos]);
	}
	m_fTimeHistory[m_nTimeHistoryPos] = GetTime();
	m_nTimeHistoryPos = (m_nTimeHistoryPos + 1) % 64;
    */

	// limit fps, if necessary
    /*
	if (m_nFpsLimit > 0 && (GetFrame() % 64) == 0 && GetFrame() > 64)
	{
		float spf_now     = 1.0f / m_fps;
		float spf_desired = 1.0f / (float)m_nFpsLimit;

		float new_sleep = m_fFPSLimitSleep + (spf_desired - spf_now)*1000.0f;
		
		if (GetFrame() <= 128)
			m_fFPSLimitSleep = new_sleep;
		else
			m_fFPSLimitSleep = m_fFPSLimitSleep*0.8f + 0.2f*new_sleep;
		
		if (m_fFPSLimitSleep < 0) m_fFPSLimitSleep = 0;
		if (m_fFPSLimitSleep > 100) m_fFPSLimitSleep = 100;

		//sprintf(m_szUserMessage, "sleep=%f", m_fFPSLimitSleep);
		//m_fShowUserMessageUntilThisTime = GetTime() + 3.0f;
	}

	static float deficit;
	if (GetFrame()==0) deficit = 0;
	float ideal_sleep = (m_fFPSLimitSleep + deficit);
	int actual_sleep = (int)ideal_sleep;
	if (actual_sleep > 0)
		Sleep(actual_sleep);
	deficit = ideal_sleep - actual_sleep; 
	if (deficit < 0) deficit = 0;	// just in case
	if (deficit > 1) deficit = 1;	// just in case
    */

	// randomly change the preset, if it's time
	if (m_fNextPresetTime < GetTime())
	{
		LoadRandomPreset(m_fBlendTimeAuto);
	}
/*
	// randomly spawn Song Title, if time
	if (m_fTimeBetweenRandomSongTitles > 0 && 
		!m_supertext.bRedrawSuperText &&
		GetTime() >= m_supertext.fStartTime + m_supertext.fDuration + 1.0f/GetFps())
	{		
		int n = GetNumToSpawn(GetTime(), fDeltaT, 1.0f/m_fTimeBetweenRandomSongTitles, 0.5f, m_nSongTitlesSpawned);
		if (n > 0)
		{
			LaunchSongTitleAnim();
			m_nSongTitlesSpawned += n;
		}
	}

	// randomly spawn Custom Message, if time
	if (m_fTimeBetweenRandomCustomMsgs > 0 && 
		!m_supertext.bRedrawSuperText &&
		GetTime() >= m_supertext.fStartTime + m_supertext.fDuration + 1.0f/GetFps())
	{		
		int n = GetNumToSpawn(GetTime(), fDeltaT, 1.0f/m_fTimeBetweenRandomCustomMsgs, 0.5f, m_nCustMsgsSpawned);
		if (n > 0)
		{
			LaunchCustomMessage(-1);
			m_nCustMsgsSpawned += n;
		}
	}
*/
	// update m_fBlendProgress;
	if (m_pState->m_bBlending)
	{
		m_pState->m_fBlendProgress = (GetTime() - m_pState->m_fBlendStartTime) / m_pState->m_fBlendDuration;
		if (m_pState->m_fBlendProgress > 1.0f)
		{
			m_pState->m_bBlending = false;
		}
	}

	// handle hard cuts here (just after new sound analysis)
	static float m_fHardCutThresh;
	if (GetFrame() == 0)
		m_fHardCutThresh = m_fHardCutLoudnessThresh*2.0f;
	if (GetFps() > 1.0f && !m_bHardCutsDisabled && !m_bPresetLockedByUser && !m_bPresetLockedByCode)
	{
		if (mysound.imm_rel[0] + mysound.imm_rel[1] + mysound.imm_rel[2] > m_fHardCutThresh*3.0f)
		{
			LoadRandomPreset(0.0f);
			m_fHardCutThresh *= 2.0f;
		}
		else
		{
			float halflife_modified = m_fHardCutHalflife*0.5f;
			//thresh = (thresh - 1.5f)*0.99f + 1.5f;		
			float k = -0.69315f / halflife_modified;
			float single_frame_multiplier = powf(2.7183f, k / GetFps());
			m_fHardCutThresh = (m_fHardCutThresh - m_fHardCutLoudnessThresh)*single_frame_multiplier + m_fHardCutLoudnessThresh;
		}
	}

	// smooth & scale the audio data, according to m_state, for display purposes
	float scale = m_pState->m_fWaveScale.eval(GetTime()) / 128.0f;
	mysound.fWave[0][0] *= scale;
	mysound.fWave[1][0] *= scale;
	float mix2 = m_pState->m_fWaveSmoothing.eval(GetTime());
	float mix1 = scale*(1.0f - mix2);
	for (i=1; i<576; i++)
	{
		mysound.fWave[0][i] = mysound.fWave[0][i]*mix1 + mysound.fWave[0][i-1]*mix2;
		mysound.fWave[1][i] = mysound.fWave[1][i]*mix1 + mysound.fWave[1][i-1]*mix2;
	}

	RunPerFrameEquations();

	// restore any lost surfaces
	//m_lpDD->RestoreAllSurfaces();

    LPDIRECT3DDEVICE9 lpDevice = GetDevice();
    if (!lpDevice)
        return;

    // Remember the original backbuffer and zbuffer
    LPDIRECT3DSURFACE9 pBackBuffer, pZBuffer;
    lpDevice->GetRenderTarget(0, &pBackBuffer );
    lpDevice->GetDepthStencilSurface( &pZBuffer );

	D3DSURFACE_DESC	desc; 
	pBackBuffer->GetDesc(&desc);

	m_backBufferWidth = desc.Width;
	m_backBufferHeight = desc.Height;

    // set up render state
    {
        DWORD texaddr = (*m_pState->var_pf_wrap) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
        lpDevice->SetRenderState(D3DRS_WRAP0, 0);
        lpDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, texaddr);
        lpDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, texaddr);
        lpDevice->SetSamplerState(0, D3DSAMP_ADDRESSW, texaddr);
        
        lpDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
	      lpDevice->SetRenderState( D3DRS_SPECULARENABLE, FALSE );
        lpDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
        lpDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
        lpDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
        lpDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
        lpDevice->SetRenderState( D3DRS_COLORVERTEX, TRUE );
        lpDevice->SetRenderState( D3DRS_FILLMODE,  D3DFILL_SOLID );
        lpDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	    lpDevice->SetRenderState( D3DRS_AMBIENT, 0xFFFFFFFF );  //?
//        lpDevice->SetRenderState( D3DRS_CLIPPING, TRUE );

        // set min/mag/mip filtering modes; use anisotropy if available.
        if (m_bAnisotropicFiltering && (GetCaps()->TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC))
	        lpDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
//        else if (GetCaps()->TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR)
		else
	        lpDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
//        else
//            SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);

        if (m_bAnisotropicFiltering && (GetCaps()->TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC))
	        lpDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
		else
//        else if (GetCaps()->TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR)
	        lpDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  //      else
//	        SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);

        lpDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

        // note: this texture stage state setup works for 0 or 1 texture.
        // if you set a texture, it will be modulated with the current diffuse color.
        // if you don't set a texture, it will just use the current diffuse color.
        lpDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	      lpDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
	      lpDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
        lpDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	      lpDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
        lpDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
        lpDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	    if (GetCaps()->RasterCaps & D3DPRASTERCAPS_DITHER)
	    	lpDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);    
	    /* WISO: if (GetCaps()->RasterCaps & D3DPRASTERCAPS_ANTIALIASEDGES)
		    lpDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);*/

        // NOTE: don't forget to call SetTexture and SetVertexShader before drawing!
        // Examples:
        //      SPRITEVERTEX verts[4];          // has texcoords
        //   	lpDevice->SetTexture(0, m_sprite_tex);
        //      lpDevice->SetVertexShader( SPRITEVERTEX_FORMAT );
        //      
        //      WFVERTEX verts[4];              // no texcoords
        //   	lpDevice->SetTexture(0, NULL);
        //      lpDevice->SetVertexShader( WFVERTEX_FORMAT );
    }

    // render string to m_lpDDSTitle, if necessary
	if (m_supertext.bRedrawSuperText)
	{
		if (!RenderStringToTitleTexture())
            m_supertext.fStartTime = -1.0f;
	    m_supertext.bRedrawSuperText = false;
	}

    // set up to render [from NULL] to VS0 (for motion vectors).
    {
	    lpDevice->SetTexture(0, NULL);

        IDirect3DSurface9* pNewTarget = NULL;
        if (m_lpVS[0]->GetSurfaceLevel(0, &pNewTarget) != D3D_OK) 
            return;
        lpDevice->SetRenderTarget(0, pNewTarget);
        pNewTarget->Release();
		lpDevice->SetDepthStencilSurface( NULL );

	    lpDevice->SetTexture(0, NULL);
    }

    // draw motion vectors to VS0
    DrawMotionVectors();

    // set up to render [from VS0] to VS1.
    {
	    lpDevice->SetTexture(0, NULL);

        IDirect3DSurface9* pNewTarget = NULL;
        if (m_lpVS[1]->GetSurfaceLevel(0, &pNewTarget) != D3D_OK) 
            return;
        lpDevice->SetRenderTarget(0, pNewTarget);
		lpDevice->SetDepthStencilSurface( NULL );
        pNewTarget->Release();
    }

	// do the warping for this frame
	if (GetCaps()->RasterCaps & D3DPRASTERCAPS_DITHER)
	    lpDevice->SetRenderState(D3DRS_DITHERENABLE, TRUE);    
	/* WISO: if (GetCaps()->RasterCaps & D3DPRASTERCAPS_ANTIALIASEDGES)
		lpDevice->SetRenderState(D3DRS_EDGEANTIALIAS, FALSE);*/
	WarpedBlitFromVS0ToVS1();
	if (GetCaps()->RasterCaps & D3DPRASTERCAPS_DITHER)
	    lpDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);    
	/*if (GetCaps()->RasterCaps & D3DPRASTERCAPS_ANTIALIASEDGES)
		lpDevice->SetRenderState(D3DRS_EDGEANTIALIAS, TRUE);*/

	// draw audio data
    DrawCustomShapes(); // draw these first; better for feedback if the waves draw *over* them.
	DrawCustomWaves();
	DrawWave(mysound.fWave[0], mysound.fWave[1]);
	DrawSprites();

	// if song title animation just ended, render it into the VS:
	if (m_supertext.fStartTime >= 0 &&
		GetTime() >= m_supertext.fStartTime + m_supertext.fDuration &&
		!m_supertext.bRedrawSuperText)
	{
		m_supertext.fStartTime = -1.0f;	// 'off' state
		ShowSongTitleAnim(m_nTexSize, m_nTexSize, 1.0f);
	}

  // Change the rendertarget back to the original setup
  lpDevice->SetTexture(0, NULL);
  // WISO: lpDevice->SetRenderTarget( pBackBuffer, pZBuffer );
  lpDevice->SetRenderTarget(0, pBackBuffer);
  lpDevice->SetDepthStencilSurface( pZBuffer );

  SafeRelease(pBackBuffer);
  SafeRelease(pZBuffer);

	/* WISO: if (GetCaps()->RasterCaps & D3DPRASTERCAPS_ANTIALIASEDGES)
		lpDevice->SetRenderState(D3DRS_EDGEANTIALIAS, FALSE);*/

	// show it to user
	ShowToUser(bRedraw);


	// finally, render song title animation to back buffer
	if (m_supertext.fStartTime >= 0 &&
		GetTime() < m_supertext.fStartTime + m_supertext.fDuration &&
		!m_supertext.bRedrawSuperText)
	{
		float fProgress = (GetTime() - m_supertext.fStartTime) / m_supertext.fDuration;
		ShowSongTitleAnim(GetWidth(), GetHeight(), fProgress);
	}

	DrawUserSprites();

	// flip buffers
	IDirect3DTexture9* pTemp = m_lpVS[0];
	m_lpVS[0] = m_lpVS[1];
	m_lpVS[1] = pTemp;

	/* WISO: if (GetCaps()->RasterCaps & D3DPRASTERCAPS_ANTIALIASEDGES)
		lpDevice->SetRenderState(D3DRS_EDGEANTIALIAS, FALSE);*/
	if (GetCaps()->RasterCaps & D3DPRASTERCAPS_DITHER)
	    lpDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);    
}


void CPlugin::DrawMotionVectors()
{
	// FLEXIBLE MOTION VECTOR FIELD
	if ((float)*m_pState->var_pf_mv_a >= 0.001f)
	{
        //-------------------------------------------------------
        LPDIRECT3DDEVICE9 lpDevice = GetDevice();
        if (!lpDevice)
            return;

        lpDevice->SetTexture(0, NULL);
        lpDevice->SetFVF(WFVERTEX_FORMAT);
        //-------------------------------------------------------

		int x,y;

		int nX = (int)(*m_pState->var_pf_mv_x);// + 0.999f);
		int nY = (int)(*m_pState->var_pf_mv_y);// + 0.999f);
		float dx = (float)*m_pState->var_pf_mv_x - nX;
		float dy = (float)*m_pState->var_pf_mv_y - nY;
		if (nX > 64) { nX = 64; dx = 0; }
		if (nY > 48) { nY = 48; dy = 0; }
		
		if (nX > 0 && nY > 0)
		{
			/*
			float dx2 = m_fMotionVectorsTempDx;//(*m_pState->var_pf_mv_dx) * 0.05f*GetTime();		// 0..1 range
			float dy2 = m_fMotionVectorsTempDy;//(*m_pState->var_pf_mv_dy) * 0.05f*GetTime();		// 0..1 range
			if (GetFps() > 2.0f && GetFps() < 300.0f)
			{
				dx2 += (float)(*m_pState->var_pf_mv_dx) * 0.05f / GetFps();
				dy2 += (float)(*m_pState->var_pf_mv_dy) * 0.05f / GetFps();
			}
			if (dx2 > 1.0f) dx2 -= (int)dx2;
			if (dy2 > 1.0f) dy2 -= (int)dy2;
			if (dx2 < 0.0f) dx2 = 1.0f - (-dx2 - (int)(-dx2));
			if (dy2 < 0.0f) dy2 = 1.0f - (-dy2 - (int)(-dy2));
			// hack: when there is only 1 motion vector on the screem, to keep it in
			//       the center, we gradually migrate it toward 0.5.
			dx2 = dx2*0.995f + 0.5f*0.005f;	
			dy2 = dy2*0.995f + 0.5f*0.005f;
			// safety catch
			if (dx2 < 0 || dx2 > 1 || dy2 < 0 || dy2 > 1)
			{
				dx2 = 0.5f;
				dy2 = 0.5f;
			}
			m_fMotionVectorsTempDx = dx2;
			m_fMotionVectorsTempDy = dy2;*/
			float dx2 = (float)(*m_pState->var_pf_mv_dx);
			float dy2 = (float)(*m_pState->var_pf_mv_dy);

			float len_mult = (float)*m_pState->var_pf_mv_l;
			if (dx < 0) dx = 0;
			if (dy < 0) dy = 0;
			if (dx > 1) dx = 1;
			if (dy > 1) dy = 1;
			//dx = dx * 1.0f/(float)nX;
			//dy = dy * 1.0f/(float)nY;
			float inv_texsize = 1.0f/(float)m_nTexSize;
			float min_len = 1.0f*inv_texsize;


			WFVERTEX v[(64+1)*2];
			ZeroMemory(v, sizeof(WFVERTEX)*(64+1)*2);
			v[0].Diffuse = D3DCOLOR_RGBA_01((float)*m_pState->var_pf_mv_r,(float)*m_pState->var_pf_mv_g,(float)*m_pState->var_pf_mv_b,(float)*m_pState->var_pf_mv_a);
			for (x=1; x<(nX+1)*2; x++)
				v[x].Diffuse = v[0].Diffuse;

			lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
			lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

			for (y=0; y<nY; y++)
			{
				float fy = (y + 0.25f)/(float)(nY + dy + 0.25f - 1.0f);

				// now move by offset
				fy -= dy2;

				if (fy > 0.0001f && fy < 0.9999f)
				{
					int n = 0;
					for (x=0; x<nX; x++)
					{
						//float fx = (x + 0.25f)/(float)(nX + dx + 0.25f - 1.0f);
						float fx = (x + 0.25f)/(float)(nX + dx + 0.25f - 1.0f);

						// now move by offset
						fx += dx2;

						if (fx > 0.0001f && fx < 0.9999f)
						{
							float fx2, fy2;
							ReversePropagatePoint(fx, fy, &fx2, &fy2);	// NOTE: THIS IS REALLY A REVERSE-PROPAGATION
							//fx2 = fx*2 - fx2;
							//fy2 = fy*2 - fy2;
							//fx2 = fx + 1.0f/(float)m_nTexSize;
							//fy2 = 1-(fy + 1.0f/(float)m_nTexSize);

							// enforce minimum trail lengths:
							{	
								float dx = (fx2 - fx);
								float dy = (fy2 - fy);
								dx *= len_mult;
								dy *= len_mult;
								float len = sqrtf(dx*dx + dy*dy);

								if (len > min_len)
								{

								}
								else if (len > 0.00000001f)
								{
									len = min_len/len;
									dx *= len;
									dy *= len;
								}
								else
								{
									dx = min_len;
									dy = min_len;
								}
									
								fx2 = fx + dx;
								fy2 = fy + dy;
							}
							/**/

							v[n].x = fx * 2.0f - 1.0f;
							v[n].y = fy * 2.0f - 1.0f;
							v[n+1].x = fx2 * 2.0f - 1.0f;
							v[n+1].y = fy2 * 2.0f - 1.0f; 

							// actually, project it in the reverse direction
							//v[n+1].x = v[n].x*2.0f - v[n+1].x;// + dx*2; 
							//v[n+1].y = v[n].y*2.0f - v[n+1].y;// + dy*2; 
							//v[n].x += dx*2;
							//v[n].y += dy*2;

							n += 2;
						}
					}

					// draw it
					if (n != 0)
						lpDevice->DrawPrimitiveUP(D3DPT_LINELIST, n/2, v, sizeof(WFVERTEX));
				}
			}

			lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		}
	}
}

/*
void CPlugin::UpdateSongInfo()
{
	if (m_bShowSongTitle || m_bSongTitleAnims)
	{
		char szOldSongMessage[512];
		strcpy(szOldSongMessage, m_szSongMessage);

		if (::GetWindowText(m_hWndParent, m_szSongMessage, sizeof(m_szSongMessage)))
		{
			// remove ' - Winamp' at end
			if (strlen(m_szSongMessage) > 9)
			{
				int check_pos = strlen(m_szSongMessage) - 9;
				if (strcmp(" - Winamp", (char *)(m_szSongMessage + check_pos)) == 0)
					m_szSongMessage[check_pos] = 0;
			}

			// remove ' - Winamp [Paused]' at end
			if (strlen(m_szSongMessage) > 18)
			{
				int check_pos = strlen(m_szSongMessage) - 18;
				if (strcmp(" - Winamp [Paused]", (char *)(m_szSongMessage + check_pos)) == 0)
					m_szSongMessage[check_pos] = 0;
			}

			// remove song # and period from beginning
			char *p = m_szSongMessage;
			while (*p >= '0' && *p <= '9') p++;
			if (*p == '.' && *(p+1) == ' ')
			{
				p += 2;
				int pos = 0;
				while (*p != 0)
				{
					m_szSongMessage[pos++] = *p;
					p++;
				}
				m_szSongMessage[pos++] = 0;
			}

			// fix &'s for display
			/*
			{
				int pos = 0;
				int len = strlen(m_szSongMessage);
				while (m_szSongMessage[pos])
				{
					if (m_szSongMessage[pos] == '&')
					{
						for (int x=len; x>=pos; x--)
							m_szSongMessage[x+1] = m_szSongMessage[x];
						len++;
						pos++;
					}
					pos++;
				}
			}*/
/*
			if (m_bSongTitleAnims && 
				((strcmp(szOldSongMessage, m_szSongMessage) != 0) || (GetFrame()==0)))
			{
				// launch song title animation
				LaunchSongTitleAnim();

				/*
				m_supertext.bRedrawSuperText = true;
				m_supertext.bIsSongTitle = true;
				strcpy(m_supertext.szText, m_szSongMessage);
				strcpy(m_supertext.nFontFace, m_szTitleFontFace);
				m_supertext.fFontSize   = (float)m_nTitleFontSize;
				m_supertext.bBold       = m_bTitleFontBold;
				m_supertext.bItal       = m_bTitleFontItalic;
				m_supertext.fX          = 0.5f;
				m_supertext.fY          = 0.5f;
				m_supertext.fGrowth     = 1.0f;
				m_supertext.fDuration   = m_fSongTitleAnimDuration;
				m_supertext.nColorR     = 255;
				m_supertext.nColorG     = 255;
				m_supertext.nColorB     = 255;

				m_supertext.fStartTime  = GetTime(); 
				*/
/*			}
		}
		else
		{
			sprintf(m_szSongMessage, "<couldn't get song title>");
		}
	}

	m_nTrackPlaying = SendMessage(m_hWndParent,WM_USER, 0, 125);

	// append song time
	if (m_bShowSongTime && m_nSongPosMS >= 0)
	{
		float time_s = m_nSongPosMS*0.001f;
		
		int minutes = (int)(time_s/60);
		time_s -= minutes*60;
		int seconds = (int)time_s;
		time_s -= seconds;
		int dsec = (int)(time_s*100);

		sprintf(m_szSongTime, "%d:%02d.%02d", minutes, seconds, dsec);
	}

	// append song length
	if (m_bShowSongLen && m_nSongLenMS > 0)
	{
		int len_s = m_nSongLenMS/1000;
		int minutes = len_s/60;
		int seconds = len_s - minutes*60;

		char buf[512];
		sprintf(buf, " / %d:%02d", minutes, seconds);
		strcat(m_szSongTime, buf);
	}
}
*/

bool CPlugin::ReversePropagatePoint(float fx, float fy, float *fx2, float *fy2)
{
	//float fy = y/(float)nMotionVectorsY;
	int   y0 = (int)(fy*m_nGridY);
	float dy = fy*m_nGridY - y0;

	//float fx = x/(float)nMotionVectorsX;
	int   x0 = (int)(fx*m_nGridX);
	float dx = fx*m_nGridX - x0;

	int x1 = x0 + 1;
	int y1 = y0 + 1;

	if (x0 < 0) return false;
	if (y0 < 0) return false;
	//if (x1 < 0) return false;
	//if (y1 < 0) return false;
	//if (x0 > m_nGridX) return false;
	//if (y0 > m_nGridY) return false;
	if (x1 > m_nGridX) return false;
	if (y1 > m_nGridY) return false;

	float tu, tv;
	tu  = m_verts[y0*(m_nGridX+1)+x0].tu * (1-dx)*(1-dy);
	tv  = m_verts[y0*(m_nGridX+1)+x0].tv * (1-dx)*(1-dy);
	tu += m_verts[y0*(m_nGridX+1)+x1].tu * (dx)*(1-dy);
	tv += m_verts[y0*(m_nGridX+1)+x1].tv * (dx)*(1-dy);
	tu += m_verts[y1*(m_nGridX+1)+x0].tu * (1-dx)*(dy);
	tv += m_verts[y1*(m_nGridX+1)+x0].tv * (1-dx)*(dy);
	tu += m_verts[y1*(m_nGridX+1)+x1].tu * (dx)*(dy);
	tv += m_verts[y1*(m_nGridX+1)+x1].tv * (dx)*(dy);

	*fx2 = tu;
	*fy2 = 1.0f - tv;
	return true;

}


void CPlugin::WarpedBlitFromVS0ToVS1()
{
	MungeFPCW(NULL);	// puts us in single-precision mode & disables exceptions

    LPDIRECT3DDEVICE9 lpDevice = GetDevice();
    if (!lpDevice)
        return;

	lpDevice->SetTexture(0, m_lpVS[0]);
  lpDevice->SetFVF( SPRITEVERTEX_FORMAT );

	// warp stuff
	float fWarpTime = GetTime() * m_pState->m_fWarpAnimSpeed;
  float fWarpScale = m_pState->m_fWarpScale.eval(GetTime());
  float fWarpScaleInv = 1.0f;
  if(fWarpScale != 0.0f)
    fWarpScaleInv = 1.0f / fWarpScale;
	float f[4];
	f[0] = 11.68f + 4.0f*cosf(fWarpTime*1.413f + 10);
	f[1] =  8.77f + 3.0f*cosf(fWarpTime*1.113f + 7);
	f[2] = 10.54f + 3.0f*cosf(fWarpTime*1.233f + 3);
	f[3] = 11.49f + 4.0f*cosf(fWarpTime*0.933f + 5);

	int num_reps = (m_pState->m_bBlending) ? 2 : 1;
	float fCosineBlend  = 0.50f*(m_pState->m_fBlendProgress) + 0.50f*CosineInterp(m_pState->m_fBlendProgress);
	float fCosineBlend2 = 0.40f*(m_pState->m_fBlendProgress) + 0.60f*CosineInterp(m_pState->m_fBlendProgress);

	// decay
	float fDecay = (float)(*m_pState->var_pf_decay);

	// texel alignment
	float texel_offset = 0.5f / (float)m_nTexSize;

	//if (m_pState->m_bBlending)
	//	fDecay = fDecay*(fCosineBlend) + (1.0f-fCosineBlend)*((float)(*m_pOldState->var_pf_decay));

    if (m_bAutoGamma && GetFrame()==0)
	{
		if (strstr(GetDriverDescription(), "nvidia") ||
			strstr(GetDriverDescription(), "nVidia") ||
			strstr(GetDriverDescription(), "NVidia") ||
			strstr(GetDriverDescription(), "NVIDIA"))
			m_n16BitGamma = 2;
		else if (strstr(GetDriverDescription(), "ATI RAGE MOBILITY M"))
	        m_n16BitGamma = 2;
		else 
            m_n16BitGamma = 0;
	}

	if (m_n16BitGamma > 0 && 
        (GetBackBufFormat()==D3DFMT_R5G6B5 || GetBackBufFormat()==D3DFMT_X1R5G5B5 || GetBackBufFormat()==D3DFMT_A1R5G5B5 || GetBackBufFormat()==D3DFMT_A4R4G4B4) && 
        fDecay < 0.9999f)
    {
		fDecay = min(fDecay, (32.0f - m_n16BitGamma)/32.0f);
    }

	D3DCOLOR cDecay = D3DCOLOR_RGBA_01(fDecay,fDecay,fDecay,1);


	for (int rep=0; rep<num_reps; rep++)
	{
		CState *pState;

		if (rep==0)
			pState = m_pState;
		else
			pState = m_pOldState;

		resetVars(pState->m_pv_vars);

		// cache the doubles as floats so that computations are a bit faster
		float fZoom		= (float)(*pState->var_pf_zoom);
		float fZoomExp	= (float)(*pState->var_pf_zoomexp);
		float fRot		= (float)(*pState->var_pf_rot);
		float fWarp		= (float)(*pState->var_pf_warp);
		float fCX		= (float)(*pState->var_pf_cx);
		float fCY		= (float)(*pState->var_pf_cy);
		float fDX		= (float)(*pState->var_pf_dx);
		float fDY		= (float)(*pState->var_pf_dy);
		float fSX		= (float)(*pState->var_pf_sx);
		float fSY		= (float)(*pState->var_pf_sy);
		
		int n = 0;

		for (int y=0; y<=m_nGridY; y++)
		{
			for (int x=0; x<=m_nGridX; x++)
			{
				// Note: x, y, z are now set at init. time - no need to mess with them!
				//m_verts[n].x = i/(float)m_nGridX*2.0f - 1.0f;
				//m_verts[n].y = j/(float)m_nGridY*2.0f - 1.0f;
				//m_verts[n].z = 0.0f;
				
				if (pState->m_pp_codehandle)
				{
					// restore all the variables to their original states,
					//  run the user-defined equations,
					//  then move the results into local vars for computation as floats

					*pState->var_pv_x		= (double)(m_verts[n].x*0.5f + 0.5f);
					*pState->var_pv_y		= (double)(m_verts[n].y*-0.5f + 0.5f);
					*pState->var_pv_rad		= (double)m_vertinfo[n].rad;
					*pState->var_pv_ang		= (double)m_vertinfo[n].ang;
					*pState->var_pv_zoom	= *pState->var_pf_zoom;
					*pState->var_pv_zoomexp	= *pState->var_pf_zoomexp;
					*pState->var_pv_rot		= *pState->var_pf_rot;
					*pState->var_pv_warp	= *pState->var_pf_warp;
					*pState->var_pv_cx		= *pState->var_pf_cx;
					*pState->var_pv_cy		= *pState->var_pf_cy;
					*pState->var_pv_dx		= *pState->var_pf_dx;
					*pState->var_pv_dy		= *pState->var_pf_dy;
					*pState->var_pv_sx		= *pState->var_pf_sx;
					*pState->var_pv_sy		= *pState->var_pf_sy;
					/*
                    *pState->var_pv_q1		= *pState->var_pf_q1;
					*pState->var_pv_q2		= *pState->var_pf_q2;
					*pState->var_pv_q3		= *pState->var_pf_q3;
					*pState->var_pv_q4		= *pState->var_pf_q4;
					*pState->var_pv_q5		= *pState->var_pf_q5;
					*pState->var_pv_q6		= *pState->var_pf_q6;
					*pState->var_pv_q7		= *pState->var_pf_q7;
					*pState->var_pv_q8		= *pState->var_pf_q8;
                    */
					//*pState->var_pv_time		= *pState->var_pv_time;		// (these are all now initialized 
					//*pState->var_pv_bass		= *pState->var_pv_bass;		//  just once per frame)
					//*pState->var_pv_mid		= *pState->var_pv_mid;		
					//*pState->var_pv_treb		= *pState->var_pv_treb;	
					//*pState->var_pv_bass_att	= *pState->var_pv_bass_att;
					//*pState->var_pv_mid_att	= *pState->var_pv_mid_att;	
					//*pState->var_pv_treb_att	= *pState->var_pv_treb_att;

#ifndef _NO_EXPR_
					executeCode(pState->m_pp_codehandle);
#endif

					fZoom = (float)(*pState->var_pv_zoom);
					fZoomExp = (float)(*pState->var_pv_zoomexp);
					fRot  = (float)(*pState->var_pv_rot);
					fWarp = (float)(*pState->var_pv_warp);
					fCX   = (float)(*pState->var_pv_cx);
					fCY   = (float)(*pState->var_pv_cy);
					fDX   = (float)(*pState->var_pv_dx);
					fDY   = (float)(*pState->var_pv_dy);
					fSX   = (float)(*pState->var_pv_sx);
					fSY   = (float)(*pState->var_pv_sy);
				}

				float fZoom2 = powf(fZoom, powf(fZoomExp, m_vertinfo[n].rad*2.0f - 1.0f));

				// initial texcoords, w/built-in zoom factor
				float fZoom2Inv = 1.0f/fZoom2;
				float u = m_verts[n].x*0.5f*fZoom2Inv + 0.5f;
				float v = -m_verts[n].y*0.5f*fZoom2Inv + 0.5f;

				// stretch on X, Y:
				u = (u - fCX)/fSX + fCX;
				v = (v - fCY)/fSY + fCY;

				// warping:
				//if (fWarp > 0.001f || fWarp < -0.001f)
				//{
					u += fWarp*0.0035f*sinf(fWarpTime*0.333f + fWarpScaleInv*(m_verts[n].x*f[0] - m_verts[n].y*f[3]));
					v += fWarp*0.0035f*cosf(fWarpTime*0.375f - fWarpScaleInv*(m_verts[n].x*f[2] + m_verts[n].y*f[1]));
					u += fWarp*0.0035f*cosf(fWarpTime*0.753f - fWarpScaleInv*(m_verts[n].x*f[1] - m_verts[n].y*f[2]));
					v += fWarp*0.0035f*sinf(fWarpTime*0.825f + fWarpScaleInv*(m_verts[n].x*f[0] + m_verts[n].y*f[3]));
				//}

				// rotation:
				float u2 = u - fCX;
				float v2 = v - fCY;
				
				float cos_rot = cosf(fRot);
				float sin_rot = sinf(fRot);
				u = u2*cos_rot - v2*sin_rot + fCX;
				v = u2*sin_rot + v2*cos_rot + fCY;

				// translation:
				u -= fDX - texel_offset;
				v -= fDY - texel_offset;
				
				if (rep==0)
				{
					//m_verts[n].Diffuse = cDecay;		// see below
					m_verts[n].tu = u;
					m_verts[n].tv = v;
				}
				else
				{
                    float mix2 = m_vertinfo[n].a*m_pState->m_fBlendProgress + m_vertinfo[n].c;//fCosineBlend2;
                    mix2 = max(0,min(1,mix2));
					m_verts[n].tu = m_verts[n].tu*(mix2) + u*(1-mix2);
					m_verts[n].tv = m_verts[n].tv*(mix2) + v*(1-mix2);
				}

                /*
                if (rep==num_reps-1)
                {
                    m_verts[n].tu += 0.25f*(FRAND*2-1)/(float)m_nTexSize;
                    m_verts[n].tv += 0.25f*(FRAND*2-1)/(float)m_nTexSize;
                }
                */

				n++;
			}
		}

        /*
	    pState->q_values_after_init_code[0] = *pState->var_pv_q1;
	    pState->q_values_after_init_code[1] = *pState->var_pv_q2;
	    pState->q_values_after_init_code[2] = *pState->var_pv_q3;
	    pState->q_values_after_init_code[3] = *pState->var_pv_q4;
	    pState->q_values_after_init_code[4] = *pState->var_pv_q5;
	    pState->q_values_after_init_code[5] = *pState->var_pv_q6;
	    pState->q_values_after_init_code[6] = *pState->var_pv_q7;
	    pState->q_values_after_init_code[7] = *pState->var_pv_q8;
        */

		resetVars(NULL);
	}



	// hurl the triangle strips at the video card

	int poly;

	for (poly=0; poly<m_nGridX+2; poly++)
	{
		m_verts_temp[poly].Diffuse = cDecay;
	}

	for (int strip=0; strip<m_nGridY*2; strip++)
	{
		int index = strip * (m_nGridX+2);

		for (poly=0; poly<m_nGridX+2; poly++)
		{
			int ref_vert = m_indices[index];
			m_verts_temp[poly].x = m_verts[ref_vert].x;
			m_verts_temp[poly].y = -m_verts[ref_vert].y;
			m_verts_temp[poly].z = m_verts[ref_vert].z;
			m_verts_temp[poly].tu = m_verts[ref_vert].tu;
			m_verts_temp[poly].tv = m_verts[ref_vert].tv;
		    //m_verts_temp[poly].Diffuse = 0xFFFFFFFF;
			index++;
		}

        lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, m_nGridX, (void*)m_verts_temp, sizeof(SPRITEVERTEX));
	}
}

void CPlugin::DrawCustomShapes()
{
    LPDIRECT3DDEVICE9 lpDevice = GetDevice();
    if (!lpDevice)
        return;

    //lpDevice->SetTexture(0, m_lpVS[0]);//NULL);
    //lpDevice->SetVertexShader( SPRITEVERTEX_FORMAT );

	int num_reps = (m_pState->m_bBlending) ? 2 : 1;
	for (int rep=0; rep<num_reps; rep++)
	{
        CState *pState = (rep==0) ? m_pState : m_pOldState;
        float alpha_mult = 1;
        if (num_reps==2)
            alpha_mult = (rep==0) ? m_pState->m_fBlendProgress : (1-m_pState->m_fBlendProgress);

        for (int i=0; i<MAX_CUSTOM_SHAPES; i++)
        {
            if (pState->m_shape[i].enabled)
            {
                /*
                int bAdditive = 0;
                int nSides = 3;//3 + ((int)GetTime() % 8);
                int bThickOutline = 0;
                float x = 0.5f + 0.1f*cosf(GetTime()*0.8f+1);
                float y = 0.5f + 0.1f*sinf(GetTime()*0.8f+1);
                float rad = 0.15f + 0.07f*sinf(GetTime()*1.1f+3);
                float ang = GetTime()*1.5f;

                // inside colors
                float r = 1;    
                float g = 0;
                float b = 0;
                float a = 0.4f;//0.1f + 0.1f*sinf(GetTime()*0.31f); 

                // outside colors
                float r2 = 0;       
                float g2 = 1;
                float b2 = 0;
                float a2 = 0;

                // border colors
                float border_r = 1;       
                float border_g = 1;
                float border_b = 1;
                float border_a = 0.5f;
                */

                // 1. execute per-frame code
                LoadCustomShapePerFrameEvallibVars(pState, i);

			    #ifndef _NO_EXPR_
				    if (pState->m_shape[i].m_pf_codehandle)
				    {
					    resetVars(pState->m_shape[i].m_pf_vars);
					    executeCode(pState->m_shape[i].m_pf_codehandle);
					    resetVars(NULL);
				    }
			    #endif

                // save changes to t1-t8 this frame
                /*
		        pState->m_shape[i].t_values_after_init_code[0] = *pState->m_shape[i].var_pf_t1;
		        pState->m_shape[i].t_values_after_init_code[1] = *pState->m_shape[i].var_pf_t2;
		        pState->m_shape[i].t_values_after_init_code[2] = *pState->m_shape[i].var_pf_t3;
		        pState->m_shape[i].t_values_after_init_code[3] = *pState->m_shape[i].var_pf_t4;
		        pState->m_shape[i].t_values_after_init_code[4] = *pState->m_shape[i].var_pf_t5;
		        pState->m_shape[i].t_values_after_init_code[5] = *pState->m_shape[i].var_pf_t6;
		        pState->m_shape[i].t_values_after_init_code[6] = *pState->m_shape[i].var_pf_t7;
		        pState->m_shape[i].t_values_after_init_code[7] = *pState->m_shape[i].var_pf_t8;
                */

                int sides = (int)(*pState->m_shape[i].var_pf_sides);
                if (sides<3) sides=3;
                if (sides>100) sides=100;

	            lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
                lpDevice->SetRenderState(D3DRS_DESTBLEND, ((int)(*pState->m_shape[i].var_pf_additive) != 0) ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA);

                SPRITEVERTEX v[512];  // for textured shapes (has texcoords)
                WFVERTEX v2[512];     // for untextured shapes + borders

                v[0].x = (float)(*pState->m_shape[i].var_pf_x* 2-1);// * ASPECT;
                v[0].y = (float)(*pState->m_shape[i].var_pf_y*-2+1);
                v[0].z = 0;
                v[0].tu = 0.5f;
                v[0].tv = 0.5f;
                v[0].Diffuse = 
                    ((((int)(*pState->m_shape[i].var_pf_a * 255 * alpha_mult)) & 0xFF) << 24) |
                    ((((int)(*pState->m_shape[i].var_pf_r * 255)) & 0xFF) << 16) |
                    ((((int)(*pState->m_shape[i].var_pf_g * 255)) & 0xFF) <<  8) |
                    ((((int)(*pState->m_shape[i].var_pf_b * 255)) & 0xFF)      );
                v[1].Diffuse = 
                    ((((int)(*pState->m_shape[i].var_pf_a2 * 255 * alpha_mult)) & 0xFF) << 24) |
                    ((((int)(*pState->m_shape[i].var_pf_r2 * 255)) & 0xFF) << 16) |
                    ((((int)(*pState->m_shape[i].var_pf_g2 * 255)) & 0xFF) <<  8) |
                    ((((int)(*pState->m_shape[i].var_pf_b2 * 255)) & 0xFF)      );

                for (int j=1; j<sides+1; j++)
                {
                    float t = (j-1)/(float)sides;
                    v[j].x = v[0].x + (float)*pState->m_shape[i].var_pf_rad*cosf(t*3.1415927f*2 + (float)*pState->m_shape[i].var_pf_ang + 3.1415927f*0.25f)*ASPECT;  // DON'T TOUCH!
                    v[j].y = v[0].y + (float)*pState->m_shape[i].var_pf_rad*sinf(t*3.1415927f*2 + (float)*pState->m_shape[i].var_pf_ang + 3.1415927f*0.25f);         // DON'T TOUCH!
                    v[j].z = 0;
                    v[j].tu = 0.5f + 0.5f*cosf(t*3.1415927f*2 + (float)*pState->m_shape[i].var_pf_tex_ang + 3.1415927f*0.25f)/((float)*pState->m_shape[i].var_pf_tex_zoom) * ASPECT; // DON'T TOUCH!
                    v[j].tv = 0.5f + 0.5f*sinf(t*3.1415927f*2 + (float)*pState->m_shape[i].var_pf_tex_ang + 3.1415927f*0.25f)/((float)*pState->m_shape[i].var_pf_tex_zoom);   // DON'T TOUCH!
                    v[j].Diffuse = v[1].Diffuse;
                }
                v[sides+1] = v[1];

                if ((int)(*pState->m_shape[i].var_pf_textured) != 0)
                {
                    // draw textured version
                    lpDevice->SetTexture(0, m_lpVS[0]);
                    lpDevice->SetFVF( SPRITEVERTEX_FORMAT );
                    lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, sides, (void*)v, sizeof(SPRITEVERTEX));
                }
                else
                {
                    // no texture
                    for (int j=0; j < sides+2; j++)
                    {
                        v2[j].x       = v[j].x;
                        v2[j].y       = v[j].y;
                        v2[j].z       = v[j].z;
                        v2[j].Diffuse = v[j].Diffuse;
                    }
                    lpDevice->SetTexture(0, NULL);
                    lpDevice->SetFVF( WFVERTEX_FORMAT );
                    lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, sides, (void*)v2, sizeof(WFVERTEX));
                }
                    

                // DRAW BORDER
                if (*pState->m_shape[i].var_pf_border_a > 0)
                {
                    lpDevice->SetTexture(0, NULL);
                    lpDevice->SetFVF( WFVERTEX_FORMAT );

                    v2[0].Diffuse = 
                        ((((int)(*pState->m_shape[i].var_pf_border_a * 255 * alpha_mult)) & 0xFF) << 24) |
                        ((((int)(*pState->m_shape[i].var_pf_border_r * 255)) & 0xFF) << 16) |
                        ((((int)(*pState->m_shape[i].var_pf_border_g * 255)) & 0xFF) <<  8) |
                        ((((int)(*pState->m_shape[i].var_pf_border_b * 255)) & 0xFF)      );
                    for (int j=0; j<sides+2; j++)
                    {
                        v2[j].x = v[j].x;
                        v2[j].y = v[j].y;
                        v2[j].z = v[j].z;
                        v2[j].Diffuse = v2[0].Diffuse;
                    }
                    
                    int its = ((int)(*pState->m_shape[i].var_pf_thick) != 0) ? 4 : 1;
		            float x_inc = 2.0f / (float)m_nTexSize;
                    for (int it=0; it<its; it++)
                    {
			            switch(it)
			            {
			            case 0: break;
			            case 1: for (int j=0; j<sides+2; j++) v2[j].x += x_inc; break;		// draw fat dots
			            case 2: for (int j=0; j<sides+2; j++) v2[j].y += x_inc; break;		// draw fat dots
			            case 3: for (int j=0; j<sides+2; j++) v2[j].x -= x_inc; break;		// draw fat dots
			            }
                        lpDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, sides, (void*)&v2[1], sizeof(WFVERTEX));
                    }

                    lpDevice->SetTexture(0, m_lpVS[0]);
                    lpDevice->SetFVF( SPRITEVERTEX_FORMAT );
                }
            }
        }
    }

	lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
	lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
}

void CPlugin::LoadCustomShapePerFrameEvallibVars(CState* pState, int i)
{
	*pState->m_shape[i].var_pf_time		= (double)(GetTime() - m_fStartTime);
	*pState->m_shape[i].var_pf_frame	= (double)GetFrame();
	*pState->m_shape[i].var_pf_fps      = (double)GetFps();
	*pState->m_shape[i].var_pf_progress = (GetTime() - m_fPresetStartTime) / (m_fNextPresetTime - m_fPresetStartTime);
	*pState->m_shape[i].var_pf_bass		= (double)mysound.imm_rel[0];
	*pState->m_shape[i].var_pf_mid		= (double)mysound.imm_rel[1];
	*pState->m_shape[i].var_pf_treb		= (double)mysound.imm_rel[2];
	*pState->m_shape[i].var_pf_bass_att	= (double)mysound.avg_rel[0];
	*pState->m_shape[i].var_pf_mid_att	= (double)mysound.avg_rel[1];
	*pState->m_shape[i].var_pf_treb_att	= (double)mysound.avg_rel[2];
	*pState->m_shape[i].var_pf_q1       = *pState->var_pf_q1;//pState->q_values_after_init_code[0];//0.0f;
	*pState->m_shape[i].var_pf_q2       = *pState->var_pf_q2;//pState->q_values_after_init_code[1];//0.0f;
	*pState->m_shape[i].var_pf_q3       = *pState->var_pf_q3;//pState->q_values_after_init_code[2];//0.0f;
	*pState->m_shape[i].var_pf_q4       = *pState->var_pf_q4;//pState->q_values_after_init_code[3];//0.0f;
	*pState->m_shape[i].var_pf_q5       = *pState->var_pf_q5;//pState->q_values_after_init_code[4];//0.0f;
	*pState->m_shape[i].var_pf_q6       = *pState->var_pf_q6;//pState->q_values_after_init_code[5];//0.0f;
	*pState->m_shape[i].var_pf_q7       = *pState->var_pf_q7;//pState->q_values_after_init_code[6];//0.0f;
	*pState->m_shape[i].var_pf_q8       = *pState->var_pf_q8;//pState->q_values_after_init_code[7];//0.0f;
	*pState->m_shape[i].var_pf_t1       = pState->m_shape[i].t_values_after_init_code[0];//0.0f;
	*pState->m_shape[i].var_pf_t2       = pState->m_shape[i].t_values_after_init_code[1];//0.0f;
	*pState->m_shape[i].var_pf_t3       = pState->m_shape[i].t_values_after_init_code[2];//0.0f;
	*pState->m_shape[i].var_pf_t4       = pState->m_shape[i].t_values_after_init_code[3];//0.0f;
	*pState->m_shape[i].var_pf_t5       = pState->m_shape[i].t_values_after_init_code[4];//0.0f;
	*pState->m_shape[i].var_pf_t6       = pState->m_shape[i].t_values_after_init_code[5];//0.0f;
	*pState->m_shape[i].var_pf_t7       = pState->m_shape[i].t_values_after_init_code[6];//0.0f;
	*pState->m_shape[i].var_pf_t8       = pState->m_shape[i].t_values_after_init_code[7];//0.0f;
	*pState->m_shape[i].var_pf_x        = pState->m_shape[i].x;
	*pState->m_shape[i].var_pf_y        = pState->m_shape[i].y;
	*pState->m_shape[i].var_pf_rad      = pState->m_shape[i].rad;
	*pState->m_shape[i].var_pf_ang      = pState->m_shape[i].ang;
	*pState->m_shape[i].var_pf_tex_zoom = pState->m_shape[i].tex_zoom;
	*pState->m_shape[i].var_pf_tex_ang  = pState->m_shape[i].tex_ang;
	*pState->m_shape[i].var_pf_sides    = pState->m_shape[i].sides;
    *pState->m_shape[i].var_pf_additive = pState->m_shape[i].additive;
    *pState->m_shape[i].var_pf_textured = pState->m_shape[i].textured;
    *pState->m_shape[i].var_pf_thick    = pState->m_shape[i].thickOutline;
	*pState->m_shape[i].var_pf_r        = pState->m_shape[i].r;
	*pState->m_shape[i].var_pf_g        = pState->m_shape[i].g;
	*pState->m_shape[i].var_pf_b        = pState->m_shape[i].b;
	*pState->m_shape[i].var_pf_a        = pState->m_shape[i].a;
	*pState->m_shape[i].var_pf_r2       = pState->m_shape[i].r2;
	*pState->m_shape[i].var_pf_g2       = pState->m_shape[i].g2;
	*pState->m_shape[i].var_pf_b2       = pState->m_shape[i].b2;
	*pState->m_shape[i].var_pf_a2       = pState->m_shape[i].a2;
	*pState->m_shape[i].var_pf_border_r = pState->m_shape[i].border_r;
	*pState->m_shape[i].var_pf_border_g = pState->m_shape[i].border_g;
	*pState->m_shape[i].var_pf_border_b = pState->m_shape[i].border_b;
	*pState->m_shape[i].var_pf_border_a = pState->m_shape[i].border_a;
}

void CPlugin::LoadCustomWavePerFrameEvallibVars(CState* pState, int i)
{
	*pState->m_wave[i].var_pf_time		= (double)(GetTime() - m_fStartTime);
	*pState->m_wave[i].var_pf_frame		= (double)GetFrame();
	*pState->m_wave[i].var_pf_fps       = (double)GetFps();
	*pState->m_wave[i].var_pf_progress  = (GetTime() - m_fPresetStartTime) / (m_fNextPresetTime - m_fPresetStartTime);
	*pState->m_wave[i].var_pf_bass		= (double)mysound.imm_rel[0];
	*pState->m_wave[i].var_pf_mid		= (double)mysound.imm_rel[1];
	*pState->m_wave[i].var_pf_treb		= (double)mysound.imm_rel[2];
	*pState->m_wave[i].var_pf_bass_att	= (double)mysound.avg_rel[0];
	*pState->m_wave[i].var_pf_mid_att	= (double)mysound.avg_rel[1];
	*pState->m_wave[i].var_pf_treb_att	= (double)mysound.avg_rel[2];
	*pState->m_wave[i].var_pf_q1        = *pState->var_pf_q1;//pState->q_values_after_init_code[0];//0.0f;
	*pState->m_wave[i].var_pf_q2        = *pState->var_pf_q2;//pState->q_values_after_init_code[1];//0.0f;
	*pState->m_wave[i].var_pf_q3        = *pState->var_pf_q3;//pState->q_values_after_init_code[2];//0.0f;
	*pState->m_wave[i].var_pf_q4        = *pState->var_pf_q4;//pState->q_values_after_init_code[3];//0.0f;
	*pState->m_wave[i].var_pf_q5        = *pState->var_pf_q5;//pState->q_values_after_init_code[4];//0.0f;
	*pState->m_wave[i].var_pf_q6        = *pState->var_pf_q6;//pState->q_values_after_init_code[5];//0.0f;
	*pState->m_wave[i].var_pf_q7        = *pState->var_pf_q7;//pState->q_values_after_init_code[6];//0.0f;
	*pState->m_wave[i].var_pf_q8        = *pState->var_pf_q8;//pState->q_values_after_init_code[7];//0.0f;
	*pState->m_wave[i].var_pf_t1        = pState->m_wave[i].t_values_after_init_code[0];//0.0f;
	*pState->m_wave[i].var_pf_t2        = pState->m_wave[i].t_values_after_init_code[1];//0.0f;
	*pState->m_wave[i].var_pf_t3        = pState->m_wave[i].t_values_after_init_code[2];//0.0f;
	*pState->m_wave[i].var_pf_t4        = pState->m_wave[i].t_values_after_init_code[3];//0.0f;
	*pState->m_wave[i].var_pf_t5        = pState->m_wave[i].t_values_after_init_code[4];//0.0f;
	*pState->m_wave[i].var_pf_t6        = pState->m_wave[i].t_values_after_init_code[5];//0.0f;
	*pState->m_wave[i].var_pf_t7        = pState->m_wave[i].t_values_after_init_code[6];//0.0f;
	*pState->m_wave[i].var_pf_t8        = pState->m_wave[i].t_values_after_init_code[7];//0.0f;
	*pState->m_wave[i].var_pf_r         = pState->m_wave[i].r;
	*pState->m_wave[i].var_pf_g         = pState->m_wave[i].g;
	*pState->m_wave[i].var_pf_b         = pState->m_wave[i].b;
	*pState->m_wave[i].var_pf_a         = pState->m_wave[i].a;
}

void CPlugin::DrawCustomWaves()
{
    LPDIRECT3DDEVICE9 lpDevice = GetDevice();
    if (!lpDevice)
        return;

    lpDevice->SetTexture(0, NULL);
    lpDevice->SetFVF( WFVERTEX_FORMAT );

    // note: read in all sound data from CPluginShell's m_sound
	int num_reps = (m_pState->m_bBlending) ? 2 : 1;
	for (int rep=0; rep<num_reps; rep++)
	{
        CState *pState = (rep==0) ? m_pState : m_pOldState;
        float alpha_mult = 1;
        if (num_reps==2)
            alpha_mult = (rep==0) ? m_pState->m_fBlendProgress : (1-m_pState->m_fBlendProgress);
    
        for (int i=0; i<MAX_CUSTOM_WAVES; i++)
        {
            if (pState->m_wave[i].enabled)
            {
                int nSamples = pState->m_wave[i].samples;
                int max_samples = pState->m_wave[i].bSpectrum ? 512 : NUM_WAVEFORM_SAMPLES;
				if (nSamples > max_samples)
                    nSamples = max_samples;
                nSamples -= pState->m_wave[i].sep;

                if ((nSamples >= 2) || (pState->m_wave[i].bUseDots && nSamples >= 1))
                {
                    int j;
                    float tempdata[2][512];
                    float mult = ((pState->m_wave[i].bSpectrum) ? 0.15f : 0.004f) * pState->m_wave[i].scaling * pState->m_fWaveScale.eval(-1);
                    float *pdata1 = (pState->m_wave[i].bSpectrum) ? m_sound.fSpectrum[0] : m_sound.fWaveform[0];
                    float *pdata2 = (pState->m_wave[i].bSpectrum) ? m_sound.fSpectrum[1] : m_sound.fWaveform[1];
                
                    // initialize tempdata[2][512]
                    int j0 = (pState->m_wave[i].bSpectrum) ? 0 : (max_samples - nSamples)/2/**(1-pState->m_wave[i].bSpectrum)*/ - pState->m_wave[i].sep/2;
                    int j1 = (pState->m_wave[i].bSpectrum) ? 0 : (max_samples - nSamples)/2/**(1-pState->m_wave[i].bSpectrum)*/ + pState->m_wave[i].sep/2;
                    float t = (pState->m_wave[i].bSpectrum) ? (max_samples - pState->m_wave[i].sep)/(float)nSamples : 1;
                    float mix1 = powf(pState->m_wave[i].smoothing*0.98f, 0.5f);  // lower exponent -> more default smoothing
                    float mix2 = 1-mix1;
                    // SMOOTHING:
                    tempdata[0][0] = pdata1[j0];
                    tempdata[1][0] = pdata2[j1];
                    for (j=1; j<nSamples; j++) 
                    {
                        tempdata[0][j] = pdata1[(int)(j*t)+j0]*mix2 + tempdata[0][j-1]*mix1;
                        tempdata[1][j] = pdata2[(int)(j*t)+j1]*mix2 + tempdata[1][j-1]*mix1;
                    }
                    // smooth again, backwards: [this fixes the asymmetry of the beginning & end..]
                    for (j=nSamples-2; j>=0; j--)
                    {
                        tempdata[0][j] = tempdata[0][j]*mix2 + tempdata[0][j+1]*mix1;
                        tempdata[1][j] = tempdata[1][j]*mix2 + tempdata[1][j+1]*mix1;
                    }
                    // finally, scale to final size:
                    for (j=0; j<nSamples; j++) 
                    {
                        tempdata[0][j] *= mult;
                        tempdata[1][j] *= mult;
                    }

                    // 1. execute per-frame code
                    LoadCustomWavePerFrameEvallibVars(pState, i);

			            // 2.a. do just a once-per-frame init for the *per-point* *READ-ONLY* variables
			            //     (the non-read-only ones will be reset/restored at the start of each vertex)
			            *pState->m_wave[i].var_pp_time		= *pState->m_wave[i].var_pf_time;	
			            *pState->m_wave[i].var_pp_fps       = *pState->m_wave[i].var_pf_fps;
			            *pState->m_wave[i].var_pp_frame		= *pState->m_wave[i].var_pf_frame;
		                *pState->m_wave[i].var_pp_progress  = *pState->m_wave[i].var_pf_progress;
			            *pState->m_wave[i].var_pp_bass		= *pState->m_wave[i].var_pf_bass;	
			            *pState->m_wave[i].var_pp_mid		= *pState->m_wave[i].var_pf_mid;		
			            *pState->m_wave[i].var_pp_treb		= *pState->m_wave[i].var_pf_treb;	
			            *pState->m_wave[i].var_pp_bass_att	= *pState->m_wave[i].var_pf_bass_att;
			            *pState->m_wave[i].var_pp_mid_att	= *pState->m_wave[i].var_pf_mid_att;	
			            *pState->m_wave[i].var_pp_treb_att	= *pState->m_wave[i].var_pf_treb_att;

				    executeCode(pState->m_wave[i].m_pf_codehandle);

                        *pState->m_wave[i].var_pp_q1        = *pState->m_wave[i].var_pf_q1;
                        *pState->m_wave[i].var_pp_q2        = *pState->m_wave[i].var_pf_q2;
                        *pState->m_wave[i].var_pp_q3        = *pState->m_wave[i].var_pf_q3;
                        *pState->m_wave[i].var_pp_q4        = *pState->m_wave[i].var_pf_q4;
                        *pState->m_wave[i].var_pp_q5        = *pState->m_wave[i].var_pf_q5;
                        *pState->m_wave[i].var_pp_q6        = *pState->m_wave[i].var_pf_q6;
                        *pState->m_wave[i].var_pp_q7        = *pState->m_wave[i].var_pf_q7;
                        *pState->m_wave[i].var_pp_q8        = *pState->m_wave[i].var_pf_q8;
                        *pState->m_wave[i].var_pp_t1        = *pState->m_wave[i].var_pf_t1;
                        *pState->m_wave[i].var_pp_t2        = *pState->m_wave[i].var_pf_t2;
                        *pState->m_wave[i].var_pp_t3        = *pState->m_wave[i].var_pf_t3;
                        *pState->m_wave[i].var_pp_t4        = *pState->m_wave[i].var_pf_t4;
                        *pState->m_wave[i].var_pp_t5        = *pState->m_wave[i].var_pf_t5;
                        *pState->m_wave[i].var_pp_t6        = *pState->m_wave[i].var_pf_t6;
                        *pState->m_wave[i].var_pp_t7        = *pState->m_wave[i].var_pf_t7;
                        *pState->m_wave[i].var_pp_t8        = *pState->m_wave[i].var_pf_t8;

                    // 2. for each point, execute per-point code

                    #ifndef _NO_EXPR_
                        resetVars(pState->m_wave[i].m_pp_vars);
                    #endif

                    // to do:
                    //  -add any of the m_wave[i].xxx menu-accessible vars to the code?
                    WFVERTEX v[512];
                    float j_mult = 1.0f/(float)(nSamples-1); 
                    for (j=0; j<nSamples; j++)
                    {
                        float t = j*j_mult;
                        float value1 = tempdata[0][j];
                        float value2 = tempdata[1][j];
                        *pState->m_wave[i].var_pp_sample = t;
                        *pState->m_wave[i].var_pp_value1 = value1;
                        *pState->m_wave[i].var_pp_value2 = value2;
                        *pState->m_wave[i].var_pp_x      = 0.5f + value1;
                        *pState->m_wave[i].var_pp_y      = 0.5f + value2;
                        *pState->m_wave[i].var_pp_r      = *pState->m_wave[i].var_pf_r;
                        *pState->m_wave[i].var_pp_g      = *pState->m_wave[i].var_pf_g;
                        *pState->m_wave[i].var_pp_b      = *pState->m_wave[i].var_pf_b;
                        *pState->m_wave[i].var_pp_a      = *pState->m_wave[i].var_pf_a;

                        #ifndef _NO_EXPR_
                            executeCode(pState->m_wave[i].m_pp_codehandle);
                        #endif

                        v[j].x = (float)(*pState->m_wave[i].var_pp_x* 2-1);//*ASPECT;
                        v[j].y = (float)(*pState->m_wave[i].var_pp_y*-2+1);
                        v[j].z = 0;
                        v[j].Diffuse = 
                            ((((int)(*pState->m_wave[i].var_pp_a * 255 * alpha_mult)) & 0xFF) << 24) |
                            ((((int)(*pState->m_wave[i].var_pp_r * 255)) & 0xFF) << 16) |
                            ((((int)(*pState->m_wave[i].var_pp_g * 255)) & 0xFF) <<  8) |
                            ((((int)(*pState->m_wave[i].var_pp_b * 255)) & 0xFF)      );
                    }

                    #ifndef _NO_EXPR_
                        resetVars(NULL);
                    #endif

                    // save changes to t1-t8 this frame
                    /*
		            pState->m_wave[i].t_values_after_init_code[0] = *pState->m_wave[i].var_pp_t1;
		            pState->m_wave[i].t_values_after_init_code[1] = *pState->m_wave[i].var_pp_t2;
		            pState->m_wave[i].t_values_after_init_code[2] = *pState->m_wave[i].var_pp_t3;
		            pState->m_wave[i].t_values_after_init_code[3] = *pState->m_wave[i].var_pp_t4;
		            pState->m_wave[i].t_values_after_init_code[4] = *pState->m_wave[i].var_pp_t5;
		            pState->m_wave[i].t_values_after_init_code[5] = *pState->m_wave[i].var_pp_t6;
		            pState->m_wave[i].t_values_after_init_code[6] = *pState->m_wave[i].var_pp_t7;
		            pState->m_wave[i].t_values_after_init_code[7] = *pState->m_wave[i].var_pp_t8;
                    */

                    // 3. draw it
	                lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	                lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
                    lpDevice->SetRenderState(D3DRS_DESTBLEND, pState->m_wave[i].bAdditive ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA);
                
                    float ptsize = ((m_nTexSize >= 1024) ? 2.0f : 1.0f) + (pState->m_wave[i].bDrawThick ? 1.0f : 0.0f);
                    if (pState->m_wave[i].bUseDots)
                        lpDevice->SetRenderState(D3DRS_POINTSIZE, *((DWORD*)&ptsize) ); 

                    int its = (pState->m_wave[i].bDrawThick && !pState->m_wave[i].bUseDots) ? 4 : 1;
		            float x_inc = 2.0f / (float)m_nTexSize;
                    for (int it=0; it<its; it++)
                    {
			            switch(it)
			            {
			            case 0: break;
			            case 1: for (j=0; j<nSamples; j++) v[j].x += x_inc; break;		// draw fat dots
			            case 2: for (j=0; j<nSamples; j++) v[j].y += x_inc; break;		// draw fat dots
			            case 3: for (j=0; j<nSamples; j++) v[j].x -= x_inc; break;		// draw fat dots
			            }
                        lpDevice->DrawPrimitiveUP(pState->m_wave[i].bUseDots ? D3DPT_POINTLIST : D3DPT_LINESTRIP, nSamples - (pState->m_wave[i].bUseDots ? 0 : 1), (void*)v, sizeof(WFVERTEX));
                    }

                    ptsize = 1.0f;
                    if (pState->m_wave[i].bUseDots)
                        lpDevice->SetRenderState(D3DRS_POINTSIZE, *((DWORD*)&ptsize) ); 
                }
            }
        }
    }

	lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
	lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
}

void CPlugin::DrawWave(float *fL, float *fR)
{
	//return;

    LPDIRECT3DDEVICE9 lpDevice = GetDevice();
    if (!lpDevice)
        return;

    lpDevice->SetTexture(0, NULL);
    lpDevice->SetFVF( WFVERTEX_FORMAT );

	int i;
	WFVERTEX v1[576+1], v2[576+1];

    /*
	m_lpD3DDev->lpDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD); //D3DSHADE_FLAT
	m_lpD3DDev->lpDevice->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);
	m_lpD3DDev->lpDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
	if (m_D3DDevDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_DITHER)
		m_lpD3DDev->lpDevice->SetRenderState(D3DRENDERSTATE_DITHERENABLE, TRUE);
	m_lpD3DDev->lpDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
	m_lpD3DDev->lpDevice->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);            
	m_lpD3DDev->lpDevice->SetRenderState(D3DRENDERSTATE_COLORVERTEX, TRUE);
	m_lpD3DDev->lpDevice->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME);  // vs. SOLID
	m_lpD3DDev->lpDevice->SetRenderState(D3DRENDERSTATE_AMBIENT, D3DCOLOR_RGBA_01(1,1,1,1));

	hr = m_lpD3DDev->SetTexture(0, NULL);
	if (hr != D3D_OK) 
	{
		//dumpmsg("Draw(): ERROR: SetTexture");
		//IdentifyD3DError(hr);
	}
    */

	lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
	lpDevice->SetRenderState(D3DRS_DESTBLEND, (*m_pState->var_pf_wave_additive) ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA);

	//float cr = m_pState->m_waveR.eval(GetTime());
	//float cg = m_pState->m_waveG.eval(GetTime());
	//float cb = m_pState->m_waveB.eval(GetTime());
	float cr = (float)(*m_pState->var_pf_wave_r);
	float cg = (float)(*m_pState->var_pf_wave_g);
	float cb = (float)(*m_pState->var_pf_wave_b);
	float cx = (float)(*m_pState->var_pf_wave_x);
	float cy = (float)(*m_pState->var_pf_wave_y); // note: it was backwards (top==1) in the original milkdrop, so we keep it that way!
	float fWaveParam = (float)(*m_pState->var_pf_wave_mystery);
	
	/*if (m_pState->m_bBlending)
	{
		cr = cr*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress)*((float)(*m_pOldState->var_pf_wave_r));
		cg = cg*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress)*((float)(*m_pOldState->var_pf_wave_g));
		cb = cb*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress)*((float)(*m_pOldState->var_pf_wave_b));
		cx = cx*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress)*((float)(*m_pOldState->var_pf_wave_x));
		cy = cy*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress)*((float)(*m_pOldState->var_pf_wave_y));
		fWaveParam = fWaveParam*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress)*((float)(*m_pOldState->var_pf_wave_mystery));
	}*/

	if (cr < 0) cr = 0;
	if (cg < 0) cg = 0;
	if (cb < 0) cb = 0;
	if (cr > 1) cr = 1;
	if (cg > 1) cg = 1;
	if (cb > 1) cb = 1;
	
	// maximize color:
	if (*m_pState->var_pf_wave_brighten)
	{
		float fMaximizeWaveColorAmount = 1.0f;
		float max = cr;
		if (max < cg) max = cg;
		if (max < cb) max = cb;
		if (max > 0.01f)
		{
			cr = cr/max*fMaximizeWaveColorAmount + cr*(1.0f - fMaximizeWaveColorAmount);
			cg = cg/max*fMaximizeWaveColorAmount + cg*(1.0f - fMaximizeWaveColorAmount);
			cb = cb/max*fMaximizeWaveColorAmount + cb*(1.0f - fMaximizeWaveColorAmount);
		}
	}

	float fWavePosX = cx*2.0f - 1.0f; // go from 0..1 user-range to -1..1 D3D range
	float fWavePosY = cy*2.0f - 1.0f;

	float bass_rel   = mysound.imm[0];
	float mid_rel    = mysound.imm[1];
	float treble_rel = mysound.imm[2];

	int sample_offset = 0;
	int new_wavemode = (int)(*m_pState->var_pf_wave_mode) % NUM_WAVES;  // since it can be changed from per-frame code!

	int its = (m_pState->m_bBlending && (new_wavemode != m_pState->m_nOldWaveMode)) ? 2 : 1;
	int nVerts1 = 0;
	int nVerts2 = 0;
	int nBreak1 = -1;
	int nBreak2 = -1;
	float alpha1, alpha2;

	for (int it=0; it<its; it++)
	{
		int   wave   = (it==0) ? new_wavemode : m_pState->m_nOldWaveMode;
		int   nVerts = NUM_WAVEFORM_SAMPLES;		// allowed to peek ahead 64 (i.e. left is [i], right is [i+64])
		int   nBreak = -1;

		WFVERTEX *v = (it==0) ? v1 : v2;
		ZeroMemory(v, sizeof(WFVERTEX)*nVerts);

		float alpha = (float)(*m_pState->var_pf_wave_a);//m_pState->m_fWaveAlpha.eval(GetTime());

		switch(wave)
		{
		case 0:
			// circular wave

			nVerts /= 2;
			sample_offset = (NUM_WAVEFORM_SAMPLES-nVerts)/2;//mysound.GoGoAlignatron(nVerts * 12/10);	// only call this once nVerts is final!

			if (m_pState->m_bModWaveAlphaByVolume)
				alpha *= ((mysound.imm_rel[0] + mysound.imm_rel[1] + mysound.imm_rel[2])*0.333f - m_pState->m_fModWaveAlphaStart.eval(GetTime()))/(m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
			if (alpha < 0) alpha = 0;
			if (alpha > 1) alpha = 1;
			//color = D3DCOLOR_RGBA_01(cr, cg, cb, alpha);

			{
				float inv_nverts_minus_one = 1.0f/(float)(nVerts-1);

				for (i=0; i<nVerts; i++)
				{
					float rad = 0.5f + 0.4f*fR[i+sample_offset] + fWaveParam;
					float ang = (i)*inv_nverts_minus_one*6.28f + GetTime()*0.2f;
					if (i < nVerts/10)
					{
						float mix = i/(nVerts*0.1f);
						mix = 0.5f - 0.5f*cosf(mix * 3.1416f);
						float rad_2 = 0.5f + 0.4f*fR[i + nVerts + sample_offset] + fWaveParam;
						rad = rad_2*(1.0f-mix) + rad*(mix);
					}
					v[i].x = rad*cosf(ang) * ASPECT + fWavePosX;		// 0.75 = adj. for aspect ratio
					v[i].y = rad*sinf(ang) + fWavePosY;
					//v[i].Diffuse = color;
				}
			}

			// dupe last vertex to connect the lines; skip if blending
			if (!m_pState->m_bBlending)
			{
				nVerts++;
				memcpy(&v[nVerts-1], &v[0], sizeof(WFVERTEX));
			}

			break;

		case 1:
			// x-y osc. that goes around in a spiral, in time

			alpha *= 1.25f;
			if (m_pState->m_bModWaveAlphaByVolume)
				alpha *= ((mysound.imm_rel[0] + mysound.imm_rel[1] + mysound.imm_rel[2])*0.333f - m_pState->m_fModWaveAlphaStart.eval(GetTime()))/(m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
			if (alpha < 0) alpha = 0;
			if (alpha > 1) alpha = 1;
			//color = D3DCOLOR_RGBA_01(cr, cg, cb, alpha);

			nVerts /= 2;	

			for (i=0; i<nVerts; i++)
			{
				float rad = 0.53f + 0.43f*fR[i] + fWaveParam;
				float ang = fL[i+32] * 1.57f + GetTime()*2.3f;
				v[i].x = rad*cosf(ang) * ASPECT + fWavePosX;		// 0.75 = adj. for aspect ratio
				v[i].y = rad*sinf(ang) + fWavePosY;
				//v[i].Diffuse = color;//(D3DCOLOR_RGBA_01(cr, cg, cb, alpha*min(1, max(0, fL[i])));
			}

			break;
			
		case 2:
			// centered spiro (alpha constant)
			//	 aimed at not being so sound-responsive, but being very "nebula-like"
			//   difference is that alpha is constant (and faint), and waves a scaled way up

			switch(m_nTexSize)
			{
			case 256:  alpha *= 0.07f; break;
			case 512:  alpha *= 0.09f; break;
			case 1024: alpha *= 0.11f; break;
			case 2048: alpha *= 0.13f; break;
			}

			if (m_pState->m_bModWaveAlphaByVolume)
				alpha *= ((mysound.imm_rel[0] + mysound.imm_rel[1] + mysound.imm_rel[2])*0.333f - m_pState->m_fModWaveAlphaStart.eval(GetTime()))/(m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
			if (alpha < 0) alpha = 0;
			if (alpha > 1) alpha = 1;
			//color = D3DCOLOR_RGBA_01(cr, cg, cb, alpha);
			
			for (i=0; i<nVerts; i++)
			{
				v[i].x = fR[i] * ASPECT + fWavePosX;//((pR[i] ^ 128) - 128)/90.0f * ASPECT; // 0.75 = adj. for aspect ratio
				v[i].y = fL[i+32] + fWavePosY;//((pL[i+32] ^ 128) - 128)/90.0f;
				//v[i].Diffuse = color;
			}

			break;
		case 3:
			// centered spiro (alpha tied to volume)
			//	 aimed at having a strong audio-visual tie-in
			//   colors are always bright (no darks)

			switch(m_nTexSize)
			{
			case 256:  alpha = 0.075f; break;
			case 512:  alpha = 0.150f; break;
			case 1024: alpha = 0.220f; break;
			case 2048: alpha = 0.330f; break;
			}
			
			alpha *= 1.3f;
			alpha *= powf(treble_rel, 2.0f);
			if (m_pState->m_bModWaveAlphaByVolume)
				alpha *= ((mysound.imm_rel[0] + mysound.imm_rel[1] + mysound.imm_rel[2])*0.333f - m_pState->m_fModWaveAlphaStart.eval(GetTime()))/(m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
			if (alpha < 0) alpha = 0;
			if (alpha > 1) alpha = 1;
			//color = D3DCOLOR_RGBA_01(cr, cg, cb, alpha);

			for (i=0; i<nVerts; i++)
			{
				v[i].x = fR[i] * ASPECT + fWavePosX;//((pR[i] ^ 128) - 128)/90.0f * ASPECT; // 0.75 = adj. for aspect ratio
				v[i].y = fL[i+32] + fWavePosY;//((pL[i+32] ^ 128) - 128)/90.0f;
				//v[i].Diffuse = color;

			}
			break;
		case 4:
			// horizontal "script", left channel

			if (nVerts > m_nTexSize/3) 
				nVerts = m_nTexSize/3;

			sample_offset = (NUM_WAVEFORM_SAMPLES-nVerts)/2;//mysound.GoGoAlignatron(nVerts + 25);	// only call this once nVerts is final!

			/*
			if (treble_rel > treb_thresh_for_wave6)
			{
				//alpha = 1.0f;
				treb_thresh_for_wave6 = treble_rel * 1.025f;
			}
			else
			{
				alpha *= 0.2f;
				treb_thresh_for_wave6 *= 0.996f;		// fixme: make this fps-independent
			}
			*/

			if (m_pState->m_bModWaveAlphaByVolume)
				alpha *= ((mysound.imm_rel[0] + mysound.imm_rel[1] + mysound.imm_rel[2])*0.333f - m_pState->m_fModWaveAlphaStart.eval(GetTime()))/(m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
			if (alpha < 0) alpha = 0;
			if (alpha > 1) alpha = 1;
			//color = D3DCOLOR_RGBA_01(cr, cg, cb, alpha);

			{
				float w1 = 0.45f + 0.5f*(fWaveParam*0.5f + 0.5f);		// 0.1 - 0.9
				float w2 = 1.0f - w1;

				float inv_nverts = 1.0f/(float)(nVerts);

				for (i=0; i<nVerts; i++)
				{
					v[i].x = -1.0f + 2.0f*(i*inv_nverts) + fWavePosX;
					v[i].y = fL[i+sample_offset]*0.47f + fWavePosY;//((pL[i] ^ 128) - 128)/270.0f;
					v[i].x += fR[i+25+sample_offset]*0.44f;//((pR[i+25] ^ 128) - 128)/290.0f;
					//v[i].Diffuse = color;

					// momentum
					if (i>1)
					{
						v[i].x = v[i].x*w2 + w1*(v[i-1].x*2.0f - v[i-2].x);
						v[i].y = v[i].y*w2 + w1*(v[i-1].y*2.0f - v[i-2].y);
					}
				}

				/*
				// center on Y
				float avg_y = 0;
				for (i=0; i<nVerts; i++) 
					avg_y += v[i].y;
				avg_y /= (float)nVerts;
				avg_y *= 0.5f;		// damp the movement
				for (i=0; i<nVerts; i++) 
					v[i].y -= avg_y;
				*/
			}

			break;

		case 5:
			// weird explosive complex # thingy

			switch(m_nTexSize)
			{
			case 256:  alpha *= 0.07f; break;
			case 512:  alpha *= 0.09f; break;
			case 1024: alpha *= 0.11f; break;
			case 2048: alpha *= 0.13f; break;
			}

			if (m_pState->m_bModWaveAlphaByVolume)
				alpha *= ((mysound.imm_rel[0] + mysound.imm_rel[1] + mysound.imm_rel[2])*0.333f - m_pState->m_fModWaveAlphaStart.eval(GetTime()))/(m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
			if (alpha < 0) alpha = 0;
			if (alpha > 1) alpha = 1;
			//color = D3DCOLOR_RGBA_01(cr, cg, cb, alpha);
			
			{
				float cos_rot = cosf(GetTime()*0.3f);
				float sin_rot = sinf(GetTime()*0.3f);

				for (i=0; i<nVerts; i++)
				{
					float x0 = (fR[i]*fL[i+32] + fL[i]*fR[i+32]);
					float y0 = (fR[i]*fR[i] - fL[i+32]*fL[i+32]);
					v[i].x = (x0*cos_rot - y0*sin_rot)*ASPECT + fWavePosX;
					v[i].y = (x0*sin_rot + y0*cos_rot) + fWavePosY;
					//v[i].Diffuse = color;
				}
			}

			break;

		case 6:
		case 7:
		case 8:
			// 6: angle-adjustable left channel, with temporal wave alignment;
			//   fWaveParam controls the angle at which it's drawn
			//	 fWavePosX slides the wave away from the center, transversely.
			//   fWavePosY does nothing
			//
			// 7: same, except there are two channels shown, and
			//   fWavePosY determines the separation distance.
			// 
			// 8: same as 6, except using the spectrum analyzer (UNFINISHED)
			// 
			nVerts /= 2;

			if (nVerts > m_nTexSize/3) 
				nVerts = m_nTexSize/3;

			if (wave==8)
				nVerts = 256;
			else
				sample_offset = (NUM_WAVEFORM_SAMPLES-nVerts)/2;//mysound.GoGoAlignatron(nVerts);	// only call this once nVerts is final!

			if (m_pState->m_bModWaveAlphaByVolume)
				alpha *= ((mysound.imm_rel[0] + mysound.imm_rel[1] + mysound.imm_rel[2])*0.333f - m_pState->m_fModWaveAlphaStart.eval(GetTime()))/(m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
			if (alpha < 0) alpha = 0;
			if (alpha > 1) alpha = 1;
			//color = D3DCOLOR_RGBA_01(cr, cg, cb, alpha);

			{
				float ang = 1.57f*fWaveParam;	// from -PI/2 to PI/2
				float dx  = cosf(ang);
				float dy  = sinf(ang);

				float edge_x[2], edge_y[2];
				
				//edge_x[0] = fWavePosX - dx*3.0f;
				//edge_y[0] = fWavePosY - dy*3.0f;
				//edge_x[1] = fWavePosX + dx*3.0f;
				//edge_y[1] = fWavePosY + dy*3.0f;
				edge_x[0] = fWavePosX*cosf(ang + 1.57f) - dx*3.0f;
				edge_y[0] = fWavePosX*sinf(ang + 1.57f) - dy*3.0f;
				edge_x[1] = fWavePosX*cosf(ang + 1.57f) + dx*3.0f;
				edge_y[1] = fWavePosX*sinf(ang + 1.57f) + dy*3.0f;


				for (i=0; i<2; i++)	// for each point defining the line
				{
					// clip the point against 4 edges of screen
					// be a bit lenient (use +/-1.1 instead of +/-1.0) 
					//	 so the dual-wave doesn't end too soon, after the channels are moved apart
					for (int j=0; j<4; j++)
					{
						float t;
						bool bClip = false;

						switch(j)
						{
						case 0:
							if (edge_x[i] > 1.1f)
							{
								t = (1.1f - edge_x[1-i]) / (edge_x[i] - edge_x[1-i]);
								bClip = true;
							}
							break;
						case 1:
							if (edge_x[i] < -1.1f)
							{
								t = (-1.1f - edge_x[1-i]) / (edge_x[i] - edge_x[1-i]);
								bClip = true;
								}
							break;
						case 2:
							if (edge_y[i] > 1.1f)
							{
								t = (1.1f - edge_y[1-i]) / (edge_y[i] - edge_y[1-i]);
								bClip = true;
							}
							break;
						case 3:
							if (edge_y[i] < -1.1f)
							{
								t = (-1.1f - edge_y[1-i]) / (edge_y[i] - edge_y[1-i]);
								bClip = true;
							}
							break;
						}

						if (bClip)
						{
							float dx = edge_x[i] - edge_x[1-i];
							float dy = edge_y[i] - edge_y[1-i];
							edge_x[i] = edge_x[1-i] + dx*t;
							edge_y[i] = edge_y[1-i] + dy*t;
						}
					}
				}

				dx = (edge_x[1] - edge_x[0]) / (float)nVerts;
				dy = (edge_y[1] - edge_y[0]) / (float)nVerts;
				float ang2 = atan2f(dy,dx);
				float perp_dx = cosf(ang2 + 1.57f);
				float perp_dy = sinf(ang2 + 1.57f);

				if (wave == 6)
					for (i=0; i<nVerts; i++)
					{
						v[i].x = edge_x[0] + dx*i + perp_dx*0.25f*fL[i + sample_offset];
						v[i].y = edge_y[0] + dy*i + perp_dy*0.25f*fL[i + sample_offset];
						//v[i].Diffuse = color;
					}
				else if (wave == 8)
					//256 verts
					for (i=0; i<nVerts; i++)
					{
						float f = 0.1f*logf(mysound.fSpecLeft[i*2] + mysound.fSpecLeft[i*2+1]);
						v[i].x = edge_x[0] + dx*i + perp_dx*f;
						v[i].y = edge_y[0] + dy*i + perp_dy*f;
						//v[i].Diffuse = color;
					}
				else
				{
					float sep = powf(fWavePosY*0.5f + 0.5f, 2.0f);
					for (i=0; i<nVerts; i++)
					{
						v[i].x = edge_x[0] + dx*i + perp_dx*(0.25f*fL[i + sample_offset] + sep);
						v[i].y = edge_y[0] + dy*i + perp_dy*(0.25f*fL[i + sample_offset] + sep);
						//v[i].Diffuse = color;
					}

					//D3DPRIMITIVETYPE primtype = (*m_pState->var_pf_wave_usedots) ? D3DPT_POINTLIST : D3DPT_LINESTRIP;
					//m_lpD3DDev->DrawPrimitive(primtype, D3DFVF_LVERTEX, (LPVOID)v, nVerts, NULL);

					for (i=0; i<nVerts; i++)
					{
						v[i+nVerts].x = edge_x[0] + dx*i + perp_dx*(0.25f*fR[i + sample_offset] - sep);
						v[i+nVerts].y = edge_y[0] + dy*i + perp_dy*(0.25f*fR[i + sample_offset] - sep);
						//v[i+nVerts].Diffuse = color;
					}

					nBreak = nVerts;
					nVerts *= 2;
				}
					
			}

			break;
		}

		if (it==0)
		{
			nVerts1 = nVerts;
			nBreak1 = nBreak;
			alpha1  = alpha;
		}
		else
		{
			nVerts2 = nVerts;
			nBreak2 = nBreak;
			alpha2  = alpha;
		}
	}	

	// v1[] is for the current waveform
	// v2[] is for the old waveform (from prev. preset - only used if blending)
	// nVerts1 is the # of vertices in v1
	// nVerts2 is the # of vertices in v2
	// nBreak1 is the index of the point at which to break the solid line in v1[] (-1 if no break)
	// nBreak2 is the index of the point at which to break the solid line in v2[] (-1 if no break)

	float mix = CosineInterp(m_pState->m_fBlendProgress);
	float mix2 = 1.0f - mix;

	// blend 2 waveforms
	if (nVerts2 > 0)
	{
		// note: this won't yet handle the case where (nBreak1 > 0 && nBreak2 > 0)
		//       in this case, code must break wave into THREE segments
		float m = (nVerts2-1)/(float)nVerts1;
		float x,y;
		for (int i=0; i<nVerts1; i++)
		{
			float fIdx = i*m;
			int   nIdx = (int)fIdx;
			float t = fIdx - nIdx;
			if (nIdx == nBreak2-1)
			{
				x = v2[nIdx].x;
				y = v2[nIdx].y;
				nBreak1 = i+1;
			}
			else
			{
				x = v2[nIdx].x*(1-t) + v2[nIdx+1].x*(t);
				y = v2[nIdx].y*(1-t) + v2[nIdx+1].y*(t);
			}
			v1[i].x = v1[i].x*(mix) + x*(mix2);
			v1[i].y = v1[i].y*(mix) + y*(mix2);
		}
	}

	// determine alpha
	if (nVerts2 > 0)
	{
		alpha1 = alpha1*(mix) + alpha2*(1.0f-mix);
	}

	// apply color & alpha
    // ALSO reverse all y values, to stay consistent with the pre-VMS milkdrop,
    //  which DIDN'T:
	v1[0].Diffuse = D3DCOLOR_RGBA_01(cr, cg, cb, alpha1);
	for (i=0; i<nVerts1; i++)
    {
		v1[i].Diffuse = v1[0].Diffuse;
        v1[i].y = -v1[i].y;
    }
	
	// draw primitives
	{
		//D3DPRIMITIVETYPE primtype = (*m_pState->var_pf_wave_usedots) ? D3DPT_POINTLIST : D3DPT_LINESTRIP;
		float x_inc = 2.0f / (float)m_nTexSize;
		int drawing_its = ((*m_pState->var_pf_wave_thick || *m_pState->var_pf_wave_usedots) && (m_nTexSize >= 512)) ? 4 : 1;

		for (int it=0; it<drawing_its; it++)
		{
			int j;

			switch(it)
			{
			case 0: break;
			case 1: for (j=0; j<nVerts1; j++) v1[j].x += x_inc; break;		// draw fat dots
			case 2: for (j=0; j<nVerts1; j++) v1[j].y += x_inc; break;		// draw fat dots
			case 3: for (j=0; j<nVerts1; j++) v1[j].x -= x_inc; break;		// draw fat dots
			}

			if (nBreak1 == -1)
			{
				//m_lpD3DDev->DrawPrimitive(primtype, D3DFVF_LVERTEX, (LPVOID)v1, nVerts1, NULL);
                if (*m_pState->var_pf_wave_usedots)
                    lpDevice->DrawPrimitiveUP(D3DPT_POINTLIST, nVerts1, (void*)v1, sizeof(WFVERTEX));
                else
                    lpDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, nVerts1-1, (void*)v1, sizeof(WFVERTEX));
			}
			else
			{
				//m_lpD3DDev->DrawPrimitive(primtype, D3DFVF_LVERTEX, (LPVOID)v1, nBreak1, NULL);
				//m_lpD3DDev->DrawPrimitive(primtype, D3DFVF_LVERTEX, (LPVOID)&v1[nBreak1], nVerts1-nBreak1, NULL);
                if (*m_pState->var_pf_wave_usedots)
                {
                    lpDevice->DrawPrimitiveUP(D3DPT_POINTLIST, nBreak1, (void*)v1, sizeof(WFVERTEX));
                    lpDevice->DrawPrimitiveUP(D3DPT_POINTLIST, nVerts1-nBreak1, (void*)&v1[nBreak1], sizeof(WFVERTEX));
                }
                else
                {
                    lpDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, nBreak1-1, (void*)v1, sizeof(WFVERTEX));
                    lpDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, nVerts1-nBreak1-1, (void*)&v1[nBreak1], sizeof(WFVERTEX));
                }
			}
		}
	}

	lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
}



void CPlugin::DrawSprites()
{
    LPDIRECT3DDEVICE9 lpDevice = GetDevice();
    if (!lpDevice)
        return;

    lpDevice->SetTexture(0, NULL);
    lpDevice->SetFVF( WFVERTEX_FORMAT );

	if (*m_pState->var_pf_darken_center)
	{
		lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);//SRCALPHA);
		lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		WFVERTEX v3[6];
		ZeroMemory(v3, sizeof(WFVERTEX)*6);
		
		// colors:
		v3[0].Diffuse = D3DCOLOR_RGBA_01(0, 0, 0, 3.0f/32.0f);
		v3[1].Diffuse = D3DCOLOR_RGBA_01(0, 0, 0, 0.0f/32.0f);
		v3[2].Diffuse = v3[1].Diffuse;
		v3[3].Diffuse = v3[1].Diffuse;
		v3[4].Diffuse = v3[1].Diffuse;
		v3[5].Diffuse = v3[1].Diffuse;

		// positioning:
		float fHalfSize = 0.05f;
		v3[0].x = 0.0f; 	
		v3[1].x = 0.0f - fHalfSize*ASPECT; 	
		v3[2].x = 0.0f; 	
		v3[3].x = 0.0f + fHalfSize*ASPECT; 	
		v3[4].x = 0.0f; 
		v3[5].x = v3[1].x;
		v3[0].y = 0.0f; 	
		v3[1].y = 0.0f;
		v3[2].y = 0.0f - fHalfSize; 	
		v3[3].y = 0.0f;
		v3[4].y = 0.0f + fHalfSize; 	
		v3[5].y = v3[1].y;
		//v3[0].tu = 0;	v3[1].tu = 1;	v3[2].tu = 0;	v3[3].tu = 1;
		//v3[0].tv = 1;	v3[1].tv = 1;	v3[2].tv = 0;	v3[3].tv = 0;

		lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 4, (LPVOID)v3, sizeof(WFVERTEX));

		lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	}


	// do borders
	{
		float fOuterBorderSize = (float)*m_pState->var_pf_ob_size;
		float fInnerBorderSize = (float)*m_pState->var_pf_ib_size;

		lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
		lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		for (int it=0; it<2; it++)
		{
			WFVERTEX v3[4];
			ZeroMemory(v3, sizeof(WFVERTEX)*4);
			
			// colors:
			float r = (it==0) ? (float)*m_pState->var_pf_ob_r : (float)*m_pState->var_pf_ib_r;
			float g = (it==0) ? (float)*m_pState->var_pf_ob_g : (float)*m_pState->var_pf_ib_g;
			float b = (it==0) ? (float)*m_pState->var_pf_ob_b : (float)*m_pState->var_pf_ib_b;
			float a = (it==0) ? (float)*m_pState->var_pf_ob_a : (float)*m_pState->var_pf_ib_a;
			if (a > 0.001f)
			{
				v3[0].Diffuse = D3DCOLOR_RGBA_01(r,g,b,a);
				v3[1].Diffuse = v3[0].Diffuse;
				v3[2].Diffuse = v3[0].Diffuse;
				v3[3].Diffuse = v3[0].Diffuse;

				// positioning:
				float fInnerRad = (it==0) ? 1.0f - fOuterBorderSize : 1.0f - fOuterBorderSize - fInnerBorderSize;
				float fOuterRad = (it==0) ? 1.0f                    : 1.0f - fOuterBorderSize;
				v3[0].x =  fInnerRad;
				v3[1].x =  fOuterRad; 	
				v3[2].x =  fOuterRad;
				v3[3].x =  fInnerRad;
				v3[0].y =  fInnerRad;
				v3[1].y =  fOuterRad;
				v3[2].y = -fOuterRad;
				v3[3].y = -fInnerRad;

				for (int rot=0; rot<4; rot++)
				{
		            lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, (LPVOID)v3, sizeof(WFVERTEX));

					// rotate by 90 degrees
					for (int v=0; v<4; v++)
					{
						float t = 1.570796327f;
						float x = v3[v].x;
						float y = v3[v].y;
						v3[v].x = x*cosf(t) - y*sinf(t);
						v3[v].y = x*sinf(t) + y*cosf(t);
					}
				}
			}
		}
		lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	}
}

/*
bool CPlugin::SetMilkdropRenderTarget(LPDIRECTDRAWSURFACE7 lpSurf, int w, int h, char *szErrorMsg)
{
	HRESULT hr = m_lpD3DDev->SetRenderTarget(lpSurf, 0);
	if (hr != D3D_OK) 
	{
		//if (szErrorMsg && szErrorMsg[0]) dumpmsg(szErrorMsg);
		//IdentifyD3DError(hr);
		return false;
	}

	//DDSURFACEDESC2 ddsd;
	//ddsd.dwSize = sizeof(ddsd);
	//lpSurf->GetSurfaceDesc(&ddsd);

	D3DVIEWPORT7 viewData;
	ZeroMemory(&viewData, sizeof(D3DVIEWPORT7));
	viewData.dwWidth  = w;	// not: in windowed mode, when lpSurf is the back buffer, chances are good that w,h are smaller than the full surface size (since real size is fullscreen, but we're only using a portion of it as big as the window).
	viewData.dwHeight = h;
	hr = m_lpD3DDev->SetViewport(&viewData);

	return true;
}
*/

void CPlugin::DrawUserSprites()	// from system memory, to back buffer.
{
#if 0

    LPDIRECT3DDEVICE8 lpDevice = GetDevice();
    if (!lpDevice)
        return;

    lpDevice->SetTexture(0, NULL);
    lpDevice->SetVertexShader( SPRITEVERTEX_FORMAT );

    lpDevice->SetRenderState(D3DRS_WRAP0, 0);
    SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    SetTextureStageState(0, D3DTSS_ADDRESSW, D3DTADDRESS_WRAP);

    // reset these to the standard safe mode:
    SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
	SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
    SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
    SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
    SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    /*
	lpDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD); //D3DSHADE_GOURAUD
	lpDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
	lpDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	if (m_D3DDevDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_DITHER)
		lpDevice->SetRenderState(D3DRS_DITHERENABLE, TRUE);
	lpDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	lpDevice->SetRenderState(D3DRS_LIGHTING, FALSE);            
	lpDevice->SetRenderState(D3DRS_COLORVERTEX, TRUE);
	lpDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);  // vs. wireframe
	lpDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_RGBA_01(1,1,1,1));
	SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR );
	SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR );
	SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_LINEAR );
    */

	for (int iSlot=0; iSlot < NUM_TEX; iSlot++)
	{
		if (m_texmgr.m_tex[iSlot].pSurface)
		{
			int k;

			// set values of input variables:
			*(m_texmgr.m_tex[iSlot].var_time)     = (double)(GetTime() - m_texmgr.m_tex[iSlot].fStartTime);
			*(m_texmgr.m_tex[iSlot].var_frame)    = (double)(GetFrame() - m_texmgr.m_tex[iSlot].nStartFrame);
			*(m_texmgr.m_tex[iSlot].var_fps)      = (double)GetFps();
			*(m_texmgr.m_tex[iSlot].var_progress) = (double)m_pState->m_fBlendProgress;
			*(m_texmgr.m_tex[iSlot].var_bass)     = (double)mysound.imm_rel[0];
			*(m_texmgr.m_tex[iSlot].var_mid)      = (double)mysound.imm_rel[1];
			*(m_texmgr.m_tex[iSlot].var_treb)     = (double)mysound.imm_rel[2];
			*(m_texmgr.m_tex[iSlot].var_bass_att) = (double)mysound.avg_rel[0];
			*(m_texmgr.m_tex[iSlot].var_mid_att)  = (double)mysound.avg_rel[1];
			*(m_texmgr.m_tex[iSlot].var_treb_att) = (double)mysound.avg_rel[2];

			// evaluate expressions
			#ifndef _NO_EXPR_
				if (m_texmgr.m_tex[iSlot].m_codehandle)
				{
					resetVars(m_texmgr.m_tex[iSlot].m_vars);
					executeCode(m_texmgr.m_tex[iSlot].m_codehandle);
					resetVars(NULL);
				}
			#endif

			bool bKillSprite = (*m_texmgr.m_tex[iSlot].var_done != 0.0);
			bool bBurnIn = (*m_texmgr.m_tex[iSlot].var_burn != 0.0);

            // Remember the original backbuffer and zbuffer
            LPDIRECT3DSURFACE8 pBackBuffer, pZBuffer;
            lpDevice->GetRenderTarget( &pBackBuffer );
            lpDevice->GetDepthStencilSurface( &pZBuffer );

			if (/*bKillSprite &&*/ bBurnIn)
			{
                // set up to render [from NULL] to VS1 (for burn-in).

	            lpDevice->SetTexture(0, NULL);

                IDirect3DSurface8* pNewTarget = NULL;
                if (m_lpVS[1]->GetSurfaceLevel(0, &pNewTarget) != D3D_OK) 
                    return;
                lpDevice->SetRenderTarget(pNewTarget, NULL);
                pNewTarget->Release();

	            lpDevice->SetTexture(0, NULL);
            }

			// finally, use the results to draw the sprite.
			if (lpDevice->SetTexture(0, m_texmgr.m_tex[iSlot].pSurface) != D3D_OK) 
				return;

			SPRITEVERTEX v3[4];
			ZeroMemory(v3, sizeof(SPRITEVERTEX)*4);

            /*
            int dest_w, dest_h;
            {
                LPDIRECT3DSURFACE8 pRT;
                lpDevice->GetRenderTarget( &pRT );

                D3DSURFACE_DESC desc;
                pRT->GetDesc(&desc);
                dest_w = desc.Width;
                dest_h = desc.Height;                
                pRT->Release();
            }*/

			float x   = min(1000.0f, max(-1000.0f, (float)(*m_texmgr.m_tex[iSlot].var_x) * 2.0f - 1.0f ));
			float y   = min(1000.0f, max(-1000.0f, (float)(*m_texmgr.m_tex[iSlot].var_y) * 2.0f - 1.0f ));
			float sx  = min(1000.0f, max(-1000.0f, (float)(*m_texmgr.m_tex[iSlot].var_sx) ));
			float sy  = min(1000.0f, max(-1000.0f, (float)(*m_texmgr.m_tex[iSlot].var_sy) ));
			float rot = (float)(*m_texmgr.m_tex[iSlot].var_rot);
			int flipx = (*m_texmgr.m_tex[iSlot].var_flipx == 0.0) ? 0 : 1;
			int flipy = (*m_texmgr.m_tex[iSlot].var_flipy == 0.0) ? 0 : 1;
			float repeatx = min(100.0f, max(0.01f, (float)(*m_texmgr.m_tex[iSlot].var_repeatx) ));
			float repeaty = min(100.0f, max(0.01f, (float)(*m_texmgr.m_tex[iSlot].var_repeaty) ));

			int blendmode = min(4, max(0, ((int)(*m_texmgr.m_tex[iSlot].var_blendmode))));
			float r = min(1.0f, max(0.0f, ((float)(*m_texmgr.m_tex[iSlot].var_r))));
			float g = min(1.0f, max(0.0f, ((float)(*m_texmgr.m_tex[iSlot].var_g))));
			float b = min(1.0f, max(0.0f, ((float)(*m_texmgr.m_tex[iSlot].var_b))));
			float a = min(1.0f, max(0.0f, ((float)(*m_texmgr.m_tex[iSlot].var_a))));

			// set x,y coords
			v3[0+flipx].x = -sx;
			v3[1-flipx].x =  sx;
			v3[2+flipx].x = -sx;
			v3[3-flipx].x =  sx;
			v3[0+flipy*2].y = -sy;
			v3[1+flipy*2].y = -sy;
			v3[2-flipy*2].y =  sy;
			v3[3-flipy*2].y =  sy;

			// first aspect ratio: adjust for non-1:1 images
			{
				float aspect = m_texmgr.m_tex[iSlot].img_h / (float)m_texmgr.m_tex[iSlot].img_w;

				if (aspect < 1)
					for (k=0; k<4; k++) v3[k].y *= aspect;		// wide image
				else
					for (k=0; k<4; k++) v3[k].x /= aspect;		// tall image
			}

			// 2D rotation
			{
				float cos_rot = cosf(rot);
				float sin_rot = sinf(rot);
				for (k=0; k<4; k++)
				{
					float x2 = v3[k].x*cos_rot - v3[k].y*sin_rot;
					float y2 = v3[k].x*sin_rot + v3[k].y*cos_rot;
					v3[k].x = x2;
					v3[k].y = y2;
				}
			}

			// translation
			for (k=0; k<4; k++)
			{
				v3[k].x += x;
				v3[k].y += y;
			}

			// second aspect ratio: normalize to width of screen
			{
				float aspect = GetWidth() / (float)(GetHeight());
				
				if (aspect > 1)
					for (k=0; k<4; k++) v3[k].y *= aspect;
				else
					for (k=0; k<4; k++) v3[k].x /= aspect;
			}

			// third aspect ratio: adjust for burn-in
			if (bKillSprite && bBurnIn)	// final render-to-VS1
			{
				float aspect = GetWidth()/(float)(GetHeight()*4.0f/3.0f);
				if (aspect < 1.0f)
					for (k=0; k<4; k++) v3[k].x *= aspect;
				else
					for (k=0; k<4; k++) v3[k].y /= aspect;
			}

			// finally, flip 'y' for annoying DirectX
			//for (k=0; k<4; k++) v3[k].y *= -1.0f;

			// set u,v coords
			{
				float dtu = 0.5f;// / (float)m_texmgr.m_tex[iSlot].tex_w;
				float dtv = 0.5f;// / (float)m_texmgr.m_tex[iSlot].tex_h;
				v3[0].tu = -dtu;
				v3[1].tu =  dtu;///*m_texmgr.m_tex[iSlot].img_w / (float)m_texmgr.m_tex[iSlot].tex_w*/ - dtu;	
				v3[2].tu = -dtu;	
				v3[3].tu =  dtu;///*m_texmgr.m_tex[iSlot].img_w / (float)m_texmgr.m_tex[iSlot].tex_w*/ - dtu;	
				v3[0].tv = -dtv;	
				v3[1].tv = -dtv;	
				v3[2].tv =  dtv;///*m_texmgr.m_tex[iSlot].img_h / (float)m_texmgr.m_tex[iSlot].tex_h*/ - dtv;	
				v3[3].tv =  dtv;///*m_texmgr.m_tex[iSlot].img_h / (float)m_texmgr.m_tex[iSlot].tex_h*/ - dtv;

				// repeat on x,y
				for (k=0; k<4; k++) 
				{
					v3[k].tu = (v3[k].tu - 0.0f)*repeatx + 0.5f;
					v3[k].tv = (v3[k].tv - 0.0f)*repeaty + 0.5f;
				}
			}

			// blendmodes                                      src alpha:        dest alpha:
			// 0   blend      r,g,b=modulate     a=opacity     SRCALPHA          INVSRCALPHA
			// 1   decal      r,g,b=modulate     a=modulate    D3DBLEND_ONE      D3DBLEND_ZERO
			// 2   additive   r,g,b=modulate     a=modulate    D3DBLEND_ONE      D3DBLEND_ONE
			// 3   srccolor   r,g,b=no effect    a=no effect   SRCCOLOR          INVSRCCOLOR
			// 4   colorkey   r,g,b=modulate     a=no effect   
			switch(blendmode)
			{
			case 0:
			default:
				// alpha blend

				/*
				Q. I am rendering with alpha blending and setting the alpha 
				of the diffuse vertex component to determine the opacity.  
				It works when there is no texture set, but as soon as I set 
				a texture the alpha that I set is no longer applied.  Why?

				The problem originates in the texture blending stages, rather 
				than in the subsequent alpha blending.  Alpha can come from 
				several possible sources.  If this has not been specified, 
				then the alpha will be taken from the texture, if one is selected.  
				If no texture is selected, then the default will use the alpha 
				channel of the diffuse vertex component.

				Explicitly specifying the diffuse vertex component as the source 
				for alpha will insure that the alpha is drawn from the alpha value 
				you set, whether a texture is selected or not:

				pDevice->SetTextureStageState(D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
				pDevice->SetTextureStageState(D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);

				If you later need to use the texture alpha as the source, set 
				D3DTSS_ALPHAARG1 to D3DTA_TEXTURE.
				*/

                SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
				SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
				lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
				lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
				for (k=0; k<4; k++) v3[k].Diffuse = D3DCOLOR_RGBA_01(r,g,b,a);
				break;
			case 1:
				// decal
				lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
				//lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ONE);
				//lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
				for (k=0; k<4; k++) v3[k].Diffuse = D3DCOLOR_RGBA_01(r*a,g*a,b*a,1);
				break;
			case 2:
				// additive
				lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ONE);
				lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
				for (k=0; k<4; k++) v3[k].Diffuse = D3DCOLOR_RGBA_01(r*a,g*a,b*a,1);
				break;
			case 3:
				// srccolor
				lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCCOLOR);
				lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCCOLOR);
				for (k=0; k<4; k++) v3[k].Diffuse = D3DCOLOR_RGBA_01(1,1,1,1);
				break;
			case 4:
				// color keyed texture: use the alpha value in the texture to 
				//  determine which texels get drawn.  
				/*lpDevice->SetRenderState(D3DRS_ALPHAREF, 0);
				lpDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_NOTEQUAL);
				lpDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
                */
                
                SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	            SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
	            SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
                SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	            SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
                SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
                SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
                SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

				// also, smoothly blend this in-between texels:
				lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
				lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
				for (k=0; k<4; k++) v3[k].Diffuse = D3DCOLOR_RGBA_01(r,g,b,a);
				break;
			}

			lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));

			if (/*bKillSprite &&*/ bBurnIn)	// final render-to-VS1
			{
                // Change the rendertarget back to the original setup
                lpDevice->SetTexture(0, NULL);
                lpDevice->SetRenderTarget( pBackBuffer, pZBuffer );
			    lpDevice->SetTexture(0, m_texmgr.m_tex[iSlot].pSurface);

				// undo aspect ratio changes (that were used to fit it to VS1):
				{
					float aspect = GetWidth()/(float)(GetHeight()*4.0f/3.0f);
					if (aspect < 1.0f)
						for (k=0; k<4; k++) v3[k].x /= aspect;
					else
						for (k=0; k<4; k++) v3[k].y *= aspect;
				}

			    lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));
			}

            SafeRelease(pBackBuffer);
            SafeRelease(pZBuffer);

			if (bKillSprite)
			{
				KillSprite(iSlot);
			}

	        SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
            SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
            SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		}
	}

	lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    // reset these to the standard safe mode:
    SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
	SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
    SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
    SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
    SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
#endif
}

void CPlugin::ShowToUser(int bRedraw)
{
    LPDIRECT3DDEVICE9 lpDevice = GetDevice();
    if (!lpDevice)
        return;

	lpDevice->SetTexture(0, m_lpVS[1]);
    lpDevice->SetFVF( SPRITEVERTEX_FORMAT );

	lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

	float fZoom = 1.0f;
	SPRITEVERTEX v3[4];
	ZeroMemory(v3, sizeof(SPRITEVERTEX)*4);

  D3DRECT rect;
  rect.x1 = m_posX;
  rect.x2 = m_posX + GetWidth();
  rect.y1 = m_posY;
  rect.y2 = m_posY + GetHeight();

  // WISO: lpDevice->SetScissors(1, false, &rect);


	// extend the poly we draw by 1 pixel around the viewable image area, 
	//  in case the video card wraps u/v coords with a +0.5-texel offset 
	//  (otherwise, a 1-pixel-wide line of the image would wrap at the top and left edges).

/*
	float fOnePlusInvWidth  = 1.0f + 1.0f/(float)GetWidth();
	float fOnePlusInvHeight = 1.0f + 1.0f/(float)GetHeight();

	v3[0].x = -fOnePlusInvWidth;
	v3[1].x =  fOnePlusInvWidth;
	v3[2].x = -fOnePlusInvWidth;
	v3[3].x =  fOnePlusInvWidth;
	v3[0].y =  fOnePlusInvHeight;
	v3[1].y =  fOnePlusInvHeight;
	v3[2].y = -fOnePlusInvHeight;
	v3[3].y = -fOnePlusInvHeight;
*/

	v3[0].x = ((m_posX / (float)m_backBufferWidth) * 2.0f) - 1.0f;
	v3[1].x = (((m_posX + GetWidth()) / (float)m_backBufferWidth) * 2.0f) - 1.0f;
	v3[2].x =  v3[0].x;
	v3[3].x =  v3[1].x;
	v3[0].y =  (((m_posY + GetHeight()) / (float)m_backBufferHeight) * 2.0f) - 1.0f;
	v3[1].y =  v3[0].y;
	v3[2].y =  ((m_posY / (float)m_backBufferHeight) * 2.0f) - 1.0f;
	v3[3].y = v3[2].y;



  float aspect = (float)GetWidth() / (GetHeight() / ASPECT);  // normal aspect in 1:1 pixels
  aspect *= m_pixelRatio; // adjust for non-square TV pixels
  float x_aspect_mult = 1.0f;
  float y_aspect_mult = 1.0f;

  if (aspect>1)
    y_aspect_mult = aspect;
  else
    x_aspect_mult = 1.0f/aspect;

  for (int n=0; n<4; n++) 
  {
    v3[n].x *= x_aspect_mult;
    v3[n].y *= y_aspect_mult;
  }

  float xRange = v3[1].x - v3[0].x;
  float yRange = v3[0].y - v3[2].y;

    
	if (m_pState->m_bRedBlueStereo || m_bAlways3D)
	{
		// red/blue stereo 3D
        // NOTE: video echo is not yet supported. 
        // NOTE: gamma IS now supported (As of v1.04), but it's really slow.
        //           -no biggie though; 3d effect dies w/high gamma, so it won't get used beyond 2.0X.

        float fGammaAdj = (float)*m_pState->var_pf_gamma;//m_pState->m_fGammaAdj.eval(GetTime());
        int gamma_passes = (int)(fGammaAdj + 0.999f);

		lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        
        for (int pass=0; pass<gamma_passes; pass++)
        {
            float colormult = min(1,max(0, (fGammaAdj-pass) )) / 255.0f;

		    for (int rep=0; rep<2; rep++)
		    {
			    int x, y;
			    int n = 0;

			    for (y=0; y<=m_nGridY; y++)
			    {
				    for (x=0; x<=m_nGridX; x++)
				    {
					    if (rep==0)
					    {
						    float du = fabsf((m_verts[n].x*0.5f + 0.5f) - m_verts[n].tu);
						    float dv = fabsf(0.5f - (m_verts[n].y*0.5f) - m_verts[n].tv);
						    //float mag_motion = sqrtf(m_verts[n].tu*m_verts[n].tu + m_verts[n].tv*m_verts[n].tv);
						    float mag_motion = sqrtf(du*du+dv*dv);
						    m_verts[n].tu = (m_verts[n].x*0.5f + 0.5f) + mag_motion*0.3f*m_fStereoSep;
						    //if (m_verts[n].tu < 0) m_verts[n].tu = 0;
						    //if (m_verts[n].tu > 1) m_verts[n].tu = 1;
					    }
					    else
					    {
						    //float du = fabsf((m_verts[n].x*0.5f + 0.5f) - m_verts[n].tu);
						    //float dv = fabsf(0.5f - (m_verts[n].y*0.5f) - m_verts[n].tv);
						    //float mag_motion = sqrtf(du*du+dv*dv);
						    //m_verts[n].tu = (m_verts[n].x*0.5f + 0.5f) - mag_motion*0.3f;
						    m_verts[n].tu = m_verts[n].x + 1.0f - m_verts[n].tu;
						    //if (m_verts[n].tu < 0) m_verts[n].tu = 0;
						    //if (m_verts[n].tu > 1) m_verts[n].tu = 1;
					    }

					    n++;
				    }
			    }

			    if (rep==0)
				    m_verts_temp[0].Diffuse = D3DCOLOR_RGBA_01(m_cRightEye3DColor[0]*colormult,m_cRightEye3DColor[1]*colormult,m_cRightEye3DColor[2]*colormult,1);		// should be the color of right lens (~red)
			    else
				    m_verts_temp[0].Diffuse = D3DCOLOR_RGBA_01(m_cLeftEye3DColor[0]*colormult,m_cLeftEye3DColor[1]*colormult,m_cLeftEye3DColor[2]*colormult,1);		// should be the color of left lens (~blue)

			    for (int poly=1; poly<m_nGridX+2; poly++)
				    m_verts_temp[poly].Diffuse = m_verts_temp[0].Diffuse;

			    for (int strip=0; strip<m_nGridY*2; strip++)
			    {
				    int index = strip * (m_nGridX+2);

				    for (int poly=0; poly<m_nGridX+2; poly++)
				    {
					    int ref_vert = m_indices[index];


						m_verts_temp[poly].x = ((m_verts[ref_vert].x * 0.5f + 0.5f)) * xRange + v3[0].x;
						m_verts_temp[poly].y = ((-m_verts[ref_vert].y * 0.5f + 0.5f)) * yRange + v3[2].y;
						m_verts_temp[poly].z = m_verts[ref_vert].z;
						m_verts_temp[poly].tu = m_verts[ref_vert].tu;
						m_verts_temp[poly].tv = m_verts[ref_vert].tv;

//						m_verts_temp[poly].x = m_verts[ref_vert].x*x_aspect_mult;
//				    m_verts_temp[poly].y = -m_verts[ref_vert].y*y_aspect_mult;
//					    m_verts_temp[poly].z = m_verts[ref_vert].z;
//					    m_verts_temp[poly].tu = m_verts[ref_vert].tu;
//					    m_verts_temp[poly].tv = m_verts[ref_vert].tv;
					    index++;
				    }

                    lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, m_nGridX, (void*)m_verts_temp, sizeof(SPRITEVERTEX));
			    }

			    lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			    lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ONE);
			    lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		    }
        }
		lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

	}
	else	// regular display (non-stereo)

	{
		float shade[4][3] = { 
			{ 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f } };  // for each vertex, then each comp.

		float fShaderAmount = m_pState->m_fShader.eval(GetTime());

		if (fShaderAmount > 0.001f)
		{
			for (int i=0; i<4; i++)
			{
				shade[i][0] = 0.6f + 0.3f*sinf(GetTime()*30.0f*0.0143f + 3 + i*21 + m_fRandStart[3]);
				shade[i][1] = 0.6f + 0.3f*sinf(GetTime()*30.0f*0.0107f + 1 + i*13 + m_fRandStart[1]);
				shade[i][2] = 0.6f + 0.3f*sinf(GetTime()*30.0f*0.0129f + 6 + i*9  + m_fRandStart[2]);
				float max = ((shade[i][0] > shade[i][1]) ? shade[i][0] : shade[i][1]);
				if (shade[i][2] > max) max = shade[i][2];
				for (int k=0; k<3; k++)
				{
					shade[i][k] /= max;
					shade[i][k] = 0.5f + 0.5f*shade[i][k];
				}
				for (int k=0; k<3; k++)
				{
					shade[i][k] = shade[i][k]*(fShaderAmount) + 1.0f*(1.0f - fShaderAmount);
				}
				v3[i].Diffuse = D3DCOLOR_RGBA_01(shade[i][0],shade[i][1],shade[i][2],1);
			}
		}

		float fVideoEchoZoom        = (float)(*m_pState->var_pf_echo_zoom);//m_pState->m_fVideoEchoZoom.eval(GetTime());
		float fVideoEchoAlpha       = (float)(*m_pState->var_pf_echo_alpha);//m_pState->m_fVideoEchoAlpha.eval(GetTime());
		int   nVideoEchoOrientation = (int)  (*m_pState->var_pf_echo_orient) % 4;//m_pState->m_nVideoEchoOrientation;
		float fGammaAdj             = (float)(*m_pState->var_pf_gamma);//m_pState->m_fGammaAdj.eval(GetTime());

		if (m_pState->m_bBlending && 
			m_pState->m_fVideoEchoAlpha.eval(GetTime()) > 0.01f &&
			m_pState->m_fVideoEchoAlphaOld > 0.01f &&
			m_pState->m_nVideoEchoOrientation != m_pState->m_nVideoEchoOrientationOld)
		{
			if (m_pState->m_fBlendProgress < 0.5f)
			{
				nVideoEchoOrientation = m_pState->m_nVideoEchoOrientationOld;
				fVideoEchoAlpha *= 1.0f - 2.0f*CosineInterp(m_pState->m_fBlendProgress);
			}
			else
			{
				fVideoEchoAlpha *= 2.0f*CosineInterp(m_pState->m_fBlendProgress) - 1.0f;
			}
		}

		if (fVideoEchoAlpha > 0.001f)
		{
			// video echo
			lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ONE);
			lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);

			for (int i=0; i<2; i++)
			{
				fZoom = (i==0) ? 1.0f : fVideoEchoZoom;

				float temp_lo = (0.5f - 0.5f/fZoom) + 0.5f / m_nTexSize;
				float temp_hi = (0.5f + 0.5f/fZoom) + 0.5f / m_nTexSize;

				v3[0].tu = temp_lo;
				v3[0].tv = temp_hi;
				v3[1].tu = temp_hi;
				v3[1].tv = temp_hi;
				v3[2].tu = temp_lo;
				v3[2].tv = temp_lo;
				v3[3].tu = temp_hi;
				v3[3].tv = temp_lo;

				// flipping
				if (i==1)
				{
					for (int j=0; j<4; j++)
					{
						if (nVideoEchoOrientation % 2)
							v3[j].tu = 1.0f - v3[j].tu;
						if (nVideoEchoOrientation >= 2)
							v3[j].tv = 1.0f - v3[j].tv;
					}
				}

				float mix = (i==1) ? fVideoEchoAlpha : 1.0f - fVideoEchoAlpha;
				for (int k=0; k<4; k++)	
					v3[k].Diffuse = D3DCOLOR_RGBA_01(mix*shade[k][0],mix*shade[k][1],mix*shade[k][2],1);

                lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v3, sizeof(SPRITEVERTEX));

				if (i==0)
				{
					lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ONE);
					lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
				}

				if (fGammaAdj > 0.001f)
				{
					// draw layer 'i' a 2nd (or 3rd, or 4th...) time, additively
					int nRedraws = (int)(fGammaAdj - 0.0001f);
					float gamma;

					for (int nRedraw=0; nRedraw < nRedraws; nRedraw++)
					{
						if (nRedraw == nRedraws-1)
							gamma = fGammaAdj - (int)(fGammaAdj - 0.0001f);
						else
							gamma = 1.0f;

						for (int k=0; k<4; k++)
							v3[k].Diffuse = D3DCOLOR_RGBA_01(gamma*mix*shade[k][0],gamma*mix*shade[k][1],gamma*mix*shade[k][2],1);
                        lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v3, sizeof(SPRITEVERTEX));
					}
				}
			}
		}
		else
		{
			// no video echo
			v3[0].tu = 0.5f / m_nTexSize;	v3[1].tu = 1;	v3[2].tu = 0.5f / m_nTexSize;	v3[3].tu = 1;
			v3[0].tv = 1;	v3[1].tv = 1;	v3[2].tv = 0.5f / m_nTexSize;	v3[3].tv = 0.5f / m_nTexSize;

			lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ONE);
			lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);

			// draw it iteratively, solid the first time, and additively after that
			int nPasses = (int)(fGammaAdj - 0.001f) + 1;
			float gamma;

			for (int nPass=0; nPass < nPasses; nPass++)
			{
				if (nPass == nPasses - 1)
					gamma = fGammaAdj - (float)nPass;
				else
					gamma = 1.0f;

				for (int k=0; k<4; k++)
					v3[k].Diffuse = D3DCOLOR_RGBA_01(gamma*shade[k][0],gamma*shade[k][1],gamma*shade[k][2],1);
                lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v3, sizeof(SPRITEVERTEX));

				if (nPass==0)
				{
					lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
					lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ONE);
					lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
				}
			}
		}
/*
		SPRITEVERTEX v3[4];
		ZeroMemory(v3, sizeof(SPRITEVERTEX)*4);
		float fOnePlusInvWidth  = 1.0f + 1.0f/(float)GetWidth();
		float fOnePlusInvHeight = 1.0f + 1.0f/(float)GetHeight();
		v3[0].x = -fOnePlusInvWidth;
		v3[1].x =  fOnePlusInvWidth;
		v3[2].x = -fOnePlusInvWidth;
		v3[3].x =  fOnePlusInvWidth;
		v3[0].y =  fOnePlusInvHeight;
		v3[1].y =  fOnePlusInvHeight;
		v3[2].y = -fOnePlusInvHeight;
		v3[3].y = -fOnePlusInvHeight;
*/
		for (int i=0; i<4; i++) v3[i].Diffuse = D3DCOLOR_RGBA_01(1,1,1,1);

		if (*m_pState->var_pf_brighten &&
			(GetCaps()->SrcBlendCaps  & D3DPBLENDCAPS_INVDESTCOLOR ) &&
			(GetCaps()->DestBlendCaps & D3DPBLENDCAPS_DESTCOLOR)
			)
		{
			// square root filter

			//lpDevice->SetRenderState(D3DRS_COLORVERTEX, FALSE);       //?
			//lpDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT); //?

			lpDevice->SetTexture(0, NULL);
			lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

			// first, a perfect invert
			lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_INVDESTCOLOR);
			lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
            lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v3, sizeof(SPRITEVERTEX));

			// then modulate by self (square it)
			lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ZERO);
			lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_DESTCOLOR);
            lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v3, sizeof(SPRITEVERTEX));

			// then another perfect invert
			lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_INVDESTCOLOR);
			lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
            lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v3, sizeof(SPRITEVERTEX));
		}

		if (*m_pState->var_pf_darken && 
			(GetCaps()->DestBlendCaps & D3DPBLENDCAPS_DESTCOLOR)
			)
		{
			// squaring filter

			//lpDevice->SetRenderState(D3DRS_COLORVERTEX, FALSE);          //?
			//lpDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);    //?

			lpDevice->SetTexture(0, NULL);
			lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

			lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ZERO);
			lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_DESTCOLOR);
            lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v3, sizeof(SPRITEVERTEX));

			//lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_DESTCOLOR);
			//lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			//lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v3, sizeof(SPRITEVERTEX));

		}
		
		if (*m_pState->var_pf_solarize && 
			(GetCaps()->SrcBlendCaps  & D3DPBLENDCAPS_DESTCOLOR ) &&
			(GetCaps()->DestBlendCaps & D3DPBLENDCAPS_INVDESTCOLOR)
			)
		{
			//lpDevice->SetRenderState(D3DRS_COLORVERTEX, FALSE);        //?
			//lpDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);  //?

			lpDevice->SetTexture(0, NULL);
			lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

			lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ZERO);
			lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVDESTCOLOR);
            lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v3, sizeof(SPRITEVERTEX));

			lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_DESTCOLOR);
			lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v3, sizeof(SPRITEVERTEX));
		}

		if (*m_pState->var_pf_invert && 
			(GetCaps()->SrcBlendCaps  & D3DPBLENDCAPS_INVDESTCOLOR )
			)
		{
			//lpDevice->SetRenderState(D3DRS_COLORVERTEX, FALSE);        //?
			//lpDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);  //?

			lpDevice->SetTexture(0, NULL);
			lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_INVDESTCOLOR);
			lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
			
            lpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v3, sizeof(SPRITEVERTEX));
		}
		

		lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	}
}


void CPlugin::ShowSongTitleAnim(int w, int h, float fProgress)
{
#if 0
	int i,x,y;

    if (!m_lpDDSTitle)  // this *can* be NULL, if not much video mem!
        return;

    LPDIRECT3DDEVICE8 lpDevice = GetDevice();
    if (!lpDevice)
        return;

	lpDevice->SetTexture(0, m_lpDDSTitle);
    lpDevice->SetVertexShader( SPRITEVERTEX_FORMAT );

	lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ONE);
	lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	SPRITEVERTEX v3[128];
	ZeroMemory(v3, sizeof(SPRITEVERTEX)*128);

	if (m_supertext.bIsSongTitle)
	{
		// positioning:
		float fSizeX = 50.0f / (float)m_supertext.nFontSizeUsed * powf(1.5f, m_supertext.fFontSize - 2.0f);
		float fSizeY = fSizeX * m_nTitleTexSizeY/(float)m_nTitleTexSizeX;// * m_nWidth/(float)m_nHeight;

		if (fSizeX > 0.88f)
		{
			fSizeY *= 0.88f/fSizeX;
			fSizeX = 0.88f;
		}

        //fixme
		if (fProgress < 1.0f)//(w!=h)	// regular render-to-backbuffer
		{
			float aspect = w/(float)(h*4.0f/3.0f);
			fSizeY *= aspect;
		}
		else 	// final render-to-VS0
		{
			float aspect = GetWidth()/(float)(GetHeight()*4.0f/3.0f);
			if (aspect < 1.0f)
			{
				fSizeX *= aspect;
				fSizeY *= aspect;
			}
		}

		//if (fSizeX > 0.92f) fSizeX = 0.92f;
		//if (fSizeY > 0.92f) fSizeY = 0.92f;
		i = 0;
		float vert_clip = VERT_CLIP;//1.0f;//0.45f;	// warning: visible clipping has been observed at 0.4!
		for (y=0; y<8; y++)
		{
			for (x=0; x<16; x++)
			{
				v3[i].tu = x/15.0f;
				v3[i].tv = (y/7.0f - 0.5f)*vert_clip + 0.5f;
				v3[i].x = (v3[i].tu*2.0f - 1.0f)*fSizeX;
				v3[i].y = (v3[i].tv*2.0f - 1.0f)*fSizeY;
				i++;
			}
		}

		// warping
		float ramped_progress = max(0.0f, 1-fProgress*1.5f);
		float t2 = powf(ramped_progress, 1.8f)*1.3f;
		for (y=0; y<8; y++)
		{
			for (x=0; x<16; x++)
			{
				i = y*16+x;
				v3[i].x += t2*0.070f*sinf(GetTime()*0.31f + v3[i].x*0.39f - v3[i].y*1.94f);
				v3[i].x += t2*0.044f*sinf(GetTime()*0.81f - v3[i].x*1.91f + v3[i].y*0.27f);
				v3[i].x += t2*0.061f*sinf(GetTime()*1.31f + v3[i].x*0.61f + v3[i].y*0.74f);
				v3[i].y += t2*0.061f*sinf(GetTime()*0.37f + v3[i].x*1.83f + v3[i].y*0.69f);
				v3[i].y += t2*0.070f*sinf(GetTime()*0.67f + v3[i].x*0.42f - v3[i].y*1.39f);
				v3[i].y += t2*0.087f*sinf(GetTime()*1.07f + v3[i].x*3.55f + v3[i].y*0.89f);
			}
		}

		// scale down over time
		float scale = 1.01f/(powf(fProgress, 0.21f) + 0.01f);
		for (i=0; i<128; i++)
		{
			v3[i].x *= scale;
			v3[i].y *= scale;
		}
	}
	else
	{
		// positioning:
		float fSizeX = (float)m_nTexSize/1024.0f * 100.0f / (float)m_supertext.nFontSizeUsed * powf(1.033f, m_supertext.fFontSize - 50.0f);
		float fSizeY = fSizeX * m_nTitleTexSizeY/(float)m_nTitleTexSizeX;

        //fixme
		if (fProgress < 1.0f)//w!=h)	// regular render-to-backbuffer
		{
			float aspect = w/(float)(h*4.0f/3.0f);
			fSizeY *= aspect;
		}
		else 	// final render-to-VS0
		{
			float aspect = GetWidth()/(float)(GetHeight()*4.0f/3.0f);
			if (aspect < 1.0f)
			{
				fSizeX *= aspect;
				fSizeY *= aspect;
			}
		}

		//if (fSize > 0.92f) fSize = 0.92f;
		i = 0;
		float vert_clip = VERT_CLIP;//0.67f;	// warning: visible clipping has been observed at 0.5 (for very short strings) and even 0.6 (for wingdings)!
		for (y=0; y<8; y++)
		{
			for (x=0; x<16; x++)
			{
				v3[i].tu = x/15.0f;
				v3[i].tv = (y/7.0f - 0.5f)*vert_clip + 0.5f;
				v3[i].x = (v3[i].tu*2.0f - 1.0f)*fSizeX;
				v3[i].y = (v3[i].tv*2.0f - 1.0f)*fSizeY;
				i++;
			}
		}

		// apply 'growth' factor and move to user-specified (x,y)
		//if (fabsf(m_supertext.fGrowth-1.0f) > 0.001f)
		{
			float t = (1.0f)*(1-fProgress) + (fProgress)*(m_supertext.fGrowth);
			float dx = (m_supertext.fX*2-1);
			float dy = (m_supertext.fY*2-1);
			if (w!=h)	// regular render-to-backbuffer
			{
				float aspect = w/(float)(h*4.0f/3.0f);
				if (aspect < 1)
					dx /= aspect;
				else
					dy *= aspect;
			}

			for (i=0; i<128; i++)
			{
				// note: (x,y) are in (-1,1) range, but m_supertext.f{X|Y} are in (0..1) range
				v3[i].x = (v3[i].x)*t + dx;
				v3[i].y = (v3[i].y)*t + dy;
			}
		}
	}

	WORD indices[7*15*6];
	i = 0;	
	for (y=0; y<7; y++)
	{
		for (x=0; x<15; x++)
		{
			indices[i++] = y*16 + x;
			indices[i++] = y*16 + x + 1;
			indices[i++] = y*16 + x + 16;
			indices[i++] = y*16 + x + 1;
			indices[i++] = y*16 + x + 16;
			indices[i++] = y*16 + x + 17;
		}
	}

	// final flip on y
	//for (i=0; i<128; i++)
	//	v3[i].y *= -1.0f;
	for (i=0; i<128; i++)
		v3[i].y /= ASPECT;

	for (int it=0; it<2; it++)
	{
		// colors
		{
			float t;
			
			if (m_supertext.bIsSongTitle)
				t = powf(fProgress, 0.3f)*1.0f;
			else
				t = CosineInterp(min(1.0f, (fProgress/m_supertext.fFadeTime)));
			
			if (it==0)
				v3[0].Diffuse = D3DCOLOR_RGBA_01(t,t,t,t);
			else
				v3[0].Diffuse = D3DCOLOR_RGBA_01(t*m_supertext.nColorR/255.0f,t*m_supertext.nColorG/255.0f,t*m_supertext.nColorB/255.0f,t);

			for (i=1; i<128; i++)
				v3[i].Diffuse = v3[0].Diffuse;
		}

		// nudge down & right for shadow, up & left for solid text
		float offset_x = 0, offset_y = 0;
		switch(it)
		{
		case 0:
			offset_x = 2.0f/(float)m_nTitleTexSizeX; 
			offset_y = 2.0f/(float)m_nTitleTexSizeY; 
			break;
		case 1:
			offset_x = -4.0f/(float)m_nTitleTexSizeX; 
			offset_y = -4.0f/(float)m_nTitleTexSizeY; 
			break;
		}

		for (i=0; i<128; i++)
		{
			v3[i].x += offset_x;
			v3[i].y += offset_y;
		}
	
		if (it == 0)
		{
			lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ZERO);//SRCALPHA);
			lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCCOLOR);
    }
    else 
    {
      lpDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ONE);//SRCALPHA);
      lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    }

    lpDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 128, 15*7*6/3, indices, D3DFMT_INDEX16, v3, sizeof(SPRITEVERTEX));
  }

  lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
#endif
}
