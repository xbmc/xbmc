
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
#include <GL/glew.h>
#include "libprojectM/ConfigFile.h"
#include "libprojectM/projectM.hpp"
#include "libprojectM/Preset.hpp"
#include "libprojectM/PCM.hpp"
#include <string>
/*#ifdef WIN32
#include "libprojectM/win32-dirent.h"
#include <io.h>
#else
#include <dirent.h>
#endif*/

projectM *globalPM = NULL;

extern int preset_index;

// some projectm globals
int maxSamples=512;
int texsize=512;
int gx=40,gy=30;
int fps=100;
char *disp;
char g_visName[512];
viz_preset_list_t g_presets=NULL;
projectM::Settings g_configPM;
std::string g_configFile;

// Some helper Functions
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

//-- Create -------------------------------------------------------------------
// Called once when the visualisation is created by XBMC. Do any setup here.
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS Create(void* hdl, void* props)
{
  if (!props)
    return STATUS_UNKNOWN;

  strcpy(g_visName, "projectM");

  VIS_PROPS* visprops = (VIS_PROPS*)props;

  g_configFile = string(visprops->datastore) + string("/projectm.conf");
  std::string presetsDir = string(visprops->presets);

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

  globalPM = new projectM(g_configFile);

  return STATUS_NEED_SETTINGS;
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
    projectM::writeConfig(g_configFile,globalPM->settings());
    delete globalPM;
    globalPM = NULL;
  }
  if (g_presets)
    viz_release(g_presets);
}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
extern "C" void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  globalPM->pcm()->addPCM16Data(pAudioData, iAudioDataLength);
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
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
extern "C" viz_preset_list_t GetPresets()
{
  if (!g_presets)
  {
    g_presets = viz_preset_list_create();
    if (g_presets)
    {
      viz_preset_t preset;
      for (unsigned int i = 0; i < globalPM->getPlaylistSize() ; i++)
      {
        preset = viz_preset_create();
        if (viz_preset_set_name(preset, globalPM->getPresetName(i).c_str()))
        {
          viz_preset_list_add_item(g_presets, preset);
        }
      }
    }
  }

  // XBMC never alters this presetlist
  return g_presets;
}

//-- GetPreset ----------------------------------------------------------------
// Return the index of the current playing preset
//-----------------------------------------------------------------------------
extern "C" unsigned GetPreset()
{
  if (g_presets)
  {
    unsigned preset;
    if(globalPM->selectedPresetIndex(preset))
      return preset;
  }
  return 0;
}

//-- IsLocked -----------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" bool IsLocked()
{
  if(globalPM)
    return globalPM->isPresetLocked();
  else
    return false;
}

//-- Remove -------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void Remove()
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

extern "C" addon_settings_t GetSettings()
{

  addon_settings_t settings = addon_settings_create();
  addon_setting_t quality = addon_setting_create(SETTING_ENUM, "Quality");
  addon_setting_set_type(quality, SETTING_ENUM);
  addon_setting_set_label(quality, "30000");
  addon_setting_set_lvalues(quality, "30001|30002|30003|30004");

  addon_setting_t shuffleMode = addon_setting_create(SETTING_BOOL, "Shuffle");
  addon_setting_set_type(quality, SETTING_BOOL);
  addon_setting_set_label(shuffleMode, "30010");

  addon_setting_t smoothPresetDuration = addon_setting_create(SETTING_ENUM, "Smooth Preset Duration");
  addon_setting_set_type(quality, SETTING_ENUM);
  addon_setting_set_label(smoothPresetDuration, "30020");

  addon_settings_add_item(settings, quality);
  addon_settings_add_item(settings, shuffleMode);
  addon_settings_add_item(settings, smoothPresetDuration);
/*  for (int i=0; i < 50; i++)
  {
    char temp[10];
    sprintf(temp, "%i secs", i);
    smoothPresetDuration.AddEntry(temp);
  }

  VisSetting presetDuration(VisSetting::SPIN, "Preset Duration");
  for (int i=0; i < 50; i++)
  {
    char temp[10];
    sprintf(temp, "%i secs", i);
    presetDuration.AddEntry(temp);
  }
  presetDuration.current = (int)(g_configPM.presetDuration);
  m_vecSettings.push_back(presetDuration);

  VisSetting beatSensitivity(VisSetting::SPIN, "Beat Sensitivity");
  for (int i=0; i <= 100; i++)
  {
    char temp[10];
    sprintf(temp, "%2.1f", (float)(i + 1)/5);
    beatSensitivity.AddEntry(temp);
  }
  beatSensitivity.current = (int)(g_configPM.beatSensitivity * 5 - 1);
  m_vecSettings.push_back(beatSensitivity);

  m_uiVisElements = VisUtils::VecToStruct(m_vecSettings, &m_structSettings);
  *sSet = m_structSettings;*/
  return settings;
}

//-- UpdateSetting ------------------------------------------------------------
// Handle setting change request from XBMC
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS SetSetting(const char* id, const void* value)
{
  if (!id || !value)
    return STATUS_UNKNOWN;

  if (strcmp(id, "Quality")==0)
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
  else if (strcmp(id, "Shuffle")==0)
  {
    g_configPM.shuffleEnabled = !g_configPM.shuffleEnabled;
    if (globalPM)
      OnAction(VIS_ACTION_RANDOM_PRESET, value);
  }

  return STATUS_OK;
}

//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" int GetSubModules(char ***names, char ***paths)
{
  return 0; // this vis supports 0 sub modules
}
