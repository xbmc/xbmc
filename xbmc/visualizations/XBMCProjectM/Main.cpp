/*
 *      Copyright (C) 2007-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

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
d4rk@xbmc.org

*/

#include "addons/include/xbmc_vis_dll.h"
#include "addons/include/xbmc_addon_cpp_dll.h"
#include <GL/glew.h>
#include "libprojectM/projectM.hpp"
#include <string>
#include <utils/log.h>

projectM *globalPM = NULL;

// some projectm globals
int maxSamples=512;
int texsize=512;
int gx=40,gy=30;
int fps=100;
char *disp;
char g_visName[512];
char **g_presets=NULL;
unsigned int g_numPresets = 0;
projectM::Settings g_configPM;

bool g_UserPackFolder;
char lastPresetDir[1024];
bool lastLockStatus;
int lastPresetIdx;
unsigned int lastLoggedPresetIdx;

//-- Create -------------------------------------------------------------------
// Called once when the visualisation is created by XBMC. Do any setup here.
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!props)
    return ADDON_STATUS_UNKNOWN;

  VIS_PROPS* visprops = (VIS_PROPS*)props;

  strcpy(g_visName, visprops->name);
  g_configPM.meshX = gx;
  g_configPM.meshY = gy;
  g_configPM.fps = fps;
  g_configPM.textureSize = texsize;
  g_configPM.windowWidth = visprops->width;
  g_configPM.windowHeight = visprops->height;
  g_configPM.aspectCorrection = true;
  g_configPM.easterEgg = 0.0;
  g_configPM.windowLeft = visprops->x;
  g_configPM.windowBottom = visprops->y;
  lastLoggedPresetIdx = lastPresetIdx;

  return ADDON_STATUS_NEED_SAVEDSETTINGS;
}

//-- Start --------------------------------------------------------------------
// Called when a new soundtrack is played
//-----------------------------------------------------------------------------
extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
  //printf("Got Start Command\n");
}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
extern "C" void AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  if (globalPM)
    globalPM->pcm()->addPCMfloat(pAudioData, iAudioDataLength);
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  if (globalPM)
  {
    globalPM->renderFrame();
    unsigned preset;
    globalPM->selectedPresetIndex(preset);
    if (lastLoggedPresetIdx != preset) CLog::Log(LOGDEBUG,"PROJECTM - Changed preset to: %s",g_presets[preset]);
    lastLoggedPresetIdx = preset;
  }
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
extern "C" bool OnAction(long flags, const void *param)
{
  bool ret = false;

  if (!globalPM)
    return false;

  if (flags == VIS_ACTION_LOAD_PRESET && param)
  {
    int pindex = *((int *)param);
    globalPM->selectPreset(pindex);
    ret = true;
  }
  else if (flags == VIS_ACTION_NEXT_PRESET)
  {
//    switchPreset(ALPHA_NEXT, SOFT_CUT);
    if (!globalPM->isShuffleEnabled())
      globalPM->key_handler(PROJECTM_KEYDOWN, PROJECTM_K_n, PROJECTM_KMOD_CAPS); //ignore PROJECTM_KMOD_CAPS
    else
      globalPM->key_handler(PROJECTM_KEYDOWN, PROJECTM_K_r, PROJECTM_KMOD_CAPS); //ignore PROJECTM_KMOD_CAPS
    ret = true;
  }
  else if (flags == VIS_ACTION_PREV_PRESET)
  {
//    switchPreset(ALPHA_PREVIOUS, SOFT_CUT);
    if (!globalPM->isShuffleEnabled())
      globalPM->key_handler(PROJECTM_KEYDOWN, PROJECTM_K_p, PROJECTM_KMOD_CAPS); //ignore PROJECTM_KMOD_CAPS
    else
      globalPM->key_handler(PROJECTM_KEYDOWN, PROJECTM_K_r, PROJECTM_KMOD_CAPS); //ignore PROJECTM_KMOD_CAPS

    ret = true;
  }
  else if (flags == VIS_ACTION_RANDOM_PRESET)
  {
    globalPM->setShuffleEnabled(g_configPM.shuffleEnabled);
    ret = true;
  }
  else if (flags == VIS_ACTION_LOCK_PRESET)
  {
    globalPM->setPresetLock(!globalPM->isPresetLocked());
    unsigned preset;
    globalPM->selectedPresetIndex(preset);
    globalPM->selectPreset(preset);
    ret = true;
  }
  return ret;
}

//-- GetPresets ---------------------------------------------------------------
// Return a list of presets to XBMC for display
//-----------------------------------------------------------------------------
extern "C" unsigned int GetPresets(char ***presets)
{
  g_numPresets = globalPM ? globalPM->getPlaylistSize() : 0;
  if (g_numPresets > 0)
  {
    g_presets = (char**) malloc(sizeof(char*)*g_numPresets);
    for (unsigned i = 0; i < g_numPresets; i++)
    {
      g_presets[i] = (char*) malloc(strlen(globalPM->getPresetName(i).c_str())+2);
      if (g_presets[i])
        strcpy(g_presets[i], globalPM->getPresetName(i).c_str());
    }
    *presets = g_presets;
  }
  return g_numPresets;
}

//-- GetPreset ----------------------------------------------------------------
// Return the index of the current playing preset
//-----------------------------------------------------------------------------
extern "C" unsigned GetPreset()
{
  if (g_presets)
  {
    unsigned preset;
    if(globalPM && globalPM->selectedPresetIndex(preset))
      return preset;
  }
  return 0;
}

