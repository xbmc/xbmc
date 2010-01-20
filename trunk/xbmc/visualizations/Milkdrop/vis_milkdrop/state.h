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
#ifndef _MILKDROP_STATE_
#define _MILKDROP_STATE_ 1


#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

#include "evallib/eval.h"
#include "md_defines.h"

// flags for CState::RecompileExpressions():
#define RECOMPILE_PRESET_CODE  1
#define RECOMPILE_WAVE_CODE    2
#define RECOMPILE_SHAPE_CODE   4

class CBlendableFloat
{
public:
	CBlendableFloat();
	~CBlendableFloat();

	float operator = (float f) { 
		val = f; 
		m_bBlending = false; 
        return val;
	};
	float operator *= (float f) { 
		val *= f; 
		m_bBlending = false; 
        return val;
	};
	float operator /= (float f) { 
		val /= f; 
		m_bBlending = false; 
        return val;
	};
	float operator -= (float f) { 
		val -= f; 
		m_bBlending = false; 
        return val;
	};
	float operator += (float f) { 
		val += f; 
		m_bBlending = false; 
        return val;
	};

	float eval(float fTime);	// call this from animation code.  if fTime < 0, it will return unblended 'val'.
	void  StartBlendFrom(CBlendableFloat *f_from, float fAnimTime, float fDuration);

protected:
	float val;
	bool  m_bBlending;
	float m_fBlendStartTime;
	float m_fBlendDuration;
	float m_fBlendFrom;
};

class CShape
{
public:
    int  Import(char* szSection, char* szFile, int i);
    int  Export(char* szSection, char* szFile, int i, FILE* fOut=NULL);

    int   enabled;
    int   sides;
    int   additive;
    int   thickOutline;
    int   textured;
    float x,y,rad,ang;
    float r,g,b,a;
    float r2,g2,b2,a2;
    float border_r,border_g,border_b,border_a;
    float tex_ang, tex_zoom;

    char  m_szInit[8192]; // note: only executed once -> don't need to save codehandle
    char  m_szPerFrame[8192];
    //char  m_szPerPoint[8192];
    int   m_pf_codehandle;
    //int   m_pp_codehandle;

	// for per-frame expression evaluation:
    varType	m_pf_vars[EVAL_MAX_VARS];
	double *var_pf_time, *var_pf_fps;
	double *var_pf_frame;
	double *var_pf_progress;
	double *var_pf_q1, *var_pf_q2, *var_pf_q3, *var_pf_q4, *var_pf_q5, *var_pf_q6, *var_pf_q7, *var_pf_q8;
	double *var_pf_t1, *var_pf_t2, *var_pf_t3, *var_pf_t4, *var_pf_t5, *var_pf_t6, *var_pf_t7, *var_pf_t8;
	double *var_pf_bass, *var_pf_mid, *var_pf_treb, *var_pf_bass_att, *var_pf_mid_att, *var_pf_treb_att;
	double *var_pf_r, *var_pf_g, *var_pf_b, *var_pf_a;
	double *var_pf_r2, *var_pf_g2, *var_pf_b2, *var_pf_a2;
	double *var_pf_border_r, *var_pf_border_g, *var_pf_border_b, *var_pf_border_a;
    double *var_pf_x, *var_pf_y, *var_pf_rad, *var_pf_ang;
    double *var_pf_sides, *var_pf_textured, *var_pf_additive, *var_pf_thick;
    double *var_pf_tex_zoom, *var_pf_tex_ang;

	// for per-point expression evaluation:
    /*
    varType m_pp_vars[EVAL_MAX_VARS];
	double *var_pp_time, *var_pp_fps;
	double *var_pp_frame;
	double *var_pp_progress;
	double *var_pp_q1, *var_pp_q2, *var_pp_q3, *var_pp_q4, *var_pp_q5, *var_pp_q6, *var_pp_q7, *var_pp_q8;
	double *var_pp_t1, *var_pp_t2, *var_pp_t3, *var_pp_t4, *var_pp_t5, *var_pp_t6, *var_pp_t7, *var_pp_t8;
	double *var_pp_bass, *var_pp_mid, *var_pp_treb, *var_pp_bass_att, *var_pp_mid_att, *var_pp_treb_att;
	double *var_pp_r, *var_pp_g, *var_pp_b, *var_pp_a;
	double *var_pp_r2, *var_pp_g2, *var_pp_b2, *var_pp_a2;
	double *var_pp_border_r, *var_pp_border_g, *var_pp_border_b, *var_pp_border_a;
    double *var_pp_x, *var_pp_y, *var_pp_rad, *var_pp_ang, *var_pp_sides;
    */

