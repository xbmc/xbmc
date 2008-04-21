/**
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


#include <math.h>
#ifndef _WIN32PC
#include <unistd.h>
#include <FTGL/FTGL.h>
#include <FTGL/FTGLPixmapFont.h>
#include <FTGL/FTGLPolygonFont.h>
#else
#include <FTGL.h>
#include <FTGLPixmapFont.h>
#include <FTGLPolygonFont.h>
#endif

#include "wipemalloc.h"
#include "fatal.h"
#include "common.h"

#include "timer.h"

//#include <xmms/plugin.h>
#include "projectM.h"
#include "beat_detect.h"

#include "preset_types.h"
#include "preset.h"
#include "per_pixel_eqn_types.h"
#include "per_pixel_eqn.h"
#include "interface_types.h"
#include "console_interface.h"
#include "menu.h"
#include "PCM.h"                    //Sound data handler (buffering, FFT, etc.)
#include "custom_wave_types.h"
#include "custom_wave.h"
#include "custom_shape_types.h"
#include "custom_shape.h"
#include "pbuffer.h"

preset_t *active_preset;
preset_t *old_preset;
projectM_t *PM;

FTGLPixmapFont *title_font;
FTGLPixmapFont *other_font;
FTGLPolygonFont *poly_font;
    
/** Renders a single frame */
void renderFrame( projectM_t *pm ) { 

#ifdef DEBUG
char fname[1024];
FILE *f = NULL;
int index = 0;
int x, y;
#endif 
  
//            printf("Start of loop at %d\n",timestart);

      pm->mspf=(int)(1000.0/(float)pm->fps); //milliseconds per frame
      pm->totalframes++; //total amount of frames since startup

#ifndef WIN32
      pm->Time = getTicks( &pm->startTime ) * 0.001;
#else
      pm->Time = getTicks( pm->startTime ) * 0.001;
#endif /** !WIN32 */
      
      pm->frame++;  //number of frames for current preset
      pm->progress= pm->frame/(float)pm->avgtime;
#ifdef DEBUG2
fprintf( debugFile, "frame: %d\tprogress: %f\tavgtime: %d\n", pm->frame, pm->progress, pm->avgtime );
fflush( debugFile );
#endif
      if (pm->progress>1.0) pm->progress=1.0;
//       printf("start:%d at:%d min:%d stop:%d on:%d %d\n",startframe, frame frame-startframe,avgtime,  noSwitch,progress);
     
      //evalInitConditions(active_preset);
      evalPerFrameEquations(active_preset);
       
      //evalCustomWaveInitConditions(active_preset);
      //evalCustomShapeInitConditions(active_preset);
 
//     printf("%f %d\n",Time,frame);
 
      reset_per_pixel_matrices( pm );

      pm->numsamples = getPCMnew(pm->pcmdataR,1,0,pm->fWaveSmoothing,0,0);
      getPCMnew(pm->pcmdataL,0,0,pm->fWaveSmoothing,0,1);
      getPCM(pm->vdataL,512,0,1,0,0);
      getPCM(pm->vdataR,512,1,1,0,0);

      pm->bass_old = pm->bass;
      pm->bass=0;pm->mid=0;pm->treb=0;

      getBeatVals(pm,pm->vdataL,pm->vdataR);
//      printf("=== %f %f %f %f ===\n",pm->vol,pm->bass,pm->mid,pm->treb);

      if (pm->noSwitch==0) {
          pm->nohard--;
          if((pm->bass-pm->bass_old>pm->beat_sensitivity ||
             pm->frame>pm->avgtime ) && pm->nohard<0)
          { 
//              printf("%f %d %d\n", pm->bass-pm->bass_old,pm->frame,pm->avgtime);
              switchPreset(RANDOM_NEXT, HARD_CUT);
              pm->nohard=pm->fps*5;
          }
      }

      pm->count++;
      
#ifdef DEBUG2
    fprintf( debugFile, "start Pass 1 \n" );
    fflush( debugFile );
#endif

      //BEGIN PASS 1
      //
      //This pass is used to render our texture
      //the texture is drawn to a subsection of the framebuffer
      //and then we perform our manipulations on it
      //in pass 2 we will copy the texture into texture memory

      if ( pm->renderTarget != NULL ) {
        lockPBuffer( pm->renderTarget, PBUFFER_PASS1 );
      }
      
      //   glPushAttrib( GL_ALL_ATTRIB_BITS ); /* Overkill, but safe */
      
      glViewport( pm->vx, pm->vy, pm->renderTarget->texsize, pm->renderTarget->texsize );
       
      glBindTexture( GL_TEXTURE_2D,pm->renderTarget->textureID[0] );
      glMatrixMode(GL_TEXTURE);  
      glLoadIdentity();
      
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  GL_DECAL);
      glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
      
      glMatrixMode( GL_MODELVIEW );
      glPushMatrix();
      glLoadIdentity();
      
      glMatrixMode( GL_PROJECTION );
      glPushMatrix();
      glLoadIdentity();  
      glOrtho(0.0, 1, 0.0, 1,10,40);
      
#ifdef DEBUG2
    if ( debugFile != NULL ) {
        fprintf( debugFile, "renderFrame: renderTarget->texsize: %d x %d\n", pm->renderTarget->texsize, pm->renderTarget->texsize );
        fflush( debugFile );
      }
#endif
    
    if ( pm->doPerPixelEffects ) {
      do_per_pixel_math( pm );
    }

    if(pm->renderTarget->usePbuffers)
      {
	draw_motion_vectors( pm );        //draw motion vectors
	unlockPBuffer( pm->renderTarget);
	lockPBuffer( pm->renderTarget, PBUFFER_PASS1 );
      }
    do_per_frame( pm );               //apply per-frame effects
    render_interpolation( pm );       //apply per-pixel effects
   
    draw_title_to_texture( pm );      //draw title to texture

    if(!pm->renderTarget->usePbuffers) draw_motion_vectors( pm );        //draw motion vectors
    draw_shapes( pm );
    draw_custom_waves( pm );
    draw_waveform( pm );
    if(pm->bDarkenCenter)darken_center( pm );
    draw_borders( pm );               //draw borders

    /** Restore original view state */
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();

    glMatrixMode( GL_PROJECTION );
    glPopMatrix();

    /** Restore all original attributes */
    //  glPopAttrib();
    glFlush();
  
    if ( pm->renderTarget != NULL ) {
        unlockPBuffer( pm->renderTarget );
      }

    /** Reset the viewport size */
    glViewport( pm->vx, pm->vy, pm->vw, pm->vh );

    if ( pm->renderTarget ) {
        glBindTexture( GL_TEXTURE_2D, pm->renderTarget->textureID[0] );
      }

      //BEGIN PASS 2
      //
      //end of texture rendering
      //now we copy the texture from the framebuffer to
      //video texture memory and render fullscreen on a quad surface.
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();  
      
      glOrtho(-0.5, 0.5, -0.5,0.5,10,40);
     	
      glLineWidth( pm->renderTarget->texsize < 512 ? 1 : pm->renderTarget->texsize/512.0);
      if(pm->studio%2)render_texture_to_studio( pm );     
      else render_texture_to_screen( pm );

      // glClear(GL_COLOR_BUFFER_BIT);     
      //render_Studio();

      //preset editing menu
      glMatrixMode(GL_MODELVIEW);
   
      glTranslated(-0.5,-0.5,-1);  
      refreshConsole( pm );
      draw_title_to_screen( pm );
      if(pm->showhelp%2)draw_help( pm );
      if(pm->showtitle%2)draw_title( pm );
      if(pm->showfps%2)draw_fps( pm, pm->realfps);
      if(pm->showpreset%2)draw_preset( pm );
      if(pm->showstats%2)draw_stats( pm );
      glTranslatef(0.5 ,0.5,1);

      /** Frame-rate limiter */
      /** Compute once per preset */
      if (pm->count%100==0) 
      {
#ifndef WIN32
	      pm->realfps=100.0/((getTicks(&pm->startTime)-pm->fpsstart)/1000);
	      pm->fpsstart=getTicks(&pm->startTime);      	 
      }

      int timediff = getTicks(&pm->startTime)-pm->timestart;
#else
        pm->realfps=100.0/((getTicks(pm->startTime)-pm->fpsstart)/1000);
	      pm->fpsstart=getTicks(pm->startTime); 
      }

      int timediff = getTicks(pm->startTime)-pm->timestart;
#endif

      if ( timediff < pm->mspf) 
	    {	 
	      // printf("%s:",pm->mspf-timediff);
	      if (1 /* usleep( (unsigned int)( pm->mspf-timediff ) * 1000 ) != 0 */) 
	      {	      
	      }	  
	    }
#ifndef WIN32
      pm->timestart=getTicks(&pm->startTime);
#else
      pm->timestart=getTicks(pm->startTime);
