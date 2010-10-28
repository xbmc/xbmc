/*
 *      Copyright (C) 2007-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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

#include "xbmc_vis_dll.h"
#include "xbmc_addon_cpp_dll.h"
#include <GL/glew.h>
#include "libprojectM/ConfigFile.h"
#include "libprojectM/projectM.hpp"
#include <string>

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
std::string g_configFile;

// settings vector
std::vector<DllSetting> g_vecSettings;
StructSetting** g_structSettings;
unsigned int g_uiVisElements;

//-- Create -------------------------------------------------------------------
// Called once when the visualisation is created by XBMC. Do any setup here.
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS Create(void* hdl, void* props)
{
  if (!props)
    return STATUS_UNKNOWN;

  g_vecSettings.clear();
  g_uiVisElements = 0;


  VIS_PROPS* visprops = (VIS_PROPS*)props;

  strcpy(g_visName, visprops->name);
  g_configFile = string(visprops->profile) + string("/projectm.conf");
  std::string presetsDir = "zip://special%3A%2F%2Fxbmc%2Faddons%2Fvisualization%2Eprojectm%2Fresources%2Fpresets%2Ezip";

  g_configPM.meshX = gx;
  g_configPM.meshY = gy;
  g_configPM.fps = fps;
  g_configPM.textureSize = texsize;
  g_configPM.windowWidth = visprops->width;
  g_configPM.windowHeight = visprops->height;
  g_configPM.presetURL = presetsDir;
  g_configPM.smoothPresetDuration = 5;
  g_configPM.presetDuration = 15;
  g_configPM.beatSensitivity = 10.0;
  g_configPM.aspectCorrection = true;
  g_configPM.easterEgg = 0.0;
  g_configPM.shuffleEnabled = true;
  g_configPM.windowLeft = visprops->x;
  g_configPM.windowBottom = visprops->y;

  // if no config file exists, create a blank one as Config ctor throws an exception!
  FILE *f;
  f = fopen(g_configFile.c_str(), "r");
  if (!f) f = fopen(g_configFile.c_str(), "w");

  if (f)
    fclose(f);

  // save our config
  try
  {
    projectM::writeConfig(g_configFile, g_configPM);
  }
  catch (...)
  {
    printf("exception in projectM::WriteConfig");
    return STATUS_UNKNOWN;
  }

  if (globalPM)
    delete globalPM;

  try
  {
    globalPM = new projectM(g_configFile);
  }
  catch (...)
  {
    printf("exception in projectM ctor");
    return STATUS_UNKNOWN;
  }

  DllSetting quality(DllSetting::SPIN, "quality", "30000");
  quality.AddEntry("30001");
  quality.AddEntry("30002");
  quality.AddEntry("30003");
  quality.AddEntry("30004");
  if (g_configPM.textureSize == 2048)
  {
    quality.current = 3;
  }
  else if (g_configPM.textureSize == 1024)
  {
    quality.current = 2;
  }
  else if (g_configPM.textureSize == 512)
  {
    quality.current = 1;
  }
  else if (g_configPM.textureSize == 256)
  {
    quality.current = 0;
  }
  g_vecSettings.push_back(quality);

  DllSetting shuffleMode(DllSetting::CHECK, "shuffle", "30005");
  shuffleMode.current = globalPM->isShuffleEnabled();
  g_vecSettings.push_back(shuffleMode);

  DllSetting smoothPresetDuration(DllSetting::SPIN, "smooth_duration", "30006");
  for (int i=0; i < 50; i++)
  {
    char temp[10];
    sprintf(temp, "%i secs", i);
    smoothPresetDuration.AddEntry(temp);
  }
  smoothPresetDuration.current = (int)(g_configPM.smoothPresetDuration);
  g_vecSettings.push_back(smoothPresetDuration);

  DllSetting presetDuration(DllSetting::SPIN, "preset_duration", "30007");
  for (int i=0; i < 50; i++)
  {
    char temp[10];
    sprintf(temp, "%i secs", i);
    presetDuration.AddEntry(temp);
  }
  presetDuration.current = (int)(g_configPM.presetDuration);
  g_vecSettings.push_back(presetDuration);

  DllSetting beatSensitivity(DllSetting::SPIN, "beat_sens", "30008");
  for (int i=0; i <= 100; i++)
  {
    char temp[10];
    sprintf(temp, "%2.1f", (float)(i + 1)/5);
    beatSensitivity.AddEntry(temp);
  }
  beatSensitivity.current = (int)(g_configPM.beatSensitivity * 5 - 1);
  g_vecSettings.push_back(beatSensitivity);

  return STATUS_NEED_SETTINGS;
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
extern "C" void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  if (globalPM)
    globalPM->pcm()->addPCM16Data(pAudioData, iAudioDataLength);
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  if (globalPM)
    globalPM->renderFrame();
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
extern "C" void Stop()
{
  if (globalPM)
  {
    projectM::writeConfig(g_configFile,globalPM->settings());
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
extern "C" void Destroy()
{
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" bool HasSettings()
{
  return true;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS GetStatus()
{
  return STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
//-----------------------------------------------------------------------------

extern "C" unsigned int GetSettings(StructSetting ***sSet)
{
  g_uiVisElements = DllUtils::VecToStruct(g_vecSettings, &g_structSettings);
  *sSet = g_structSettings;
  return g_uiVisElements;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
//-----------------------------------------------------------------------------

extern "C" void FreeSettings()
{
  DllUtils::FreeStruct(g_uiVisElements, &g_structSettings);
}

//-- UpdateSetting ------------------------------------------------------------
// Handle setting change request from XBMC
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS SetSetting(const char* id, const void* value)
{
  if (!id || !value)
    return STATUS_UNKNOWN;

  if (strcmp(id, "quality")==0)
  {
    switch (*(int*) value)
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
  else if (strcmp(id, "shuffle")==0)
  {
    g_configPM.shuffleEnabled = !g_configPM.shuffleEnabled;
    if (globalPM)
      OnAction(VIS_ACTION_RANDOM_PRESET, value);
  }
  else if (strcmp(id, "smooth_duration")==0)
    g_configPM.smoothPresetDuration = *(int*)value;
  else if (strcmp(id, "preset_duration")==0)
    g_configPM.presetDuration = *(int*)value;
  else if (strcmp(id, "beat_sens")==0)
    g_configPM.beatSensitivity = *(int*)value;

  return STATUS_OK;
}

//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" unsigned int GetSubModules(char ***names)
{
  return 0; // this vis supports 0 sub modules
}