	double t_values_after_init_code[8];

};

class CWave
{
public:
    int  Import(char* szSection, char *szFile, int i);
    int  Export(char* szSection, char* szFile, int i, FILE* fOut=NULL);

    int   enabled;
    int   samples;
    int   sep;
    float scaling;
    float smoothing;
    float x,y,r,g,b,a;
    int   bSpectrum;
    int   bUseDots;
    int   bDrawThick;
    int   bAdditive;

    char  m_szInit[8192]; // note: only executed once -> don't need to save codehandle
    char  m_szPerFrame[8192];
    char  m_szPerPoint[8192];
    int   m_pf_codehandle;
    int   m_pp_codehandle;

	// for per-frame expression evaluation:
    varType	m_pf_vars[EVAL_MAX_VARS];
	double *var_pf_time, *var_pf_fps;
	double *var_pf_frame;
	double *var_pf_progress;
	double *var_pf_q1, *var_pf_q2, *var_pf_q3, *var_pf_q4, *var_pf_q5, *var_pf_q6, *var_pf_q7, *var_pf_q8;
	double *var_pf_t1, *var_pf_t2, *var_pf_t3, *var_pf_t4, *var_pf_t5, *var_pf_t6, *var_pf_t7, *var_pf_t8;
	double *var_pf_bass, *var_pf_mid, *var_pf_treb, *var_pf_bass_att, *var_pf_mid_att, *var_pf_treb_att;
	double *var_pf_r, *var_pf_g, *var_pf_b, *var_pf_a;

	// for per-point expression evaluation:
    varType m_pp_vars[EVAL_MAX_VARS];
	double *var_pp_time, *var_pp_fps;
	double *var_pp_frame;
	double *var_pp_progress;
	double *var_pp_q1, *var_pp_q2, *var_pp_q3, *var_pp_q4, *var_pp_q5, *var_pp_q6, *var_pp_q7, *var_pp_q8;
	double *var_pp_t1, *var_pp_t2, *var_pp_t3, *var_pp_t4, *var_pp_t5, *var_pp_t6, *var_pp_t7, *var_pp_t8;
	double *var_pp_bass, *var_pp_mid, *var_pp_treb, *var_pp_bass_att, *var_pp_mid_att, *var_pp_treb_att;
    double *var_pp_sample, *var_pp_value1, *var_pp_value2;
	double *var_pp_x, *var_pp_y, *var_pp_r, *var_pp_g, *var_pp_b, *var_pp_a;

	double t_values_after_init_code[8];

};

typedef struct 
{
	int   type;		
	int   in_var;	
	int   out_var;	
	float constant;
	float min;
	float max;
	float in_scale;	
	float amp;		// for sine functions
	float freq;		// for sine functions
	float freq2;	// for sine functions
	float phase;	// for sine functions
	float phase2;	// for sine functions
} td_modifier;


//#define MAX_EVALS 8


class CState
{
public:
	CState();
	~CState();

	void Default();
	void Randomize(int nMode);
	void StartBlendFrom(CState *s_from, float fAnimTime, float fTimespan);
	void Import(char *szSectionName, char *szIniFile);
	bool Export(char *szSectionName, char *szIniFile);
	void RecompileExpressions(int flags=0xFFFFFFFF, int bReInit=1);

	char m_szDesc[512];		// this is just the filename, without a path or extension.
	//char m_szSection[256];

	float				m_fRating;		// 0..5
	// post-processing:
	CBlendableFloat		m_fGammaAdj;	// +0 -> +1.0 (double), +2.0 (triple)...
	CBlendableFloat		m_fVideoEchoZoom;
	CBlendableFloat 	m_fVideoEchoAlpha;
	float				m_fVideoEchoAlphaOld;
	int					m_nVideoEchoOrientation;
	int					m_nVideoEchoOrientationOld;