#endif
     
}


 void projectM_reset( projectM_t *pm ) {

#ifdef DEBUG
    if ( debugFile != NULL ) {
        fprintf( debugFile, "projectM_reset(): in\n" );
        fflush( debugFile );
      }
#endif
    active_preset = NULL;
    PM=pm;
 
    pm->presetURL = NULL;
    pm->fontURL = NULL;

    /** Default variable settings */
    pm->hasInit = 0;

    pm->noSwitch = 0;
    pm->pcmframes = 1;
    pm->freqframes = 0;
    pm->totalframes = 1;

    pm->showfps = 0;
    pm->showtitle = 0;
    pm->showpreset = 0;
    pm->showhelp = 0;
    pm->showstats = 0;
    pm->studio = 0;

   

    /** Allocate a new render target */
#ifdef PANTS
    if ( pm->renderTarget ) {
        if ( pm->renderTarget->renderTarget ) {
            /** Free existing */
            free( pm->renderTarget->renderTarget );
	    pm->renderTarget->renderTarget = NULL;
          }
        free( pm->renderTarget );
        pm->renderTarget = NULL;
      }
#endif
    pm->renderTarget = (RenderTarget *)wipemalloc( sizeof( RenderTarget ) );        

#ifdef MACOS
    pm->renderTarget->origContext = NULL;
    pm->renderTarget->pbufferContext = NULL;
    pm->renderTarget->pbuffer = NULL;
#endif

    /** Configurable engine variables */
    pm->renderTarget->texsize = 512;
    pm->fvw = 800;
    pm->fvh = 600;
    pm->wvw = 512;
    pm->wvh = 512;
    pm->fullscreen = 0;

    /** Configurable mesh size */
    pm->gx = 48;
    pm->gy = 36;

    /** PCM data */
    pm->maxsamples = 2048;
    pm->numsamples = 0;
    pm->pcmdataL = NULL;
    pm->pcmdataR = NULL;


    /** Frames per preset */
    pm->avgtime = 500;

    pm->title = NULL;    

    /** Other stuff... */
    pm->correction = 0;
    pm->vol = 0;

    /** Per pixel equation variables */
    pm->gridx = NULL;
    pm->gridy = NULL;
    pm->origtheta = NULL;
    pm->origrad = NULL;
    pm->origx = NULL;
    pm->origy = NULL;

    /** More other stuff */
    pm->mspf = 0;
    pm->timed = 0;
    pm->timestart = 0;   
    pm->nohard = 0;
    pm->count = 0;
    pm->realfps = 0;
    pm->fpsstart = 0;

    projectM_resetengine( pm );
  }

 void projectM_init( projectM_t *pm ) {
    PM=pm;
    /** Reset fonts */
    title_font = NULL;
    other_font = NULL;
    poly_font = NULL;
    /** Initialise engine variables */
    projectM_initengine( pm );


    DWRITE("projectM plugin: Initializing\n");

    /** Initialise start time */
#ifndef WIN32
    gettimeofday(&pm->startTime, NULL);
#else
    pm->startTime = GetTickCount();
#endif /** !WIN32 */

    /** Nullify frame stash */
    pm->fbuffer = NULL;

    /** Initialise per-pixel matrix calculations */
    init_per_pixel_matrices( pm );

    /* Preset loading function */
    initPresetLoader(pm);
    if ( loadPresetDir( pm->presetURL ) == PROJECTM_ERROR ) {
        switchToIdlePreset();
      }

#ifdef PANTS
  /* Load default preset directory */
#ifdef MACOS2
    /** Probe the bundle for info */
    CFBundleRef bundle = CFBundleGetMainBundle();
    char msg[1024];
    sprintf( msg, "bundle: %X\n", bundle );
    DWRITE( msg );
    if ( bundle != NULL ) {
        CFPlugInRef pluginRef = CFBundleGetPlugIn( bundle );
        if ( pluginRef != NULL ) {
#ifdef DEBUG
            if ( debugFile != NULL ) {
                fprintf( debugFile, "located plugin ref\n" );
                fflush( debugFile );
              }
#endif
          } else {
#ifdef DEBUG
            if ( debugFile != NULL ) {
                fprintf( debugFile, "failed to find plugin ref\n" );
                fflush( debugFile );
              }
#endif
          }

        CFURLRef bundleURL = CFBundleCopyBundleURL( bundle );
        if ( bundleURL == NULL ) {
#ifdef DEBUG
            if ( debugFile != NULL ) {
                fprintf( debugFile, "bundleURL failed\n" );
                fflush( debugFile );
              }
#endif
          } else {
#ifdef DEBUG
            if ( debugFile != NULL ) {
                fprintf( debugFile, "bundleURL OK\n" );
                fflush( debugFile );
              }
#endif
          }
        char *bundleName = 
            (char *)CFStringGetCStringPtr( CFURLGetString( bundleURL ), kCFStringEncodingMacRoman );
#ifdef DEBUG
        if ( debugFile != NULL ) {
            fprintf( debugFile, "bundleURL: %s\n", bundleName );
            fflush( debugFile );
          }
#endif

#ifdef PANTS
        presetURL = CFBundleCopyResourceURL( bundle, purl, NULL, NULL );
        if ( presetURL != NULL ) {
            pm->presetURL = (char *)CFStringGetCStringPtr( CFURLCopyPath( presetURL ), kCFStringEncodingMacRoman);
            sprintf( msg, "Preset: %s\n", pm->presetURL );
            DWRITE( msg );
            printf( msg );

            /** Stash the short preset name */

          } else {
            DWRITE( "Failed to probe 'presets' bundle ref\n" );
            pm->presetURL = NULL;
          }

        fontURL = CFBundleCopyResourceURL( bundle, furl, NULL, NULL );
        if ( fontURL != NULL ) {
            pm->fontURL = (char *)CFStringGetCStringPtr( CFURLCopyPath( fontURL ), kCFStringEncodingMacRoman);
            sprintf( msg, "Font: %s\n", pm->fontURL );
            DWRITE( msg );
            printf( msg );
          } else {
            DWRITE( "Failed to probe 'fonts' bundle ref\n" );
            pm->fontURL = NULL;
          }
#endif
      }

    /** Sanity check */
    if ( bundle == NULL || pm->presetURL == NULL || pm->fontURL == NULL ) {
        sprintf( msg, "defaulting presets\n" );
        DWRITE( msg );
        pm->fontURL = (char *)wipemalloc( sizeof( char ) * 512 );
//        strcpy( pm->fontURL, "../../fonts/" );
        strcpy( pm->fontURL, "/Users/descarte/tmp/projectM/fonts" );
        pm->fontURL[34] = '\0';
//        loadPresetDir( "../../presets/" );
        loadPresetDir( "/Users/descarte/tmp/projectM/presets_projectM" );
      } else {
        printf( "PresetDir: %s\n", pm->presetURL );
        loadPresetDir( pm->presetURL );
      }
#else
    if ( pm->presetURL == NULL || pm->fontURL == NULL ) {
        char msg[1024];
        sprintf( msg, "defaulting presets\n" );
        DWRITE( msg );
        pm->fontURL = (char *)wipemalloc( sizeof( char ) * 512 );
#ifdef WIN32
        strcpy( pm->fontURL, "c:\\tmp\\projectM\\fonts" );
        pm->fontURL[24] = '\0';
#else
        strcpy( pm->fontURL, "/Users/descarte/tmp/projectM/fonts" );
        pm->fontURL[34] = '\0';
        fprintf( debugFile, "loading font URL directly: %s\n", pm->fontURL );
        fflush( debugFile );
#endif
#ifdef WIN32
        loadPresetDir( "c:\\tmp\\projectM\\presets_projectM" );
#else
        loadPresetDir( "/Users/descarte/tmp/projectM/presets_projectM" );
#endif
      } else {
        printf( "PresetDir: %s\n", pm->presetURL );
        loadPresetDir( pm->presetURL );
      }

#endif
#endif /** PANTS */

printf( "pre init_display()\n" );

printf( "post init_display()\n" );

 pm->mspf=(int)(1000.0/(float)pm->fps);


   
  //create off-screen pbuffer (or not if unsupported)
//  CreateRenderTarget(pm->renderTarget->texsize, &pm->textureID, &pm->renderTarget);
printf( "post CreaterenderTarget\n" );
  
  pm->drawtitle=0;


    /** Allocate PCM data structures */
    pm->pcmdataL=(float *)wipemalloc(pm->maxsamples*sizeof(float));
    pm->pcmdataR=(float *)wipemalloc(pm->maxsamples*sizeof(float));
  
  initMenu();  
DWRITE( "post initMenu()\n" );

    printf("mesh: %d %d\n", pm->gx,pm->gy );
    printf( "maxsamples: %d\n", pm->maxsamples );

  initPCM(pm->maxsamples);
  initBeatDetect();
DWRITE( "post PCM init\n" );

 pm->avgtime=pm->fps*20;

    pm->hasInit = 1;


  createPBuffers( pm->renderTarget->texsize, pm->renderTarget->texsize , pm->renderTarget );

printf( "exiting projectM_init()\n" );
}

void free_per_pixel_matrices( projectM_t *pm )
{
  int x;

 for(x = 0; x < pm->gx; x++)
    {
      
      free(pm->gridx[x]);
      free(pm->gridy[x]); 
      free(pm->origtheta[x]);
      free(pm->origrad[x]);
      free(pm->origx[x]);
      free(pm->origy[x]);
      free(pm->origx2[x]);
      free(pm->origy2[x]);
      free(pm->x_mesh[x]);
      free(pm->y_mesh[x]);
      free(pm->rad_mesh[x]);
      free(pm->theta_mesh[x]);
      
    }

  
  free(pm->origx);
  free(pm->origy);
  free(pm->origx2);
  free(pm->origy2);
  free(pm->gridx);
  free(pm->gridy);
  free(pm->x_mesh);
  free(pm->y_mesh);
  free(pm->rad_mesh);
  free(pm->theta_mesh);

  pm->origx = NULL;
  pm->origy = NULL;
  pm->origx2 = NULL;
  pm->origy2 = NULL;
  pm->gridx = NULL;
  pm->gridy = NULL;
  pm->x_mesh = NULL;
  pm->y_mesh = NULL;
  pm->rad_mesh = NULL;
  pm->theta_mesh = NULL;
}


