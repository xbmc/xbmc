
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
#include "state.h"
#include "support.h"
#include "evallib/compiler.h"
#include "plugin.h"
#include "utility.h"

//#include <stdlib.h>
//#include <windows.h>
#include <stdio.h>
#include <math.h>

extern CPlugin* g_plugin;		// declared in main.cpp



CState::CState()
{
	//Default();

	// this is the list of variables that can be used for a PER-FRAME calculation;
	// it is a SUBSET of the per-vertex calculation variable list.
	m_pf_codehandle = NULL;
	m_pp_codehandle = NULL;
    for (int i=0; i<MAX_CUSTOM_WAVES; i++)
    {
        m_wave[i].m_pf_codehandle = NULL;
        m_wave[i].m_pp_codehandle = NULL;
    }
    for (int i=0; i<MAX_CUSTOM_SHAPES; i++)
    {
        m_shape[i].m_pf_codehandle = NULL;
        //m_shape[i].m_pp_codehandle = NULL;
    }
	//RegisterBuiltInVariables();
}

CState::~CState()
{
	FreeVarsAndCode();
}

//--------------------------------------------------------------------------------

void CState::RegisterBuiltInVariables(int flags)
{
    if (flags & RECOMPILE_PRESET_CODE)
    {
	    resetVars(m_pf_vars);
        var_pf_zoom		= registerVar("zoom");		// i/o
	    var_pf_zoomexp  = registerVar("zoomexp");	// i/o
	    var_pf_rot		= registerVar("rot");		// i/o
	    var_pf_warp		= registerVar("warp");		// i/o
	    var_pf_cx		= registerVar("cx");		// i/o
	    var_pf_cy		= registerVar("cy");		// i/o
	    var_pf_dx		= registerVar("dx");		// i/o
	    var_pf_dy		= registerVar("dy");		// i/o
	    var_pf_sx		= registerVar("sx");		// i/o
	    var_pf_sy		= registerVar("sy");		// i/o
	    var_pf_time		= registerVar("time");		// i
	    var_pf_fps      = registerVar("fps");       // i
	    var_pf_bass		= registerVar("bass");		// i
	    var_pf_mid		= registerVar("mid");		// i
	    var_pf_treb		= registerVar("treb");		// i
	    var_pf_bass_att	= registerVar("bass_att");	// i
	    var_pf_mid_att	= registerVar("mid_att");	// i
	    var_pf_treb_att	= registerVar("treb_att");	// i
	    var_pf_frame    = registerVar("frame");
	    var_pf_decay	= registerVar("decay");
	    var_pf_wave_a	= registerVar("wave_a");
	    var_pf_wave_r	= registerVar("wave_r");
	    var_pf_wave_g	= registerVar("wave_g");
	    var_pf_wave_b	= registerVar("wave_b");
	    var_pf_wave_x	= registerVar("wave_x");
	    var_pf_wave_y	= registerVar("wave_y");
	    var_pf_wave_mystery = registerVar("wave_mystery");
	    var_pf_wave_mode = registerVar("wave_mode");
	    var_pf_q1       = registerVar("q1");
	    var_pf_q2       = registerVar("q2");
	    var_pf_q3       = registerVar("q3");
	    var_pf_q4       = registerVar("q4");
	    var_pf_q5       = registerVar("q5");
	    var_pf_q6       = registerVar("q6");
	    var_pf_q7       = registerVar("q7");
	    var_pf_q8       = registerVar("q8");
	    var_pf_progress = registerVar("progress");
	    var_pf_ob_size	= registerVar("ob_size");
	    var_pf_ob_r		= registerVar("ob_r");
	    var_pf_ob_g		= registerVar("ob_g");
	    var_pf_ob_b		= registerVar("ob_b");
	    var_pf_ob_a		= registerVar("ob_a");
	    var_pf_ib_size	= registerVar("ib_size");
	    var_pf_ib_r		= registerVar("ib_r");
	    var_pf_ib_g		= registerVar("ib_g");
	    var_pf_ib_b		= registerVar("ib_b");
	    var_pf_ib_a		= registerVar("ib_a");
	    var_pf_mv_x		= registerVar("mv_x");
	    var_pf_mv_y		= registerVar("mv_y");
	    var_pf_mv_dx	= registerVar("mv_dx");
	    var_pf_mv_dy	= registerVar("mv_dy");
	    var_pf_mv_l		= registerVar("mv_l");
	    var_pf_mv_r		= registerVar("mv_r");
	    var_pf_mv_g		= registerVar("mv_g");
	    var_pf_mv_b		= registerVar("mv_b");
	    var_pf_mv_a		= registerVar("mv_a");
	    var_pf_monitor  = registerVar("monitor");
	    var_pf_echo_zoom   = registerVar("echo_zoom");
	    var_pf_echo_alpha  = registerVar("echo_alpha");
	    var_pf_echo_orient = registerVar("echo_orient");
        var_pf_wave_usedots  = registerVar("wave_usedots");
        var_pf_wave_thick    = registerVar("wave_thick");
        var_pf_wave_additive = registerVar("wave_additive");
        var_pf_wave_brighten = registerVar("wave_brighten");
        var_pf_darken_center = registerVar("darken_center");
        var_pf_gamma         = registerVar("gamma");
        var_pf_wrap          = registerVar("wrap");
        var_pf_invert        = registerVar("invert");
        var_pf_brighten      = registerVar("brighten");
        var_pf_darken        = registerVar("darken");
        var_pf_solarize      = registerVar("solarize");
        var_pf_meshx         = registerVar("meshx");
        var_pf_meshy         = registerVar("meshy");

	    resetVars(NULL);

	    // this is the list of variables that can be used for a PER-VERTEX calculation:
	    // ('vertex' meaning a vertex on the mesh) (as opposed to a once-per-frame calculation)
	    
        resetVars(m_pv_vars);

        var_pv_zoom		= registerVar("zoom");		// i/o
	    var_pv_zoomexp  = registerVar("zoomexp");	// i/o
	    var_pv_rot		= registerVar("rot");		// i/o
	    var_pv_warp		= registerVar("warp");		// i/o
	    var_pv_cx		= registerVar("cx");		// i/o
	    var_pv_cy		= registerVar("cy");		// i/o
	    var_pv_dx		= registerVar("dx");		// i/o
	    var_pv_dy		= registerVar("dy");		// i/o
	    var_pv_sx		= registerVar("sx");		// i/o
	    var_pv_sy		= registerVar("sy");		// i/o
	    var_pv_time		= registerVar("time");		// i
	    var_pv_fps 		= registerVar("fps");		// i
	    var_pv_bass		= registerVar("bass");		// i
	    var_pv_mid		= registerVar("mid");		// i
	    var_pv_treb		= registerVar("treb");		// i
	    var_pv_bass_att	= registerVar("bass_att");	// i
	    var_pv_mid_att	= registerVar("mid_att");	// i
	    var_pv_treb_att	= registerVar("treb_att");	// i
	    var_pv_frame    = registerVar("frame");
	    var_pv_x		= registerVar("x");			// i
	    var_pv_y		= registerVar("y");			// i
	    var_pv_rad		= registerVar("rad");		// i
	    var_pv_ang		= registerVar("ang");		// i
	    var_pv_q1       = registerVar("q1");
	    var_pv_q2       = registerVar("q2");
	    var_pv_q3       = registerVar("q3");
	    var_pv_q4       = registerVar("q4");
	    var_pv_q5       = registerVar("q5");
	    var_pv_q6       = registerVar("q6");
	    var_pv_q7       = registerVar("q7");
	    var_pv_q8       = registerVar("q8");
	    var_pv_progress = registerVar("progress");
        var_pv_meshx    = registerVar("meshx");
        var_pv_meshy    = registerVar("meshy");
	    resetVars(NULL);
    }

    if (flags & RECOMPILE_WAVE_CODE)
    {
        for (int i=0; i<MAX_CUSTOM_WAVES; i++)
        {
	        resetVars(m_wave[i].m_pf_vars);
	        m_wave[i].var_pf_time		= registerVar("time");		// i
	        m_wave[i].var_pf_fps 		= registerVar("fps");		// i
	        m_wave[i].var_pf_frame      = registerVar("frame");     // i
	        m_wave[i].var_pf_progress   = registerVar("progress");  // i
	        m_wave[i].var_pf_q1         = registerVar("q1");        // i
	        m_wave[i].var_pf_q2         = registerVar("q2");        // i
	        m_wave[i].var_pf_q3         = registerVar("q3");        // i
	        m_wave[i].var_pf_q4         = registerVar("q4");        // i
	        m_wave[i].var_pf_q5         = registerVar("q5");        // i
	        m_wave[i].var_pf_q6         = registerVar("q6");        // i
	        m_wave[i].var_pf_q7         = registerVar("q7");        // i
	        m_wave[i].var_pf_q8         = registerVar("q8");        // i
	        m_wave[i].var_pf_t1         = registerVar("t1");        // i/o
	        m_wave[i].var_pf_t2         = registerVar("t2");        // i/o
	        m_wave[i].var_pf_t3         = registerVar("t3");        // i/o
	        m_wave[i].var_pf_t4         = registerVar("t4");        // i/o
	        m_wave[i].var_pf_t5         = registerVar("t5");        // i/o
	        m_wave[i].var_pf_t6         = registerVar("t6");        // i/o
	        m_wave[i].var_pf_t7         = registerVar("t7");        // i/o
	        m_wave[i].var_pf_t8         = registerVar("t8");        // i/o
	        m_wave[i].var_pf_bass		= registerVar("bass");		// i
	        m_wave[i].var_pf_mid		= registerVar("mid");		// i
	        m_wave[i].var_pf_treb		= registerVar("treb");		// i
	        m_wave[i].var_pf_bass_att	= registerVar("bass_att");	// i
	        m_wave[i].var_pf_mid_att	= registerVar("mid_att");	// i
	        m_wave[i].var_pf_treb_att	= registerVar("treb_att");	// i
	        m_wave[i].var_pf_r          = registerVar("r");         // i/o
	        m_wave[i].var_pf_g          = registerVar("g");         // i/o
	        m_wave[i].var_pf_b          = registerVar("b");         // i/o
	        m_wave[i].var_pf_a          = registerVar("a");         // i/o
	        resetVars(NULL);

	        resetVars(m_wave[i].m_pp_vars);
	        m_wave[i].var_pp_time		= registerVar("time");		// i
	        m_wave[i].var_pp_fps 		= registerVar("fps");		// i
	        m_wave[i].var_pp_frame      = registerVar("frame");     // i
	        m_wave[i].var_pp_progress   = registerVar("progress");  // i
	        m_wave[i].var_pp_q1         = registerVar("q1");        // i
	        m_wave[i].var_pp_q2         = registerVar("q2");        // i
	        m_wave[i].var_pp_q3         = registerVar("q3");        // i
	        m_wave[i].var_pp_q4         = registerVar("q4");        // i
	        m_wave[i].var_pp_q5         = registerVar("q5");        // i
	        m_wave[i].var_pp_q6         = registerVar("q6");        // i
	        m_wave[i].var_pp_q7         = registerVar("q7");        // i
	        m_wave[i].var_pp_q8         = registerVar("q8");        // i
	        m_wave[i].var_pp_t1         = registerVar("t1");        // i
	        m_wave[i].var_pp_t2         = registerVar("t2");        // i
	        m_wave[i].var_pp_t3         = registerVar("t3");        // i
	        m_wave[i].var_pp_t4         = registerVar("t4");        // i
	        m_wave[i].var_pp_t5         = registerVar("t5");        // i
	        m_wave[i].var_pp_t6         = registerVar("t6");        // i
	        m_wave[i].var_pp_t7         = registerVar("t7");        // i
	        m_wave[i].var_pp_t8         = registerVar("t8");        // i
	        m_wave[i].var_pp_bass		= registerVar("bass");		// i
	        m_wave[i].var_pp_mid		= registerVar("mid");		// i
	        m_wave[i].var_pp_treb		= registerVar("treb");		// i
	        m_wave[i].var_pp_bass_att	= registerVar("bass_att");	// i
	        m_wave[i].var_pp_mid_att	= registerVar("mid_att");	// i
	        m_wave[i].var_pp_treb_att	= registerVar("treb_att");	// i
            m_wave[i].var_pp_sample     = registerVar("sample");    // i
            m_wave[i].var_pp_value1     = registerVar("value1");    // i
            m_wave[i].var_pp_value2     = registerVar("value2");    // i
	        m_wave[i].var_pp_x          = registerVar("x");         // i/o
	        m_wave[i].var_pp_y          = registerVar("y");         // i/o
	        m_wave[i].var_pp_r          = registerVar("r");         // i/o
	        m_wave[i].var_pp_g          = registerVar("g");         // i/o
	        m_wave[i].var_pp_b          = registerVar("b");         // i/o
	        m_wave[i].var_pp_a          = registerVar("a");         // i/o
	        resetVars(NULL);
        }
    }

    if (flags & RECOMPILE_SHAPE_CODE)
    {
        for (int i=0; i<MAX_CUSTOM_SHAPES; i++)
        {
	        resetVars(m_shape[i].m_pf_vars);
	        m_shape[i].var_pf_time		= registerVar("time");		// i
	        m_shape[i].var_pf_fps 		= registerVar("fps");		// i
	        m_shape[i].var_pf_frame      = registerVar("frame");     // i
	        m_shape[i].var_pf_progress   = registerVar("progress");  // i
	        m_shape[i].var_pf_q1         = registerVar("q1");        // i
	        m_shape[i].var_pf_q2         = registerVar("q2");        // i
	        m_shape[i].var_pf_q3         = registerVar("q3");        // i
	        m_shape[i].var_pf_q4         = registerVar("q4");        // i
	        m_shape[i].var_pf_q5         = registerVar("q5");        // i
	        m_shape[i].var_pf_q6         = registerVar("q6");        // i
	        m_shape[i].var_pf_q7         = registerVar("q7");        // i
	        m_shape[i].var_pf_q8         = registerVar("q8");        // i
	        m_shape[i].var_pf_t1         = registerVar("t1");        // i/o
	        m_shape[i].var_pf_t2         = registerVar("t2");        // i/o
	        m_shape[i].var_pf_t3         = registerVar("t3");        // i/o
	        m_shape[i].var_pf_t4         = registerVar("t4");        // i/o
	        m_shape[i].var_pf_t5         = registerVar("t5");        // i/o
	        m_shape[i].var_pf_t6         = registerVar("t6");        // i/o
	        m_shape[i].var_pf_t7         = registerVar("t7");        // i/o
	        m_shape[i].var_pf_t8         = registerVar("t8");        // i/o
	        m_shape[i].var_pf_bass		= registerVar("bass");		// i
	        m_shape[i].var_pf_mid		= registerVar("mid");		// i
	        m_shape[i].var_pf_treb		= registerVar("treb");		// i
	        m_shape[i].var_pf_bass_att	= registerVar("bass_att");	// i
	        m_shape[i].var_pf_mid_att	= registerVar("mid_att");	// i
	        m_shape[i].var_pf_treb_att	= registerVar("treb_att");	// i
	        m_shape[i].var_pf_x          = registerVar("x");         // i/o
	        m_shape[i].var_pf_y          = registerVar("y");         // i/o
	        m_shape[i].var_pf_rad        = registerVar("rad");         // i/o
	        m_shape[i].var_pf_ang        = registerVar("ang");         // i/o
	        m_shape[i].var_pf_tex_ang    = registerVar("tex_ang");         // i/o
	        m_shape[i].var_pf_tex_zoom   = registerVar("tex_zoom");         // i/o
	        m_shape[i].var_pf_sides      = registerVar("sides");         // i/o
	        m_shape[i].var_pf_textured   = registerVar("textured");         // i/o
	        m_shape[i].var_pf_additive   = registerVar("additive");         // i/o
	        m_shape[i].var_pf_thick      = registerVar("thick");         // i/o
	        m_shape[i].var_pf_r          = registerVar("r");         // i/o
	        m_shape[i].var_pf_g          = registerVar("g");         // i/o
	        m_shape[i].var_pf_b          = registerVar("b");         // i/o
	        m_shape[i].var_pf_a          = registerVar("a");         // i/o
	        m_shape[i].var_pf_r2         = registerVar("r2");         // i/o
	        m_shape[i].var_pf_g2         = registerVar("g2");         // i/o
	        m_shape[i].var_pf_b2         = registerVar("b2");         // i/o
	        m_shape[i].var_pf_a2         = registerVar("a2");         // i/o
	        m_shape[i].var_pf_border_r   = registerVar("border_r");         // i/o
	        m_shape[i].var_pf_border_g   = registerVar("border_g");         // i/o
	        m_shape[i].var_pf_border_b   = registerVar("border_b");         // i/o
	        m_shape[i].var_pf_border_a   = registerVar("border_a");         // i/o
	        resetVars(NULL);

            /*
	        resetVars(m_shape[i].m_pp_vars);
	        m_shape[i].var_pp_time		= registerVar("time");		// i
	        m_shape[i].var_pp_fps 		= registerVar("fps");		// i
	        m_shape[i].var_pp_frame      = registerVar("frame");     // i
	        m_shape[i].var_pp_progress   = registerVar("progress");  // i
	        m_shape[i].var_pp_q1         = registerVar("q1");        // i
	        m_shape[i].var_pp_q2         = registerVar("q2");        // i
	        m_shape[i].var_pp_q3         = registerVar("q3");        // i
	        m_shape[i].var_pp_q4         = registerVar("q4");        // i
	        m_shape[i].var_pp_q5         = registerVar("q5");        // i
	        m_shape[i].var_pp_q6         = registerVar("q6");        // i
	        m_shape[i].var_pp_q7         = registerVar("q7");        // i
	        m_shape[i].var_pp_q8         = registerVar("q8");        // i
	        m_shape[i].var_pp_t1         = registerVar("t1");        // i/o
	        m_shape[i].var_pp_t2         = registerVar("t2");        // i/o
	        m_shape[i].var_pp_t3         = registerVar("t3");        // i/o
	        m_shape[i].var_pp_t4         = registerVar("t4");        // i/o
	        m_shape[i].var_pp_t5         = registerVar("t5");        // i/o
	        m_shape[i].var_pp_t6         = registerVar("t6");        // i/o
	        m_shape[i].var_pp_t7         = registerVar("t7");        // i/o
	        m_shape[i].var_pp_t8         = registerVar("t8");        // i/o
	        m_shape[i].var_pp_bass		= registerVar("bass");		// i
	        m_shape[i].var_pp_mid		= registerVar("mid");		// i
	        m_shape[i].var_pp_treb		= registerVar("treb");		// i
	        m_shape[i].var_pp_bass_att	= registerVar("bass_att");	// i
	        m_shape[i].var_pp_mid_att	= registerVar("mid_att");	// i
	        m_shape[i].var_pp_treb_att	= registerVar("treb_att");	// i
	        m_shape[i].var_pp_x          = registerVar("x");         // i/o
	        m_shape[i].var_pp_y          = registerVar("y");         // i/o
	        m_shape[i].var_pp_rad        = registerVar("rad");         // i/o
	        m_shape[i].var_pp_ang        = registerVar("ang");         // i/o
	        m_shape[i].var_pp_sides      = registerVar("sides");         // i/o
	        m_shape[i].var_pp_r          = registerVar("r");         // i/o
	        m_shape[i].var_pp_g          = registerVar("g");         // i/o
	        m_shape[i].var_pp_b          = registerVar("b");         // i/o
	        m_shape[i].var_pp_a          = registerVar("a");         // i/o
	        m_shape[i].var_pp_r          = registerVar("r2");         // i/o
	        m_shape[i].var_pp_g          = registerVar("g2");         // i/o
	        m_shape[i].var_pp_b          = registerVar("b2");         // i/o
	        m_shape[i].var_pp_a          = registerVar("a2");         // i/o
	        m_shape[i].var_pp_border_r   = registerVar("border_r");         // i/o
	        m_shape[i].var_pp_border_g   = registerVar("border_g");         // i/o
	        m_shape[i].var_pp_border_b   = registerVar("border_b");         // i/o
	        m_shape[i].var_pp_border_a   = registerVar("border_a");         // i/o
	        resetVars(NULL);
            */
        }
    }
}
void CState::Default()
{
	// DON'T FORGET TO ADD NEW VARIABLES TO BLEND FUNCTION, IMPORT, and EXPORT AS WELL!!!!!!!!

	strcpy(m_szDesc, "<no description>");
	//strcpy(m_szSection, "n/a");

	m_fRating				= 3.0f;
	m_bBlending				= false;

	m_fGammaAdj				= 2.0f;		// 1.0 = reg; +2.0 = double, +3.0 = triple...
	m_fVideoEchoZoom		= 2.0f;
	m_fVideoEchoAlpha		= 0.0f;
	m_nVideoEchoOrientation	= 0;		// 0-3
	
	m_fDecay				= 0.98f;	// 1.0 = none, 0.95 = heavy decay

	m_nWaveMode				= 0;
	m_nOldWaveMode			= -1;
	m_bAdditiveWaves		= false;
	m_fWaveAlpha			= 0.8f;
	m_fWaveScale			= 1.0f;
	m_fWaveSmoothing		= 0.75f;	// 0 = no smoothing, 0.9 = HEAVY smoothing
	m_bWaveDots				= false;
	m_bWaveThick            = false;
	m_fWaveParam			= 0.0f;
	m_bModWaveAlphaByVolume = false;
	m_fModWaveAlphaStart	= 0.75f;		// when relative volume hits this level, alpha -> 0
	m_fModWaveAlphaEnd		= 0.95f;		// when relative volume hits this level, alpha -> 1

	m_fWarpAnimSpeed		= 1.0f;		// additional timescaling for warp animation
	m_fWarpScale			= 1.0f;
	m_fZoomExponent			= 1.0f;
	m_fShader				= 0.0f;
	m_bMaximizeWaveColor	= true;
	m_bTexWrap				= true;
	m_bDarkenCenter			= false;
	m_bRedBlueStereo		= false;
	m_fMvX				  	= 12.0f;
	m_fMvY					= 9.0f;
	m_fMvDX                 = 0.0f;
	m_fMvDY                 = 0.0f;
	m_fMvL				  	= 0.9f;
	m_fMvR                  = 1.0f;
	m_fMvG                  = 1.0f;
	m_fMvB                  = 1.0f;
	m_fMvA                  = 1.0f;
	m_bBrighten				= false;
	m_bDarken				= false;
	m_bSolarize				= false;
	m_bInvert				= false;

	// DON'T FORGET TO ADD NEW VARIABLES TO BLEND FUNCTION, IMPORT, and EXPORT AS WELL!!!!!!!!
	// ALSO BE SURE TO REGISTER THEM ON THE MAIN MENU (SEE MILKDROP.CPP)

	// time-varying variables:   base,  var,   varFreq1, varFreq2
	m_fZoom			= 1.0f;
	m_fRot 			= 0.0f;
	m_fRotCX		= 0.5f;
	m_fRotCY		= 0.5f;
	m_fXPush		= 0.0f;
	m_fYPush		= 0.0f;
	m_fWarpAmount	= 1.0f;
	m_fStretchX     = 1.0f;
	m_fStretchY     = 1.0f;
	m_fWaveR		= 1.0f;
	m_fWaveG		= 1.0f;
	m_fWaveB		= 1.0f;
	m_fWaveX		= 0.5f;
	m_fWaveY		= 0.5f;
	m_fOuterBorderSize = 0.01f;
	m_fOuterBorderR	= 0.0f;
	m_fOuterBorderG	= 0.0f;
	m_fOuterBorderB	= 0.0f;
	m_fOuterBorderA	= 0.0f;
	m_fInnerBorderSize = 0.01f;
	m_fInnerBorderR	= 0.25f;
	m_fInnerBorderG	= 0.25f;
	m_fInnerBorderB	= 0.25f;
	m_fInnerBorderA	= 0.0f;

    for (int i=0; i<MAX_CUSTOM_WAVES; i++)
    {
        m_wave[i].enabled = 0;
        m_wave[i].samples = 512;
        m_wave[i].sep = 0;
        m_wave[i].scaling = 1.0f;
        m_wave[i].smoothing = 0.5f;
        m_wave[i].r = 1.0f;
        m_wave[i].g = 1.0f;
        m_wave[i].b = 1.0f;
        m_wave[i].a = 1.0f;
        m_wave[i].bSpectrum = 0;
        m_wave[i].bUseDots = 0;
        m_wave[i].bDrawThick = 0;
        m_wave[i].bAdditive = 0;
    }

    for (int i=0; i<MAX_CUSTOM_SHAPES; i++)
    {
        m_shape[i].enabled = 0;
        m_shape[i].sides   = 4;
        m_shape[i].additive = 0;
        m_shape[i].thickOutline = 0;
        m_shape[i].textured = 0;
        m_shape[i].tex_zoom = 1.0f;
        m_shape[i].tex_ang  = 0.0f;
        m_shape[i].x = 0.5f;
        m_shape[i].y = 0.5f;
        m_shape[i].rad = 0.1f;
        m_shape[i].ang = 0.0f;
        m_shape[i].r = 1.0f;
        m_shape[i].g = 0.0f;
        m_shape[i].b = 0.0f;
        m_shape[i].a = 1.0f;
        m_shape[i].r2 = 0.0f;
        m_shape[i].g2 = 1.0f;
        m_shape[i].b2 = 0.0f;
        m_shape[i].a2 = 0.0f;
        m_shape[i].border_r = 1.0f;
        m_shape[i].border_g = 1.0f;
        m_shape[i].border_b = 1.0f;
        m_shape[i].border_a = 0.1f;
    }

    // clear all code strings:
    m_szPerFrameInit[0] = 0;
    m_szPerFrameExpr[0] = 0;
    m_szPerPixelExpr[0] = 0;
    for (int i=0; i<MAX_CUSTOM_WAVES; i++)
    {
        m_wave[i].m_szInit[0] = 0;
        m_wave[i].m_szPerFrame[0] = 0;
        m_wave[i].m_szPerPoint[0] = 0;
    }
    for (int i=0; i<MAX_CUSTOM_SHAPES; i++)
    {
        m_shape[i].m_szInit[0] = 0;
        m_shape[i].m_szPerFrame[0] = 0;
        //m_shape[i].m_szPerPoint[0] = 0;
    }
	
	FreeVarsAndCode();
}