	// fps-dependant:
	CBlendableFloat		m_fDecay;			// 1.0 = none, 0.95 = heavy decay

	// other:
	int					m_nWaveMode;
	int					m_nOldWaveMode;
	bool				m_bAdditiveWaves;
	CBlendableFloat		m_fWaveAlpha;
	CBlendableFloat		m_fWaveScale;
	CBlendableFloat		m_fWaveSmoothing;	
	bool				m_bWaveDots;
	bool                m_bWaveThick;
	CBlendableFloat		m_fWaveParam;		// -1..1; 0 is normal
	bool				m_bModWaveAlphaByVolume;
	CBlendableFloat		m_fModWaveAlphaStart;	// when relative volume hits this level, alpha -> 0
	CBlendableFloat		m_fModWaveAlphaEnd;		// when relative volume hits this level, alpha -> 1
	float				m_fWarpAnimSpeed;	// 1.0 = normal, 2.0 = double, 0.5 = half, etc.
	CBlendableFloat		m_fWarpScale;
	CBlendableFloat		m_fZoomExponent;
	CBlendableFloat		m_fShader;			// 0 = no color shader, 1 = full color shader
	bool				m_bMaximizeWaveColor;
	bool				m_bTexWrap;
	bool				m_bDarkenCenter;
	bool				m_bRedBlueStereo;
	bool				m_bBrighten;
	bool				m_bDarken;
	bool				m_bSolarize;
	bool				m_bInvert;
	/*
	bool				m_bPlates;
	int					m_nPlates;
	CBlendableFloat		m_fPlateAlpha;		// 0 = off, 0.1 = barely visible, 1.0 = solid
	CBlendableFloat		m_fPlateR;
	CBlendableFloat		m_fPlateG;
	CBlendableFloat		m_fPlateB;
	CBlendableFloat		m_fPlateWidth;		// 1.0=normal, 2.0=double, etc.
	CBlendableFloat		m_fPlateLength;		// 1.0=normal, 2.0=double, etc.
	float				m_fPlateSpeed;		// 1.0=normal, 2.0=double, etc.
	bool				m_bPlatesAdditive;
	*/

	// map controls:
	CBlendableFloat		m_fZoom;
	CBlendableFloat		m_fRot;	
	CBlendableFloat		m_fRotCX;	
	CBlendableFloat		m_fRotCY;	
	CBlendableFloat		m_fXPush;
	CBlendableFloat		m_fYPush;
	CBlendableFloat		m_fWarpAmount;
	CBlendableFloat		m_fStretchX;
	CBlendableFloat		m_fStretchY;
	CBlendableFloat		m_fWaveR;
	CBlendableFloat		m_fWaveG;
	CBlendableFloat		m_fWaveB;
	CBlendableFloat		m_fWaveX;
	CBlendableFloat		m_fWaveY;
	CBlendableFloat		m_fOuterBorderSize;
	CBlendableFloat		m_fOuterBorderR;
	CBlendableFloat		m_fOuterBorderG;
	CBlendableFloat		m_fOuterBorderB;
	CBlendableFloat		m_fOuterBorderA;
	CBlendableFloat		m_fInnerBorderSize;
	CBlendableFloat		m_fInnerBorderR;
	CBlendableFloat		m_fInnerBorderG;
	CBlendableFloat		m_fInnerBorderB;
	CBlendableFloat		m_fInnerBorderA;
	CBlendableFloat		m_fMvX;
	CBlendableFloat		m_fMvY;
	CBlendableFloat		m_fMvDX;
	CBlendableFloat		m_fMvDY;
	CBlendableFloat		m_fMvL;
	CBlendableFloat		m_fMvR;
	CBlendableFloat		m_fMvG;
	CBlendableFloat		m_fMvB;
	CBlendableFloat		m_fMvA;

    CShape              m_shape[MAX_CUSTOM_SHAPES];
    CWave               m_wave[MAX_CUSTOM_WAVES];
	
	//COscillator			m_waveR;
	//COscillator			m_waveG;
	//COscillator			m_waveB;
	//COscillator			m_wavePosX;		// 0 = centered
	//COscillator			m_wavePosY;		// 0 = centered

