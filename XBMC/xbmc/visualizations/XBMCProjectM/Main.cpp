
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

#include "../../addons/include/libaddon++.h"
#include "../../addons/include/xbmc_vis_dll.h"
#include <GL/glew.h>
#include "libprojectM/ConfigFile.h"
#include "libprojectM/projectM.hpp"
#include "libprojectM/Preset.hpp"
#include "libprojectM/PCM.hpp"
#include <string>
#ifdef WIN32
#include "libprojectM/win32-dirent.h"
#include <io.h>
#else
#include <dirent.h>
#endif

projectM *globalPM = NULL;

extern int preset_index;

// some projectm globals
int maxSamples=512;
int texsize=512;
int gx=40,gy=30;
int fps=100;
char *disp;
char g_visName[512];
projectM::Settings g_configPM;
std::string g_configFile;
PresetsList currentPresetsList;

#define QUALITY_LOW "Low"
#define QUALITY_MEDIUM "Medium"
#define QUALITY_HIGH "High"
#define QUALITY_MAX "Maximum"
#define PROJECTM_QUALITY (VIS_ACTION_USER+1)

// Some helper Functions

// case-insensitive alpha sort from projectM's win32-dirent.cc
#ifndef WIN32
int alphasort(const void* lhs, const void* rhs)
{
  const struct dirent* lhs_ent = *(struct dirent**)lhs;
  const struct dirent* rhs_ent = *(struct dirent**)rhs;
  return strcasecmp(lhs_ent->d_name, rhs_ent->d_name);
}
#endif

// check for a valid preset extension
#ifdef __APPLE__
int check_valid_extension(struct dirent* ent)
#else
int check_valid_extension(const struct dirent* ent)
#endif
{
  const char* ext = 0;

  if (!ent) return 0;

  ext = strrchr(ent->d_name, '.');
  if (!ext) ext = ent->d_name;

  if (0 == strcasecmp(ext, ".milk")) return 1;
  if (0 == strcasecmp(ext, ".prjm")) return 1;

  return 0;
}


std::string GetConfigFile()
{
  std::string configFile;
  configFile = XBMC_get_user_directory();
  configFile = XBMC_translate_path(configFile);
  configFile += "/projectM.conf";
  XBMC_log(LOG_INFO, "Using '%s' as location for config file", configFile.c_str());
  return configFile;
}

std::string GetPresetDir()
{
  std::string presetsDir;
  presetsDir = XBMC_get_addon_directory();
  presetsDir = XBMC_translate_path(presetsDir);
  presetsDir += "/resources/presets";
  XBMC_log(LOG_INFO, "Searching presets in: %s", presetsDir.c_str());
  return presetsDir;
}



