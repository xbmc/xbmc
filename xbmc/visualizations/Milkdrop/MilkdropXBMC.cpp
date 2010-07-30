/*
 *      Copyright (C) 2004-2010 Team XBMC
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

#include <windows.h>
#include <io.h>
#include "vis_milkdrop/Plugin.h"
#include "../../addons/include/xbmc_vis_dll.h"
#include "../../addons/include/xbmc_addon_cpp_dll.h"
#include "XmlDocument.h"
#include <string>
#include <direct.h>


#define strnicmp _strnicmp
#define strcmpi  _strcmpi

CPlugin* g_plugin=NULL;
char g_visName[512];

#define PRESETS_DIR "special://xbmc/addons/visualization.milkdrop/presets"

char m_szPresetSave[256] = "";
char g_packFolder[256] = "Milkdrop";
std::string g_configFile;

// settings vector
std::vector<DllSetting> g_vecSettings;
StructSetting** g_structSettings;
unsigned int g_uiVisElements;

int htoi(const char *str) /* Convert hex string to integer */
{
  unsigned int digit, number = 0;
  while (*str)
  {
    if (isdigit(*str))
      digit = *str - '0';
    else
      digit = tolower(*str)-'a'+10;
    number<<=4;
    number+=digit;
    str++;
  }
  return number;
}


void SetPresetDir(const char *pack)
{
  int len = strlen(pack);
  if (len >= 4 && strcmpi(pack + len - 4, ".zip") == 0)
  {
    // Zip file
    strcpy(g_plugin->m_szPresetDir, "zip://special%3A%2F%2Fxbmc%2Faddons%2Fvisualization.milkdrop%2Fpresets%2F"); 
    strcat(g_plugin->m_szPresetDir,  pack);
    strcat(g_plugin->m_szPresetDir, "/");
  }
  else if (len >= 4 && strcmpi(pack + len - 4, ".rar") == 0)
  {
    // Rar file
    strcpy(g_plugin->m_szPresetDir, "zip://special%3A%2F%2Fxbmc%2Faddons%2Fvisualization.milkdrop%2Fpresets%2F"); 
    strcat(g_plugin->m_szPresetDir,  pack);
    strcat(g_plugin->m_szPresetDir, "/");
  }
  else
  {
    // Normal folder
    strcpy(g_plugin->m_szPresetDir,  PRESETS_DIR);
    strcat(g_plugin->m_szPresetDir,  "/");
    strcat(g_plugin->m_szPresetDir,  pack);
    strcat(g_plugin->m_szPresetDir, "/");
  }
}

void Preinit()
{
  if(!g_plugin)
  {
    g_plugin = new CPlugin;
    g_plugin->PluginPreInitialize(0, 0);
  }
}

extern "C" ADDON_STATUS Create(void* hdl, void* props)
{
  if (!props)
    return STATUS_UNKNOWN;

  VIS_PROPS* visprops = (VIS_PROPS*)props;
  strcpy(g_visName, visprops->name);
  g_configFile = std::string(visprops->profile) + std::string("milkdrop.conf");
  _mkdir(visprops->profile);
	
  Preinit();
  g_plugin->PluginInitialize((LPDIRECT3DDEVICE9)visprops->device, visprops->x, visprops->y, visprops->width, visprops->height, visprops->pixelRatio);

  return STATUS_NEED_SETTINGS;
}

extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{

}

void SaveSettings();

extern "C" void Stop()
{
  if(g_plugin)
  {
    SaveSettings();
    g_plugin->PluginQuit();
    delete g_plugin;
    g_plugin = NULL;
  }
  g_vecSettings.clear();
  g_uiVisElements = 0;
}