//-- IsLocked -----------------------------------------------------------------
// Returns true if this add-on use settings
//-----------------------------------------------------------------------------
extern "C" bool IsLocked()
{
  if(globalPM)
    return globalPM->isPresetLocked();
  else
    return false;
}

//-- Stop ---------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Stop()
{
  if (globalPM)
  {
    delete globalPM;
    globalPM = NULL;
  }
  if (g_presets)
  {
    for (unsigned i = 0; i <g_numPresets; i++)
    {
      free(g_presets[i]);
    }
    free(g_presets);
    g_presets = NULL;
  }
  g_numPresets = 0;
}

//-- Destroy-------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Destroy()
{
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" bool ADDON_HasSettings()
{
  return true;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
//-----------------------------------------------------------------------------

extern "C" unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
//-----------------------------------------------------------------------------

extern "C" void ADDON_FreeSettings()
{
}

void ChooseQuality (int pvalue)
{
  switch (pvalue)
  {
    case 0:
      g_configPM.textureSize = 256;
      break;
    case 1:
      g_configPM.textureSize = 512;
      break;
    case 2:
      g_configPM.textureSize = 1024;
      break;
    case 3:
      g_configPM.textureSize = 2048;
      break;
  }
}

void ChoosePresetPack(int pvalue)
{
  g_UserPackFolder = false;
  if (pvalue == 0)
    g_configPM.presetURL = "zip://special%3A%2F%2Fxbmc%2Faddons%2Fvisualization%2Eprojectm%2Fresources%2Fpresets%2Ezip";
  else if (pvalue == 1) //User preset folder has been chosen
    g_UserPackFolder = true;
}

void ChooseUserPresetFolder(std::string pvalue)
{
  if (g_UserPackFolder)
  {
    pvalue.erase(pvalue.length()-1,1);  //Remove "/" from the end
    g_configPM.presetURL = pvalue;
  }
}

bool InitProjectM()
{
  if (globalPM) delete globalPM; //We are re-initalizing the engine
  try
  {
    globalPM = new projectM(g_configPM);
    if (g_configPM.presetURL == lastPresetDir)  //If it is not the first run AND if this is the same preset pack as last time
    {
      globalPM->setPresetLock(lastLockStatus);
      globalPM->selectPreset(lastPresetIdx);
    }
    else
    {
      //If it is the first run or a newly chosen preset pack we choose a random preset as first
      globalPM->selectPreset((rand() % (globalPM->getPlaylistSize())));
    }
    return true;
  }
  catch (...)
  {
    printf("exception in projectM ctor");
    return false;
  }
}

//-- UpdateSetting ------------------------------------------------------------
// Handle setting change request from XBMC
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_SetSetting(const char* id, const void* value)
{
  if (!id || !value)
    return ADDON_STATUS_UNKNOWN;

  if (strcmp(id, "###GetSavedSettings") == 0) // We have some settings to be saved in the settings.xml file
  {
    if (!globalPM)
    {
      return ADDON_STATUS_UNKNOWN;
    }
    if (strcmp((char*)value, "0") == 0)
    {
      strcpy((char*)id, "lastpresetfolder");
      strcpy((char*)value, globalPM->settings().presetURL.c_str());
    }
    if (strcmp((char*)value, "1") == 0)
    {
      strcpy((char*)id, "lastlockedstatus");
      strcpy((char*)value, (globalPM->isPresetLocked() ? "true" : "false"));
    }
    if (strcmp((char*)value, "2") == 0)
    {
      strcpy((char*)id, "lastpresetidx");
      unsigned int lastindex;
      globalPM->selectedPresetIndex(lastindex);
      sprintf ((char*)value, "%i", (int)lastindex);
    }
    if (strcmp((char*)value, "3") == 0)
    {
      strcpy((char*)id, "###End");
    }
    return ADDON_STATUS_OK;
  }
  // It is now time to set the settings got from xmbc
  if (strcmp(id, "quality")==0)
    ChooseQuality (*(int*)value);
  else if (strcmp(id, "shuffle")==0)
    g_configPM.shuffleEnabled = *(bool*)value;
  
  else if (strcmp(id, "lastpresetidx")==0)
    lastPresetIdx = *(int*)value;
  else if (strcmp(id, "lastlockedstatus")==0)
    lastLockStatus = *(bool*)value;
  else if (strcmp(id, "lastpresetfolder")==0)
    strcpy(lastPresetDir, (char*)value);
  
  else if (strcmp(id, "smooth_duration")==0)
    g_configPM.smoothPresetDuration = (*(int*)value * 5 + 5);
  else if (strcmp(id, "preset_duration")==0)
    g_configPM.presetDuration = (*(int*)value * 5 + 5);
  else if (strcmp(id, "preset pack")==0)
    ChoosePresetPack(*(int*)value);
  else if (strcmp(id, "user preset folder") == 0)
    ChooseUserPresetFolder((char*)value);
  else if (strcmp(id, "beat_sens")==0)
  {
    g_configPM.beatSensitivity = *(int*)value * 2;
    if (!InitProjectM())    //The last setting value is already set so we (re)initalize
      return ADDON_STATUS_UNKNOWN;
  }
  return ADDON_STATUS_OK;
}

//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" unsigned int GetSubModules(char ***names)
{
  return 0; // this vis supports 0 sub modules
}