void CState::StartBlendFrom(CState *s_from, float fAnimTime, float fTimespan)
{
	CState *s_to = this;

	// bools, ints, and strings instantly change

	s_to->m_fVideoEchoAlphaOld		 = s_from->m_fVideoEchoAlpha.eval(-1);
	s_to->m_nVideoEchoOrientationOld = s_from->m_nVideoEchoOrientation;
	s_to->m_nOldWaveMode			 = s_from->m_nWaveMode;

	/*
	s_to->m_fVideoEchoAlphaOld		 = s_from->m_fVideoEchoAlpha.eval(-1);
	s_to->m_nVideoEchoOrientationOld = s_from->m_nVideoEchoOrientation;

	s_to->m_nOldWaveMode			= s_from->m_nWaveMode;
	s_to->m_nWaveMode				= s_from->m_nWaveMode;
	s_to->m_bAdditiveWaves			= s_from->m_bAdditiveWaves;
	s_to->m_nVideoEchoOrientation	= s_from->m_nVideoEchoOrientation;
	s_to->m_fWarpAnimSpeed			= s_from->m_fWarpAnimSpeed;	// would req. 10 phase-matches to blend this one!!!
	s_to->m_bWaveDots				= s_from->m_bWaveDots;
	s_to->m_bWaveThick				= s_from->m_bWaveThick;
	s_to->m_bModWaveAlphaByVolume	= s_from->m_bModWaveAlphaByVolume;
	s_to->m_bMaximizeWaveColor		= s_from->m_bMaximizeWaveColor;
	s_to->m_bTexWrap				= s_from->m_bTexWrap;			
	s_to->m_bDarkenCenter			= s_from->m_bDarkenCenter;
	s_to->m_bRedBlueStereo			= s_from->m_bRedBlueStereo;
	s_to->m_bBrighten				= s_from->m_bBrighten;
	s_to->m_bDarken					= s_from->m_bDarken;
	s_to->m_bSolarize				= s_from->m_bSolarize;
	s_to->m_bInvert					= s_from->m_bInvert;
	s_to->m_fRating					= s_from->m_fRating;
	*/

	// expr. eval. also copies over immediately (replaces prev.)
	m_bBlending = true;
	m_fBlendStartTime = fAnimTime;
	m_fBlendDuration = fTimespan;
	
	/*
	//for (int e=0; e<MAX_EVALS; e++)
	{
		char szTemp[8192];

		strcpy(szTemp, m_szPerFrameExpr);
		strcpy(m_szPerFrameExpr, s_to->m_szPerFrameExpr);
		strcpy(s_to->m_szPerFrameExpr, szTemp);

		strcpy(szTemp, m_szPerPixelExpr);
		strcpy(m_szPerPixelExpr, s_to->m_szPerPixelExpr);
		strcpy(s_to->m_szPerPixelExpr, szTemp);

		strcpy(szTemp, m_szPerFrameInit);
		strcpy(m_szPerFrameInit, s_to->m_szPerFrameInit);
		strcpy(s_to->m_szPerFrameInit, szTemp);
	}
	RecompileExpressions();
	s_to->RecompileExpressions();

	strcpy(m_szDesc,    s_to->m_szDesc);
	//strcpy(m_szSection, s_to->m_szSection);
	*/
	
	// CBlendableFloats & SuperValues blend over time 
	m_fGammaAdj      .StartBlendFrom(&s_from->m_fGammaAdj      , fAnimTime, fTimespan);
	m_fVideoEchoZoom .StartBlendFrom(&s_from->m_fVideoEchoZoom , fAnimTime, fTimespan);
	m_fVideoEchoAlpha.StartBlendFrom(&s_from->m_fVideoEchoAlpha, fAnimTime, fTimespan);
	m_fDecay         .StartBlendFrom(&s_from->m_fDecay         , fAnimTime, fTimespan);
	m_fWaveAlpha     .StartBlendFrom(&s_from->m_fWaveAlpha     , fAnimTime, fTimespan);
	m_fWaveScale     .StartBlendFrom(&s_from->m_fWaveScale     , fAnimTime, fTimespan);
	m_fWaveSmoothing .StartBlendFrom(&s_from->m_fWaveSmoothing , fAnimTime, fTimespan);
	m_fWaveParam     .StartBlendFrom(&s_from->m_fWaveParam     , fAnimTime, fTimespan);
	m_fWarpScale     .StartBlendFrom(&s_from->m_fWarpScale     , fAnimTime, fTimespan);
	m_fZoomExponent  .StartBlendFrom(&s_from->m_fZoomExponent  , fAnimTime, fTimespan);
	m_fShader        .StartBlendFrom(&s_from->m_fShader        , fAnimTime, fTimespan);
	m_fModWaveAlphaStart.StartBlendFrom(&s_from->m_fModWaveAlphaStart, fAnimTime, fTimespan);
	m_fModWaveAlphaEnd  .StartBlendFrom(&s_from->m_fModWaveAlphaEnd, fAnimTime, fTimespan);

	m_fZoom		.StartBlendFrom(&s_from->m_fZoom		, fAnimTime, fTimespan);
	m_fRot 		.StartBlendFrom(&s_from->m_fRot 		, fAnimTime, fTimespan);
	m_fRotCX	.StartBlendFrom(&s_from->m_fRotCX		, fAnimTime, fTimespan);
	m_fRotCY	.StartBlendFrom(&s_from->m_fRotCY		, fAnimTime, fTimespan);
	m_fXPush	.StartBlendFrom(&s_from->m_fXPush		, fAnimTime, fTimespan);
	m_fYPush	.StartBlendFrom(&s_from->m_fYPush		, fAnimTime, fTimespan);
	m_fWarpAmount.StartBlendFrom(&s_from->m_fWarpAmount,fAnimTime, fTimespan);
	m_fStretchX .StartBlendFrom(&s_from->m_fStretchX	, fAnimTime, fTimespan);
	m_fStretchY .StartBlendFrom(&s_from->m_fStretchY	, fAnimTime, fTimespan);
	m_fWaveR	.StartBlendFrom(&s_from->m_fWaveR		, fAnimTime, fTimespan);
	m_fWaveG	.StartBlendFrom(&s_from->m_fWaveG		, fAnimTime, fTimespan);
	m_fWaveB	.StartBlendFrom(&s_from->m_fWaveB		, fAnimTime, fTimespan);
	m_fWaveX	.StartBlendFrom(&s_from->m_fWaveX		, fAnimTime, fTimespan);
	m_fWaveY	.StartBlendFrom(&s_from->m_fWaveY		, fAnimTime, fTimespan);
	m_fOuterBorderSize	.StartBlendFrom(&s_from->m_fOuterBorderSize	, fAnimTime, fTimespan);
	m_fOuterBorderR		.StartBlendFrom(&s_from->m_fOuterBorderR	, fAnimTime, fTimespan);
	m_fOuterBorderG		.StartBlendFrom(&s_from->m_fOuterBorderG	, fAnimTime, fTimespan);
	m_fOuterBorderB		.StartBlendFrom(&s_from->m_fOuterBorderB	, fAnimTime, fTimespan);
	m_fOuterBorderA		.StartBlendFrom(&s_from->m_fOuterBorderA	, fAnimTime, fTimespan);
	m_fInnerBorderSize	.StartBlendFrom(&s_from->m_fInnerBorderSize	, fAnimTime, fTimespan);
	m_fInnerBorderR		.StartBlendFrom(&s_from->m_fInnerBorderR	, fAnimTime, fTimespan);
	m_fInnerBorderG		.StartBlendFrom(&s_from->m_fInnerBorderG	, fAnimTime, fTimespan);
	m_fInnerBorderB		.StartBlendFrom(&s_from->m_fInnerBorderB	, fAnimTime, fTimespan);
	m_fInnerBorderA		.StartBlendFrom(&s_from->m_fInnerBorderA	, fAnimTime, fTimespan);
	m_fMvX				.StartBlendFrom(&s_from->m_fMvX				, fAnimTime, fTimespan);
	m_fMvY				.StartBlendFrom(&s_from->m_fMvY				, fAnimTime, fTimespan);
	m_fMvDX				.StartBlendFrom(&s_from->m_fMvDX			, fAnimTime, fTimespan);
	m_fMvDY				.StartBlendFrom(&s_from->m_fMvDY			, fAnimTime, fTimespan);
	m_fMvL				.StartBlendFrom(&s_from->m_fMvL				, fAnimTime, fTimespan);
	m_fMvR				.StartBlendFrom(&s_from->m_fMvR				, fAnimTime, fTimespan);
	m_fMvG				.StartBlendFrom(&s_from->m_fMvG				, fAnimTime, fTimespan);
	m_fMvB				.StartBlendFrom(&s_from->m_fMvB				, fAnimTime, fTimespan);
	m_fMvA				.StartBlendFrom(&s_from->m_fMvA				, fAnimTime, fTimespan);

	// if motion vectors were transparent before, don't morph the # in X and Y - just
	// start in the right place, and fade them in.
	bool bOldStateTransparent = (s_from->m_fMvA.eval(-1) < 0.001f);
	bool bNewStateTransparent = (s_to->m_fMvA.eval(-1) < 0.001f);
	if (!bOldStateTransparent && bNewStateTransparent)
	{
		s_from->m_fMvX = s_to->m_fMvX.eval(fAnimTime);
		s_from->m_fMvY = s_to->m_fMvY.eval(fAnimTime);
		s_from->m_fMvDX = s_to->m_fMvDX.eval(fAnimTime);
		s_from->m_fMvDY = s_to->m_fMvDY.eval(fAnimTime);
		s_from->m_fMvL = s_to->m_fMvL.eval(fAnimTime);
		s_from->m_fMvR = s_to->m_fMvR.eval(fAnimTime);
		s_from->m_fMvG = s_to->m_fMvG.eval(fAnimTime);
		s_from->m_fMvB = s_to->m_fMvB.eval(fAnimTime);
	}
	if (bNewStateTransparent && !bOldStateTransparent)
	{
		s_to->m_fMvX = s_from->m_fMvX.eval(fAnimTime);
		s_to->m_fMvY = s_from->m_fMvY.eval(fAnimTime);
		s_to->m_fMvDX = s_from->m_fMvDX.eval(fAnimTime);
		s_to->m_fMvDY = s_from->m_fMvDY.eval(fAnimTime);
		s_to->m_fMvL = s_from->m_fMvL.eval(fAnimTime);
		s_to->m_fMvR = s_from->m_fMvR.eval(fAnimTime);
		s_to->m_fMvG = s_from->m_fMvG.eval(fAnimTime);
		s_to->m_fMvB = s_from->m_fMvB.eval(fAnimTime);
	}

}