unsigned char waves[2][576];

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
extern "C" void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
	int ipos=0;
	while (ipos < 576)
	{
		for (int i=0; i < iAudioDataLength; i+=2)
		{
      waves[0][ipos] = char ((pAudioData[i] / 65535.0f) * 255.0f);
      waves[1][ipos] = char ((pAudioData[i+1] / 65535.0f) * 255.0f);
			ipos++;
			if (ipos >= 576) break;
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
  /*if (ret)
    SaveSettings();*/
	return ret;
}

void FindPresetPacks()
{
  struct _finddata_t c_file;
  long hFile;
  int numPacks = 0;
  DllSetting setting10(DllSetting::SPIN, "Preset Pack", "30009");
  char searchFolder[255];
  sprintf(searchFolder, "%s/*", PRESETS_DIR);

  if( (hFile = _findfirst(searchFolder, &c_file )) != -1L )		// note: returns filename -without- path
  {
    do
    {
      char szFilename[512];
      strcpy(szFilename, c_file.name);

      bool pack = false;
      int len = strlen(c_file.name);
      if (len < 4 || (strcmpi(c_file.name + len - 4, ".zip") != 0 &&  strcmpi(c_file.name + len - 4, ".rar") != 0))
      {
        if (c_file.attrib &  _A_SUBDIR && c_file.name[0] != '.')
        {
          pack = true;
        }
      }
      else
      {
        pack = true;
      }

      if (pack)
      {
        setting10.AddEntry(szFilename);

        if(strcmp(m_szPresetSave, "") == 0)
          strcpy(m_szPresetSave, szFilename);

        if (strcmpi(m_szPresetSave, szFilename) == 0)
        {
          // Found current
          setting10.current = numPacks;
        }
        numPacks++;
      }
    }
    while(_findnext(hFile,&c_file) == 0);

    _findclose( hFile );
  }

  g_vecSettings.push_back(setting10);

}

void LoadSettings()
{
	XmlNode node;
	CXmlDocument doc;

	char szXMLFile[1024];
  strcpy(szXMLFile,g_configFile.c_str());

  // update our settings structure
  // setup our settings structure (passable to GUI)
  g_vecSettings.clear();
  g_uiVisElements = 0;

	// Load the config file
	if (doc.Load(szXMLFile) >= 0)
	{
		node = doc.GetNextNode(XML_ROOT_NODE);
		while(node > 0)
		{
			if (!strcmpi(doc.GetNodeTag(node),"PresetPack"))
			{
				char* nodeStr = doc.GetNodeText(node);
				
				// Check if its a zip or a folder
        SetPresetDir(nodeStr);
        // save dir so that we can resave the .xml file
        strcpy(m_szPresetSave, nodeStr);
      }
      if (!strcmpi(doc.GetNodeTag(node),"PresetPackFolder"))
      {
        strcpy(g_packFolder, doc.GetNodeText(node));
      }
      else if (!strcmpi(doc.GetNodeTag(node),"EnableRating"))
      {
        g_plugin->m_bEnableRating = !strcmpi(doc.GetNodeText(node),"true");
      }
      else if (!strcmpi(doc.GetNodeTag(node),"InstaScan"))
      {
        g_plugin->m_bInstaScan = !strcmpi(doc.GetNodeText(node),"true");
      }
      else if (!strcmpi(doc.GetNodeTag(node),"HardCutsDisabled"))
      {
        g_plugin->m_bHardCutsDisabled = !strcmpi(doc.GetNodeText(node),"true");
      }
      else if (!strcmpi(doc.GetNodeTag(node),"TexSize"))
      {
        g_plugin->m_nTexSize = atoi(doc.GetNodeText(node));
        if (g_plugin->m_nTexSize != 256 && g_plugin->m_nTexSize != 512 && g_plugin->m_nTexSize != 1024 && g_plugin->m_nTexSize != 2048)
        {
          g_plugin->m_nTexSize = 1024;
        }
      }
      else if (!strcmpi(doc.GetNodeTag(node),"MeshSize"))
      {
        g_plugin->m_nGridX = atoi(doc.GetNodeText(node));
        if (g_plugin->m_nGridX <= 8)
        {
          g_plugin->m_nGridX = 8;
        }

        g_plugin->m_nGridY = g_plugin->m_nGridX*3/4;

        if (g_plugin->m_nGridX > MAX_GRID_X)
          g_plugin->m_nGridX = MAX_GRID_X;
        if (g_plugin->m_nGridY > MAX_GRID_Y)
          g_plugin->m_nGridY = MAX_GRID_Y;
      }
      else if (!strcmpi(doc.GetNodeTag(node),"BlendTimeAuto"))
      {
        g_plugin->m_fBlendTimeAuto = (float)atof(doc.GetNodeText(node));
      }
      else if (!strcmpi(doc.GetNodeTag(node),"TimeBetweenPresets"))
      {
        g_plugin->m_fTimeBetweenPresets = (float)atof(doc.GetNodeText(node));
        char txt[255];
        sprintf(txt, "Time between %d\n", g_plugin->m_fTimeBetweenPresets);
        OutputDebugString(txt);
      }
      else if (!strcmpi(doc.GetNodeTag(node),"TimeBetweenPresetsRand"))
      {
        g_plugin->m_fTimeBetweenPresetsRand = (float)atof(doc.GetNodeText(node));
      }
      else if (!strcmpi(doc.GetNodeTag(node),"HardCutLoudnessThresh"))
      {
        g_plugin->m_fHardCutLoudnessThresh = (float)atof(doc.GetNodeText(node));
      }
      else if (!strcmpi(doc.GetNodeTag(node),"HardCutHalflife"))
      {
        g_plugin->m_fHardCutHalflife = (float)atof(doc.GetNodeText(node));
      }
      else if (!strcmpi(doc.GetNodeTag(node),"MaxFPS"))
      {
        g_plugin->m_max_fps_fs = atoi(doc.GetNodeText(node));
      }
      else if (!strcmpi(doc.GetNodeTag(node),"HoldPreset"))
      {
        g_plugin->m_bHoldPreset = atoi(doc.GetNodeText(node)) == 1;
      }
      else if (!strcmpi(doc.GetNodeTag(node),"CurrentPreset"))
      {
        g_plugin->m_nCurrentPreset = atoi(doc.GetNodeText(node));
        printf("loaded current preset = %i", g_plugin->m_nCurrentPreset);
      }
	  else if (!strcmpi(doc.GetNodeTag(node),"Stereo3d"))
	  {
		  g_plugin->m_bAlways3D = !strcmpi(doc.GetNodeText(node),"true");
	  }
	  else if (!strcmpi(doc.GetNodeTag(node),"LeftEyeCol"))
	  {
		  int col = htoi((doc.GetNodeText(node)));
	    char* cPtr = (char*)&col;
		  g_plugin->m_cLeftEye3DColor[0] = cPtr[3];
		  g_plugin->m_cLeftEye3DColor[1] = cPtr[2];
		  g_plugin->m_cLeftEye3DColor[2] = cPtr[1];
	  }
	  else if (!strcmpi(doc.GetNodeTag(node),"RightEyeCol"))
	  {
		  int col = htoi((doc.GetNodeText(node)));
      char* cPtr = (char*)&col;
		  g_plugin->m_cRightEye3DColor[0] = cPtr[3];
		  g_plugin->m_cRightEye3DColor[1] = cPtr[2];
		  g_plugin->m_cRightEye3DColor[2] = cPtr[1];
	  }
      node = doc.GetNextNode(node);
    }

    doc.Close();
    FindPresetPacks();
  }
  else
  {
    FindPresetPacks();
    if(strcmp(m_szPresetSave,"") != 0)
    {
      SetPresetDir(m_szPresetSave);
    }
  }

  g_plugin->UpdatePresetList();

  DllSetting setting(DllSetting::SPIN, "Automatic Blend Time", "30000");
  for (int i=0; i < 50; i++)
  {
    char temp[10];
    sprintf(temp, "%2.1f secs", (float)(i + 1)/5);
    setting.AddEntry(temp);
  }
  setting.current = (int)(g_plugin->m_fBlendTimeAuto * 5 - 1);
  g_vecSettings.push_back(setting);
  DllSetting setting2(DllSetting::SPIN, "Time Between Presets", "30001");
  for (int i=0; i < 55; i++)
  {
    char temp[10];
    sprintf(temp, "%i secs", i + 5);
    setting2.AddEntry(temp);
  }
  setting2.current = (int)(g_plugin->m_fTimeBetweenPresets - 5);
  g_vecSettings.push_back(setting2);
  DllSetting setting3(DllSetting::SPIN, "Additional Random Time", "30002");
  for (int i=0; i < 55; i++)
  {
    char temp[10];
    sprintf(temp, "%i secs", i + 5);
    setting3.AddEntry(temp);
  }
  setting3.current = (int)(g_plugin->m_fTimeBetweenPresetsRand - 5);
  g_vecSettings.push_back(setting3);
  DllSetting setting5(DllSetting::CHECK, "Enable Hard Cuts", "30004");
  setting5.current = g_plugin->m_bHardCutsDisabled ? 0 : 1;
  g_vecSettings.push_back(setting5);
  DllSetting setting6(DllSetting::SPIN, "Loudness Threshold For Hard Cuts", "30005");
  for (int i=0; i <= 100; i+=5)
  {
    char temp[10];
    sprintf(temp, "%i%%", i);
    setting6.AddEntry(temp);
  }
  setting6.current = (int)((g_plugin->m_fHardCutLoudnessThresh - 1.25f) * 10.0f);
  g_vecSettings.push_back(setting6);
  DllSetting setting7(DllSetting::SPIN, "Average Time Between Hard Cuts", "30006");
  for (int i=0; i <= 115; i+=5)
  {
    char temp[10];
    sprintf(temp, "%i secs", i+5);
    setting7.AddEntry(temp);
  }
  setting7.current = (int)((g_plugin->m_fHardCutHalflife - 5)/5);
  g_vecSettings.push_back(setting7);
  DllSetting setting8(DllSetting::SPIN, "Maximum Refresh Rate" ,"30007");
  for (int i=20; i <= 60; i+=5)
  {
    char temp[10];
    sprintf(temp, "%i fps", i);
    setting8.AddEntry(temp);
  }
  setting8.current = (g_plugin->m_max_fps_fs - 20) / 5;
  g_vecSettings.push_back(setting8);

  DllSetting setting9(DllSetting::CHECK, "Enable Stereo 3d" ,"30008");
  setting9.current = g_plugin->m_bAlways3D ? 1 : 0;
  g_vecSettings.push_back(setting9);

}

void SaveSettings()
{
  char szXMLFile[1024];
  strcpy(szXMLFile,g_configFile.c_str());

  WriteXML doc;
  if (!doc.Open(szXMLFile, "visualisation"))
    return;

  doc.WriteTag("PresetPackFolder", g_packFolder);
  doc.WriteTag("PresetPack", m_szPresetSave);
  doc.WriteTag("EnableRating", g_plugin->m_bEnableRating);
  doc.WriteTag("InstaScan", g_plugin->m_bInstaScan);
  doc.WriteTag("HardCutsDisabled", g_plugin->m_bHardCutsDisabled);
  doc.WriteTag("TexSize", g_plugin->m_nTexSize);
  doc.WriteTag("MeshSize", g_plugin->m_nGridX);
  doc.WriteTag("BlendTimeAuto", g_plugin->m_fBlendTimeAuto);
  doc.WriteTag("TimeBetweenPresets", g_plugin->m_fTimeBetweenPresets);
  doc.WriteTag("TimeBetweenPresetsRand", g_plugin->m_fTimeBetweenPresetsRand);

  doc.WriteTag("HardCutLoudnessThresh", g_plugin->m_fHardCutLoudnessThresh);
  doc.WriteTag("HardCutHalflife", g_plugin->m_fHardCutHalflife);
  doc.WriteTag("MaxFPS", g_plugin->m_max_fps_fs);
  doc.WriteTag("HoldPreset", g_plugin->m_bHoldPreset ? 1 : 0);
  doc.WriteTag("CurrentPreset", g_plugin->m_nCurrentPreset);
  doc.WriteTag("Stereo3d", g_plugin->m_bAlways3D ? true : false);

  unsigned int col;
  char* c = (char*)&col;
  c[0] = 0;
  c[1] = g_plugin->m_cLeftEye3DColor[2];
  c[2] = g_plugin->m_cLeftEye3DColor[1];
  c[3] = g_plugin->m_cLeftEye3DColor[0];
  doc.WriteTag("LeftEyeCol", col, "%08X");
  c[0] = 0;
  c[1] = g_plugin->m_cRightEye3DColor[2];
  c[2] = g_plugin->m_cRightEye3DColor[1];
  c[3] = g_plugin->m_cRightEye3DColor[0];
  doc.WriteTag("RightEyeCol", col, "%08X");

  doc.Close();
}

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
extern "C" void Destroy()
{
  Stop();
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
  Preinit();
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

  Preinit();

  if (strcmpi(id, "Use Preset") == 0)
    OnAction(34, &value);
  else if (strcmpi(id, "Automatic Blend Time") == 0)
    g_plugin->m_fBlendTimeAuto = (float)(*(int*)value + 1) / 5.0f;
  else if (strcmpi(id, "Time Between Presets") == 0)
    g_plugin->m_fTimeBetweenPresets = (float)(*(int*)value + 5);
  else if (strcmpi(id, "Additional Random Time") == 0)
    g_plugin->m_fTimeBetweenPresetsRand = (float)(*(int*)value + 5);
  else if (strcmpi(id, "Enable Hard Cuts") == 0)
    g_plugin->m_bHardCutsDisabled = *(int*)value == 0;
  else if (strcmpi(id, "Loudness Threshold For Hard Cuts") == 0)
    g_plugin->m_fHardCutLoudnessThresh = (float)(*(int*)value)/10.0f + 1.25f;
  else if (strcmpi(id, "Average Time Between Hard Cuts") == 0)
    g_plugin->m_fHardCutHalflife = (float)*(int*)value*5 + 5;
  else if (strcmpi(id, "Maximum Refresh Rate") == 0)
    g_plugin->m_max_fps_fs = *(int*)value*5 + 20;
  else if (strcmpi(id, "Enable Stereo 3d") == 0)
    g_plugin->m_bAlways3D = *(int*)value == 1;
  else if (strcmpi(id, "Preset Pack") == 0)
  {
   
    if(!g_vecSettings.empty() && !g_vecSettings[0].entry.empty() && *(int*)value < g_vecSettings[0].entry.size())
    {
      if(strncmp(m_szPresetSave, g_vecSettings[0].entry[*(int*)value], sizeof(m_szPresetSave)) != 0)
      {
        // Check if its a zip or a folder
        SetPresetDir(g_vecSettings[0].entry[*(int*)value]);

        // save dir so that we can resave the .xml file
        sprintf(m_szPresetSave, "%s", g_vecSettings[0].entry[*(int*)value]);

        g_plugin->m_bHoldPreset = false; // Disable locked preset as its no longer there
        g_plugin->UpdatePresetList();	

        // set current preset index to -1 because current preset is no longer in the list
        g_plugin->m_nCurrentPreset = -1;
        g_plugin->LoadRandomPreset(g_plugin->m_fBlendTimeUser);
      }
    }
  }
  else
    return STATUS_UNKNOWN;

  SaveSettings();

  return STATUS_OK;
}

//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" unsigned int GetSubModules(char ***names)
{
  return 0; // this vis supports 0 sub modules
}