//#include <xtl.h>
#include <windows.h>
#include <io.h>
#include "vis_milkdrop/Plugin.h"
#include "xbmc_vis_dll.h"
#include "libXBMC_addon.h"
#include "XmlDocument.h"
#include <string>

#define strnicmp _strnicmp
#define strcmpi  _strcmpi

using namespace std;

CPlugin* g_plugin;
char g_visName[512];
std::string g_configFile;
std::string g_presetsDir;
std::string g_presetsPack;
char **g_presets=NULL;
unsigned int g_numPresets = 0;

// settings vector
std::vector<DllSetting> g_vecSettings;
StructSetting** g_structSettings;
unsigned int g_uiVisElements;

#define PRESETS_DIR "special://xbmc/visualisations/Milkdrop"
#define CONFIG_FILE "special://profile/visualisations/Milkdrop.conf"

char m_szPresetSave[256] = "";
char g_packFolder[256] = "Milkdrop";

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



//void SaveSettings()
//{
//  char szXMLFile[1024];
//  strcpy(szXMLFile,CONFIG_FILE);
//
//  WriteXML doc;
//  if (!doc.Open(szXMLFile, "visualisation"))
//    return;
//
//  doc.WriteTag("PresetPackFolder", g_packFolder);
//  doc.WriteTag("PresetPack", m_szPresetSave);
//  doc.WriteTag("EnableRating", g_plugin->m_bEnableRating);
//  doc.WriteTag("InstaScan", g_plugin->m_bInstaScan);
//  doc.WriteTag("HardCutsDisabled", g_plugin->m_bHardCutsDisabled);
////  doc.WriteTag("AnisotropicFiltering", g_plugin->m_bAnisotropicFiltering);
//  doc.WriteTag("TexSize", g_plugin->m_nTexSize);
//  doc.WriteTag("MeshSize", g_plugin->m_nGridX);
//  doc.WriteTag("BlendTimeAuto", g_plugin->m_fBlendTimeAuto);
//  doc.WriteTag("TimeBetweenPresets", g_plugin->m_fTimeBetweenPresets);
//  doc.WriteTag("TimeBetweenPresetsRand", g_plugin->m_fTimeBetweenPresetsRand);
//
//  doc.WriteTag("HardCutLoudnessThresh", g_plugin->m_fHardCutLoudnessThresh);
//  doc.WriteTag("HardCutHalflife", g_plugin->m_fHardCutHalflife);
//  doc.WriteTag("MaxFPS", g_plugin->m_max_fps_fs);
//  doc.WriteTag("HoldPreset", g_plugin->m_bHoldPreset ? 1 : 0);
//  doc.WriteTag("CurrentPreset", g_plugin->m_nCurrentPreset);
//  doc.WriteTag("Stereo3d", g_plugin->m_bAlways3D ? true : false);
//
//  unsigned int col;
//  char* c = (char*)&col;
//  c[0] = 0;
//  c[1] = g_plugin->m_cLeftEye3DColor[2];
//  c[2] = g_plugin->m_cLeftEye3DColor[1];
//  c[3] = g_plugin->m_cLeftEye3DColor[0];
//  doc.WriteTag("LeftEyeCol", col, "%08X");
//  c[0] = 0;
//  c[1] = g_plugin->m_cRightEye3DColor[2];
//  c[2] = g_plugin->m_cRightEye3DColor[1];
//  c[3] = g_plugin->m_cRightEye3DColor[0];
//  doc.WriteTag("RightEyeCol", col, "%08X");
//
//  doc.Close();
//}
//
//extern "C" void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
//{
//  if (!pPresets || !numPresets || !currentPreset || !locked || !g_plugin) return;
//  *pPresets = g_plugin->m_pPresetAddr;
//  *numPresets = g_plugin->m_nPresets;
//  *currentPreset = g_plugin->m_nCurrentPreset;
//  *locked = g_plugin->m_bHoldPreset;
//}
//
//extern "C" unsigned int GetSettings(StructSetting*** sSet)
//{
//  m_uiVisElements = VisUtils::VecToStruct(m_vecSettings, &m_structSettings);
//  *sSet = m_structSettings;
//  return m_uiVisElements;
//}
//
//extern "C" void FreeSettings()
//{
//  VisUtils::FreeStruct(m_uiVisElements, &m_structSettings);
//}
//
//extern "C" void UpdateSetting(int num, StructSetting*** sSet)
//{
//  VisUtils::StructToVec(m_uiVisElements, sSet, &m_vecSettings);
//  VisSetting &setting = m_vecSettings[num];
//  if (strcmpi(setting.name, "Use Preset") == 0)
//    OnAction(34, (void *)&setting.current);
//  else if (strcmpi(setting.name, "Automatic Blend Time") == 0)
//    g_plugin->m_fBlendTimeAuto = (float)(setting.current + 1) / 5.0f;
//  else if (strcmpi(setting.name, "Time Between Presets") == 0)
//    g_plugin->m_fTimeBetweenPresets = (float)(setting.current + 5);
//  else if (strcmpi(setting.name, "Additional Random Time") == 0)
//    g_plugin->m_fTimeBetweenPresetsRand = (float)(setting.current + 5);
////  else if (strcmpi(setting.name, "Enable Anisotropic Filtering") == 0)
////    g_plugin->m_bAnisotropicFiltering = setting.current == 1;
//  else if (strcmpi(setting.name, "Enable Hard Cuts") == 0)
//    g_plugin->m_bHardCutsDisabled = setting.current == 0;
//  else if (strcmpi(setting.name, "Loudness Threshold For Hard Cuts") == 0)
//    g_plugin->m_fHardCutLoudnessThresh = (float)setting.current/10.0f + 1.25f;
//  else if (strcmpi(setting.name, "Average Time Between Hard Cuts") == 0)
//    g_plugin->m_fHardCutHalflife = (float)setting.current*5 + 5;
//  else if (strcmpi(setting.name, "Maximum Refresh Rate") == 0)
//    g_plugin->m_max_fps_fs = setting.current*5 + 20;
//  else if (strcmpi(setting.name, "Enable Stereo 3d") == 0)
//    g_plugin->m_bAlways3D = setting.current == 1;
//  else if (strcmpi(setting.name, "Preset Pack") == 0)
//  {
//
//    // Check if its a zip or a folder
//    SetPresetDir(setting.entry[setting.current]);
//
//    // save dir so that we can resave the .xml file
//    sprintf(m_szPresetSave, "%s", setting.entry[setting.current]);
//
//    g_plugin->m_bHoldPreset = false; // Disable locked preset as its no longer there
//    g_plugin->UpdatePresetList();
//
//    // set current preset index to -1 because current preset is no longer in the list
//    g_plugin->m_nCurrentPreset = -1;
//    g_plugin->LoadRandomPreset(g_plugin->m_fBlendTimeUser);
//  }
//  SaveSettings();
//}