//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
//-----------------------------------------------------------------------------
extern "C" {

void Remove()
{
}

bool HasSettings()
{
  return false;
}

addon_status_t GetStatus()
{
  return STATUS_OK;
}

addon_status_t SetSetting(const char *settingName, const void *settingValue)
{
  string str = settingName;
  if (str == "shufflemode")
  {
    globalPM->setShuffleEnabled(*(bool*) settingValue);
  }
  else
  {
    if (globalPM)
    {
      g_configPM = globalPM->settings();
      projectM::writeConfig(g_configFile,globalPM->settings());
      delete globalPM;
      globalPM = NULL;
    }

    if (str == "renderquality")
    {
      switch (*(int*) settingValue)
      {
        case 0:
          g_configPM.useFBO = false;
          g_configPM.textureSize = 256;
          break;

        case 1:
          g_configPM.useFBO = false;
          g_configPM.textureSize = 512;
          break;

        case 2:
          g_configPM.useFBO = false;
          g_configPM.textureSize = 1024;
          break;

        case 3:
          g_configPM.useFBO = false;
          g_configPM.textureSize = 2048;
          break;
      }
    }
    else if (str == "smoothpresetduration")
      g_configPM.smoothPresetDuration = *(int*) settingValue;
    else if (str == "presetduration")
      g_configPM.presetDuration = *(int*) settingValue;
    else if (str == "beatsensitivity")
      g_configPM.beatSensitivity = (float)(*(int*) settingValue + 1) / 5.0f;

    projectM::writeConfig(g_configFile, g_configPM);
    globalPM = new projectM(g_configFile);
  }
  return STATUS_OK;
}

//-- Create -------------------------------------------------------------------
// Called once when the visualisation is created by XBMC. Do any setup here.
//-----------------------------------------------------------------------------
#ifdef HAS_XBOX_HARDWARE
addon_status_t Create(ADDON_HANDLE hdl, LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName,
                       float fPixelRatio)
#else
addon_status_t Create(ADDON_HANDLE hdl, void* pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName,
                       float fPixelRatio)
#endif
{
  int tmp;
  bool tmp2;

  if (!XBMC_register_me(hdl))
    return STATUS_UNKNOWN;

  currentPresetsList.numPresets = 0;

  strcpy(g_visName, szVisualisationName);

  /** Initialise projectM */

  g_configFile = GetConfigFile();
  std::string presetsDir = GetPresetDir();

  g_configPM.meshX = gx;
  g_configPM.meshY = gy;
  g_configPM.fps = fps;
  g_configPM.textureSize = texsize;
  g_configPM.windowWidth = iWidth;
  g_configPM.windowHeight = iHeight;
  g_configPM.presetURL = presetsDir;
  g_configPM.smoothPresetDuration = 5;
  g_configPM.presetDuration = 15;
  g_configPM.beatSensitivity = 10.0;
  g_configPM.aspectCorrection = true;
  g_configPM.easterEgg = 0.0;
  g_configPM.shuffleEnabled = true;
  g_configPM.windowLeft = iPosX;
  g_configPM.windowBottom = iPosY;

  {
    FILE *f;
    f = fopen(g_configFile.c_str(), "r");
    if (f) {   // Config exists.  Let's preserve settings except for iWidth, iHeight, iPosX, iPosY
      fclose(f);
      ConfigFile config(g_configFile.c_str());
      if (config.keyExists("Mesh X")) g_configPM.meshX = config.read<int> ("Mesh X", gx);
      if (config.keyExists("Mesh Y")) g_configPM.meshY = config.read<int> ("Mesh Y", gy);
      if (config.keyExists("Texture Size")) g_configPM.textureSize = config.read<int> ("Texture Size", texsize);
      if (config.keyExists("Preset Path")) g_configPM.presetURL = config.read<string> ("Preset Path", presetsDir);
      if (config.keyExists("Smooth Preset Duration")) g_configPM.smoothPresetDuration = config.read<int> ("Smooth Preset Duration", 5);
      if (config.keyExists("Preset Duration")) g_configPM.presetDuration = config.read<int> ("Preset Duration", 15);
      if (config.keyExists("FPS")) g_configPM.fps = config.read<int> ("FPS", fps);
      if (config.keyExists("Hard Cut Sensitivity")) g_configPM.beatSensitivity = config.read<float> ("Hard Cut Sensitivity", 10.0);
      if (config.keyExists("Aspect Correction")) g_configPM.aspectCorrection = config.read<bool> ("Aspect Correction", true);
      if (config.keyExists("Easter Egg")) g_configPM.easterEgg = config.read<float> ("Easter Egg", 0.0);
      if (config.keyExists("Shuffle Enabled")) g_configPM.shuffleEnabled = config.read<bool> ("Shuffle Enabled", true);
      if (config.keyExists("Use FBO")) g_configPM.useFBO = config.read<bool> ("Use FBO", false);
    }
    else {
      f = fopen(g_configFile.c_str(), "w");   // Config does not exist, but we still need at least a blank file.
      fclose(f);
    }
    projectM::writeConfig(g_configFile, g_configPM);
  }

  if (globalPM)
    delete globalPM;

  globalPM = new projectM(g_configFile);

  if (XBMC_get_setting("renderquality", &tmp))
    SetSetting("renderquality", &tmp);

  if (XBMC_get_setting("shufflemode", &tmp2))
    SetSetting("shufflemode", &tmp2);

  if (XBMC_get_setting("smoothpresetduration", &tmp))
    SetSetting("smoothpresetduration", &tmp);

  if (XBMC_get_setting("presetduration", &tmp))
    SetSetting("presetduration", &tmp);

  if (XBMC_get_setting("beatsensitivity", &tmp))
    SetSetting("beatsensitivity", &tmp);

  return STATUS_OK;
}

//-- Start --------------------------------------------------------------------
// Called when a new soundtrack is played
//-----------------------------------------------------------------------------
void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
  //printf("Got Start Command\n");
}

//-- Stop ---------------------------------------------------------------------
// Called when the visualisation is closed by XBMC
//-----------------------------------------------------------------------------
void Stop()
{
  if (globalPM)
  {
    projectM::writeConfig(g_configFile,globalPM->settings());
    delete globalPM;
    globalPM = NULL;
  }
  if (currentPresetsList.Presets)
  {
    for (int i = 0 ; i < currentPresetsList.numPresets ; i++)
      free(currentPresetsList.Presets[i]);

    free(currentPresetsList.Presets);
    currentPresetsList.Presets = NULL;
  }

  currentPresetsList.currentPreset = 0;
  currentPresetsList.numPresets = 0;
}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  globalPM->pcm()->addPCM16Data(pAudioData, iAudioDataLength);
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
void Render()
{
  globalPM->renderFrame();
}

//-- GetInfo ------------------------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
void GetInfo(VIS_INFO* pInfo)
{
  pInfo->bWantsFreq = false;
  pInfo->iSyncDelay = 0;
}

//-- OnAction -----------------------------------------------------------------
// Handle XBMC actions such as next preset, lock preset, album art changed etc
//-----------------------------------------------------------------------------
bool OnAction(long flags, void *param)
{
  bool ret = false;

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
    globalPM->setShuffleEnabled(!globalPM->isShuffleEnabled());
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
void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
{
  if (!currentPresetsList.Presets)
  {
    if (globalPM->getPlaylistSize() > 0)
    {
      currentPresetsList.numPresets = globalPM->getPlaylistSize();
      currentPresetsList.Presets = (char**) calloc (currentPresetsList.numPresets+1,sizeof(char*));
      if (currentPresetsList.Presets)
      {
        for (unsigned int i = 0; i < globalPM->getPlaylistSize() ; i++)
        {
          currentPresetsList.Presets[i] = (char*)malloc(strlen(globalPM->getPresetName(i).c_str())+2);
          if (currentPresetsList.Presets[i])
          {
            strcpy(currentPresetsList.Presets[i], globalPM->getPresetName(i).c_str());
          }
        }
      }
    }
  }

  if (currentPresetsList.Presets)
  {
    *pPresets = currentPresetsList.Presets;
    *numPresets = currentPresetsList.numPresets;
    unsigned int presetIndex;
    if (globalPM->selectedPresetIndex(presetIndex) && presetIndex >= 0 && (int)presetIndex < currentPresetsList.numPresets)
      *currentPreset = presetIndex;
  }
  *locked = globalPM->isPresetLocked();
}

}