void WriteCode(FILE* fOut, int i, char* pStr, char* prefix)
{
	char szLineName[32];
	int line = 1;
	int start_pos = 0;
	int char_pos = 0;

	while (pStr[start_pos] != 0)
	{
		while (	pStr[char_pos] != 0 &&
				pStr[char_pos] != LINEFEED_CONTROL_CHAR)
			char_pos++;

		sprintf(szLineName, "%s%d", prefix, line);

		char ch = pStr[char_pos];
		pStr[char_pos] = 0;
		//if (!WritePrivateProfileString(szSectionName,szLineName,&pStr[start_pos],szIniFile)) return false;
		fprintf(fOut, "%s=%s\n", szLineName, &pStr[start_pos]);
		pStr[char_pos] = ch;

		if (pStr[char_pos] != 0) char_pos++;
		start_pos = char_pos;
		line++;
	}
}

bool CState::Export(char *szSectionName, char *szIniFile)
{
	FILE *fOut = fopen(szIniFile, "w");
	if (!fOut) return false;

	fprintf(fOut, "[%s]\n", szSectionName);

	fprintf(fOut, "%s=%f\n", "fRating",                m_fRating);         
	fprintf(fOut, "%s=%f\n", "fGammaAdj",              m_fGammaAdj.eval(-1));         
	fprintf(fOut, "%s=%f\n", "fDecay",                 m_fDecay.eval(-1));            
	fprintf(fOut, "%s=%f\n", "fVideoEchoZoom",         m_fVideoEchoZoom.eval(-1));    
	fprintf(fOut, "%s=%f\n", "fVideoEchoAlpha",        m_fVideoEchoAlpha.eval(-1));   
	fprintf(fOut, "%s=%d\n", "nVideoEchoOrientation",  m_nVideoEchoOrientation);      

	fprintf(fOut, "%s=%d\n", "nWaveMode",              m_nWaveMode);                  
	fprintf(fOut, "%s=%d\n", "bAdditiveWaves",         m_bAdditiveWaves);             
	fprintf(fOut, "%s=%d\n", "bWaveDots",              m_bWaveDots);                  
	fprintf(fOut, "%s=%d\n", "bWaveThick",             m_bWaveThick);                  
	fprintf(fOut, "%s=%d\n", "bModWaveAlphaByVolume",  m_bModWaveAlphaByVolume);      
	fprintf(fOut, "%s=%d\n", "bMaximizeWaveColor",     m_bMaximizeWaveColor);         
	fprintf(fOut, "%s=%d\n", "bTexWrap",               m_bTexWrap			);         
	fprintf(fOut, "%s=%d\n", "bDarkenCenter",          m_bDarkenCenter		);         
	fprintf(fOut, "%s=%d\n", "bRedBlueStereo",         m_bRedBlueStereo     );
	fprintf(fOut, "%s=%d\n", "bBrighten",              m_bBrighten			);         
	fprintf(fOut, "%s=%d\n", "bDarken",                m_bDarken			);         
	fprintf(fOut, "%s=%d\n", "bSolarize",              m_bSolarize			);         
	fprintf(fOut, "%s=%d\n", "bInvert",                m_bInvert			);         

	fprintf(fOut, "%s=%f\n", "fWaveAlpha",             m_fWaveAlpha.eval(-1)); 		  
	fprintf(fOut, "%s=%f\n", "fWaveScale",             m_fWaveScale.eval(-1));        
	fprintf(fOut, "%s=%f\n", "fWaveSmoothing",         m_fWaveSmoothing.eval(-1));    
	fprintf(fOut, "%s=%f\n", "fWaveParam",             m_fWaveParam.eval(-1));        
	fprintf(fOut, "%s=%f\n", "fModWaveAlphaStart",     m_fModWaveAlphaStart.eval(-1));
	fprintf(fOut, "%s=%f\n", "fModWaveAlphaEnd",       m_fModWaveAlphaEnd.eval(-1));  
	fprintf(fOut, "%s=%f\n", "fWarpAnimSpeed",         m_fWarpAnimSpeed);             
	fprintf(fOut, "%s=%f\n", "fWarpScale",             m_fWarpScale.eval(-1));        
	fprintf(fOut, "%s=%f\n", "fZoomExponent",          m_fZoomExponent.eval(-1));     
	fprintf(fOut, "%s=%f\n", "fShader",                m_fShader.eval(-1));           

	fprintf(fOut, "%s=%f\n", "zoom",                   m_fZoom      .eval(-1));       
	fprintf(fOut, "%s=%f\n", "rot",                    m_fRot       .eval(-1));       
	fprintf(fOut, "%s=%f\n", "cx",                     m_fRotCX     .eval(-1));       
	fprintf(fOut, "%s=%f\n", "cy",                     m_fRotCY     .eval(-1));       
	fprintf(fOut, "%s=%f\n", "dx",                     m_fXPush     .eval(-1));       
	fprintf(fOut, "%s=%f\n", "dy",                     m_fYPush     .eval(-1));       
	fprintf(fOut, "%s=%f\n", "warp",                   m_fWarpAmount.eval(-1));       
	fprintf(fOut, "%s=%f\n", "sx",                     m_fStretchX  .eval(-1));       
	fprintf(fOut, "%s=%f\n", "sy",                     m_fStretchY  .eval(-1));       
	fprintf(fOut, "%s=%f\n", "wave_r",                 m_fWaveR     .eval(-1));       
	fprintf(fOut, "%s=%f\n", "wave_g",                 m_fWaveG     .eval(-1));       
	fprintf(fOut, "%s=%f\n", "wave_b",                 m_fWaveB     .eval(-1));       
	fprintf(fOut, "%s=%f\n", "wave_x",                 m_fWaveX     .eval(-1));       
	fprintf(fOut, "%s=%f\n", "wave_y",                 m_fWaveY     .eval(-1));       

	fprintf(fOut, "%s=%f\n", "ob_size",             m_fOuterBorderSize.eval(-1));       
	fprintf(fOut, "%s=%f\n", "ob_r",                m_fOuterBorderR.eval(-1));       
	fprintf(fOut, "%s=%f\n", "ob_g",                m_fOuterBorderG.eval(-1));       
	fprintf(fOut, "%s=%f\n", "ob_b",                m_fOuterBorderB.eval(-1));       
	fprintf(fOut, "%s=%f\n", "ob_a",                m_fOuterBorderA.eval(-1));       
	fprintf(fOut, "%s=%f\n", "ib_size",             m_fInnerBorderSize.eval(-1));       
	fprintf(fOut, "%s=%f\n", "ib_r",                m_fInnerBorderR.eval(-1));       
	fprintf(fOut, "%s=%f\n", "ib_g",                m_fInnerBorderG.eval(-1));       
	fprintf(fOut, "%s=%f\n", "ib_b",                m_fInnerBorderB.eval(-1));       
	fprintf(fOut, "%s=%f\n", "ib_a",                m_fInnerBorderA.eval(-1));       
	fprintf(fOut, "%s=%f\n", "nMotionVectorsX",     m_fMvX.eval(-1));         
	fprintf(fOut, "%s=%f\n", "nMotionVectorsY",     m_fMvY.eval(-1));         
	fprintf(fOut, "%s=%f\n", "mv_dx",               m_fMvDX.eval(-1));         
	fprintf(fOut, "%s=%f\n", "mv_dy",               m_fMvDY.eval(-1));         
	fprintf(fOut, "%s=%f\n", "mv_l",                m_fMvL.eval(-1));         
	fprintf(fOut, "%s=%f\n", "mv_r",                m_fMvR.eval(-1));       
	fprintf(fOut, "%s=%f\n", "mv_g",                m_fMvG.eval(-1));       
	fprintf(fOut, "%s=%f\n", "mv_b",                m_fMvB.eval(-1));       
	fprintf(fOut, "%s=%f\n", "mv_a",                m_fMvA.eval(-1));

  int i=0;

    for (i=0; i<MAX_CUSTOM_WAVES; i++)
        m_wave[i].Export("", szIniFile, i, fOut);

    for (i=0; i<MAX_CUSTOM_SHAPES; i++)
        m_shape[i].Export("", szIniFile, i, fOut);

	// write out arbitrary expressions, one line at a time
    WriteCode(fOut, i, m_szPerFrameInit, "per_frame_init_");
    WriteCode(fOut, i, m_szPerFrameExpr, "per_frame_"); 
    WriteCode(fOut, i, m_szPerPixelExpr, "per_pixel_"); 

    /*
    int n2 = 3 + MAX_CUSTOM_WAVES*3 + MAX_CUSTOM_SHAPES*2;
	for (int n=0; n<n2; n++)
	{
		char *pStr;
        char prefix[64];
		switch(n)
		{
		case 0: pStr = m_szPerFrameExpr; strcpy(prefix, "per_frame_"); break;
		case 1: pStr = m_szPerPixelExpr; strcpy(prefix, "per_pixel_"); break;
		case 2: pStr = m_szPerFrameInit; strcpy(prefix, "per_frame_init_"); break;
        default:
            if (n < 3 + 3*MAX_CUSTOM_WAVES)
            {
                int i = (n-3) / 3;
                int j = (n-3) % 3;
                switch(j)
                {
                case 0: pStr = m_wave[i].m_szInit;     sprintf(prefix, "wave_%d_init",      i); break;
                case 1: pStr = m_wave[i].m_szPerFrame; sprintf(prefix, "wave_%d_per_frame", i); break;
                case 2: pStr = m_wave[i].m_szPerPoint; sprintf(prefix, "wave_%d_per_point", i); break;
                }
            }
            else
            {
                int i = (n-3-3*MAX_CUSTOM_WAVES) / 2;
                int j = (n-3-3*MAX_CUSTOM_WAVES) % 2;
                switch(j)
                {
                case 0: pStr = m_shape[i].m_szInit;     sprintf(prefix, "shape_%d_init",      i); break;
                case 1: pStr = m_shape[i].m_szPerFrame; sprintf(prefix, "shape_%d_per_frame", i); break;
                }
            }
		}

		char szLineName[32];
		int line = 1;
		int start_pos = 0;
		int char_pos = 0;

		while (pStr[start_pos] != 0)
		{
			while (	pStr[char_pos] != 0 &&
					pStr[char_pos] != LINEFEED_CONTROL_CHAR)
				char_pos++;

			sprintf(szLineName, "%s%d", prefix, line);

			char ch = pStr[char_pos];
			pStr[char_pos] = 0;
			//if (!WritePrivateProfileString(szSectionName,szLineName,&pStr[start_pos],szIniFile)) return false;
			fprintf(fOut, "%s=%s\n", szLineName, &pStr[start_pos]);
			pStr[char_pos] = ch;

			if (pStr[char_pos] != 0) char_pos++;
			start_pos = char_pos;
			line++;
		}
	}	
    */

	fclose(fOut);

	return true;
}

