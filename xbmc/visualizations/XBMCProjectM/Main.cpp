
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

#include "xbmc_vis.h"
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
#include "PlatformDefs.h"
#include "Util.h"
#include "system.h"
#include "FileSystem/SpecialProtocol.h"
#include <dirent.h>
#endif

#define PRESETS_DIR "special://xbmc/visualisations/projectM"
#define CONFIG_FILE "special://profile/visualisations/projectM.conf"

projectM *globalPM = NULL;

extern int preset_index;

// some projectm globals
int maxSamples=512;
int texsize=512;
int gx=40,gy=30;
int fps=100;
char *disp;
char g_visName[512];
char **g_presets=NULL;
int g_numPresets = 0;
projectM::Settings g_configPM;
std::string g_configFile;

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

//-- Create -------------------------------------------------------------------
// Called once when the visualisation is created by XBMC. Do any setup here.
//-----------------------------------------------------------------------------
#ifdef HAS_XBOX_HARDWARE
extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName,
                       float fPixelRatio, const char *szSubModuleName)
#else
extern "C" void Create(void* pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName,
                       float fPixelRatio, const char *szSubModuleName)
#endif
{
  strcpy(g_visName, szVisualisationName);

  m_vecSettings.clear();

  /** Initialise projectM */

#ifdef WIN32
  g_configFile = string(CONFIG_FILE);
  std::string presetsDir = string(PRESETS_DIR);
#else
  g_configFile = _P(CONFIG_FILE);
  std::string presetsDir = _P(PRESETS_DIR);
#endif

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
#ifndef WIN32
      CStdString strPath;
      CUtil::GetDirectory(g_configFile, strPath);
      CUtil::CreateDirectoryEx(strPath);
#endif
      f = fopen(g_configFile.c_str(), "w");   // Config does not exist, but we still need at least a blank file.
      fclose(f);
    }
      projectM::writeConfig(g_configFile, g_configPM);
  }

  if (globalPM)
    delete globalPM;

  globalPM = new projectM(g_configFile);

  VisSetting quality(VisSetting::SPIN, "Render Quality");
  quality.AddEntry("Low");
  quality.AddEntry("Medium");
  quality.AddEntry("High");
  quality.AddEntry("Maximum");
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
  m_vecSettings.push_back(quality);

  VisSetting shuffleMode(VisSetting::CHECK, "Shuffle Mode");
  shuffleMode.current = globalPM->isShuffleEnabled();
  m_vecSettings.push_back(shuffleMode);
  
  VisSetting smoothPresetDuration(VisSetting::SPIN, "Smooth Preset Duration");
  for (int i=0; i < 50; i++)
  {
    char temp[10];
    sprintf(temp, "%i secs", i);
    smoothPresetDuration.AddEntry(temp);
  }
  smoothPresetDuration.current = (int)(g_configPM.smoothPresetDuration);
  m_vecSettings.push_back(smoothPresetDuration);
  
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
  {
    for (int i = 0 ; i<g_numPresets ; i++)
    {
      free(g_presets[i]);
    }
    free(g_presets);
    g_presets = NULL;
  }
  m_vecSettings.clear(); 
}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
extern "C" void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
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
extern "C" bool OnAction(long flags, void *param)
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
extern "C" void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
{
  if (!g_presets)
  {
    if (globalPM->getPlaylistSize() > 0)
    {
      g_numPresets = globalPM->getPlaylistSize();
      g_presets = (char **)malloc(sizeof(char*)*globalPM->getPlaylistSize());
      if (g_presets)
      {
        for (unsigned int i = 0; i < globalPM->getPlaylistSize() ; i++)
        {
          g_presets[i] = (char*)malloc(strlen(globalPM->getPresetName(i).c_str())+2);
          if (g_presets[i])
          {
            strcpy(g_presets[i], globalPM->getPresetName(i).c_str());
          }
        }
      }
    }
  }
        

  if (g_presets)
  {
    *pPresets = g_presets;
    *numPresets = g_numPresets;
    unsigned int presetIndex;
    if (globalPM->selectedPresetIndex(presetIndex) && presetIndex >= 0 &&
        (int)presetIndex < g_numPresets)
      *currentPreset = presetIndex;
  }
  *locked = globalPM->isPresetLocked();
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
//-----------------------------------------------------------------------------
extern "C" void GetSettings(vector<VisSetting> **vecSettings)
{
#ifdef WIN32
  //FIXME: windows crashes when returning the settings
  //return;
  VisSetting &setting = m_vecSettings[0];
  OutputDebugString("Hallo1\n");
  OutputDebugString(setting.name);
#endif
  if (!vecSettings)
    return;
  *vecSettings = &m_vecSettings;
}

//-- UpdateSetting ------------------------------------------------------------
// Handle setting change request from XBMC
//-----------------------------------------------------------------------------
extern "C" void UpdateSetting(int num)
{
  VisSetting &setting = m_vecSettings[num];
  if (strcasecmp(setting.name, "Use Preset")==0)
    OnAction(34, (void*)&setting.current);
  else if (strcasecmp(setting.name, "Shuffle Mode")==0)
    OnAction(VIS_ACTION_RANDOM_PRESET, (void*)&setting.current);
  else {
    if (globalPM) 
    {
      g_configPM = globalPM->settings();
      projectM::writeConfig(g_configFile,globalPM->settings());
      delete globalPM;
      globalPM = NULL;
    }
    if (strcasecmp(setting.name, "Smooth Preset Duration")==0)
      g_configPM.smoothPresetDuration = setting.current;
    else if (strcasecmp(setting.name,"Preset Duration")==0)
      g_configPM.presetDuration = setting.current;
    else if (strcasecmp(setting.name, "Beat Sensitivity")==0)
      g_configPM.beatSensitivity = (float)(setting.current + 1) / 5.0f;
    else if (strcasecmp(setting.name, "Render Quality")==0)
    {
      if ( setting.current == 0 ) // low
      {
        g_configPM.useFBO = false;
        g_configPM.textureSize = 256;
      }
      else if ( setting.current == 1 ) // med
      {
        g_configPM.useFBO = false;
        g_configPM.textureSize = 512;
      }
      else if ( setting.current == 2 ) // high
      {
        g_configPM.useFBO = false;
        g_configPM.textureSize = 1024;
      }
      else if ( setting.current == 3 ) // max
      {
        g_configPM.useFBO = false;
        g_configPM.textureSize = 2048;
      }
    }
    projectM::writeConfig(g_configFile, g_configPM);
    globalPM = new projectM(g_configFile); 
  }
  
}

//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" int GetSubModules(char ***names, char ***paths)
{
  return 0; // this vis supports 0 sub modules
}