	// for arbitrary function evaluation:
    int				m_pf_codehandle;	
    int				m_pp_codehandle;	
    char			m_szPerFrameInit[8192];
    char			m_szPerFrameExpr[8192];
    //char			m_szPerPixelInit[8192];
    char			m_szPerPixelExpr[8192];
	void			FreeVarsAndCode();
	void			RegisterBuiltInVariables(int flags);
	void			StripLinefeedCharsAndComments(char *src, char *dest);

	bool  m_bBlending;
	float m_fBlendStartTime;
	float m_fBlendDuration;
	float m_fBlendProgress;	// 0..1; updated every frame based on StartTime and Duration.

	// for once-per-frame expression evaluation:
    varType	m_pf_vars[EVAL_MAX_VARS];
    double *var_pf_zoom, *var_pf_zoomexp, *var_pf_rot, *var_pf_warp, *var_pf_cx, *var_pf_cy, *var_pf_dx, *var_pf_dy, *var_pf_sx, *var_pf_sy;
	double *var_pf_time, *var_pf_fps;
	double *var_pf_bass, *var_pf_mid, *var_pf_treb, *var_pf_bass_att, *var_pf_mid_att, *var_pf_treb_att;
	double *var_pf_wave_a, *var_pf_wave_r, *var_pf_wave_g, *var_pf_wave_b, *var_pf_wave_x, *var_pf_wave_y, *var_pf_wave_mystery, *var_pf_wave_mode;
	double *var_pf_decay;
	double *var_pf_frame;
	double *var_pf_q1, *var_pf_q2, *var_pf_q3, *var_pf_q4, *var_pf_q5, *var_pf_q6, *var_pf_q7, *var_pf_q8;
	double *var_pf_progress;
	double *var_pf_ob_size, *var_pf_ob_r, *var_pf_ob_g, *var_pf_ob_b, *var_pf_ob_a;
	double *var_pf_ib_size, *var_pf_ib_r, *var_pf_ib_g, *var_pf_ib_b, *var_pf_ib_a;
	double *var_pf_mv_x;
	double *var_pf_mv_y;
	double *var_pf_mv_dx;
	double *var_pf_mv_dy;
	double *var_pf_mv_l;
	double *var_pf_mv_r;
	double *var_pf_mv_g;
	double *var_pf_mv_b;
	double *var_pf_mv_a;
	double *var_pf_monitor;
	double *var_pf_echo_zoom, *var_pf_echo_alpha, *var_pf_echo_orient;
    // new in v1.04:
	double *var_pf_wave_usedots, *var_pf_wave_thick, *var_pf_wave_additive, *var_pf_wave_brighten;
    double *var_pf_darken_center, *var_pf_gamma, *var_pf_wrap;
    double *var_pf_invert, *var_pf_brighten, *var_pf_darken, *var_pf_solarize;
    double *var_pf_meshx, *var_pf_meshy;
    // NOTE: IF YOU ADD NEW VARIABLES, BE SURE TO INCREASE THE VALUE OF 
    // 'EVAL_MAX_VARS' IN EVAL.H.  OTHERWISE, SOME PRESETS MIGHT BREAK.
    

	// for per-vertex expression evaluation:
    varType m_pv_vars[EVAL_MAX_VARS];
    double *var_pv_zoom, *var_pv_zoomexp, *var_pv_rot, *var_pv_warp, *var_pv_cx, *var_pv_cy, *var_pv_dx, *var_pv_dy, *var_pv_sx, *var_pv_sy;
	double *var_pv_time, *var_pv_fps;
	double *var_pv_bass, *var_pv_mid, *var_pv_treb, *var_pv_bass_att, *var_pv_mid_att, *var_pv_treb_att;
	double *var_pv_x, *var_pv_y, *var_pv_rad, *var_pv_ang;
	double *var_pv_frame;
	double *var_pv_q1, *var_pv_q2, *var_pv_q3, *var_pv_q4, *var_pv_q5, *var_pv_q6, *var_pv_q7, *var_pv_q8;
	double *var_pv_progress;
    double *var_pv_meshx, *var_pv_meshy;

	double q_values_after_init_code[8];
    double monitor_after_init_code;
};






















#endif