int  CWave::Export(char* szSection, char *szFile, int i, FILE* fOut)
{
    FILE* f2 = fOut;
    if (!fOut)
    {
	    f2 = fopen(szFile, "w");
        if (!f2) return 0;
	    fprintf(f2, "[%s]\n", szSection);
    }

	fprintf(f2, "wavecode_%d_%s=%d\n", i, "enabled",    enabled);
	fprintf(f2, "wavecode_%d_%s=%d\n", i, "samples",    samples);
	fprintf(f2, "wavecode_%d_%s=%d\n", i, "sep",        sep    );
	fprintf(f2, "wavecode_%d_%s=%d\n", i, "bSpectrum",  bSpectrum);
	fprintf(f2, "wavecode_%d_%s=%d\n", i, "bUseDots",   bUseDots);
	fprintf(f2, "wavecode_%d_%s=%d\n", i, "bDrawThick", bDrawThick);
	fprintf(f2, "wavecode_%d_%s=%d\n", i, "bAdditive",  bAdditive);
	fprintf(f2, "wavecode_%d_%s=%f\n", i, "scaling",    scaling);
	fprintf(f2, "wavecode_%d_%s=%f\n", i, "smoothing",  smoothing);
	fprintf(f2, "wavecode_%d_%s=%f\n", i, "r",          r);
	fprintf(f2, "wavecode_%d_%s=%f\n", i, "g",          g);
	fprintf(f2, "wavecode_%d_%s=%f\n", i, "b",          b);
	fprintf(f2, "wavecode_%d_%s=%f\n", i, "a",          a);

    // READ THE CODE IN
    char prefix[64];
    sprintf(prefix, "wave_%d_init",      i); WriteCode(f2, i, m_szInit,     prefix);
    sprintf(prefix, "wave_%d_per_frame", i); WriteCode(f2, i, m_szPerFrame, prefix);
    sprintf(prefix, "wave_%d_per_point", i); WriteCode(f2, i, m_szPerPoint, prefix);

    if (!fOut)
	    fclose(f2); // [sic]

    return 1;
}

