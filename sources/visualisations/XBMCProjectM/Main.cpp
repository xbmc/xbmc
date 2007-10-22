/* 
xmms-projectM v0.99 - xmms-projectm.sourceforge.net
--------------------------------------------------

Lead Developers:  Carmelo Piccione (cep@andrew.cmu.edu) &
                  Peter Sperl (peter@sperl.com)

We have also been advised by some professors at CMU, namely Roger B. Dannenberg.
http://www-2.cs.cmu.edu/~rbd/    
  
The inspiration for this program was Milkdrop by Ryan Geiss. Obviously. 

This code is distributed under the GPL.


THANKS FOR THE CODE!!!
-------------------------------------------------
The base for this program was andy@nobugs.org's XMMS plugin tutorial
http://www.xmms.org/docs/vis-plugin.html

We used some FFT code by Takuya OOURA instead of XMMS' built-in fft code
fftsg.c - http://momonga.t.u-tokyo.ac.jp/~ooura/fft.html

For font rendering we used GLF by Roman Podobedov
glf.c - http://astronomy.swin.edu.au/~pbourke/opengl/glf/

and some beat detection code was inspired by Frederic Patin @
www.gamedev.net/reference/programming/features/beatdetection/
--

"ported" to XBMC by d4rk
d4rk@xboxmediacenter.com

*/

#include "../xbmc_vis.h"
#ifdef HAS_SDL_OPENGL
#include <GL/glew.h>
#endif
#include "libprojectM/src/projectM.h"
#include "libprojectM/src/PCM.h"
#include <string>

#define CONFIG_FILE "/config"
#define PRESETS_DIR "/visualisations/projectM"
#define FONTS_DIR "/fonts"
#define PROJECTM_DATADIR "UserData"

projectM_t *globalPM = NULL;

int maxSamples=512;

int texsize=1024;
//int gx=32,gy=24;
int gx=40,gy=30;
int wvw=640,wvh=480;
int fvw=1280,fvh=960;
int g_Width=0, g_Height=0;
int g_PosX=0, g_PosY=0;
int fps=200, fullscreen=0;
char *disp;

char g_visName[512];
void* g_device;
float g_fWaveform[2][512];

//-- Create -------------------------------------------------------------------
// Called once when the visualisation is created by XBMC. Do any setup here.
//-----------------------------------------------------------------------------
#ifndef _LINUX
extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName, float fPixelRatio)
#else
extern "C" void Create(void* pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName, float fPixelRatio)
#endif
{
  strcpy(g_visName, szVisualisationName);

   /** Initialise projectM */
    
  if (!globalPM)
      globalPM = (projectM_t *)malloc( sizeof( projectM_t ) );

  projectM_reset( globalPM );
  
  globalPM->fullscreen = fullscreen;
  globalPM->renderTarget->texsize = texsize;
  globalPM->gx=gx;
  globalPM->gy=gy;
  globalPM->fps=fps;
  globalPM->renderTarget->usePbuffers=0;
  globalPM->avgtime = 200;
  globalPM->maxsamples=maxSamples;

  char *rootdir = getenv("XBMC_HOME");
  std::string dirname;

  if (rootdir==NULL)
  {
    rootdir = ".";
  }
  dirname = string(rootdir) + "/" + string(PROJECTM_DATADIR) + string(FONTS_DIR);;
  fprintf(stderr, "ProjectM Fonts Dir: %s\n", dirname.c_str());
  
  globalPM->fontURL = (char *)malloc( sizeof( char ) * 512 );
  strncpy(globalPM->fontURL, dirname.c_str(), 512);
  
  dirname = string(rootdir) + "/" + string(PRESETS_DIR);;
  fprintf(stderr, "ProjectM Presets Dir: %s", dirname.c_str());
  
  globalPM->presetURL = (char *)malloc( sizeof( char ) * 512 );
  strncpy(globalPM->presetURL, dirname.c_str(), 512);
    
  projectM_init( globalPM );
  
  g_Width = iWidth;
  g_Height = iHeight;
  g_PosX = iPosX;
  g_PosY = iPosY;
}

//-- Start --------------------------------------------------------------------
// Called when a new soundtrack is played
//-----------------------------------------------------------------------------
extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
  //printf("Got Start Command\n");
}

//-- Stop ---------------------------------------------------------------------
// Called when the visualisation is closed by XBMC
//-----------------------------------------------------------------------------
extern "C" void Stop()
{
  if (globalPM) 
  {
    if (globalPM->fontURL)
      free(globalPM->fontURL);
    if (globalPM->presetURL)
      free(globalPM->presetURL);
    free(globalPM);
    globalPM = NULL;
  }
}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
extern "C" void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  if (iAudioDataLength>=512)
  {
    addPCM16Data(pAudioData, 512); 
  }
}


//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{ 

  glClearColor(0,0,0,0);

  glMatrixMode(GL_TEXTURE);
  glPushMatrix();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  // get current viewport since projectM modifies and does not reset
  GLint params[4];
  glGetIntegerv(GL_VIEWPORT, params);

  globalPM->vx = params[0];
  globalPM->vy = params[1];

  projectM_resetGL( globalPM, params[2], params[3] );

  glDisable(GL_DEPTH_TEST);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  renderFrame(globalPM);

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_TEXTURE);
  glPopMatrix();
  
  glEnable(GL_BLEND);          // Turn Blending On
  glDisable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_TEXTURE_2D);

  // reset viewport to what we got it
  glMatrixMode(GL_MODELVIEW);
  glViewport(params[0], params[1], params[2], params[3]);

}

//-- GetInfo ------------------------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
extern "C" void GetInfo(VIS_INFO* pInfo)
{
  pInfo->bWantsFreq = false;
  pInfo->iSyncDelay = 0;
}

//-- OnAction -----------------------------------------------------------------
// Handle XBMC actions such as next preset, lock preset, album art changed etc
//-----------------------------------------------------------------------------
extern "C" bool OnAction(long flags, void *param)
{
  bool ret = false;
  return ret;
}

//-- GetPresets ---------------------------------------------------------------
// Return a list of presets to XBMC for display
//-----------------------------------------------------------------------------
extern "C" void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
{
  
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
//-----------------------------------------------------------------------------
extern "C" void GetSettings(vector<VisSetting> **vecSettings)
{
  return;
}

//-- UpdateSetting ------------------------------------------------------------
// Handle setting change request from XBMC
//-----------------------------------------------------------------------------
extern "C" void UpdateSetting(int num)
{

}
