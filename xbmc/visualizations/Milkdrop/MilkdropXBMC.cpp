/*
 *      Copyright (C) 2004-2012 Team XBMC
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

#include "vis_milkdrop/Plugin.h"
#include "../../addons/include/xbmc_vis_dll.h"
#include "XmlDocument.h"
#include <string>
#include <direct.h>

CPlugin* g_plugin=NULL;
char g_visName[512];

#define PRESETS_DIR "zip://special%3A%2F%2Fxbmc%2Faddons%2Fvisualization.milkdrop%2Fpresets%2F"

bool g_UserPackFolder;
std::string g_configFile;

int lastPresetIndx =0;
char lastPresetDir[1024] = "";
bool lastLockedStatus = false;

// Sets a new preset file or directory and make it active. Also recovers last state of the preset if it is the same as last time
void SetPresetDir(const char *pack)
{
  int len = strlen(pack);
  if (len >= 4 && strcmp(pack + len - 4, ".zip") == 0)
  {
    // Zip file
    strcpy(g_plugin->m_szPresetDir, PRESETS_DIR);
    strcat(g_plugin->m_szPresetDir,  pack);
    strcat(g_plugin->m_szPresetDir, "/");
  }
  else if (len >= 4 && strcmp(pack + len - 4, ".rar") == 0)
  {
    // Rar file
    strcpy(g_plugin->m_szPresetDir, PRESETS_DIR);
    strcat(g_plugin->m_szPresetDir,  pack);
    strcat(g_plugin->m_szPresetDir, "/");
  }
  else
  {
    // Normal folder
    strcpy(g_plugin->m_szPresetDir,  pack);
  }
  if (strcmp (g_plugin->m_szPresetDir, lastPresetDir) == 0)
  {
    // If we have a valid last preset state AND the preset file(dir) is the same as last time
    g_plugin->UpdatePresetList();
    g_plugin->m_bHoldPreset = lastLockedStatus;
    g_plugin->m_nCurrentPreset = lastPresetIndx;
    strcpy(g_plugin->m_szCurrentPresetFile, g_plugin->m_szPresetDir);
    strcat(g_plugin->m_szCurrentPresetFile, g_plugin->m_pPresetAddr[g_plugin->m_nCurrentPreset]);
    g_plugin->LoadPreset(g_plugin->m_szCurrentPresetFile, g_plugin->m_fBlendTimeUser);
  }
  else
    // If it is the first run or a newly chosen preset pack we choose a random preset as first
    g_plugin->LoadRandomPreset(g_plugin->m_fBlendTimeUser);
}

void Preinit()
{
  if(!g_plugin)
  {
    g_plugin = new CPlugin;
    g_plugin->PluginPreInitialize(0, 0);
  }
}

extern "C" ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!props)
    return ADDON_STATUS_UNKNOWN;

  VIS_PROPS* visprops = (VIS_PROPS*)props;
  _mkdir(visprops->profile);

  Preinit();
  if(!g_plugin || !g_plugin->PluginInitialize((LPDIRECT3DDEVICE9)visprops->device, visprops->x, visprops->y, visprops->width, visprops->height, visprops->pixelRatio))
    return ADDON_STATUS_UNKNOWN;

  return ADDON_STATUS_NEED_SAVEDSETTINGS; // We need some settings to be saved later before we quit this plugin
}

extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{}


extern "C" void ADDON_Stop()
{
  if(g_plugin)
  {
    g_plugin->PluginQuit();
    delete g_plugin;
    g_plugin = NULL;
  }
}

unsigned char waves[2][512];

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
extern "C" void AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
	int ipos=0;
	while (ipos < 512)
	{
		for (int i=0; i < iAudioDataLength; i+=2)
		{
      waves[0][ipos] = char (pAudioData[i] * 255.0f);
      waves[1][ipos] = char (pAudioData[i+1]  * 255.0f);
			ipos++;
			if (ipos >= 512) break;
		}
	}
}

extern "C" void Render()
{
	g_plugin->PluginRender(waves[0], waves[1]);

}

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
	if (flags == VIS_ACTION_NEXT_PRESET)
	{
		g_plugin->LoadNextPreset(g_plugin->m_fBlendTimeUser);
		ret = true;
	}
	else if (flags == VIS_ACTION_PREV_PRESET)
	{
		g_plugin->LoadPreviousPreset(g_plugin->m_fBlendTimeUser);
		ret = true;
	}
  else if (flags == VIS_ACTION_LOAD_PRESET && param)
  {
    g_plugin->m_nCurrentPreset = *(int *)param;
	  strcpy(g_plugin->m_szCurrentPresetFile, g_plugin->m_szPresetDir);	// note: m_szPresetDir always ends with '\'
	  strcat(g_plugin->m_szCurrentPresetFile, g_plugin->m_pPresetAddr[g_plugin->m_nCurrentPreset]);
    g_plugin->LoadPreset(g_plugin->m_szCurrentPresetFile, g_plugin->m_fBlendTimeUser);
    ret = true;
  }
  else if (flags == VIS_ACTION_LOCK_PRESET)
  {
    g_plugin->m_bHoldPreset = !g_plugin->m_bHoldPreset;
    ret = true;
  }
  else if (flags == VIS_ACTION_RANDOM_PRESET)
  {
    g_plugin->LoadRandomPreset(g_plugin->m_fBlendTimeUser);
    ret = true;
  }
    return ret;
}

void LoadSettings()
{}

//-- GetPresets ---------------------------------------------------------------
// Return a list of presets to XBMC for display
//-----------------------------------------------------------------------------
extern "C" unsigned int GetPresets(char ***presets)
{
  if (!presets || !g_plugin) return 0;
  *presets = g_plugin->m_pPresetAddr;
  return g_plugin->m_nPresets;
}

//-- GetPreset ----------------------------------------------------------------
// Return the index of the current playing preset
//-----------------------------------------------------------------------------
extern "C" unsigned GetPreset()
{
  if (g_plugin)
    return g_plugin->m_nCurrentPreset;
  return 0;
}

//-- IsLocked -----------------------------------------------------------------
// Returns true if this add-on use settings
//-----------------------------------------------------------------------------
extern "C" bool IsLocked()
{
  if(g_plugin)
    return g_plugin->m_bHoldPreset;
  else
    return false;
}

//-- Destroy-------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Destroy()
{
  ADDON_Stop();
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

//-- UpdateSetting ------------------------------------------------------------
// Handle setting change request from XBMC
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_SetSetting(const char* id, const void* value)
{
  if (!id || !value || !g_plugin)
    return ADDON_STATUS_UNKNOWN;

  if (strcmp(id, "###GetSavedSettings") == 0) // We have some settings to be saved in the settings.xml file
  {
    if (strcmp((char*)value, "0") == 0)
    {
      strcpy((char*)id, "lastpresetfolder");
      strcpy((char*)value, g_plugin->m_szPresetDir);
    }
    if (strcmp((char*)value, "1") == 0)
    {
      strcpy((char*)id, "lastlockedstatus");
      strcpy((char*)value, (g_plugin->m_bHoldPreset ? "true" : "false"));
    }
    if (strcmp((char*)value, "2") == 0)
    {
      strcpy((char*)id, "lastpresetidx");
      sprintf ((char*)value, "%i", g_plugin->m_nCurrentPreset);
    }
    if (strcmp((char*)value, "3") == 0)
    {
      strcpy((char*)id, "###End");
    }
    return ADDON_STATUS_OK;
  }
  // It is now time to set the settings got from xmbc
  if (strcmp(id, "Use Preset") == 0)
    OnAction(34, &value);
  else if (strcmp(id, "Automatic Blend Time") == 0)
    g_plugin->m_fBlendTimeAuto = (float)(*(int*)value + 1);
  else if (strcmp(id, "Time Between Presets") == 0)
    g_plugin->m_fTimeBetweenPresets = (float)(*(int*)value*5 + 5);
  else if (strcmp(id, "Additional Random Time") == 0)
    g_plugin->m_fTimeBetweenPresetsRand = (float)(*(int*)value*5 + 5);
  else if (strcmp(id, "Enable Hard Cuts") == 0)
    g_plugin->m_bHardCutsDisabled = *(bool*)value == false;
  else if (strcmp(id, "Loudness Threshold For Hard Cuts") == 0)
    g_plugin->m_fHardCutLoudnessThresh = (float)(*(int*)value)/5.0f + 1.25f;
  else if (strcmp(id, "Average Time Between Hard Cuts") == 0)
    g_plugin->m_fHardCutHalflife = (float)*(int*)value*5 + 5;
  else if (strcmp(id, "Maximum Refresh Rate") == 0)
    g_plugin->m_max_fps_fs = *(int*)value*5 + 20;
  else if (strcmp(id, "Enable Stereo 3d") == 0)
    g_plugin->m_bAlways3D = *(bool*)value;
  else if (strcmp(id, "lastlockedstatus") == 0)
    lastLockedStatus = *(bool*)value;
  else if (strcmp(id, "lastpresetidx") == 0)
    lastPresetIndx = *(int*)value;
  else if (strcmp(id, "lastpresetfolder") == 0)
    strcpy(lastPresetDir, (char*)value);
  else if (strcmp(id, "Preset Shuffle Mode") == 0)
    g_plugin->m_bSequentialPresetOrder = !*(bool*)value;
  else if (strcmp(id, "Preset Pack") == 0)
  {
    if (*(int*)value == 0)
      {
      g_UserPackFolder = false;;
      SetPresetDir ("WA51-presets(265).zip");
      }
    else if (*(int*)value == 1)
    {
      g_UserPackFolder = false;
      SetPresetDir ("Winamp-presets(436).zip");
    }
    else if (*(int*)value == 2)
      g_UserPackFolder = true;
  }
  else if (strcmp(id, "User Preset Folder") ==0 )
  {
    if (g_UserPackFolder) SetPresetDir ((char*)value);
  }
  else
    return ADDON_STATUS_UNKNOWN;

  return ADDON_STATUS_OK;
}

//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" unsigned int GetSubModules(char ***names)
{
  return 0; // this vis supports 0 sub modules
}