int  CShape::Export(char* szSection, char *szFile, int i, FILE* fOut)
{
    FILE* f2 = fOut;
    if (!fOut)
    {
	    f2 = fopen(szFile, "w");
        if (!f2) return 0;
	    fprintf(f2, "[%s]\n", szSection);
    }

	fprintf(f2, "shapecode_%d_%s=%d\n", i, "enabled",    enabled);
	fprintf(f2, "shapecode_%d_%s=%d\n", i, "sides",      sides);
	fprintf(f2, "shapecode_%d_%s=%d\n", i, "additive",   additive);
	fprintf(f2, "shapecode_%d_%s=%d\n", i, "thickOutline",thickOutline);
	fprintf(f2, "shapecode_%d_%s=%d\n", i, "textured",   textured);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "x",          x);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "y",          y);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "rad",        rad);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "ang",        ang);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "tex_ang",    tex_ang);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "tex_zoom",   tex_zoom);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "r",          r);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "g",          g);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "b",          b);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "a",          a);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "r2",         r2);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "g2",         g2);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "b2",         b2);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "a2",         a2);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "border_r",   border_r);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "border_g",   border_g);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "border_b",   border_b);
	fprintf(f2, "shapecode_%d_%s=%f\n", i, "border_a",   border_a);

    char prefix[64];
    sprintf(prefix, "shape_%d_init",      i); WriteCode(f2, i, m_szInit,     prefix);
    sprintf(prefix, "shape_%d_per_frame", i); WriteCode(f2, i, m_szPerFrame, prefix);
    //sprintf(prefix, "shape_%d_per_point", i); WriteCode(f2, i, m_szPerPoint, prefix);

    if (!fOut)
	    fclose(f2); // [sic]

    return 1;
}

void ReadCode(char* szSectionName, char* szIniFile, char* pStr, char* prefix)
{
	// read in & compile arbitrary expressions
	char szLineName[32];
	char szLine[8192];
	int len;

	int line = 1;
	int char_pos = 0;
	bool bDone = false;

	while (!bDone)
	{
		sprintf(szLineName, "%s%d", prefix, line); 

		InternalGetPrivateProfileString(szSectionName, szLineName, "~!@#$", szLine, 8192, szIniFile);	// fixme
		len = strlen(szLine);

		if ((strcmp(szLine, "~!@#$")==0) ||		// if the key was missing,
			(len >= 8191-char_pos-1))			// or if we're out of space
		{
			bDone = true;
		}
		else 
		{
			sprintf(&pStr[char_pos], "%s%c", szLine, LINEFEED_CONTROL_CHAR);
		}
	
		char_pos += len + 1;
		line++;
	}
	pStr[char_pos++] = 0;	// null-terminate

	// read in & compile arbitrary expressions
    /*
    int n2 = 3 + MAX_CUSTOM_WAVES*3 + MAX_CUSTOM_SHAPES*2;
	for (int n=0; n<n2; n++)
	{
		char *pStr;
        char prefix[64];
		char szLineName[32];
		char szLine[8192];
		int len;

		int line = 1;
		int char_pos = 0;
		bool bDone = false;

		switch(n)
		{
		case 0: pStr = m_szPerFrameExpr; strcpy(prefix, "per_frame_"); break;
		case 1: pStr = m_szPerPixelExpr; strcpy(prefix, "per_pixel_"); break;
		case 2: pStr = m_szPerFrameInit; strcpy(prefix, "per_frame_init_"); break;
        default:
            if (n < 3 + 3*MAX_CUSTOM_WAVES)
            {
                int i = (n-3) / 3;
                int j = (n-3) % 3;
                switch(j)
                {
                case 0: pStr = m_wave[i].m_szInit;     sprintf(prefix, "wave_%d_init",      i); break;
                case 1: pStr = m_wave[i].m_szPerFrame; sprintf(prefix, "wave_%d_per_frame", i); break;
                case 2: pStr = m_wave[i].m_szPerPoint; sprintf(prefix, "wave_%d_per_point", i); break;
                }
            }
            else
            {
                int i = (n-3-3*MAX_CUSTOM_WAVES) / 2;
                int j = (n-3-3*MAX_CUSTOM_WAVES) % 2;
                switch(j)
                {
                case 0: pStr = m_shape[i].m_szInit;     sprintf(prefix, "shape_%d_init",      i); break;
                case 1: pStr = m_shape[i].m_szPerFrame; sprintf(prefix, "shape_%d_per_frame", i); break;
                }
            }
		}
		
		while (!bDone)
		{
			sprintf(szLineName, "%s%d", prefix, line); 

			InternalGetPrivateProfileString(szSectionName, szLineName, "~!@#$", szLine, 8192, szIniFile);	// fixme
			len = strlen(szLine);

			if ((strcmp(szLine, "~!@#$")==0) ||		// if the key was missing,
				(len >= 8191-char_pos-1))			// or if we're out of space
			{
				bDone = true;
			}
			else 
			{
				sprintf(&pStr[char_pos], "%s%c", szLine, LINEFEED_CONTROL_CHAR);
			}
		
			char_pos += len + 1;
			line++;
		}
		pStr[char_pos++] = 0;	// null-terminate
	}
    */
}

int CWave::Import(char* szSectionName, char *szIniFile, int i)
{
//    if (GetFileAttributes(szIniFile)==0xFFFFFFFF)
//        return 0;

    char buf[64];
    sprintf(buf, "wavecode_%d_%s", i, "enabled"   ); enabled    = InternalGetPrivateProfileInt  (szSectionName, buf, enabled   , szIniFile);
    sprintf(buf, "wavecode_%d_%s", i, "samples"   ); samples    = InternalGetPrivateProfileInt  (szSectionName, buf, samples   , szIniFile);
    sprintf(buf, "wavecode_%d_%s", i, "sep"       ); sep        = InternalGetPrivateProfileInt  (szSectionName, buf, sep       , szIniFile);
    sprintf(buf, "wavecode_%d_%s", i, "bSpectrum" ); bSpectrum  = InternalGetPrivateProfileInt  (szSectionName, buf, bSpectrum , szIniFile);
    sprintf(buf, "wavecode_%d_%s", i, "bUseDots"  ); bUseDots   = InternalGetPrivateProfileInt  (szSectionName, buf, bUseDots  , szIniFile);
    sprintf(buf, "wavecode_%d_%s", i, "bDrawThick"); bDrawThick = InternalGetPrivateProfileInt  (szSectionName, buf, bDrawThick, szIniFile);
    sprintf(buf, "wavecode_%d_%s", i, "bAdditive" ); bAdditive  = InternalGetPrivateProfileInt  (szSectionName, buf, bAdditive , szIniFile);
    sprintf(buf, "wavecode_%d_%s", i, "scaling"   ); scaling    = InternalGetPrivateProfileFloat(szSectionName, buf, scaling   , szIniFile);
    sprintf(buf, "wavecode_%d_%s", i, "smoothing" ); smoothing  = InternalGetPrivateProfileFloat(szSectionName, buf, smoothing , szIniFile);
    sprintf(buf, "wavecode_%d_%s", i, "r"         ); r          = InternalGetPrivateProfileFloat(szSectionName, buf, r         , szIniFile);
    sprintf(buf, "wavecode_%d_%s", i, "g"         ); g          = InternalGetPrivateProfileFloat(szSectionName, buf, g         , szIniFile);
    sprintf(buf, "wavecode_%d_%s", i, "b"         ); b          = InternalGetPrivateProfileFloat(szSectionName, buf, b         , szIniFile);
    sprintf(buf, "wavecode_%d_%s", i, "a"         ); a          = InternalGetPrivateProfileFloat(szSectionName, buf, a         , szIniFile);

    // READ THE CODE IN
    char prefix[64];
    sprintf(prefix, "wave_%d_init",      i); ReadCode(szSectionName, szIniFile, m_szInit,     prefix);
    sprintf(prefix, "wave_%d_per_frame", i); ReadCode(szSectionName, szIniFile, m_szPerFrame, prefix);
    sprintf(prefix, "wave_%d_per_point", i); ReadCode(szSectionName, szIniFile, m_szPerPoint, prefix);

    return 1;
}

int  CShape::Import(char* szSectionName, char *szIniFile, int i)
{
//    if (GetFileAttributes(szIniFile)==0xFFFFFFFF)
//        return 0;

    char buf[64];
	sprintf(buf, "shapecode_%d_%s", i, "enabled"     ); enabled      = InternalGetPrivateProfileInt  (szSectionName, buf, enabled     , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "sides"       ); sides        = InternalGetPrivateProfileInt  (szSectionName, buf, sides       , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "additive"    ); additive     = InternalGetPrivateProfileInt  (szSectionName, buf, additive    , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "thickOutline"); thickOutline = InternalGetPrivateProfileInt  (szSectionName, buf, thickOutline, szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "textured"    ); textured     = InternalGetPrivateProfileInt  (szSectionName, buf, textured    , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "x"           ); x            = InternalGetPrivateProfileFloat(szSectionName, buf, x           , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "y"           ); y            = InternalGetPrivateProfileFloat(szSectionName, buf, y           , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "rad"         ); rad          = InternalGetPrivateProfileFloat(szSectionName, buf, rad         , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "ang"         ); ang          = InternalGetPrivateProfileFloat(szSectionName, buf, ang         , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "tex_ang"     ); tex_ang      = InternalGetPrivateProfileFloat(szSectionName, buf, tex_ang     , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "tex_zoom"    ); tex_zoom     = InternalGetPrivateProfileFloat(szSectionName, buf, tex_zoom    , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "r"           ); r            = InternalGetPrivateProfileFloat(szSectionName, buf, r           , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "g"           ); g            = InternalGetPrivateProfileFloat(szSectionName, buf, g           , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "b"           ); b            = InternalGetPrivateProfileFloat(szSectionName, buf, b           , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "a"           ); a            = InternalGetPrivateProfileFloat(szSectionName, buf, a           , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "r2"          ); r2           = InternalGetPrivateProfileFloat(szSectionName, buf, r2          , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "g2"          ); g2           = InternalGetPrivateProfileFloat(szSectionName, buf, g2          , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "b2"          ); b2           = InternalGetPrivateProfileFloat(szSectionName, buf, b2          , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "a2"          ); a2           = InternalGetPrivateProfileFloat(szSectionName, buf, a2          , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "border_r"    ); border_r     = InternalGetPrivateProfileFloat(szSectionName, buf, border_r    , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "border_g"    ); border_g     = InternalGetPrivateProfileFloat(szSectionName, buf, border_g    , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "border_b"    ); border_b     = InternalGetPrivateProfileFloat(szSectionName, buf, border_b    , szIniFile);
	sprintf(buf, "shapecode_%d_%s", i, "border_a"    ); border_a     = InternalGetPrivateProfileFloat(szSectionName, buf, border_a    , szIniFile);

    // READ THE CODE IN
    char prefix[64];
    sprintf(prefix, "shape_%d_init",      i); ReadCode(szSectionName, szIniFile, m_szInit,     prefix);
    sprintf(prefix, "shape_%d_per_frame", i); ReadCode(szSectionName, szIniFile, m_szPerFrame, prefix);

    return 1;
}