void SetPresetDir(const char *pack)
{
  int len = strlen(pack);
  if (len >= 4 && strcmpi(pack + len - 4, ".zip") == 0)
  {
    // Zip file
    strcpy(g_plugin->m_szPresetDir, "zip://special%3A%2F%2Fxbmc%2Faddons%2Fvisualizations%2FMilkdrop%2Fpresets%2F");
    strcat(g_plugin->m_szPresetDir,  pack);
    strcat(g_plugin->m_szPresetDir, "/");
  }
  else if (len >= 4 && strcmpi(pack + len - 4, ".rar") == 0)
  {
    // Rar file
    strcpy(g_plugin->m_szPresetDir, "rar://special%3A%2F%2Fxbmc%2Faddons%2Fvisualizations%2FMilkdrop%2Fpresets%2F");
    strcat(g_plugin->m_szPresetDir,  pack);
    strcat(g_plugin->m_szPresetDir, "/");
  }
  else
  {
    // Normal folder
    strcpy(g_plugin->m_szPresetDir,  g_presetsDir.c_str());
    strcat(g_plugin->m_szPresetDir,  "/");
    strcat(g_plugin->m_szPresetDir,  pack);
    strcat(g_plugin->m_szPresetDir, "/");
  }
}

void FindPresetPacks()
{
  struct _finddata_t c_file;
  long hFile;
  int numPacks = 0;

  char searchFolder[255];
  sprintf(searchFolder, "%s/*", g_presetsDir.c_str());

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
        if (g_presetsPack == "")
          g_presetsPack = szFilename;

        numPacks++;
      }
    }
    while(_findnext(hFile,&c_file) == 0);

    _findclose( hFile );
  }
}

void LoadSettings()
{
	XmlNode node;
	CXmlDocument doc;

	char szXMLFile[1024];
  strcpy(szXMLFile,CONFIG_FILE);

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
        g_presetsPack = nodeStr;
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
//      else if (!strcmpi(doc.GetNodeTag(node),"AnisotropicFiltering"))
//      {
//        g_plugin->m_bAnisotropicFiltering = !strcmpi(doc.GetNodeText(node),"true");
//      }
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
    if(g_presetsPack != "" || g_presetsPack != "/")
    {
      SetPresetDir(g_presetsPack.c_str());
    }
  }

  int tmp;
  bool tmp2;
  const char* tmp3;

//  if (XBMC_get_setting("PresetPack", &tmp3))
//    SetSetting("PresetPack", &tmp3);

  if (XBMC_get_setting("BlendTimeAuto", &tmp))
    SetSetting("BlendTimeAuto", &tmp);

  if (XBMC_get_setting("TimeBetweenPresets", &tmp))
    SetSetting("TimeBetweenPresets", &tmp);

  if (XBMC_get_setting("TimeBetweenPresetsRand", &tmp))
    SetSetting("TimeBetweenPresetsRand", &tmp);