void init_per_pixel_matrices( projectM_t *pm )
{
  int x,y; 
  
  pm->gridx=(float **)wipemalloc(pm->gx * sizeof(float *));
   for(x = 0; x < pm->gx; x++)
    {
      pm->gridx[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->gridy=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->gridy[x] = (float *)wipemalloc(pm->gy * sizeof(float)); 
    }
  pm->origtheta=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->origtheta[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->origrad=(float **)wipemalloc(pm->gx * sizeof(float *));
     for(x = 0; x < pm->gx; x++)
    {
      pm->origrad[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->origx=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->origx[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->origy=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->origy[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->origx2=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->origx2[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }  
pm->origy2=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->origy2[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->x_mesh=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->x_mesh[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->y_mesh=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->y_mesh[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    
    }
  pm->rad_mesh=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->rad_mesh[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->theta_mesh=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->theta_mesh[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->sx_mesh=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->sx_mesh[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->sy_mesh=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->sy_mesh[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->dx_mesh=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->dx_mesh[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->dy_mesh=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->dy_mesh[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->cx_mesh=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->cx_mesh[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->cy_mesh=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->cy_mesh[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->zoom_mesh=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->zoom_mesh[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->zoomexp_mesh=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    {
      pm->zoomexp_mesh[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }
  pm->rot_mesh=(float **)wipemalloc(pm->gx * sizeof(float *));
 for(x = 0; x < pm->gx; x++)
    { 
      pm->rot_mesh[x] = (float *)wipemalloc(pm->gy * sizeof(float));
    }



  //initialize reference grid values
  for (x=0;x<pm->gx;x++)
    {
      for(y=0;y<pm->gy;y++)
	{
	   pm->origx[x][y]=x/(float)(pm->gx-1);
	   pm->origy[x][y]=-((y/(float)(pm->gy-1))-1);
	   pm->origrad[x][y]=hypot((pm->origx[x][y]-.5)*2,(pm->origy[x][y]-.5)*2) * .7071067;
  	   pm->origtheta[x][y]=atan2(((pm->origy[x][y]-.5)*2),((pm->origx[x][y]-.5)*2));
	   pm->gridx[x][y]=pm->origx[x][y]*pm->renderTarget->texsize;
	   pm->gridy[x][y]=pm->origy[x][y]*pm->renderTarget->texsize;
	   pm->origx2[x][y]=( pm->origx[x][y]-.5)*2;
	   pm->origy2[x][y]=( pm->origy[x][y]-.5)*2;
	}}
}



//calculate matrices for per_pixel
void do_per_pixel_math( projectM_t *pm )
{
  int x,y;
  float fZoom2,fZoom2Inv;

  evalPerPixelEqns(active_preset);



  if(!isPerPixelEqn(CX_OP))
       { 
      for (x=0;x<pm->gx;x++){
       
	for(y=0;y<pm->gy;y++){
	  pm->cx_mesh[x][y]=pm->cx;
	}
	
      }
    }

  if(!isPerPixelEqn(CY_OP))
        { 
      for (x=0;x<pm->gx;x++){
	for(y=0;y<pm->gy;y++){
	  pm->cy_mesh[x][y]=pm->cy;
	}}
    }
  
  if(!isPerPixelEqn(SX_OP))
    { 
      for (x=0;x<pm->gx;x++){
	for(y=0;y<pm->gy;y++){
	  pm->sx_mesh[x][y]=pm->sx;
	}}
    }
  
  if(!isPerPixelEqn(SY_OP))
    { 
      for (x=0;x<pm->gx;x++){
	for(y=0;y<pm->gy;y++){
	  pm->sy_mesh[x][y]=pm->sy;
	}}
    }

  if(!isPerPixelEqn(ZOOM_OP))
    {       
      for (x=0;x<pm->gx;x++){
	for(y=0;y<pm->gy;y++){
	  pm->zoom_mesh[x][y]=pm->zoom;
	}}
    }
 
  if(!isPerPixelEqn(ZOOMEXP_OP))
    {
      for (x=0;x<pm->gx;x++){
	for(y=0;y<pm->gy;y++){
	  pm->zoomexp_mesh[x][y]=pm->zoomexp;
	}}
    }

  if(!isPerPixelEqn(ROT_OP))
    {       
      for (x=0;x<pm->gx;x++){
	for(y=0;y<pm->gy;y++){
	  pm->rot_mesh[x][y]=pm->rot;
	}
      }
    }

  /*
  for (x=0;x<pm->gx;x++){
    for(y=0;y<pm->gy;y++){	  	  
      pm->x_mesh[x][y]=(pm->x_mesh[x][y]-.5)*2; 
    }
  }
 
  for (x=0;x<pm->gx;x++){
    for(y=0;y<pm->gy;y++){	  	  
      pm->y_mesh[x][y]=(pm->y_mesh[x][y]-.5)*2; 
    }
  }
  */

  for (x=0;x<pm->gx;x++){
    for(y=0;y<pm->gy;y++){
      fZoom2 = powf( pm->zoom_mesh[x][y], powf( pm->zoomexp_mesh[x][y], pm->rad_mesh[x][y]*2.0f - 1.0f));
      fZoom2Inv = 1.0f/fZoom2;
      pm->x_mesh[x][y]= pm->origx2[x][y]*0.5f*fZoom2Inv + 0.5f;
      pm->y_mesh[x][y]= pm->origy2[x][y]*0.5f*fZoom2Inv + 0.5f;
    }
  }
	
  for (x=0;x<pm->gx;x++){
    for(y=0;y<pm->gy;y++){
      pm->x_mesh[x][y]  = ( pm->x_mesh[x][y] - pm->cx_mesh[x][y])/pm->sx_mesh[x][y] + pm->cx_mesh[x][y];
    }
  }
  
  for (x=0;x<pm->gx;x++){
    for(y=0;y<pm->gy;y++){
      pm->y_mesh[x][y] = ( pm->y_mesh[x][y] - pm->cy_mesh[x][y])/pm->sy_mesh[x][y] + pm->cy_mesh[x][y];
    }
  }	   
	 

 for (x=0;x<pm->gx;x++){
   for(y=0;y<pm->gy;y++){
     float u2 = pm->x_mesh[x][y] - pm->cx_mesh[x][y];
     float v2 = pm->y_mesh[x][y] - pm->cy_mesh[x][y];
     
     float cos_rot = cosf(pm->rot_mesh[x][y]);
     float sin_rot = sinf(pm->rot_mesh[x][y]);
     
     pm->x_mesh[x][y] = u2*cos_rot - v2*sin_rot + pm->cx_mesh[x][y];
     pm->y_mesh[x][y] = u2*sin_rot + v2*cos_rot + pm->cy_mesh[x][y];

  }
 }	  

 if(isPerPixelEqn(DX_OP))
   {
     for (x=0;x<pm->gx;x++){
       for(y=0;y<pm->gy;y++){	      
	 pm->x_mesh[x][y] -= pm->dx_mesh[x][y];
       }
     }
   }
 
 if(isPerPixelEqn(DY_OP))
   {
     for (x=0;x<pm->gx;x++){
       for(y=0;y<pm->gy;y++){	      
	 pm->y_mesh[x][y] -= pm->dy_mesh[x][y];
       }
     }
		  	
   }

}

void reset_per_pixel_matrices( projectM_t *pm )
{
  int x,y;
  /*
  for (x=0;x<pm->gx;x++)
    {
      memcpy(pm->x_mesh[x],pm->origx[x],sizeof(float)*pm->gy);
    }
  for (x=0;x<pm->gx;x++)
    {
      memcpy(pm->y_mesh[x],pm->origy[x],sizeof(float)*pm->gy);
    }
  for (x=0;x<pm->gx;x++)
    {
      memcpy(pm->rad_mesh[x],pm->origrad[x],sizeof(float)*pm->gy);
    }
  for (x=0;x<pm->gx;x++)
    {
      memcpy(pm->theta_mesh[x],pm->origtheta[x],sizeof(float)*pm->gy);
    }
  */
  
    for (x=0;x<pm->gx;x++)
    {
      for(y=0;y<pm->gy;y++)
	{   
          pm->x_mesh[x][y]=pm->origx[x][y];
	  pm->y_mesh[x][y]=pm->origy[x][y];
	  pm->rad_mesh[x][y]=pm->origrad[x][y];
	  pm->theta_mesh[x][y]=pm->origtheta[x][y];	  
	}
    }
  
  //memcpy(pm->x_mesh,pm->origx,sizeof(float)*pm->gy*pm->gx);
  //memcpy(pm->y_mesh,pm->origy,sizeof(float)*pm->gy*pm->gx);
  //memcpy(pm->rad_mesh,pm->origrad,sizeof(float)*pm->gy*pm->gx);
  //memcpy(pm->theta_mesh,pm->origtheta,sizeof(float)*pm->gy*pm->gx);
 }

void rescale_per_pixel_matrices( projectM_t *pm ) {

    int x, y;

    for ( x = 0 ; x < pm->gx ; x++ ) {
        for ( y = 0 ; y < pm->gy ; y++ ) {
            pm->gridx[x][y]=pm->origx[x][y];
            pm->gridy[x][y]=pm->origy[x][y];

          }
      }
  }

void draw_custom_waves( projectM_t *pm )
{
  int x;
  
  custom_wave_t *wavecode;
  glPointSize(pm->renderTarget->texsize < 512 ? 1 : pm->renderTarget->texsize/512); 
 
  while ((wavecode = nextCustomWave(active_preset)) != NULL)
    {
     
      if(wavecode->enabled==1)
	{
	
	  if (wavecode->bAdditive==0)  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	  else    glBlendFunc(GL_SRC_ALPHA, GL_ONE); 
	  if (wavecode->bDrawThick==1)  glLineWidth(pm->renderTarget->texsize < 512 ? 1 : 2*pm->renderTarget->texsize/512);
	  
	  getPCM(wavecode->value1,wavecode->samples,0,wavecode->bSpectrum,wavecode->smoothing,0);
	  getPCM(wavecode->value2,wavecode->samples,1,wavecode->bSpectrum,wavecode->smoothing,0);
	  // printf("%f\n",pcmL[0]);

	  float mult=wavecode->scaling*pm->fWaveScale*(wavecode->bSpectrum ? 0.015f :1.0f);

	  for(x=0;x<wavecode->samples;x++)
	    {wavecode->value1[x]*=mult;}
	  
	  for(x=0;x<wavecode->samples;x++)
	    {wavecode->value2[x]*=mult;}

	   for(x=0;x<wavecode->samples;x++)
	     {wavecode->sample_mesh[x]=((float)x)/((float)(wavecode->samples-1));}
	  
	  // printf("mid inner loop\n");  
	  evalPerPointEqns(active_preset);
	
	  //put drawing code here
	  if (wavecode->bUseDots==1)   glBegin(GL_POINTS);
	  else   glBegin(GL_LINE_STRIP);
	  
	  for(x=0;x<wavecode->samples;x++)
	    {
	     
	      glColor4f(wavecode->r_mesh[x],wavecode->g_mesh[x],wavecode->b_mesh[x],wavecode->a_mesh[x]);
	      glVertex3f(wavecode->x_mesh[x],-(wavecode->y_mesh[x]-1),-1);
	    }
	  glEnd();
	  glPointSize(pm->renderTarget->texsize < 512 ? 1 : pm->renderTarget->texsize/512); 
	  glLineWidth(pm->renderTarget->texsize < 512 ? 1 : pm->renderTarget->texsize/512); 
	  glDisable(GL_LINE_STIPPLE); 
	  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	  //  glPopMatrix();
	  
	}
      
    }
}

void darken_center( projectM_t *pm )
{
  int unit=0.05f;

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix(); 
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
  glTranslatef(0.5,0.5, 0);

  glBegin(GL_TRIANGLE_FAN);
  glColor4f(0,0,0,3.0f/32.0f);
  glVertex3f(0,0,-1);
  glColor4f(0,0,0,-1);
  glVertex3f(-unit,0,-1);
  glVertex3f(0,-unit,-1);
  glVertex3f(unit,0,-1);
  glVertex3f(0,unit,-1);
  glVertex3f(-unit,0,-1);
  glEnd();

  glPopMatrix();
}

void draw_shapes( projectM_t *pm )
{ 
  int i;

  float theta;
  float radius;

  custom_shape_t *shapecode;
 
  float pi = 3.14159265;
  float start,inc,xval,yval;
  
  float t;

  //  more=isMoreCustomWave();
  // printf("not inner loop\n");
  
  while ((shapecode = nextCustomShape(active_preset)) != NULL)
    {

      if(shapecode->enabled==1)
	{
	  // printf("drawing shape %f\n",shapecode->ang);
	  shapecode->y=-((shapecode->y)-1);
	  radius=.5;
	  shapecode->radius=shapecode->radius*(.707*.707*.707*1.04);
	  //Additive Drawing or Overwrite
	  if (shapecode->additive==0)  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	  else    glBlendFunc(GL_SRC_ALPHA, GL_ONE); 
	  
	  glMatrixMode(GL_MODELVIEW);
	  	   glPushMatrix(); 
		   
		   if(pm->correction)
		     {		     
		       glTranslatef(0.5,0.5, 0);
		       glScalef(1.0,pm->vw/(float)pm->vh,1.0);  
		       glTranslatef(-0.5 ,-0.5,0);   
		     }
	 
	
	   xval=shapecode->x;
	   yval=shapecode->y;
	  
	  if (shapecode->textured)
	    {
	      glMatrixMode(GL_TEXTURE);
	       glPushMatrix();
	      glLoadIdentity();
	      //glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	      //glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	      //glTranslatef(.5,.5, 0);
	      //if (pm->correction) glScalef(1,pm->vw/(float)pm->vh,1);
   
	      //glRotatef((shapecode->tex_ang*360/6.280), 0, 0, 1);
	      
	      //glScalef(1/(shapecode->tex_zoom),1/(shapecode->tex_zoom),1); 
	      
	      // glScalef(1,vh/(float)vw,1);
	      //glTranslatef((-.5) ,(-.5),0);  
	      // glScalef(1,pm->vw/(float)pm->vh,1);
	      glEnable(GL_TEXTURE_2D);
	      	     

	      glBegin(GL_TRIANGLE_FAN);
	      glColor4f(0.0,0.0,0.0,shapecode->a);
	      //glColor4f(shapecode->r,shapecode->g,shapecode->b,shapecode->a);
	   
	      glTexCoord2f(.5,.5);
	      glVertex3f(xval,yval,-1);	 
	      //glColor4f(shapecode->r2,shapecode->g2,shapecode->b2,shapecode->a2);  
	      glColor4f(0.0,0.0,0.0,shapecode->a2);

	      for ( i=1;i<shapecode->sides+2;i++)
		{
		 
		  //		  theta+=inc;
		  //  glColor4f(shapecode->r2,shapecode->g2,shapecode->b2,shapecode->a2);
		  //glTexCoord2f(radius*cos(theta)+.5 ,radius*sin(theta)+.5 );
		  //glVertex3f(shapecode->radius*cos(theta)+xval,shapecode->radius*sin(theta)+yval,-1);  
		  t = (i-1)/(float)shapecode->sides;
		  
		  glTexCoord2f(  0.5f + 0.5f*cosf(t*3.1415927f*2 + shapecode->tex_ang + 3.1415927f*0.25f)/shapecode->tex_zoom, 0.5f + 0.5f*sinf(t*3.1415927f*2 + shapecode->tex_ang + 3.1415927f*0.25f)/shapecode->tex_zoom);
		   glVertex3f(shapecode->radius*cosf(t*3.1415927f*2 + shapecode->ang + 3.1415927f*0.25f)+xval, shapecode->radius*sinf(t*3.1415927f*2 + shapecode->ang + 3.1415927f*0.25f)+yval,-1);      
		}	
	      glEnd();

	    
	      
	    
	      glDisable(GL_TEXTURE_2D);
	       glPopMatrix();
	      glMatrixMode(GL_MODELVIEW);          
	    }
	  else{//Untextured (use color values)
	    //printf("untextured %f %f %f @:%f,%f %f %f\n",shapecode->a2,shapecode->a,shapecode->border_a, shapecode->x,shapecode->y,shapecode->radius,shapecode->ang);
	    //draw first n-1 triangular pieces
	      glBegin(GL_TRIANGLE_FAN);
	      
	      glColor4f(shapecode->r,shapecode->g,shapecode->b,shapecode->a);
	    
	      // glTexCoord2f(.5,.5);
	      glVertex3f(xval,yval,-1);	 
	     glColor4f(shapecode->r2,shapecode->g2,shapecode->b2,shapecode->a2);

	      for ( i=1;i<shapecode->sides+2;i++)
		{
		  
		  //theta+=inc;
		  //  glColor4f(shapecode->r2,shapecode->g2,shapecode->b2,shapecode->a2);
		  //  glTexCoord2f(radius*cos(theta)+.5 ,radius*sin(theta)+.5 );
		  //glVertex3f(shapecode->radius*cos(theta)+xval,shapecode->radius*sin(theta)+yval,-1);	  

		  t = (i-1)/(float)shapecode->sides;
		  	  
		   glVertex3f(shapecode->radius*cosf(t*3.1415927f*2 + shapecode->ang + 3.1415927f*0.25f)+xval, shapecode->radius*sinf(t*3.1415927f*2 + shapecode->ang + 3.1415927f*0.25f)+yval,-1);      
		}	
	      glEnd();

	   	 
	  }
	    if (pm->bWaveThick==1)  glLineWidth(pm->renderTarget->texsize < 512 ? 1 : 2*pm->renderTarget->texsize/512);
	      glBegin(GL_LINE_LOOP);
	      glColor4f(shapecode->border_r,shapecode->border_g,shapecode->border_b,shapecode->border_a);
	      for ( i=1;i<shapecode->sides+1;i++)
		{

		  t = (i-1)/(float)shapecode->sides;
		  	  
		  glVertex3f(shapecode->radius*cosf(t*3.1415927f*2 + shapecode->ang + 3.1415927f*0.25f)+xval, shapecode->radius*sinf(t*3.1415927f*2 + shapecode->ang + 3.1415927f*0.25f)+yval,-1); 

		  //theta+=inc;
		  //glVertex3f(shapecode->radius*cos(theta)+xval,shapecode->radius*sin(theta)+yval,-1);
		}
	      glEnd();
	  if (pm->bWaveThick==1)  glLineWidth(pm->renderTarget->texsize < 512 ? 1 : pm->renderTarget->texsize/512);
	  
	  glPopMatrix();
	}
    }
  
}


void draw_waveform( projectM_t *pm )
{

  int x;
  
  float r,theta;
 
  float offset,scale,dy2_adj;

  float co;  
  
  float wave_x_temp=0;
  float wave_y_temp=0;
  float dy_adj;
  float xx,yy; 

  float cos_rot;
  float sin_rot;    

  modulate_opacity_by_volume(pm); 
  maximize_colors(pm);
  
  if(pm->bWaveDots==1) glEnable(GL_LINE_STIPPLE);
  
  offset=pm->wave_x-.5;
  scale=505.0/512.0;


  

  //Thick wave drawing
  if (pm->bWaveThick==1)  glLineWidth( (pm->renderTarget->texsize < 512 ) ? 1 : 2*pm->renderTarget->texsize/512);

  //Additive wave drawing (vice overwrite)
  if (pm->bAdditiveWaves==0)  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
  else    glBlendFunc(GL_SRC_ALPHA, GL_ONE); 
 
      switch(pm->nWaveMode)
	{
	  
	case 8://monitor

	  glPushMatrix();
	  	  
	  glTranslatef(0.5,0.5, 0);
	  glRotated(-pm->wave_mystery*90,0,0,1);

	     glTranslatef(-0.5,-0.825, 0);
	     
	     /*
	     for (x=0;x<16;x++)
	       {
		 glBegin(GL_LINE_STRIP);
		 glColor4f(1.0-(x/15.0),.5,x/15.0,1.0);
		 glVertex3f((pm->totalframes%256)*2*scale, -pm->beat_val[x]*pm->fWaveScale+renderTarget->texsize*wave_y,-1);
		 glColor4f(.5,.5,.5,1.0);
		 glVertex3f((pm->totalframes%256)*2*scale, pm->renderTarget->texsize*pm->wave_y,-1);   
		 glColor4f(1.0,1.0,0,1.0);
		 //glVertex3f((pm->totalframes%256)*scale*2, pm->beat_val_att[x]*pm->fWaveScale+pm->renderTarget->texsize*pm->wave_y,-1);
		 glEnd();
	       
		 glTranslatef(0,pm->renderTarget->texsize*(1/36.0), 0);
	       }
	  */
	     
	    glTranslatef(0,(1/18.0), 0);

 
	     glBegin(GL_LINE_STRIP);
	     glColor4f(1.0,1.0,0.5,1.0);
	     glVertex3f((pm->totalframes%256)*2*scale, pm->treb_att*5*pm->fWaveScale+pm->wave_y,-1);
	     glColor4f(.2,.2,.2,1.0);
	     glVertex3f((pm->totalframes%256)*2*scale, pm->wave_y,-1);   
	     glColor4f(1.0,1.0,0,1.0);
	     glVertex3f((pm->totalframes%256)*scale*2, pm->treb*-5*pm->fWaveScale+pm->wave_y,-1);
	     glEnd();
	       
	       glTranslatef(0,.075, 0);
	     glBegin(GL_LINE_STRIP);
	     glColor4f(0,1.0,0.0,1.0);
	     glVertex3f((pm->totalframes%256)*2*scale, pm->mid_att*5*pm->fWaveScale+pm->wave_y,-1);
	     glColor4f(.2,.2,.2,1.0);
	     glVertex3f((pm->totalframes%256)*2*scale, pm->wave_y,-1);   
	     glColor4f(.5,1.0,.5,1.0);
	     glVertex3f((pm->totalframes%256)*scale*2, pm->mid*-5*pm->fWaveScale+pm->wave_y,-1);
	     glEnd(); 
	  
	   
	     glTranslatef(0,.075, 0);
	     glBegin(GL_LINE_STRIP);
	     glColor4f(1.0,0,0,1.0);
	     glVertex3f((pm->totalframes%256)*2*scale, pm->bass_att*5*pm->fWaveScale+pm->wave_y,-1);
	     glColor4f(.2,.2,.2,1.0);
	     glVertex3f((pm->totalframes%256)*2*scale, pm->wave_y,-1);   
	     glColor4f(1.0,.5,.5,1.0);
	     glVertex3f((pm->totalframes%256)*scale*2, pm->bass*-5*pm->fWaveScale+pm->wave_y,-1);
	     glEnd(); 
	     

	     glPopMatrix();
	  break;
	  
	case 0://circular waveforms 
	  //  float co;
	  //	  glPushMatrix(); 
	  /*
	  if(pm->correction)
	    {
	      glTranslatef(pm->renderTarget->texsize*.5,pm->renderTarget->texsize*.5, 0);
	      glScalef(1.0,pm->vw/(float)pm->vh,1.0);
	      glTranslatef((-pm->renderTarget->texsize*.5) ,(-pm->renderTarget->texsize*.5),0);   
	    }
	  */
	  pm->wave_y=-1*(pm->wave_y-1.0);
 
	  glBegin(GL_LINE_STRIP);
	 
	  for ( x=0;x<pm->numsamples;x++)
	    { float inv_nverts_minus_one = 1.0f/(float)(pm->numsamples);
	    //co= -(fabs(x-((pm->numsamples*.5)-1))/pm->numsamples)+1;
	      // printf("%d %f\n",x,co);
	      //theta=x*(6.28/pm->numsamples);
	      //r= ((1+2*pm->wave_mystery)*(pm->renderTarget->texsize/5.0)+
	      //  ( co*pm->pcmdataL[x]+ (1-co)*pm->pcmdataL[-(x-(pm->numsamples-1))])
	      //  *25*pm->fWaveScale);
	      r=(0.5 + 0.4f*.12*pm->pcmdataR[x]*pm->fWaveScale + pm->wave_mystery)*.5;
	      theta=(x)*inv_nverts_minus_one*6.28f + pm->Time*0.2f;
	      /* 
	      if (x < 51)
		{
		  float mix = x/51.0;
		  mix = 0.5f - 0.5f*cosf(mix * 3.1416f);
		  float rad_2 = 0.5f + 0.4f*.12*pm->pcmdataR[x]*pm->fWaveScale + pm->wave_mystery;
		  r = rad_2*(1.0f-mix) + r*(mix);
		}
	      */
	      glVertex3f((r*cos(theta)+pm->wave_x), (r*sin(theta)+pm->wave_y),-1);
	    }

	  //	  r= ( (1+2*pm->wave_mystery)*(pm->renderTarget->texsize/5.0)+
	  //     (0.5*pm->pcmdataL[0]+ 0.5*pm->pcmdataL[pm->numsamples-1])
	  //      *20*pm->fWaveScale);
      
	  //glVertex3f(r*cos(0)+(pm->wave_x*pm->renderTarget->texsize),r*sin(0)+(pm->wave_y*pm->renderTarget->texsize),-1);

	  glEnd();
	  /*
	  glBegin(GL_LINE_LOOP);
	  
	  for ( x=0;x<(512/pcmbreak);x++)
	    {
	      theta=(blockstart+x)*((6.28*pcmbreak)/512.0);
	      r= ((1+2*pm->wave_mystery)*(pm->renderTarget->texsize/5.0)+fdata_buffer[fbuffer][0][blockstart+x]*.0025*pm->fWaveScale);
	      
	      glVertex3f(r*cos(theta)+(pm->wave_x*pm->renderTarget->texsize),r*sin(theta)+(wave_y*pm->renderTarget->texsize),-1);
	    }
	  glEnd();
	  */
	  //glPopMatrix();

	  break;	
	
	case 1://circularly moving waveform
	  //  float co;
	  glPushMatrix(); 
	  
	  glTranslatef(.5,.5, 0);
	  glScalef(1.0,pm->vw/(float)pm->vh,1.0);
	  glTranslatef((-.5) ,(-.5),0);   

	  pm->wave_y=-1*(pm->wave_y-1.0);

	  glBegin(GL_LINE_STRIP);
	  //theta=(frame%512)*(6.28/512.0);

	  for ( x=1;x<(512-32);x++)
	    {
	      //co= -(abs(x-255)/512.0)+1;
	      // printf("%d %f\n",x,co);
	      //theta=((pm->frame%256)*(2*6.28/512.0))+pm->pcmdataL[x]*.2*pm->fWaveScale;
	      //r= ((1+2*pm->wave_mystery)*(pm->renderTarget->texsize/5.0)+
	      //   (pm->pcmdataL[x]-pm->pcmdataL[x-1])*80*pm->fWaveScale);
	      theta=pm->pcmdataL[x+32]*0.06*pm->fWaveScale * 1.57 + pm->Time*2.3;
	      r=(0.53 + 0.43*pm->pcmdataR[x]*0.12*pm->fWaveScale+ pm->wave_mystery)*.5;

	      glVertex3f((r*cos(theta)+pm->wave_x),(r*sin(theta)+pm->wave_y),-1);
	    }

	  glEnd(); 
	  /*
	  pm->wave_y=-1*(pm->wave_y-1.0);  
	  wave_x_temp=(pm->wave_x*.75)+.125;	
	  wave_x_temp=-(wave_x_temp-1); 

	  glBegin(GL_LINE_STRIP);

	    
	  
	  for (x=0; x<512-32; x++)
	    {
	      float rad = (.53 + 0.43*pm->pcmdataR[x]) + pm->wave_mystery;
	      float ang = pm->pcmdataL[x+32] * 1.57f + pm->Time*2.3f;
	      glVertex3f((rad*cosf(ang)*.2*scale*pm->fWaveScale + wave_x_temp)*pm->renderTarget->texsize,(rad*sinf(ang)*pm->fWaveScale*.2*scale + pm->wave_y)*pm->renderTarget->texsize,-1);
	      
	    }
	  glEnd();
	  */
	  glPopMatrix();

	  break;
	  
	case 2://EXPERIMENTAL

	  glPushMatrix();


	  pm->wave_y=-1*(pm->wave_y-1.0);  


	  glBegin(GL_LINE_STRIP);

	  for (x=0; x<512-32; x++)
	    {
	      
	      glVertex3f((pm->pcmdataR[x]*pm->fWaveScale*0.5 + pm->wave_x),( (pm->pcmdataL[x+32]*pm->fWaveScale*0.5 + pm->wave_y)),-1);
 
	    }
	  glEnd();	
	  
   
	  glPopMatrix();
	  break;

	case 3://EXPERIMENTAL
	  glPushMatrix();	  
	  
	  pm->wave_y=-1*(pm->wave_y-1.0);  
	  //wave_x_temp=(pm->wave_x*.75)+.125;	
	  //wave_x_temp=-(wave_x_temp-1); 
	  
	 

	  glBegin(GL_LINE_STRIP);

	  for (x=0; x<512-32; x++)
	    {
	      
	      glVertex3f((pm->pcmdataR[x] * pm->fWaveScale*0.5 + pm->wave_x),( (pm->pcmdataL[x+32]*pm->fWaveScale*0.5 + pm->wave_y)),-1);
 
	    }
	  glEnd();	
	  
	  glPopMatrix();
	  break;

	case 4://single x-axis derivative waveform
	  {
	  glPushMatrix();
	  pm->wave_y=-1*(pm->wave_y-1.0);	  
	  glTranslatef(.5,.5, 0);
	  glRotated(-pm->wave_mystery*90,0,0,1);
	  glTranslatef(-.5,-.5, 0);
	 
	  float w1 = 0.45f + 0.5f*(pm->wave_mystery*0.5f + 0.5f);	       
	  float w2 = 1.0f - w1;
	  float xx[512],yy[512];
				
	  glBegin(GL_LINE_STRIP);
	  for (int i=0; i<512; i++)
	    {
	     xx[i] = -1.0f + 2.0f*(i/512.0) + pm->wave_x;
	     yy[i] =0.4* pm->pcmdataL[i]*0.47f*pm->fWaveScale + pm->wave_y;
	     xx[i] += 0.4*pm->pcmdataR[i]*0.44f*pm->fWaveScale;				      
	      
	      if (i>1)
		{
		  xx[i] = xx[i]*w2 + w1*(xx[i-1]*2.0f - xx[i-2]);
		  yy[i] = yy[i]*w2 + w1*(yy[i-1]*2.0f - yy[i-2]);
		}
	      glVertex3f(xx[i],yy[i],-1);
	    }

	  glEnd();

	  /*
	  pm->wave_x=(pm->wave_x*.75)+.125;	  
	  pm->wave_x=-(pm->wave_x-1); 
	  glBegin(GL_LINE_STRIP);
	 
	  for ( x=1;x<512;x++)
	    {
	      dy_adj=  pm->pcmdataL[x]*20*pm->fWaveScale-pm->pcmdataL[x-1]*20*pm->fWaveScale;
	      glVertex3f((x*(pm->renderTarget->texsize/512))+dy_adj, pm->pcmdataL[x]*20*pm->fWaveScale+pm->renderTarget->texsize*pm->wave_x,-1);
	    }
	  glEnd(); 
	  */
	  glPopMatrix();
	  }
	  break;

	case 5://EXPERIMENTAL
	  glPushMatrix();
	  	  
	
	  
	  pm->wave_y=-1*(pm->wave_y-1.0);  
	 
	  cos_rot = cosf(pm->Time*0.3f);
	  sin_rot = sinf(pm->Time*0.3f);

	  glBegin(GL_LINE_STRIP);

	  for (x=0; x<512; x++)
	    {	      
	      float x0 = (pm->pcmdataR[x]*pm->pcmdataL[x+32] + pm->pcmdataL[x+32]*pm->pcmdataR[x]);
	      float y0 = (pm->pcmdataR[x]*pm->pcmdataR[x] - pm->pcmdataL[x+32]*pm->pcmdataL[x+32]);
	      glVertex3f(((x0*cos_rot - y0*sin_rot)*pm->fWaveScale*0.5 + pm->wave_x),( (x0*sin_rot + y0*cos_rot)*pm->fWaveScale*0.5 + pm->wave_y) ,-1);
 
	    }
	  glEnd();	
	  
	 
	  
	  glPopMatrix();
	  break;

	case 6://single waveform


	  glTranslatef(0,0, -1);
	  
	  //glMatrixMode(GL_MODELVIEW);
	  glPushMatrix();
	  //	  	  glLoadIdentity();
	  
	  glTranslatef(.5,.5, 0);
	  glRotated(-pm->wave_mystery*90,0,0,1);
	  
	  wave_x_temp=-2*0.4142*(fabs(fabs(pm->wave_mystery)-.5)-.5);
	  glScalef(1.0+wave_x_temp,1.0,1.0);
	  glTranslatef(-.5,-.5, 0);
	  wave_x_temp=-1*(pm->wave_x-1.0);

	  glBegin(GL_LINE_STRIP);
	  //	  wave_x_temp=(wave_x*.75)+.125;	  
	  //	  wave_x_temp=-(wave_x_temp-1);
	  for ( x=0;x<pm->numsamples;x++)
	    {
     
	      //glVertex3f(x*scale, fdata_buffer[fbuffer][0][blockstart+x]*.0012*fWaveScale+renderTarget->texsize*wave_x_temp,-1);
	      glVertex3f(x/(float)pm->numsamples, pm->pcmdataR[x]*.04*pm->fWaveScale+wave_x_temp,-1);

	      //glVertex3f(x*scale, renderTarget->texsize*wave_y_temp,-1);
	    }
	  //	  printf("%f %f\n",renderTarget->texsize*wave_y_temp,wave_y_temp);
	  glEnd(); 
	    glPopMatrix();
	  break;
	  
	case 7://dual waveforms

	  glPushMatrix();  

	  glTranslatef(.5,.5, 0);
	  glRotated(-pm->wave_mystery*90,0,0,1);
	  
	  wave_x_temp=-2*0.4142*(fabs(fabs(pm->wave_mystery)-.5)-.5);
	  glScalef(1.0+wave_x_temp,1.0,1.0);
	     glTranslatef(-.5,-.5, 0);

         wave_y_temp=-1*(pm->wave_x-1);

		  glBegin(GL_LINE_STRIP);
	 
	  for ( x=0;x<pm->numsamples;x++)
	    {
     
	      glVertex3f(x/(float)pm->numsamples, pm->pcmdataL[x]*.04*pm->fWaveScale+(wave_y_temp+(pm->wave_y*pm->wave_y*.5)),-1);
	    }
	  glEnd(); 

	  glBegin(GL_LINE_STRIP);
	 

	  for ( x=0;x<pm->numsamples;x++)
	    {
     
	      glVertex3f(x/(float)pm->numsamples, pm->pcmdataR[x]*.04*pm->fWaveScale+(wave_y_temp-(pm->wave_y*pm->wave_y*.5)),-1);
	    }
	  glEnd(); 
	  glPopMatrix();
     break;
     
	default:  
 glBegin(GL_LINE_LOOP);
	  
	  for ( x=0;x<512;x++)
	    {
	      theta=(x)*(6.28/512.0);
	      r= (0.2+pm->pcmdataL[x]*.002);
	      
	      glVertex3f(r*cos(theta)+pm->wave_x,r*sin(theta)+pm->wave_y,-1);
	    }
	  glEnd();

glBegin(GL_LINE_STRIP);
	
	  for ( x=0;x<512;x++)
	    {
	      glVertex3f(x*scale, pm->pcmdataL[x]*.04*pm->fWaveScale+((pm->wave_x+.1)),-1);
	    }
	  glEnd();
	  
	  glBegin(GL_LINE_STRIP);
	  
	 for ( x=0;x<512;x++)
	    {
	      glVertex3f(x*scale, pm->pcmdataR[x]*.04*pm->fWaveScale+((pm->wave_x-.1)),-1);
	      
	    }
	  glEnd();
     break;
         if (pm->bWaveThick==1)  glLineWidth( (pm->renderTarget->texsize < 512) ? 1 : 2*pm->renderTarget->texsize/512); 
}	
      glLineWidth( pm->renderTarget->texsize < 512 ? 1 : pm->renderTarget->texsize/512);
      glDisable(GL_LINE_STIPPLE);
}

void maximize_colors( projectM_t *pm )
{

 float wave_r_switch=0,wave_g_switch=0,wave_b_switch=0;
 //wave color brightening
      //
      //forces max color value to 1.0 and scales
      // the rest accordingly
 if(pm->nWaveMode==2 || pm->nWaveMode==5)
   {
	switch(pm->renderTarget->texsize)
			{
			case 256:  pm->wave_o *= 0.07f; break;
			case 512:  pm->wave_o *= 0.09f; break;
			case 1024: pm->wave_o *= 0.11f; break;
			case 2048: pm->wave_o *= 0.13f; break;
			}
   }

 else if(pm->nWaveMode==3)
   {
	switch(pm->renderTarget->texsize)
			{
			case 256:  pm->wave_o *= 0.075f; break;
			case 512:  pm->wave_o *= 0.15f; break;
			case 1024: pm->wave_o *= 0.22f; break;
			case 2048: pm->wave_o *= 0.33f; break;
			}
	pm->wave_o*=1.3f;
	pm->wave_o*=powf(pm->treb ,2.0f);
   }

      if (pm->bMaximizeWaveColor==1)  
	{
	  if(pm->wave_r>=pm->wave_g && pm->wave_r>=pm->wave_b)   //red brightest
	    {
	      wave_b_switch=pm->wave_b*(1/pm->wave_r);
	      wave_g_switch=pm->wave_g*(1/pm->wave_r);
	      wave_r_switch=1.0;
	    }
	  else if   (pm->wave_b>=pm->wave_g && pm->wave_b>=pm->wave_r)         //blue brightest
	    {  
	      wave_r_switch=pm->wave_r*(1/pm->wave_b);
	      wave_g_switch=pm->wave_g*(1/pm->wave_b);
	      wave_b_switch=1.0;
	      
	    }	
	
	  else  if (pm->wave_g>=pm->wave_b && pm->wave_g>=pm->wave_r)         //green brightest
	    {
	      wave_b_switch=pm->wave_b*(1/pm->wave_g);
	      wave_r_switch=pm->wave_r*(1/pm->wave_g);
	      wave_g_switch=1.0;
	    }
 
	
	  glColor4f(wave_r_switch, wave_g_switch, wave_b_switch, pm->wave_o);
	}
      else
	{ 
	  glColor4f(pm->wave_r, pm->wave_g, pm->wave_b, pm->wave_o);
	}
      
}


void modulate_opacity_by_volume( projectM_t *pm )

{
 //modulate volume by opacity
      //
      //set an upper and lower bound and linearly
      //calculate the opacity from 0=lower to 1=upper
      //based on current volume


      if (pm->bModWaveAlphaByVolume==1)
	{if (pm->vol<=pm->fModWaveAlphaStart)  pm->wave_o=0.0;       
	else if (pm->vol>=pm->fModWaveAlphaEnd) pm->wave_o=pm->fWaveAlpha;
	else pm->wave_o=pm->fWaveAlpha*((pm->vol-pm->fModWaveAlphaStart)/(pm->fModWaveAlphaEnd-pm->fModWaveAlphaStart));}
      else pm->wave_o=pm->fWaveAlpha;
}

void draw_motion_vectors( projectM_t *pm )

{
  int x,y;

  float offsetx=pm->mv_dx, intervalx=1.0/(float)pm->mv_x;
  float offsety=pm->mv_dy, intervaly=1.0/(float)pm->mv_y;
  //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
  glPointSize(pm->mv_l);
  glColor4f(pm->mv_r, pm->mv_g, pm->mv_b, pm->mv_a);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();
   glTranslated(0, 0, -9);

  glBegin(GL_POINTS);
  for (x=0;x<pm->mv_x;x++){
    for(y=0;y<pm->mv_y;y++){
      glVertex3f(offsetx+x*intervalx,offsety+y*intervaly,-1);	  
    }}
  
    glEnd();
    glPopMatrix();    
}


void draw_borders( projectM_t *pm )
{
  //Draw Borders
  float of=pm->ob_size*.5;
  float iff=pm->ib_size*.5;
  float texof=1.0-of;

  //no additive drawing for borders
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
  
  glTranslatef(0,0,-1);

  glColor4d(pm->ob_r,pm->ob_g,pm->ob_b,pm->ob_a);
  
  glRectd(0,0,of,1);
  glRectd(of,0,texof,of);
  glRectd(texof,0,1,1);
  glRectd(of,1,texof,texof);
  glColor4d(pm->ib_r,pm->ib_g,pm->ib_b,pm->ib_a);
  glRectd(of,of,of+iff,texof);
  glRectd(of+iff,of,texof-iff,of+iff);
  glRectd(texof-iff,of,texof,texof);
  glRectd(of+iff,texof,texof-iff,texof-iff);
  
}



void draw_title_to_texture( projectM_t *pm )
{
  
    if (pm->drawtitle>80) 
      //    if(1)
      {
      glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
      glColor4f(1.0,1.0,1.0,1.0);
      glPushMatrix();
     
      glTranslatef(0,0.5, -1);
    
      glScalef(0.0025,-0.0025,30*.0025);
      //glTranslatef(0,0, 1.0);
      poly_font->FaceSize( 22);
    
      glRasterPos2f(0.0, 0.0);

   if ( pm->title != NULL ) {
      poly_font->Render(pm->title );
      } else {
	poly_font->Render("Unknown" );
      }
      glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
      glPopMatrix();
      pm->drawtitle=0;
    }
    
}

void draw_title_to_screen( projectM_t *pm )
{

  if(pm->drawtitle>0)
    { 
      float easein = ((80-pm->drawtitle)*.0125);
      float easein2 = easein * easein;
      float easein3 = .0025/((-easein2)+1.0);

      glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
      glColor4f(1.0,1.0,1.0,1.0);
      glPushMatrix();


      //glTranslatef(pm->vw*.5,pm->vh*.5 , -1.0);
      glTranslatef(0,0.5 , -1.0);

      glScalef(easein3,easein3,30*.0025);

      glRotatef(easein2*360,1,0,0);


      //glTranslatef(-.5*pm->vw,0, 0.0);
      
      //poly_font->Depth(1.0);  
      poly_font->FaceSize(22);

      glRasterPos2f(0.0, 0.0);
      if ( pm->title != NULL ) {
	poly_font->Render(pm->title );
      } else {
	poly_font->Render("Unknown" );
      }
      // poly_font->Depth(0.0);
      glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
      glPopMatrix();	
      
      pm->drawtitle++;

    }
}

void draw_title( projectM_t *pm )
{
  //glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
    
    glColor4f(1.0,1.0,1.0,1.0);
  //  glPushMatrix();
  //  glTranslatef(pm->vw*.001,pm->vh*.03, -1);
  //  glScalef(pm->vw*.015,pm->vh*.025,0);

      glRasterPos2f(0.01, 0.05);
      title_font->FaceSize( 20*(pm->vh/512.0));
       
      if ( pm->title != NULL ) {
       	 title_font->Render(pm->title );
      } else {
       	 title_font->Render("Unknown" );
      }
      //  glPopMatrix();
      //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
     
    
}
void draw_preset( projectM_t *pm )
{ 
  //glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
    
  glColor4f(1.0,1.0,1.0,1.0);
      //      glPushMatrix();
      //glTranslatef(pm->vw*.001,pm->vh*-.01, -1);
      //glScalef(pm->vw*.003,pm->vh*.004,0);

   
        glRasterPos2f(0.01, 0.01);

	title_font->FaceSize(12*(pm->vh/512.0));
	if(pm->noSwitch) title_font->Render("[LOCKED]  " );
	title_font->FaceSize(20*(pm->vh/512.0));
        title_font->Render(pm->presetName );

                 
        
	//glPopMatrix();
	// glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    
}

void draw_help( projectM_t *pm )
{ //glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
	 	       
      glColor4f(1.0,1.0,1.0,1.0);
      glPushMatrix();
       glTranslatef(0,1, 0);
      //glScalef(pm->vw*.02,pm->vh*.02 ,0);

     
       title_font->FaceSize( 18*(pm->vh/512.0));

      glRasterPos2f(0.01, -0.05);
       title_font->Render("Help");  
      
      glRasterPos2f(0.01, -0.09);     
       title_font->Render("----------------------------");  
      
      glRasterPos2f(0.01, -0.13); 
       title_font->Render("F1: This help menu");
  
      glRasterPos2f(0.01, -0.17);
       title_font->Render("F2: Show song title");
      
      glRasterPos2f(0.01, -0.21);
       title_font->Render("F3: Show preset name");
 
       glRasterPos2f(0.01, -0.25);
       title_font->Render("F4: Show Rendering Settings");
 
      glRasterPos2f(0.01, -0.29);
       title_font->Render("F5: Show FPS");

      glRasterPos2f(0.01, -0.35);
       title_font->Render("F: Fullscreen");

      glRasterPos2f(0.01, -0.39);
       title_font->Render("L: Lock/Unlock Preset");

      glRasterPos2f(0.01, -0.43);
       title_font->Render("M: Show Menu");
      
      glRasterPos2f(0.01, -0.49);
       title_font->Render("R: Random preset");
      
      glRasterPos2f(0.01, -0.53);
       title_font->Render("N: Next preset");
 
      glRasterPos2f(0.01, -0.57);
       title_font->Render("P: Previous preset");

       glPopMatrix();
      //         glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    
}
void draw_stats( projectM_t *pm)
{
  char buffer[128];  
  float offset= (pm->showfps%2 ? -0.05 : 0.0);
  // glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
 
  glColor4f(1.0,1.0,1.0,1.0);
  glPushMatrix();
  glTranslatef(0.01,1, 0);
  glRasterPos2f(0, -.05+offset);
  other_font->FaceSize(18*(pm->vh/512.0));

  sprintf( buffer, " texsize: %d", pm->renderTarget->texsize);
  other_font->Render(buffer);
  
  glRasterPos2f(0, -.09+offset);  
  other_font->Render((pm->renderTarget->usePbuffers ? "pbuffers: on" : "pbuffers: off"));

  glRasterPos2f(0, -.13+offset); 
  sprintf( buffer, "    mesh: %dx%d", pm->gx,pm->gy);
  other_font->Render(buffer);
  
  glPopMatrix();
  // glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    
    
}
void draw_fps( projectM_t *pm, float realfps)
{
  char bufferfps[20];  
  sprintf( bufferfps, "%.1f fps", realfps);
  // glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
 
  glColor4f(1.0,1.0,1.0,1.0);
  glPushMatrix();
  glTranslatef(0.01,1, 0);
  glRasterPos2f(0, -0.05);
  title_font->FaceSize(20*(pm->vh/512.0));
   title_font->Render(bufferfps);
  
  glPopMatrix();
  // glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    
    
}
//Here we render the interpolated mesh, and then apply the texture to it.  
//Well, we actually do the inverse, but its all the same.
void render_interpolation( projectM_t *pm )
{
  
  int x,y;  
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslated(0, 0, -9);
  
  glColor4f(0.0, 0.0, 0.0,pm->decay);          
    
  glEnable(GL_TEXTURE_2D);
  glBindTexture( GL_TEXTURE_2D, pm->renderTarget->textureID[0] );
    glLoadIdentity();
  glTranslated(0, 0, -9);
#ifdef MACOS

    /** Bind the stashed texture */
    if ( pm->renderTarget->pbuffer != NULL ) {
       glBindTexture( GL_TEXTURE_2D, pm->renderTarget->textureID[0] );
#ifdef DEBUG
        if ( glGetError() ) {
            fprintf( debugFile, "failed to bind texture\n" );
            fflush( debugFile );
          }
#endif
      }
#endif
  
  for (x=0;x<pm->gx - 1;x++){
    glBegin(GL_TRIANGLE_STRIP);
    for(y=0;y<pm->gy;y++){
      glTexCoord4f(pm->x_mesh[x][y], pm->y_mesh[x][y],-1,1); 
      glVertex4f(pm->gridx[x][y], pm->gridy[x][y],-1,1);
      glTexCoord4f(pm->x_mesh[x+1][y], pm->y_mesh[x+1][y],-1,1); 
      glVertex4f(pm->gridx[x+1][y], pm->gridy[x+1][y],-1,1);
    }
    glEnd();	
  }

#ifdef MACOS
    /** Re-bind the pbuffer */
    if ( pm->renderTarget->pbuffer != NULL ) {
      glBindTexture( GL_TEXTURE_2D, pm->renderTarget->textureID[0] );
      }
#endif

    glDisable(GL_TEXTURE_2D);
  }

void do_per_frame( projectM_t *pm )
{
 glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslated(0, 0, -9);
  //Texture wrapping( clamp vs. wrap)
  if (pm->bTexWrap==0){
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);}
  else{ glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);}
      

  //      glRasterPos2i(0,0);
      //      glClear(GL_COLOR_BUFFER_BIT);
      //      glColor4d(0.0, 0.0, 0.0,1.0);     
       
  //      glMatrixMode(GL_TEXTURE);
    //  glLoadIdentity();

      glRasterPos2i(0,0);
      glClear(GL_COLOR_BUFFER_BIT);
      glColor4d(0.0, 0.0, 0.0,1.0);

 glMatrixMode(GL_TEXTURE);
      glLoadIdentity();

      /*
      glTranslatef(pm->cx,pm->cy, 0);
     if(pm->correction)  glScalef(1,pm->vw/(float)pm->vh,1);

      if(!isPerPixelEqn(ROT_OP)) {
	//	printf("ROTATING: rot = %f\n", rot);
	glRotatef(pm->rot*90, 0, 0, 1);
      }
      if(!isPerPixelEqn(SX_OP)) glScalef(1/pm->sx,1,1);     
      if(!isPerPixelEqn(SY_OP)) glScalef(1,1/pm->sy,1); 

      if(pm->correction)glScalef(1,pm->vh/(float)pm->vw,1);
            glTranslatef((-pm->cx) ,(-pm->cy),0);  
      */

      if(!isPerPixelEqn(DX_OP)) glTranslatef(-pm->dx,0,0);   
      if(!isPerPixelEqn(DY_OP)) glTranslatef(0 ,-pm->dy,0);  
      
}


//Actually draws the texture to the screen
//
//The Video Echo effect is also applied here
void render_texture_to_screen( projectM_t *pm )
{ 
      int flipx=1,flipy=1;
   glBindTexture( GL_TEXTURE_2D,pm->renderTarget->textureID[0] );
     glMatrixMode(GL_TEXTURE);  
     glLoadIdentity();

  glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glTranslatef(0, 0, -9);  
     
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  GL_DECAL);
      glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
      
      //       glClear(GL_ACCUM_BUFFER_BIT);
      glColor4d(0.0, 0.0, 0.0,1.0f);

   glBegin(GL_QUADS);
     glVertex4d(-0.5,-0.5,-1,1);
     glVertex4d(-0.5,  0.5,-1,1);
     glVertex4d(0.5,  0.5,-1,1);
     glVertex4d(0.5, -0.5,-1,1);
      glEnd();

     

      //      glBindTexture( GL_TEXTURE_2D, tex2 );
      glEnable(GL_TEXTURE_2D); 
      glBindTexture( GL_TEXTURE_2D, pm->renderTarget->textureID[0] );
//      glBindTexture( GL_TEXTURE_2D, pm->renderTarget->textureID );

      // glAccum(GL_LOAD,0);
      // if (bDarken==1)  glBlendFunc(GL_SRC_COLOR,GL_ZERO); 
	
      //Draw giant rectangle and texture it with our texture!
      glBegin(GL_QUADS);
      glTexCoord4d(0, 1,0,1); glVertex4d(-0.5,-0.5,-1,1);
      glTexCoord4d(0, 0,0,1); glVertex4d(-0.5,  0.5,-1,1);
      glTexCoord4d(1, 0,0,1); glVertex4d(0.5,  0.5,-1,1);
      glTexCoord4d(1, 1,0,1); glVertex4d(0.5, -0.5,-1,1);
      glEnd();
       
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  //  if (bDarken==1)  glBlendFunc(GL_SRC_COLOR,GL_ONE_MINUS_SRC_ALPHA); 

  // if (bDarken==1) { glAccum(GL_ACCUM,1-fVideoEchoAlpha); glBlendFunc(GL_SRC_COLOR,GL_ZERO); }

       glMatrixMode(GL_TEXTURE);

      //draw video echo
      glColor4f(0.0, 0.0, 0.0,pm->fVideoEchoAlpha);
      glTranslatef(.5,.5,0);
      glScalef(1.0/pm->fVideoEchoZoom,1.0/pm->fVideoEchoZoom,1);
       glTranslatef(-.5,-.5,0);    

      switch (((int)pm->nVideoEchoOrientation))
	{
	case 0: flipx=1;flipy=1;break;
	case 1: flipx=-1;flipy=1;break;
  	case 2: flipx=1;flipy=-1;break;
	case 3: flipx=-1;flipy=-1;break;
	default: flipx=1;flipy=1; break;
	}
      glBegin(GL_QUADS);
      glTexCoord4d(0, 1,0,1); glVertex4f(-0.5*flipx,-0.5*flipy,-1,1);
      glTexCoord4d(0, 0,0,1); glVertex4f(-0.5*flipx,  0.5*flipy,-1,1);
      glTexCoord4d(1, 0,0,1); glVertex4f(0.5*flipx,  0.5*flipy,-1,1);
      glTexCoord4d(1, 1,0,1); glVertex4f(0.5*flipx, -0.5*flipy,-1,1);
      glEnd();

    
      glDisable(GL_TEXTURE_2D);
      glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);


      if (pm->bBrighten==1)
	{ 
	  glColor4f(1.0, 1.0, 1.0,1.0);
	  glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
	  glBegin(GL_QUADS);
	  glVertex4f(-0.5*flipx,-0.5*flipy,-1,1);
	  glVertex4f(-0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx, -0.5*flipy,-1,1);
	  glEnd();
	  glBlendFunc(GL_ZERO, GL_DST_COLOR);
	  glBegin(GL_QUADS);
	  glVertex4f(-0.5*flipx,-0.5*flipy,-1,1);
	  glVertex4f(-0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx, -0.5*flipy,-1,1);
	  glEnd();
	  glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
	  glBegin(GL_QUADS);
	  glVertex4f(-0.5*flipx,-0.5*flipy,-1,1);
	  glVertex4f(-0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx, -0.5*flipy,-1,1);
	  glEnd();

	  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	} 

      if (pm->bDarken==1)
	{ 
	  
	  glColor4f(1.0, 1.0, 1.0,1.0);
	  glBlendFunc(GL_ZERO,GL_DST_COLOR);
	  glBegin(GL_QUADS);
	  glVertex4f(-0.5*flipx,-0.5*flipy,-1,1);
	  glVertex4f(-0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx, -0.5*flipy,-1,1);
	  glEnd();
	  


	  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	} 
    

      if (pm->bSolarize==1)
	{ 
       
	  glColor4f(1.0, 1.0, 1.0,1.0);
	  glBlendFunc(GL_ZERO,GL_ONE_MINUS_DST_COLOR);
	  glBegin(GL_QUADS);
	  glVertex4f(-0.5*flipx,-0.5*flipy,-1,1);
	  glVertex4f(-0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx, -0.5*flipy,-1,1);
	  glEnd();
	  glBlendFunc(GL_DST_COLOR,GL_ONE);
	  glBegin(GL_QUADS);
	  glVertex4f(-0.5*flipx,-0.5*flipy,-1,1);
	  glVertex4f(-0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx, -0.5*flipy,-1,1);
	  glEnd();


	  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	} 

      if (pm->bInvert==1)
	{ 
	  glColor4f(1.0, 1.0, 1.0,1.0);
	  glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
	  glBegin(GL_QUADS);
	  glVertex4f(-0.5*flipx,-0.5*flipy,-1,1);
	  glVertex4f(-0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx, -0.5*flipy,-1,1);
	  glEnd();
	  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	} 
 
}
void render_texture_to_studio( projectM_t *pm )
{ 
      int x,y;
      int flipx=1,flipy=1;
 
     glMatrixMode(GL_TEXTURE);  
     glLoadIdentity();

     glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glTranslatef(0, 0, -9);  
     
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  GL_DECAL);
      glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
      
      //       glClear(GL_ACCUM_BUFFER_BIT);
      glColor4f(0.0, 0.0, 0.0,0.04);
      

   glBegin(GL_QUADS);
     glVertex4d(-0.5,-0.5,-1,1);
     glVertex4d(-0.5,  0.5,-1,1);
     glVertex4d(0.5,  0.5,-1,1);
     glVertex4d(0.5, -0.5,-1,1);
      glEnd();


      glColor4f(0.0, 0.0, 0.0,1.0);
      
      glBegin(GL_QUADS);
      glVertex4d(-0.5,0,-1,1);
      glVertex4d(-0.5,  0.5,-1,1);
      glVertex4d(0.5,  0.5,-1,1);
      glVertex4d(0.5, 0,-1,1);
      glEnd();
     
     glBegin(GL_QUADS);
     glVertex4d(0,-0.5,-1,1);
     glVertex4d(0,  0.5,-1,1);
     glVertex4d(0.5,  0.5,-1,1);
     glVertex4d(0.5, -0.5,-1,1);
     glEnd();

     glPushMatrix();
     glTranslatef(.25, .25, 0);
     glScalef(.5,.5,1);
     
     glEnable(GL_TEXTURE_2D);
    

      //Draw giant rectangle and texture it with our texture!
      glBegin(GL_QUADS);
      glTexCoord4d(0, 1,0,1); glVertex4d(-0.5,-0.5,-1,1);
      glTexCoord4d(0, 0,0,1); glVertex4d(-0.5,  0.5,-1,1);
      glTexCoord4d(1, 0,0,1); glVertex4d(0.5,  0.5,-1,1);
      glTexCoord4d(1, 1,0,1); glVertex4d(0.5, -0.5,-1,1);
      glEnd();
       
      glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 

      glMatrixMode(GL_TEXTURE);

      //draw video echo
      glColor4f(0.0, 0.0, 0.0,pm->fVideoEchoAlpha);
      glTranslated(.5,.5,0);
      glScaled(1/pm->fVideoEchoZoom,1/pm->fVideoEchoZoom,1);
      glTranslated(-.5,-.5,0);    

      switch (((int)pm->nVideoEchoOrientation))
	{
	case 0: flipx=1;flipy=1;break;
	case 1: flipx=-1;flipy=1;break;
  	case 2: flipx=1;flipy=-1;break;
	case 3: flipx=-1;flipy=-1;break;
	default: flipx=1;flipy=1; break;
	}
      glBegin(GL_QUADS);
      glTexCoord4d(0, 1,0,1); glVertex4f(-0.5*flipx,-0.5*flipy,-1,1);
      glTexCoord4d(0, 0,0,1); glVertex4f(-0.5*flipx,  0.5*flipy,-1,1);
      glTexCoord4d(1, 0,0,1); glVertex4f(0.5*flipx,  0.5*flipy,-1,1);
      glTexCoord4d(1, 1,0,1); glVertex4f(0.5*flipx, -0.5*flipy,-1,1);
      glEnd();

    
      //glDisable(GL_TEXTURE_2D);
      glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

 // if (bDarken==1) { glAccum(GL_ACCUM,fVideoEchoAlpha); glAccum(GL_RETURN,1);}


      if (pm->bInvert==1)
	{ 
	  glColor4f(1.0, 1.0, 1.0,1.0);
	  glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
	  glBegin(GL_QUADS);
	  glVertex4f(-0.5*flipx,-0.5*flipy,-1,1);
	  glVertex4f(-0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx,  0.5*flipy,-1,1);
	  glVertex4f(0.5*flipx, -0.5*flipy,-1,1);
	  glEnd();
	  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	} 

      //  glTranslated(.5,.5,0);
  //  glScaled(1/fVideoEchoZoom,1/fVideoEchoZoom,1);
      //   glTranslated(-.5,-.5,0);    
      //glTranslatef(0,.5*vh,0);

      /** Per-pixel mesh display -- bottom-right corner */
      //glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ZERO);
       
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
      glPushMatrix();
      glTranslatef(.25, -.25, 0);
      glScalef(.5,.5,1);
       glColor4f(1.0,1.0,1.0,1.0);

       for (x=0;x<pm->gx;x++){
	 glBegin(GL_LINE_STRIP);
	 for(y=0;y<pm->gy;y++){
	   glVertex4f((pm->x_mesh[x][y]-.5), (pm->y_mesh[x][y]-.5),-1,1);
	   //glVertex4f((origx[x+1][y]-.5) * vw, (origy[x+1][y]-.5) *vh ,-1,1);
	 }
	 glEnd();	
       }    
       
       for (y=0;y<pm->gy;y++){
	 glBegin(GL_LINE_STRIP);
	 for(x=0;x<pm->gx;x++){
	   glVertex4f((pm->x_mesh[x][y]-.5), (pm->y_mesh[x][y]-.5),-1,1);
	   //glVertex4f((origx[x+1][y]-.5) * vw, (origy[x+1][y]-.5) *vh ,-1,1);
	 }
	 glEnd();	
       }    
      
       /*
       for (x=0;x<pm->gx-1;x++){
	 glBegin(GL_POINTS);
	 for(y=0;y<pm->gy;y++){
	   glVertex4f((pm->origx[x][y]-.5)* pm->vw, (pm->origy[x][y]-.5)*pm->vh,-1,1);
	   glVertex4f((pm->origx[x+1][y]-.5) * pm->vw, (pm->origy[x+1][y]-.5) *pm->vh ,-1,1);
	 }
	 glEnd();	
       }    
       */
 // glTranslated(-.5,-.5,0);     glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); 

    /** Waveform display -- bottom-left */
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
       glMatrixMode(GL_MODELVIEW);
       glPushMatrix();
   glTranslatef(-.5,0, 0);

    glTranslatef(0,-0.10, 0);
   glBegin(GL_LINE_STRIP);
	     glColor4f(0,1.0,1.0,1.0);
	     glVertex3f((((pm->totalframes%256)/551.0)), pm->treb_att*-7,-1);
	     glColor4f(1.0,1.0,1.0,1.0);
	     glVertex3f((((pm->totalframes%256)/551.0)),0 ,-1);   
	     glColor4f(.5,1.0,1.0,1.0);
	     glVertex3f((((pm->totalframes%256)/551.0)), pm->treb*7,-1);
	     glEnd(); 	
	       
	       glTranslatef(0,-0.13, 0);
 glBegin(GL_LINE_STRIP);
	      glColor4f(0,1.0,0.0,1.0);
	     glVertex3f((((pm->totalframes%256)/551.0)), pm->mid_att*-7,-1);
	     glColor4f(1.0,1.0,1.0,1.0);
	     glVertex3f((((pm->totalframes%256)/551.0)),0 ,-1);   
	     glColor4f(.5,1.0,0.0,0.5);
	     glVertex3f((((pm->totalframes%256)/551.0)), pm->mid*7,-1);
	     glEnd();
	  
	   
	     glTranslatef(0,-0.13, 0);
 glBegin(GL_LINE_STRIP);
	     glColor4f(1.0,0.0,0.0,1.0);
	     glVertex3f((((pm->totalframes%256)/551.0)), pm->bass_att*-7,-1);
	     glColor4f(1.0,1.0,1.0,1.0);
	     glVertex3f((((pm->totalframes%256)/551.0)),0 ,-1);   
	     glColor4f(.7,0.2,0.2,1.0);
	     glVertex3f((((pm->totalframes%256)/551.0)), pm->bass*7,-1);
	     glEnd();

 glTranslatef(0,-0.13, 0);
 glBegin(GL_LINES);
	     
	     glColor4f(1.0,1.0,1.0,1.0);
	     glVertex3f((((pm->totalframes%256)/551.0)),0 ,-1);   
	     glColor4f(1.0,0.6,1.0,1.0);
	     glVertex3f((((pm->totalframes%256)/551.0)), pm->vol*7,-1);
	     glEnd();

	     glPopMatrix();

   glDisable(GL_TEXTURE_2D);
}


void projectM_initengine( projectM_t *pm ) {

/* PER FRAME CONSTANTS BEGIN */
 pm->zoom=1.0;
 pm->zoomexp= 1.0;
 pm->rot= 0.0;
 pm->warp= 0.0;

 pm->sx= 1.0;
 pm->sy= 1.0;
 pm->dx= 0.0;
 pm->dy= 0.0;
 pm->cx= 0.5;
 pm->cy= 0.5;

 pm->decay=.98;

 pm->wave_r= 1.0;
 pm->wave_g= 0.2;
 pm->wave_b= 0.0;
 pm->wave_x= 0.5;
 pm->wave_y= 0.5;
 pm->wave_mystery= 0.0;

 pm->ob_size= 0.0;
 pm->ob_r= 0.0;
 pm->ob_g= 0.0;
 pm->ob_b= 0.0;
 pm->ob_a= 0.0;

 pm->ib_size = 0.0;
 pm->ib_r = 0.0;
 pm->ib_g = 0.0;
 pm->ib_b = 0.0;
 pm->ib_a = 0.0;

 pm->mv_a = 0.0;
 pm->mv_r = 0.0;
 pm->mv_g = 0.0;
 pm->mv_b = 0.0;
 pm->mv_l = 1.0;
 pm->mv_x = 16.0;
 pm->mv_y = 12.0;
 pm->mv_dy = 0.02;
 pm->mv_dx = 0.02;
  
 pm->meshx = 0;
 pm->meshy = 0;
 
 pm->Time = 0;
 pm->treb = 0;
 pm->mid = 0;
 pm->bass = 0;
 pm->bass_old = 0;
 pm->treb_att = 0;
 pm->beat_sensitivity = 8.00;
 pm->mid_att = 0;
 pm->bass_att = 0;
 pm->progress = 0;
 pm->frame = 0;

    pm->avgtime = 600;
//bass_thresh = 0;

/* PER_FRAME CONSTANTS END */
 pm->fRating = 0;
 pm->fGammaAdj = 1.0;
 pm->fVideoEchoZoom = 1.0;
 pm->fVideoEchoAlpha = 0;
 pm->nVideoEchoOrientation = 0;
 
 pm->nWaveMode = 7;
 pm->bAdditiveWaves = 0;
 pm->bWaveDots = 0;
 pm->bWaveThick = 0;
 pm->bModWaveAlphaByVolume = 0;
 pm->bMaximizeWaveColor = 0;
 pm->bTexWrap = 0;
 pm->bDarkenCenter = 0;
 pm->bRedBlueStereo = 0;
 pm->bBrighten = 0;
 pm->bDarken = 0;
 pm->bSolarize = 0;
 pm->bInvert = 0;
 pm->bMotionVectorsOn = 1;
 
 pm->fWaveAlpha =1.0;
 pm->fWaveScale = 1.0;
 pm->fWaveSmoothing = 0;
 pm->fWaveParam = 0;
 pm->fModWaveAlphaStart = 0;
 pm->fModWaveAlphaEnd = 0;
 pm->fWarpAnimSpeed = 0;
 pm->fWarpScale = 0;
 pm->fShader = 0;


/* PER_PIXEL CONSTANTS BEGIN */
pm->x_per_pixel = 0;
pm->y_per_pixel = 0;
pm->rad_per_pixel = 0;
pm->ang_per_pixel = 0;

/* PER_PIXEL CONSTANT END */


/* Q AND T VARIABLES START */

pm->q1 = 0;
pm->q2 = 0;
pm->q3 = 0;
pm->q4 = 0;
pm->q5 = 0;
pm->q6 = 0;
pm->q7 = 0;
pm->q8 = 0;


/* Q AND T VARIABLES END */

//per pixel meshes
 pm->zoom_mesh = NULL;
 pm->zoomexp_mesh = NULL;
 pm->rot_mesh = NULL;
 

 pm->sx_mesh = NULL;
 pm->sy_mesh = NULL;
 pm->dx_mesh = NULL;
 pm->dy_mesh = NULL;
 pm->cx_mesh = NULL;
 pm->cy_mesh = NULL;

 pm->x_mesh = NULL;
 pm->y_mesh = NULL;
 pm->rad_mesh = NULL;
 pm->theta_mesh = NULL;

//custom wave per point meshes
  }

/* Reinitializes the engine variables to a default (conservative and sane) value */
void projectM_resetengine( projectM_t *pm ) {

  pm->doPerPixelEffects = 1;
  pm->doIterative = 1;

  pm->zoom=1.0;
  pm->zoomexp= 1.0;
  pm->rot= 0.0;
  pm->warp= 0.0;
  
  pm->sx= 1.0;
  pm->sy= 1.0;
  pm->dx= 0.0;
  pm->dy= 0.0;
  pm->cx= 0.5;
  pm->cy= 0.5;

  pm->decay=.98;
  
  pm->wave_r= 1.0;
  pm->wave_g= 0.2;
  pm->wave_b= 0.0;
  pm->wave_x= 0.5;
  pm->wave_y= 0.5;
  pm->wave_mystery= 0.0;

  pm->ob_size= 0.0;
  pm->ob_r= 0.0;
  pm->ob_g= 0.0;
  pm->ob_b= 0.0;
  pm->ob_a= 0.0;

  pm->ib_size = 0.0;
  pm->ib_r = 0.0;
  pm->ib_g = 0.0;
  pm->ib_b = 0.0;
  pm->ib_a = 0.0;

  pm->mv_a = 0.0;
  pm->mv_r = 0.0;
  pm->mv_g = 0.0;
  pm->mv_b = 0.0;
  pm->mv_l = 1.0;
  pm->mv_x = 16.0;
  pm->mv_y = 12.0;
  pm->mv_dy = 0.02;
  pm->mv_dx = 0.02;
  
  pm->meshx = 0;
  pm->meshy = 0;
 
  pm->Time = 0;
  pm->treb = 0;
  pm->mid = 0;
  pm->bass = 0;
  pm->treb_att = 0;
  pm->mid_att = 0;
  pm->bass_att = 0;
  pm->progress = 0;
  pm->frame = 0;

// bass_thresh = 0;

/* PER_FRAME CONSTANTS END */
  pm->fRating = 0;
  pm->fGammaAdj = 1.0;
  pm->fVideoEchoZoom = 1.0;
  pm->fVideoEchoAlpha = 0;
  pm->nVideoEchoOrientation = 0;
 
  pm->nWaveMode = 7;
  pm->bAdditiveWaves = 0;
  pm->bWaveDots = 0;
  pm->bWaveThick = 0;
  pm->bModWaveAlphaByVolume = 0;
  pm->bMaximizeWaveColor = 0;
  pm->bTexWrap = 0;
  pm->bDarkenCenter = 0;
  pm->bRedBlueStereo = 0;
  pm->bBrighten = 0;
  pm->bDarken = 0;
  pm->bSolarize = 0;
 pm->bInvert = 0;
 pm->bMotionVectorsOn = 1;
 
  pm->fWaveAlpha =1.0;
  pm->fWaveScale = 1.0;
  pm->fWaveSmoothing = 0;
  pm->fWaveParam = 0;
  pm->fModWaveAlphaStart = 0;
  pm->fModWaveAlphaEnd = 0;
  pm->fWarpAnimSpeed = 0;
  pm->fWarpScale = 0;
  pm->fShader = 0;


/* PER_PIXEL CONSTANTS BEGIN */
 pm->x_per_pixel = 0;
 pm->y_per_pixel = 0;
 pm->rad_per_pixel = 0;
 pm->ang_per_pixel = 0;

/* PER_PIXEL CONSTANT END */


/* Q VARIABLES START */

 pm->q1 = 0;
 pm->q2 = 0;
 pm->q3 = 0;
 pm->q4 = 0;
 pm->q5 = 0;
 pm->q6 = 0;
 pm->q7 = 0;
 pm->q8 = 0;


 /* Q VARIABLES END */
}

/** Resets OpenGL state */
 void projectM_resetGL( projectM_t *pm, int w, int h ) {
   
    char path[1024];
    int mindim, origtexsize;

#ifdef DEBUG
    if ( debugFile != NULL ) {
        fprintf( debugFile, "projectM_resetGL(): in: %d x %d\n", w, h );
        fflush( debugFile );
      }
#endif

    /** Stash the new dimensions */
    pm->vw = w;
    pm->vh = h;

#ifdef LINUX
    if (!pm->renderTarget->usePbuffers)
      {
	createPBuffers(w,h,pm->renderTarget);
      }
#endif

    if ( pm->fbuffer != NULL ) {
        free( pm->fbuffer );
      }
    pm->fbuffer = 
        (GLubyte *)malloc( sizeof( GLubyte ) * pm->renderTarget->texsize * pm->renderTarget->texsize * 3 );

    /* Our shading model--Gouraud (smooth). */
    glShadeModel( GL_SMOOTH);
    /* Culling. */
    //    glCullFace( GL_BACK );
    //    glFrontFace( GL_CCW );
    //    glEnable( GL_CULL_FACE );
    /* Set the clear color. */
    glClearColor( 0, 0, 0, 0 );
    /* Setup our viewport. */
    glViewport( pm->vx, pm->vy, w, h );
    /*
    * Change to the projection matrix and set
    * our viewing volume.
    */
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    //    gluOrtho2D(0.0, (GLfloat) width, 0.0, (GLfloat) height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    //    glFrustum(0.0, height, 0.0,width,10,40);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDrawBuffer(GL_BACK); 
    glReadBuffer(GL_BACK); 
    glEnable(GL_BLEND); 

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
   

    glEnable( GL_LINE_SMOOTH );
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);		
    
//    glEnable(GL_POINT_SMOOTH); // disabled since it craps out on intel gpus
  
    // glCopyTexImage2D(GL_TEXTURE_2D,0,GL_RGB,0,0,renderTarget->texsize,renderTarget->texsize,0);
    //glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,renderTarget->texsize,renderTarget->texsize);
    glLineStipple(2, 0xAAAA);

    /** (Re)create the offscreen for pass 1 */
   

    rescale_per_pixel_matrices( pm );

    /** Load TTF font **/
   

    
    /**f Load the standard fonts */
    if ( title_font == NULL && other_font == NULL ) {
       

        sprintf( path, "%s%cVera.ttf", pm->fontURL, PATH_SEPARATOR );
        title_font = new FTGLPixmapFont(path);
	poly_font = new FTGLPolygonFont(path);
	sprintf( path, "%s%cVeraMono.ttf", pm->fontURL, PATH_SEPARATOR );
        other_font = new FTGLPixmapFont(path);
      
   
      }
  }

/** Sets the title to display */
void projectM_setTitle( projectM_t *pm, char *title ) {
  /*
 if (strcmp(pm->title, title)!=0)
   {printf("new title\n");
      pm->drawtitle=1; 
      
      if ( pm->title != NULL ) {
	free( pm->title );
	pm->title = NULL;
      }
      
      pm->title = (char *)wipemalloc( sizeof( char ) * ( strlen( title ) + 1 ) );
      strcpy( pm->title, title );
      
    }
  */
}