void CState::Import(char *szSectionName, char *szIniFile)
{

	Default();

	//strcpy(m_szSection, szSectionName);
	//InternalGetPrivateProfileString(szSectionName, "szDesc", "<no description>", m_szDesc, sizeof(m_szDesc), szIniFile);
	
	// extract a description of the preset from the filename
	{
		// copy get the filename (without the path)
		char *p = strrchr(szIniFile, '/');
		if (p==NULL) p = szIniFile;
		strcpy(m_szDesc, p+1);		

		// next remove the extension
		RemoveExtension(m_szDesc);
	}
	
	m_fRating				= InternalGetPrivateProfileFloat(szSectionName,"fRating",m_fRating,szIniFile);
	m_fDecay                = InternalGetPrivateProfileFloat(szSectionName,"fDecay",m_fDecay.eval(-1),szIniFile);
	m_fGammaAdj             = InternalGetPrivateProfileFloat(szSectionName,"fGammaAdj" ,m_fGammaAdj.eval(-1),szIniFile);
	m_fVideoEchoZoom        = InternalGetPrivateProfileFloat(szSectionName,"fVideoEchoZoom",m_fVideoEchoZoom.eval(-1),szIniFile);
	m_fVideoEchoAlpha       = InternalGetPrivateProfileFloat(szSectionName,"fVideoEchoAlpha",m_fVideoEchoAlpha.eval(-1),szIniFile);
	m_nVideoEchoOrientation = InternalGetPrivateProfileInt  (szSectionName,"nVideoEchoOrientation",m_nVideoEchoOrientation,szIniFile);

	m_nWaveMode             = InternalGetPrivateProfileInt  (szSectionName,"nWaveMode",m_nWaveMode,szIniFile);
	m_bAdditiveWaves		= (InternalGetPrivateProfileInt (szSectionName,"bAdditiveWaves",m_bAdditiveWaves,szIniFile) != 0);
	m_bWaveDots		        = (InternalGetPrivateProfileInt (szSectionName,"bWaveDots",m_bWaveDots,szIniFile) != 0);
	m_bWaveThick            = (InternalGetPrivateProfileInt (szSectionName,"bWaveThick",m_bWaveThick,szIniFile) != 0);
	m_bModWaveAlphaByVolume	= (InternalGetPrivateProfileInt (szSectionName,"bModWaveAlphaByVolume",m_bModWaveAlphaByVolume,szIniFile) != 0);
	m_bMaximizeWaveColor    = (InternalGetPrivateProfileInt (szSectionName,"bMaximizeWaveColor" ,m_bMaximizeWaveColor,szIniFile) != 0);
	m_bTexWrap			    = (InternalGetPrivateProfileInt (szSectionName,"bTexWrap", m_bTexWrap,szIniFile) != 0);
	m_bDarkenCenter			= (InternalGetPrivateProfileInt (szSectionName,"bDarkenCenter", m_bDarkenCenter,szIniFile) != 0);
	m_bRedBlueStereo        = (InternalGetPrivateProfileInt (szSectionName,"bRedBlueStereo", m_bRedBlueStereo,szIniFile) != 0);
	m_bBrighten				= (InternalGetPrivateProfileInt (szSectionName,"bBrighten",m_bBrighten	,szIniFile) != 0);
	m_bDarken				= (InternalGetPrivateProfileInt (szSectionName,"bDarken"  ,m_bDarken	,szIniFile) != 0);
	m_bSolarize				= (InternalGetPrivateProfileInt (szSectionName,"bSolarize",m_bSolarize	,szIniFile) != 0);
	m_bInvert				= (InternalGetPrivateProfileInt (szSectionName,"bInvert"  ,m_bInvert	,szIniFile) != 0);

	m_fWaveAlpha            = InternalGetPrivateProfileFloat(szSectionName,"fWaveAlpha",m_fWaveAlpha.eval(-1),szIniFile);
	m_fWaveScale            = InternalGetPrivateProfileFloat(szSectionName,"fWaveScale",m_fWaveScale.eval(-1),szIniFile);
	m_fWaveSmoothing        = InternalGetPrivateProfileFloat(szSectionName,"fWaveSmoothing",m_fWaveSmoothing.eval(-1),szIniFile);
	m_fWaveParam            = InternalGetPrivateProfileFloat(szSectionName,"fWaveParam",m_fWaveParam.eval(-1),szIniFile);
	m_fModWaveAlphaStart    = InternalGetPrivateProfileFloat(szSectionName,"fModWaveAlphaStart",m_fModWaveAlphaStart.eval(-1),szIniFile);
	m_fModWaveAlphaEnd      = InternalGetPrivateProfileFloat(szSectionName,"fModWaveAlphaEnd",m_fModWaveAlphaEnd.eval(-1),szIniFile);
	m_fWarpAnimSpeed        = InternalGetPrivateProfileFloat(szSectionName,"fWarpAnimSpeed",m_fWarpAnimSpeed,szIniFile);
	m_fWarpScale            = InternalGetPrivateProfileFloat(szSectionName,"fWarpScale",m_fWarpScale.eval(-1),szIniFile);
	m_fZoomExponent         = InternalGetPrivateProfileFloat(szSectionName,"fZoomExponent",m_fZoomExponent.eval(-1),szIniFile);
	m_fShader               = InternalGetPrivateProfileFloat(szSectionName,"fShader",m_fShader.eval(-1),szIniFile);

	m_fZoom					= InternalGetPrivateProfileFloat(szSectionName,"zoom",m_fZoom.eval(-1),szIniFile);	
	m_fRot					= InternalGetPrivateProfileFloat(szSectionName,"rot",m_fRot.eval(-1),szIniFile);	
	m_fRotCX				= InternalGetPrivateProfileFloat(szSectionName,"cx",m_fRotCX.eval(-1),szIniFile);	
	m_fRotCY				= InternalGetPrivateProfileFloat(szSectionName,"cy",m_fRotCY.eval(-1),szIniFile);	
	m_fXPush				= InternalGetPrivateProfileFloat(szSectionName,"dx",m_fXPush.eval(-1),szIniFile);	
	m_fYPush				= InternalGetPrivateProfileFloat(szSectionName,"dy",m_fYPush.eval(-1),szIniFile);	
	m_fWarpAmount			= InternalGetPrivateProfileFloat(szSectionName,"warp",m_fWarpAmount.eval(-1),szIniFile);	
	m_fStretchX				= InternalGetPrivateProfileFloat(szSectionName,"sx",m_fStretchX.eval(-1),szIniFile);	
	m_fStretchY				= InternalGetPrivateProfileFloat(szSectionName,"sy",m_fStretchY.eval(-1),szIniFile);	
	m_fWaveR				= InternalGetPrivateProfileFloat(szSectionName,"wave_r",m_fRot.eval(-1),szIniFile);	
	m_fWaveG				= InternalGetPrivateProfileFloat(szSectionName,"wave_g",m_fRot.eval(-1),szIniFile);	
	m_fWaveB				= InternalGetPrivateProfileFloat(szSectionName,"wave_b",m_fRot.eval(-1),szIniFile);	
	m_fWaveX				= InternalGetPrivateProfileFloat(szSectionName,"wave_x",m_fRot.eval(-1),szIniFile);	
	m_fWaveY				= InternalGetPrivateProfileFloat(szSectionName,"wave_y",m_fRot.eval(-1),szIniFile);	

	m_fOuterBorderSize	= InternalGetPrivateProfileFloat(szSectionName,"ob_size",m_fOuterBorderSize.eval(-1),szIniFile);	
	m_fOuterBorderR		= InternalGetPrivateProfileFloat(szSectionName,"ob_r",   m_fOuterBorderR.eval(-1),szIniFile);	
	m_fOuterBorderG		= InternalGetPrivateProfileFloat(szSectionName,"ob_g",   m_fOuterBorderG.eval(-1),szIniFile);	
	m_fOuterBorderB		= InternalGetPrivateProfileFloat(szSectionName,"ob_b",   m_fOuterBorderB.eval(-1),szIniFile);	
	m_fOuterBorderA		= InternalGetPrivateProfileFloat(szSectionName,"ob_a",   m_fOuterBorderA.eval(-1),szIniFile);	
	m_fInnerBorderSize	= InternalGetPrivateProfileFloat(szSectionName,"ib_size",m_fInnerBorderSize.eval(-1),szIniFile);	
	m_fInnerBorderR		= InternalGetPrivateProfileFloat(szSectionName,"ib_r",   m_fInnerBorderR.eval(-1),szIniFile);	
	m_fInnerBorderG		= InternalGetPrivateProfileFloat(szSectionName,"ib_g",   m_fInnerBorderG.eval(-1),szIniFile);	
	m_fInnerBorderB		= InternalGetPrivateProfileFloat(szSectionName,"ib_b",   m_fInnerBorderB.eval(-1),szIniFile);	
	m_fInnerBorderA		= InternalGetPrivateProfileFloat(szSectionName,"ib_a",   m_fInnerBorderA.eval(-1),szIniFile);	
	m_fMvX				= InternalGetPrivateProfileFloat(szSectionName,"nMotionVectorsX",  m_fMvX.eval(-1),szIniFile);
	m_fMvY           	= InternalGetPrivateProfileFloat(szSectionName,"nMotionVectorsY",  m_fMvY.eval(-1),szIniFile);
	m_fMvDX				= InternalGetPrivateProfileFloat(szSectionName,"mv_dx",  m_fMvDX.eval(-1),szIniFile);
	m_fMvDY				= InternalGetPrivateProfileFloat(szSectionName,"mv_dy",  m_fMvDY.eval(-1),szIniFile);
	m_fMvL				= InternalGetPrivateProfileFloat(szSectionName,"mv_l",   m_fMvL.eval(-1),szIniFile);
	m_fMvR				= InternalGetPrivateProfileFloat(szSectionName,"mv_r",   m_fMvR.eval(-1),szIniFile);	
	m_fMvG				= InternalGetPrivateProfileFloat(szSectionName,"mv_g",   m_fMvG.eval(-1),szIniFile);	
	m_fMvB				= InternalGetPrivateProfileFloat(szSectionName,"mv_b",   m_fMvB.eval(-1),szIniFile);	
	m_fMvA				= (InternalGetPrivateProfileInt (szSectionName,"bMotionVectorsOn",false,szIniFile) == 0) ? 0.0f : 1.0f; // for backwards compatibility
	m_fMvA				= InternalGetPrivateProfileFloat(szSectionName,"mv_a",   m_fMvA.eval(-1),szIniFile);	

    for (int i=0; i<MAX_CUSTOM_WAVES; i++)
    {
        m_wave[i].Import(szSectionName, szIniFile, i);
    }

    for (int i=0; i<MAX_CUSTOM_SHAPES; i++)
    {
        m_shape[i].Import(szSectionName, szIniFile, i);
    }

    ReadCode(szSectionName, szIniFile, m_szPerFrameInit, "per_frame_init_");
    ReadCode(szSectionName, szIniFile, m_szPerFrameExpr, "per_frame_");
    ReadCode(szSectionName, szIniFile, m_szPerPixelExpr, "per_pixel_");

    /*
	// read in & compile arbitrary expressions
    int n2 = 3 + MAX_CUSTOM_WAVES*3 + MAX_CUSTOM_SHAPES*2;
	for (int n=0; n<n2; n++)
	{
		char *pStr;
        char prefix[64];
		char szLineName[32];
		char szLine[8192];
		int len;

		int line = 1;
		int char_pos = 0;
		bool bDone = false;

		switch(n)
		{
		case 0: pStr = m_szPerFrameExpr; strcpy(prefix, "per_frame_"); break;
		case 1: pStr = m_szPerPixelExpr; strcpy(prefix, "per_pixel_"); break;
		case 2: pStr = m_szPerFrameInit; strcpy(prefix, "per_frame_init_"); break;
        default:
            if (n < 3 + 3*MAX_CUSTOM_WAVES)
            {
                int i = (n-3) / 3;
                int j = (n-3) % 3;
                switch(j)
                {
                case 0: pStr = m_wave[i].m_szInit;     sprintf(prefix, "wave_%d_init",      i); break;
                case 1: pStr = m_wave[i].m_szPerFrame; sprintf(prefix, "wave_%d_per_frame", i); break;
                case 2: pStr = m_wave[i].m_szPerPoint; sprintf(prefix, "wave_%d_per_point", i); break;
                }
            }
            else
            {
                int i = (n-3-3*MAX_CUSTOM_WAVES) / 2;
                int j = (n-3-3*MAX_CUSTOM_WAVES) % 2;
                switch(j)
                {
                case 0: pStr = m_shape[i].m_szInit;     sprintf(prefix, "shape_%d_init",      i); break;
                case 1: pStr = m_shape[i].m_szPerFrame; sprintf(prefix, "shape_%d_per_frame", i); break;
                }
            }
		}
		
		while (!bDone)
		{
			sprintf(szLineName, "%s%d", prefix, line); 

			InternalGetPrivateProfileString(szSectionName, szLineName, "~!@#$", szLine, 8192, szIniFile);	// fixme
			len = strlen(szLine);

			if ((strcmp(szLine, "~!@#$")==0) ||		// if the key was missing,
				(len >= 8191-char_pos-1))			// or if we're out of space
			{
				bDone = true;
			}
			else 
			{
				sprintf(&pStr[char_pos], "%s%c", szLine, LINEFEED_CONTROL_CHAR);
			}
		
			char_pos += len + 1;
			line++;
		}
		pStr[char_pos++] = 0;	// null-terminate
	}
    */

	RecompileExpressions();
}