//  if (XBMC_get_setting("AnisotropicFiltering", &tmp2))
//    SetSetting("AnisotropicFiltering", &tmp2);

  if (XBMC_get_setting("HardCutsEnabled", &tmp2))
    SetSetting("HardCutsEnabled", &tmp2);

  if (XBMC_get_setting("HardCutLoudnessThresh", &tmp))
    SetSetting("HardCutLoudnessThresh", &tmp);

  if (XBMC_get_setting("HardCutsHalflife", &tmp))
    SetSetting("HardCutsHalflife", &tmp);

  if (XBMC_get_setting("MaxRefreshRate", &tmp))
    SetSetting("MaxRefreshRate", &tmp);

  if (XBMC_get_setting("Stereo3d", &tmp2))
    SetSetting("Stereo3d", &tmp2);

  g_plugin->UpdatePresetList();
}

//-- Create -------------------------------------------------------------------
// Called once when the visualisation is created by XBMC. Do any setup here.
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS Create(void* hdl, void* props)
{
  if (!props || !hdl)
    return STATUS_UNKNOWN;

  VIS_PROPS* visprops = (VIS_PROPS*)props;

  XBMC_register_me(hdl);

  g_vecSettings.clear();
  g_uiVisElements = 0;

  strcpy(g_visName, visprops->name);
  g_configFile = string(visprops->profile) + string("/Milkdrop.conf");
  //std::string presetsDir = string(visprops->presets) + string("/resources/presets.zip/");
  g_presetsDir = string(visprops->presets) + string("/presets");

	g_plugin = new CPlugin;
	g_plugin->PluginPreInitialize(0, 0);
	g_plugin->PluginInitialize((LPDIRECT3DDEVICE9) visprops->device, visprops->x, visprops->y, visprops->width, visprops->height, visprops->pixelRatio);
  return STATUS_OK;
}

extern "C" void Destroy()
{
	g_plugin->PluginQuit();
	delete g_plugin;
  g_vecSettings.clear();
  g_uiVisElements = 0;
}

//-- Start --------------------------------------------------------------------
// Called when a new soundtrack is played
//-----------------------------------------------------------------------------
extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{

}

//-- Stop ---------------------------------------------------------------------
// Called when the visualisation is closed by XBMC
//-----------------------------------------------------------------------------
extern "C" void Stop()
{
  if (g_presets)
  {
    for (unsigned i = 0; i <g_numPresets; i++)
    {
      free(g_presets[i]);
    }
    free(g_presets);
    g_presets = NULL;
  }
}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
unsigned char waves[2][576];
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

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  g_plugin->PluginRender(waves[0], waves[1]);
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

//-- GetPresets ---------------------------------------------------------------
// Return a list of presets to XBMC for display
//-----------------------------------------------------------------------------
extern "C" unsigned int GetPresets(char ***presets)
{
  g_numPresets = g_plugin->m_nPresets;
  if (g_numPresets > 0)
  {
    g_presets = (char**) malloc(sizeof(char*)*g_numPresets);
    for (unsigned i = 0; i < g_numPresets; i++)
    {
      g_presets[i] = (char*) malloc(strlen(g_plugin->m_pPresetAddr[i])+2);
      if (g_presets[i])
        strcpy(g_presets[i], g_plugin->m_pPresetAddr[i]);
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
    return g_plugin->m_nCurrentPreset;
  return 0;
}

//-- IsLocked -----------------------------------------------------------------
// Returns true if this add-on use settings
//-----------------------------------------------------------------------------
extern "C" bool IsLocked()
{
  return g_plugin->m_bHoldPreset;
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

  if (strcmp(id, "PresetPack")==0)
  {
    g_presetsPack = (const char*) value;
    SetPresetDir(g_presetsPack.c_str());
  }
  else if (strcmp(id, "BlendTimeAuto")==0)
  {
    g_plugin->m_fBlendTimeAuto = (float)((*(int*) value) + 1) / 5.0f;
  }
  else if (strcmp(id, "TimeBetweenPresets")==0)
  {
    g_plugin->m_fTimeBetweenPresets = (float)((*(int*) value) + 5);
  }
  else if (strcmp(id, "TimeBetweenPresetsRand")==0)
  {
    g_plugin->m_fTimeBetweenPresetsRand = (float)((*(int*) value) + 5);
  }
  else if (strcmp(id, "AnisotropicFiltering")==0)
  {
    g_plugin->m_bAnisotropicFiltering = (*(bool*) value);
  }
  else if (strcmp(id, "HardCutsEnabled")==0)
  {
    g_plugin->m_bHardCutsDisabled = !(*(bool*) value);
  }
  else if (strcmp(id, "HardCutLoudnessThresh")==0)
  {
    g_plugin->m_fHardCutLoudnessThresh = (float)(*(int*) value)/10.0f + 1.25f;
  }
  else if (strcmp(id, "HardCutsHalflife")==0)
  {
    g_plugin->m_fHardCutHalflife = (float)(*(int*) value);
  }
  else if (strcmp(id, "MaxRefreshRate")==0)
  {
    g_plugin->m_max_fps_fs = (*(int*) value);
  }
  else if (strcmp(id, "Stereo3d")==0)
  {
    g_plugin->m_bAlways3D = (*(bool*) value);
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
