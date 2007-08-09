/*
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2004 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */
/**
 * $Id: projectM.h,v 1.1.1.1 2005/12/23 18:05:11 psperl Exp $
 *
 * Encapsulation of ProjectM engine
 *
 */

#ifndef _PROJECTM_H
#define _PROJECTM_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef MACOS
#include <MacWindows.h>
#include <gl.h>
#include <glu.h>
#else
#ifdef WIN32
#include <windows.h>
#endif /** WIN32 */
#include <GL/gl.h>
#include <GL/glu.h>
#endif /** MACOS */
#ifdef WIN32
#define inline
#endif /** WIN32 */
#ifndef WIN32
#include <sys/time.h>
#else
#endif /** !WIN32 */

#include "pbuffer.h"

//#include <dmalloc.h>

#ifdef WIN32
#pragma warning (disable:4244)
#pragma warning (disable:4305)
#endif /** WIN32 */

#ifdef MACOS
#define inline
#endif

/** KEEP THIS UP TO DATE! */
#define PROJECTM_VERSION "0.99"
#define PROJECTM_TITLE "projectM 0.99"

#ifdef MACOS
#define kTVisualPluginName                      "\pprojectM"
#define kTVisualPluginCreator           'hook'

#define kTVisualPluginMajorVersion      1
#define kTVisualPluginMinorVersion      0
#define kTVisualPluginReleaseStage      finalStage
#define kTVisualPluginNonFinalRelease   0
#endif

/** Per-platform path separators */
#define WIN32_PATH_SEPARATOR '\\'
#define UNIX_PATH_SEPARATOR '/'
#ifdef WIN32
#define PATH_SEPARATOR WIN32_PATH_SEPARATOR
#else
#define PATH_SEPARATOR UNIX_PATH_SEPARATOR
#endif /** WIN32 */

/** External debug file */
#ifdef DEBUG
extern FILE *debugFile;
#endif

/** Thread state */
typedef enum { GO, STOP } PMThreadState;

typedef struct PROJECTM {

    char *presetURL;
    char *presetName;
    char *fontURL;

    int hasInit;

    int noSwitch;
    int pcmframes;
    int freqframes;
    int totalframes;

    int showfps;
    int showtitle;
    int showpreset;
    int showhelp;
    int showstats;

    int studio;

    GLubyte *fbuffer;

    

#ifndef WIN32
    /* The first ticks value of the application */
    struct timeval startTime;
#else
    long startTime;
#endif /** !WIN32 */
    float Time;

    /** Render target texture ID */
    RenderTarget *renderTarget;

    char disp[80];

    float wave_o;

    //int texsize=1024;   //size of texture to do actual graphics
    int fvw;     //fullscreen dimensions
    int fvh;
    int wvw;      //windowed dimensions
    int wvh;
    int vw;           //runtime dimensions
    int vh;
    int vx;
    int vy;
    int fullscreen;
    
    int maxsamples; //size of PCM buffer
    int numsamples; //size of new PCM info
    float *pcmdataL;     //holder for most recent pcm data 
    float *pcmdataR;     //holder for most recent pcm data 
    
    int avgtime;  //# frames per preset
    
    char *title;
    int drawtitle;
  
    int correction;
    
    float vol;
    
    //per pixel equation variables
    float **gridx;  //grid containing interpolated mesh 
    float **gridy;  
    float **origtheta;  //grid containing interpolated mesh reference values
    float **origrad;  
    float **origx;  //original mesh 
    float **origy;
    float **origx2;  //original mesh 
    float **origy2;

    /** Timing information */
    int mspf;      
    int timed;
    int timestart;
    int nohard;    
    int count;
    float realfps,
           fpsstart;

    /** PCM data */
    float vdataL[512];  //holders for FFT data (spectrum)
    float vdataR[512];

    /** Various toggles */
    int doPerPixelEffects;
    int doIterative;

    /** ENGINE VARIABLES */
    /** From engine_vars.h */
    char preset_name[256];

    /* PER FRAME CONSTANTS BEGIN */
    float zoom;
    float zoomexp;
    float rot;
    float warp;

    float sx;
    float sy;
    float dx;
    float dy;
    float cx;
    float cy;

    int gy;
    int gx;

    float decay;

    float wave_r;
    float wave_g;
    float wave_b;
    float wave_x;
    float wave_y;
    float wave_mystery;

    float ob_size;
    float ob_r;
    float ob_g;
    float ob_b;
    float ob_a;

    float ib_size;
    float ib_r;
    float ib_g;
    float ib_b;
    float ib_a;

    int meshx;
    int meshy;

    float mv_a ;
    float mv_r ;
    float mv_g ;
    float mv_b ;
    float mv_l;
    float mv_x;
    float mv_y;
    float mv_dy;
    float mv_dx;

    float treb ;
    float mid ;
    float bass ;
    float bass_old ;
	float beat_sensitivity;
    float treb_att ;
    float mid_att ;
    float bass_att ;
    float progress ;
    int frame ;

        /* PER_FRAME CONSTANTS END */

    /* PER_PIXEL CONSTANTS BEGIN */

    float x_per_pixel;
    float y_per_pixel;
    float rad_per_pixel;
    float ang_per_pixel;

    /* PER_PIXEL CONSTANT END */


    float fRating;
    float fGammaAdj;
    float fVideoEchoZoom;
    float fVideoEchoAlpha;
    
    int nVideoEchoOrientation;
    int nWaveMode;
    int bAdditiveWaves;
    int bWaveDots;
    int bWaveThick;
    int bModWaveAlphaByVolume;
    int bMaximizeWaveColor;
    int bTexWrap;
    int bDarkenCenter;
    int bRedBlueStereo;
    int bBrighten;
    int bDarken;
    int bSolarize;
    int bInvert;
    int bMotionVectorsOn;
    int fps; 
    
    float fWaveAlpha ;
    float fWaveScale;
    float fWaveSmoothing;
    float fWaveParam;
    float fModWaveAlphaStart;
    float fModWaveAlphaEnd;
    float fWarpAnimSpeed;
    float fWarpScale;
    float fShader;
    
    
    /* Q VARIABLES START */

    float q1;
    float q2;
    float q3;
    float q4;
    float q5;
    float q6;
    float q7;
    float q8;


    /* Q VARIABLES END */

    float **zoom_mesh;
    float **zoomexp_mesh;
    float **rot_mesh;

    float **sx_mesh;
    float **sy_mesh;
    float **dx_mesh;
    float **dy_mesh;
    float **cx_mesh;
    float **cy_mesh;

    float **x_mesh;
    float **y_mesh;
    float **rad_mesh;
    float **theta_mesh;
  } projectM_t;