void CState::FreeVarsAndCode()
{
	// free the compiled expressions
	if (m_pf_codehandle)
	{
		freeCode(m_pf_codehandle);
		m_pf_codehandle = NULL;
	}
	if (m_pp_codehandle)
	{
		freeCode(m_pp_codehandle);
		m_pp_codehandle = NULL;
	}

    for (int i=0; i<MAX_CUSTOM_WAVES; i++)
    {
	    if (m_wave[i].m_pf_codehandle)
        {
            freeCode(m_wave[i].m_pf_codehandle);
            m_wave[i].m_pf_codehandle = NULL;
        }
	    if (m_wave[i].m_pp_codehandle)
        {
            freeCode(m_wave[i].m_pp_codehandle);
            m_wave[i].m_pp_codehandle = NULL;
        }
    }

    for (int i=0; i<MAX_CUSTOM_SHAPES; i++)
    {
	    if (m_shape[i].m_pf_codehandle)
        {
            freeCode(m_shape[i].m_pf_codehandle);
            m_shape[i].m_pf_codehandle = NULL;
        }
	    /*if (m_shape[i].m_pp_codehandle)
        {
            freeCode(m_shape[i].m_pp_codehandle);
            m_shape[i].m_pp_codehandle = NULL;
        }*/
    }

	// free our text version of the expressions? - no!
	//m_szPerFrameExpr[0] = 0;
	//m_szPerPixelExpr[0] = 0;

	// free the old variable names & reregister the built-in variables (since they got nuked too)
	memset(m_pv_vars, 0, sizeof(varType)*EVAL_MAX_VARS);
	memset(m_pf_vars, 0, sizeof(varType)*EVAL_MAX_VARS);
    for (int i=0; i<MAX_CUSTOM_WAVES; i++)
    {
	    memset(m_wave[i].m_pf_vars, 0, sizeof(varType)*EVAL_MAX_VARS);
	    memset(m_wave[i].m_pp_vars, 0, sizeof(varType)*EVAL_MAX_VARS);
    }
    for (int i=0; i<MAX_CUSTOM_SHAPES; i++)
    {
	    memset(m_shape[i].m_pf_vars, 0, sizeof(varType)*EVAL_MAX_VARS);
	    //memset(m_shape[i].m_pp_vars, 0, sizeof(varType)*EVAL_MAX_VARS);
    }
	RegisterBuiltInVariables(0xFFFFFFFF);
}

void CState::StripLinefeedCharsAndComments(char *src, char *dest)
{
	// replaces all LINEFEED_CONTROL_CHAR characters in src with a space in dest;
	// also strips out all comments (beginning with '//' and going til end of line).
	// Restriction: sizeof(dest) must be >= sizeof(src).

	int i2 = 0;
	int len = strlen(src);
	int bComment = false;
	for (int i=0; i<len; i++)		
	{
		if (bComment)
		{
			if (src[i] == LINEFEED_CONTROL_CHAR)	
				bComment = false;
		}
		else
		{
			if ((src[i] =='\\' && src[i+1] =='\\') || (src[i] =='/' && src[i+1] =='/'))
				bComment = true;
			else if (src[i] != LINEFEED_CONTROL_CHAR)
				dest[i2++] = src[i];
		}
	}
	dest[i2] = 0;
}

void CState::RecompileExpressions(int flags, int bReInit)
{
    // before we get started, if we redo the init code for the preset, we have to redo
    // other things too, because q1-q8 could change.
    if ((flags & RECOMPILE_PRESET_CODE) && bReInit)
    {
        flags |= RECOMPILE_WAVE_CODE;
        flags |= RECOMPILE_SHAPE_CODE;
    }

    // free old code handles
    if (flags & RECOMPILE_PRESET_CODE)
    {
	    if (m_pf_codehandle)
	    {
		    freeCode(m_pf_codehandle);
		    m_pf_codehandle = NULL;
	    }
	    if (m_pp_codehandle)
	    {
		    freeCode(m_pp_codehandle);
		    m_pp_codehandle = NULL;
	    }
    }
    if (flags & RECOMPILE_WAVE_CODE)
    {
        for (int i=0; i<MAX_CUSTOM_WAVES; i++)
        {
		    if (m_wave[i].m_pf_codehandle)
		    {
			    freeCode(m_wave[i].m_pf_codehandle);
			    m_wave[i].m_pf_codehandle = NULL;
		    }
		    if (m_wave[i].m_pp_codehandle)
		    {
			    freeCode(m_wave[i].m_pp_codehandle);
			    m_wave[i].m_pp_codehandle = NULL;
		    }
        }
    }
    if (flags & RECOMPILE_SHAPE_CODE)
    {
        for (int i=0; i<MAX_CUSTOM_SHAPES; i++)
        {
		    if (m_shape[i].m_pf_codehandle)
		    {
			    freeCode(m_shape[i].m_pf_codehandle);
			    m_shape[i].m_pf_codehandle = NULL;
		    }
		    /*if (m_shape[i].m_pp_codehandle)
		    {
			    freeCode(m_shape[i].m_pp_codehandle);
			    m_shape[i].m_pp_codehandle = NULL;
		    }*/
        }
    }

    // if we're recompiling init code, clear vars to zero, and re-register built-in variables.
	if (bReInit)
	{
        if (flags & RECOMPILE_PRESET_CODE)
        {
    		memset(m_pv_vars, 0, sizeof(varType)*EVAL_MAX_VARS);
		    memset(m_pf_vars, 0, sizeof(varType)*EVAL_MAX_VARS);
        }
        if (flags & RECOMPILE_WAVE_CODE)
        {
            for (int i=0; i<MAX_CUSTOM_WAVES; i++)
            {
	            memset(m_wave[i].m_pf_vars, 0, sizeof(varType)*EVAL_MAX_VARS);
	            memset(m_wave[i].m_pp_vars, 0, sizeof(varType)*EVAL_MAX_VARS);
            }
        }
        if (flags & RECOMPILE_SHAPE_CODE)
        {
            for (int i=0; i<MAX_CUSTOM_SHAPES; i++)
            {
	            memset(m_shape[i].m_pf_vars, 0, sizeof(varType)*EVAL_MAX_VARS);
	            //memset(m_shape[i].m_pp_vars, 0, sizeof(varType)*EVAL_MAX_VARS);
            }
        }
		RegisterBuiltInVariables(flags);
	}

	// QUICK FIX: if the code strings ONLY have spaces and linefeeds, erase them, 
	// because for some strange reason this causes errors in compileCode().
    int n2 = 3 + MAX_CUSTOM_WAVES*3 + MAX_CUSTOM_SHAPES*2; 
	for (int n=0; n<n2; n++)
	{
		char *pOrig;
		switch(n)
		{
		case 0: pOrig = m_szPerFrameExpr; break;
		case 1: pOrig = m_szPerPixelExpr; break;
		case 2: pOrig = m_szPerFrameInit; break;
        default:
            if (n < 3 + 3*MAX_CUSTOM_WAVES)
            {
                int i = (n-3) / 3;
                int j = (n-3) % 3;
                switch(j)
                {
                case 0: pOrig = m_wave[i].m_szInit;     break;
                case 1: pOrig = m_wave[i].m_szPerFrame; break;
                case 2: pOrig = m_wave[i].m_szPerPoint; break;
                }
            }
            else
            {
                int i = (n-3-3*MAX_CUSTOM_WAVES) / 2;
                int j = (n-3-3*MAX_CUSTOM_WAVES) % 2;
                switch(j)
                {
                case 0: pOrig = m_shape[i].m_szInit;     break;
                case 1: pOrig = m_shape[i].m_szPerFrame; break;
                }
            }
		}
		char *p = pOrig;
		while (*p==' ' || *p==LINEFEED_CONTROL_CHAR) p++;
		if (*p == 0) pOrig[0] = 0;
	}

    // COMPILE NEW CODE.
	#ifndef _NO_EXPR_   
    {
    	// clear any old error msg.:
    	g_plugin->m_fShowUserMessageUntilThisTime = g_plugin->GetTime();	

	    char buf[8192*3];

        if (flags & RECOMPILE_PRESET_CODE)
        {
	        resetVars(m_pf_vars);

            // 1. compile AND EXECUTE preset init code
		    StripLinefeedCharsAndComments(m_szPerFrameInit, buf);
	        if (buf[0] && bReInit)
	        {
		        int	pf_codehandle_init;	

			    if ( ! (pf_codehandle_init = compileCode(buf)))
			    {
				    sprintf(g_plugin->m_szUserMessage, "warning: preset \"%s\": error in 'per_frame_init' code", m_szDesc);
				    g_plugin->m_fShowUserMessageUntilThisTime = g_plugin->GetTime() + 6.0f;

				    q_values_after_init_code[0] = 0;
				    q_values_after_init_code[1] = 0;
				    q_values_after_init_code[2] = 0;
				    q_values_after_init_code[3] = 0;
				    q_values_after_init_code[4] = 0;
				    q_values_after_init_code[5] = 0;
				    q_values_after_init_code[6] = 0;
				    q_values_after_init_code[7] = 0;
                    monitor_after_init_code = 0;
			    }
			    else
			    {
				    // now execute the code, save the values of q1..q8, and clean up the code!

                    g_plugin->LoadPerFrameEvallibVars(g_plugin->m_pState);

				    executeCode(pf_codehandle_init);

				    q_values_after_init_code[0] = *var_pf_q1;
				    q_values_after_init_code[1] = *var_pf_q2;
				    q_values_after_init_code[2] = *var_pf_q3;
				    q_values_after_init_code[3] = *var_pf_q4;
				    q_values_after_init_code[4] = *var_pf_q5;
				    q_values_after_init_code[5] = *var_pf_q6;
				    q_values_after_init_code[6] = *var_pf_q7;
				    q_values_after_init_code[7] = *var_pf_q8;
                    monitor_after_init_code = *var_pf_monitor;

				    freeCode(pf_codehandle_init);
				    pf_codehandle_init = NULL;
			    }
	        }

            // 2. compile preset per-frame code
            StripLinefeedCharsAndComments(m_szPerFrameExpr, buf);
	        if (buf[0])
	        {
			    if ( ! (m_pf_codehandle = compileCode(buf)))
			    {
				    sprintf(g_plugin->m_szUserMessage, "warning: preset \"%s\": error in 'per_frame' code", m_szDesc);
				    g_plugin->m_fShowUserMessageUntilThisTime = g_plugin->GetTime() + 6.0f;
			    }
	        }

	        resetVars(NULL);
    	    resetVars(m_pv_vars);

            // 3. compile preset per-pixel code
		    StripLinefeedCharsAndComments(m_szPerPixelExpr, buf);
	        if (buf[0])
	        {
			    if ( ! (m_pp_codehandle = compileCode(buf)))
			    {
				    sprintf(g_plugin->m_szUserMessage, "warning: preset \"%s\": error in 'per_pixel' code", m_szDesc);
				    g_plugin->m_fShowUserMessageUntilThisTime = g_plugin->GetTime() + 6.0f;
			    }
	        }
	        
            resetVars(NULL);
        }

        if (flags & RECOMPILE_WAVE_CODE)
        {
            for (int i=0; i<MAX_CUSTOM_WAVES; i++)
            {
                // 1. compile AND EXECUTE custom waveform init code
		        StripLinefeedCharsAndComments(m_wave[i].m_szInit, buf);
	            if (buf[0] && bReInit)
                {
                    resetVars(m_wave[i].m_pf_vars);
		            {
		                int	codehandle_temp;	
			            if ( ! (codehandle_temp = compileCode(buf)))
			            {
				            sprintf(g_plugin->m_szUserMessage, "warning: preset \"%s\": error in wave %d init code", m_szDesc, i);
				            g_plugin->m_fShowUserMessageUntilThisTime = g_plugin->GetTime() + 6.0f;

                            *m_wave[i].var_pf_q1 = q_values_after_init_code[0];
                            *m_wave[i].var_pf_q2 = q_values_after_init_code[1];
                            *m_wave[i].var_pf_q3 = q_values_after_init_code[2];
                            *m_wave[i].var_pf_q4 = q_values_after_init_code[3];
                            *m_wave[i].var_pf_q5 = q_values_after_init_code[4];
                            *m_wave[i].var_pf_q6 = q_values_after_init_code[5];
                            *m_wave[i].var_pf_q7 = q_values_after_init_code[6];
                            *m_wave[i].var_pf_q8 = q_values_after_init_code[7];
				            m_wave[i].t_values_after_init_code[0] = 0;
				            m_wave[i].t_values_after_init_code[1] = 0;
				            m_wave[i].t_values_after_init_code[2] = 0;
				            m_wave[i].t_values_after_init_code[3] = 0;
				            m_wave[i].t_values_after_init_code[4] = 0;
				            m_wave[i].t_values_after_init_code[5] = 0;
				            m_wave[i].t_values_after_init_code[6] = 0;
				            m_wave[i].t_values_after_init_code[7] = 0;
			            }
			            else
			            {
				            // now execute the code, save the values of q1..q8, and clean up the code!
                    
                            g_plugin->LoadCustomWavePerFrameEvallibVars(g_plugin->m_pState, i);

				            executeCode(codehandle_temp);

				            m_wave[i].t_values_after_init_code[0] = *m_wave[i].var_pf_t1;
				            m_wave[i].t_values_after_init_code[1] = *m_wave[i].var_pf_t2;
				            m_wave[i].t_values_after_init_code[2] = *m_wave[i].var_pf_t3;
				            m_wave[i].t_values_after_init_code[3] = *m_wave[i].var_pf_t4;
				            m_wave[i].t_values_after_init_code[4] = *m_wave[i].var_pf_t5;
				            m_wave[i].t_values_after_init_code[5] = *m_wave[i].var_pf_t6;
				            m_wave[i].t_values_after_init_code[6] = *m_wave[i].var_pf_t7;
				            m_wave[i].t_values_after_init_code[7] = *m_wave[i].var_pf_t8;

				            freeCode(codehandle_temp);
				            codehandle_temp = NULL;
			            }
		            }
                    resetVars(NULL);
                }

                // 2. compile custom waveform per-frame code
		        StripLinefeedCharsAndComments(m_wave[i].m_szPerFrame, buf);
	            if (buf[0])
                {
                    resetVars(m_wave[i].m_pf_vars);
			        if ( ! (m_wave[i].m_pf_codehandle = compileCode(buf)))
			        {
				        sprintf(g_plugin->m_szUserMessage, "warning: preset \"%s\": error in wave %d per-frame code", m_szDesc, i);
				        g_plugin->m_fShowUserMessageUntilThisTime = g_plugin->GetTime() + 6.0f;
			        }
                    resetVars(NULL);
                }

                // 3. compile custom waveform per-point code
		        StripLinefeedCharsAndComments(m_wave[i].m_szPerPoint, buf);
	            if (buf[0])
                {
                    resetVars(m_wave[i].m_pp_vars);
			        if ( ! (m_wave[i].m_pp_codehandle = compileCode(buf)))
			        {
				        sprintf(g_plugin->m_szUserMessage, "warning: preset \"%s\": error in wave %d per-point code", m_szDesc, i);
				        g_plugin->m_fShowUserMessageUntilThisTime = g_plugin->GetTime() + 6.0f;
			        }
                    resetVars(NULL);
                }
            }
        }

        if (flags & RECOMPILE_SHAPE_CODE)
        {
            for (int i=0; i<MAX_CUSTOM_SHAPES; i++)
            {
                // 1. compile AND EXECUTE custom shape init code
		        StripLinefeedCharsAndComments(m_shape[i].m_szInit, buf);
	            if (buf[0] && bReInit)
                {
                    resetVars(m_shape[i].m_pf_vars);
		            #ifndef _NO_EXPR_
		            {
		                int	codehandle_temp;	
			            if ( ! (codehandle_temp = compileCode(buf)))
			            {
				            sprintf(g_plugin->m_szUserMessage, "warning: preset \"%s\": error in shape %d init code", m_szDesc, i);
				            g_plugin->m_fShowUserMessageUntilThisTime = g_plugin->GetTime() + 6.0f;

                            *m_shape[i].var_pf_q1 = q_values_after_init_code[0];
                            *m_shape[i].var_pf_q2 = q_values_after_init_code[1];
                            *m_shape[i].var_pf_q3 = q_values_after_init_code[2];
                            *m_shape[i].var_pf_q4 = q_values_after_init_code[3];
                            *m_shape[i].var_pf_q5 = q_values_after_init_code[4];
                            *m_shape[i].var_pf_q6 = q_values_after_init_code[5];
                            *m_shape[i].var_pf_q7 = q_values_after_init_code[6];
                            *m_shape[i].var_pf_q8 = q_values_after_init_code[7];
				            m_shape[i].t_values_after_init_code[0] = 0;
				            m_shape[i].t_values_after_init_code[1] = 0;
				            m_shape[i].t_values_after_init_code[2] = 0;
				            m_shape[i].t_values_after_init_code[3] = 0;
				            m_shape[i].t_values_after_init_code[4] = 0;
				            m_shape[i].t_values_after_init_code[5] = 0;
				            m_shape[i].t_values_after_init_code[6] = 0;
				            m_shape[i].t_values_after_init_code[7] = 0;
			            }
			            else
			            {
				            // now execute the code, save the values of q1..q8, and clean up the code!
                    
                            g_plugin->LoadCustomShapePerFrameEvallibVars(g_plugin->m_pState, i);

				            executeCode(codehandle_temp);

                            m_shape[i].t_values_after_init_code[0] = *m_shape[i].var_pf_t1;
				            m_shape[i].t_values_after_init_code[1] = *m_shape[i].var_pf_t2;
				            m_shape[i].t_values_after_init_code[2] = *m_shape[i].var_pf_t3;
				            m_shape[i].t_values_after_init_code[3] = *m_shape[i].var_pf_t4;
				            m_shape[i].t_values_after_init_code[4] = *m_shape[i].var_pf_t5;
				            m_shape[i].t_values_after_init_code[5] = *m_shape[i].var_pf_t6;
				            m_shape[i].t_values_after_init_code[6] = *m_shape[i].var_pf_t7;
				            m_shape[i].t_values_after_init_code[7] = *m_shape[i].var_pf_t8;

				            freeCode(codehandle_temp);
				            codehandle_temp = NULL;
			            }
		            }
		            #endif
                    resetVars(NULL);
                }

                // 2. compile custom shape per-frame code
		        StripLinefeedCharsAndComments(m_shape[i].m_szPerFrame, buf);
	            if (buf[0])
                {
                    resetVars(m_shape[i].m_pf_vars);
		            #ifndef _NO_EXPR_
			            if ( ! (m_shape[i].m_pf_codehandle = compileCode(buf)))
			            {
				            sprintf(g_plugin->m_szUserMessage, "warning: preset \"%s\": error in shape %d per-frame code", m_szDesc, i);
				            g_plugin->m_fShowUserMessageUntilThisTime = g_plugin->GetTime() + 6.0f;
			            }
		            #endif
                    resetVars(NULL);
                }

                /*
                // 3. compile custom shape per-point code
		        StripLinefeedCharsAndComments(m_shape[i].m_szPerPoint, buf);
	            if (buf[0])
                {
                    resetVars(m_shape[i].m_pp_vars);
		            #ifndef _NO_EXPR_
			            if ( ! (m_shape[i].m_pp_codehandle = compileCode(buf)))
			            {
				            sprintf(g_plugin->m_szUserMessage, "warning: preset \"%s\": error in shape %d per-point code", m_szDesc, i);
				            g_plugin->m_fShowUserMessageUntilThisTime = g_plugin->GetTime() + 6.0f;
			            }
		            #endif
                    resetVars(NULL);
                }
                */
            }
        }
    }
    #endif
}











CBlendableFloat::CBlendableFloat()
{
	m_bBlending  = false;
}

CBlendableFloat::~CBlendableFloat()
{
}

//--------------------------------------------------------------------------------

float CBlendableFloat::eval(float fTime)
{
	if (fTime < 0)
	{
		return val;
	}

	if (m_bBlending && (fTime > m_fBlendStartTime + m_fBlendDuration) || (fTime < m_fBlendStartTime))
	{
		m_bBlending = false;
	}

	if (!m_bBlending)
	{
		return val;
	}
	else
	{
		float mix = (fTime - m_fBlendStartTime) / m_fBlendDuration;
		return (m_fBlendFrom*(1.0f - mix) + val*mix);
	}
}

//--------------------------------------------------------------------------------

void CBlendableFloat::StartBlendFrom(CBlendableFloat *f_from, float fAnimTime, float fDuration)
{
	if (fDuration < 0.001f)
		return;

	m_fBlendFrom		= f_from->eval(fAnimTime);
	m_bBlending			= true;
	m_fBlendStartTime	= fAnimTime;
	m_fBlendDuration	= fDuration;
}