/** Functions */
#ifdef __CPLUSPLUS
 void projectM_init(projectM_t *pm);
 void projectM_reset( projectM_t *pm );
 void projectM_resetGL( projectM_t *pm, int width, int height );
 void projectM_setTitle( projectM_t *pm, char *title );
 void renderFrame(projectM_t *pm);
#else
extern  void projectM_init(projectM_t *pm);
extern  void projectM_reset( projectM_t *pm );
extern  void projectM_resetGL( projectM_t *pm, int width, int height );
extern  void projectM_setTitle( projectM_t *pm, char *title );
extern  void renderFrame(projectM_t *pm);
#endif

void projectM_initengine(projectM_t *pm);
void projectM_resetengine(projectM_t *pm);
extern  void draw_help(projectM_t *pm);
extern  void draw_fps(projectM_t *pm,float fps);
extern  void draw_preset(projectM_t *pm);
extern  void draw_title(projectM_t *pm);
extern  void draw_stats(projectM_t *pm);

extern void modulate_opacity_by_volume(projectM_t *pm);
extern void maximize_colors(projectM_t *pm);
extern void do_per_pixel_math(projectM_t *pm);
extern void do_per_frame(projectM_t *pm);
extern void render_texture_to_studio(projectM_t *pm);
extern void darken_center(projectM_t *pm);

extern void render_interpolation(projectM_t *pm);
extern void render_texture_to_screen(projectM_t *pm);
extern void render_texture_to_studio(projectM_t *pm);
extern void draw_motion_vectors(projectM_t *pm);
extern void draw_borders(projectM_t *pm);
extern void draw_shapes(projectM_t *pm);
extern void draw_waveform(projectM_t *pm);
extern void draw_custom_waves(projectM_t *pm);

extern void draw_title_to_screen(projectM_t *pm);
extern void draw_title_to_texture(projectM_t *pm);
extern void get_title(projectM_t *pm);

extern void reset_per_pixel_matrices(projectM_t *pm);
extern void init_per_pixel_matrices(projectM_t *pm);
extern void rescale_per_pixel_matrices(projectM_t *pm);

#endif /** !_PROJECTM_H */
