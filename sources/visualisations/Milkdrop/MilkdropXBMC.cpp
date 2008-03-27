#include <xtl.h>
#include <io.h>
#include "vis_milkdrop/Plugin.h"
#include "xbmc_vis.h"
#include "XmlDocument.h"

CPlugin* g_plugin;
char g_visName[512];

char m_szPresetSave[256] = "Milkdrop";
char g_packFolder[256] = "Milkdrop";

extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName, float fPixelRatio)
{
	strcpy(g_visName, szVisualisationName);
	g_plugin = new CPlugin;
	g_plugin->PluginPreInitialize(0, 0);
	g_plugin->PluginInitialize(pd3dDevice, iPosX, iPosY, iWidth, iHeight, fPixelRatio);
}

extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{

}

void SaveSettings();

extern "C" void Stop()
{
  SaveSettings();
	g_plugin->PluginQuit();
	delete g_plugin;
  m_vecSettings.clear();
}

unsigned char waves[2][576];

extern "C" void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
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

extern "C" bool OnAction(long flags, void *param)
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
  if (ret)
    SaveSettings();
	return ret;
}

void FindPresetPacks()
{
  struct _finddata_t c_file;
  long hFile;
  int numPacks = 0;
  VisSetting setting10(VisSetting::SPIN, "Preset Pack");
  char searchFolder[255];
  sprintf(searchFolder, "Q:\\visualisations\\%s\\*", g_packFolder);

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
        if (c_file.attrib &  _A_SUBDIR)
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

        char nameCmp[512];
        sprintf(nameCmp, "%s\\%s", g_packFolder, szFilename);

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

  m_vecSettings.push_back(setting10);

}

void LoadSettings()
{
	XmlNode node;
	CXmlDocument doc;

	char szXMLFile[1024];
  strcpy(szXMLFile,"P:\\Visualisations\\");
  strcat(szXMLFile,g_visName);
  strcat(szXMLFile,".xml");
  FILE *f = fopen(szXMLFile,"r");
  if (!f)
  {
    strcpy(szXMLFile,"T:\\Visualisations\\");
    strcat(szXMLFile,g_visName);
    strcat(szXMLFile,".xml");
  }
  else
    fclose(f);

	//strcpy(szXMLFile, "d:\\milkdrop.xml");

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
				int len = strlen(nodeStr);

        if (len >= 4 && strcmpi(nodeStr + len - 4, ".zip") == 0)
        {
          // Zip file
          strcpy(g_plugin->m_szPresetDir, "zip://q%3A%5Cvisualisations%5C"); 
          strcat(g_plugin->m_szPresetDir,  g_packFolder);
          strcat(g_plugin->m_szPresetDir,  "%5C");
          strcat(g_plugin->m_szPresetDir,  nodeStr);
          strcat(g_plugin->m_szPresetDir, "/");
        }
        else if (len >= 4 && strcmpi(nodeStr + len - 4, ".rar") == 0)
        {
          // Rar file
          strcpy(g_plugin->m_szPresetDir, "rar://q%3A%5Cvisualisations%5C"); 
          strcat(g_plugin->m_szPresetDir,  g_packFolder);
          strcat(g_plugin->m_szPresetDir,  "%5C");
          strcat(g_plugin->m_szPresetDir,  nodeStr);
          strcat(g_plugin->m_szPresetDir, "/");
        }
        else        
				{
					// Normal folder
					strcpy(g_plugin->m_szPresetDir,  "Q:\\visualisations\\");
          strcat(g_plugin->m_szPresetDir,  g_packFolder);
          strcat(g_plugin->m_szPresetDir,  "\\");
					strcat(g_plugin->m_szPresetDir,  nodeStr);
					strcat(g_plugin->m_szPresetDir, "\\");
        }
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
  }
  // update our settings structure
  // setup our settings structure (passable to GUI)
  m_vecSettings.clear();

  FindPresetPacks();

  VisSetting setting(VisSetting::SPIN, "Automatic Blend Time");
  for (int i=0; i < 50; i++)
  {
    char temp[10];
    sprintf(temp, "%2.1f secs", (float)(i + 1)/5);
    setting.AddEntry(temp);
  }
  setting.current = (int)(g_plugin->m_fBlendTimeAuto * 5 - 1);
  m_vecSettings.push_back(setting);
  VisSetting setting2(VisSetting::SPIN, "Time Between Presets");
  for (int i=0; i < 55; i++)
  {
    char temp[10];
    sprintf(temp, "%i secs", i + 5);
    setting2.AddEntry(temp);
  }
  setting2.current = (int)(g_plugin->m_fTimeBetweenPresets - 5);
  m_vecSettings.push_back(setting2);
  VisSetting setting3(VisSetting::SPIN, "Additional Random Time");
  for (int i=0; i < 55; i++)
  {
    char temp[10];
    sprintf(temp, "%i secs", i + 5);
    setting3.AddEntry(temp);
  }
  setting3.current = (int)(g_plugin->m_fTimeBetweenPresetsRand - 5);
  m_vecSettings.push_back(setting3);
//  VisSetting setting4(VisSetting::CHECK, "Enable Anisotropic Filtering");
//  setting4.current = g_plugin->m_bAnisotropicFiltering ? 1 : 0;
//  m_vecSettings.push_back(setting4);
  VisSetting setting5(VisSetting::CHECK, "Enable Hard Cuts");
  setting5.current = g_plugin->m_bHardCutsDisabled ? 0 : 1;
  m_vecSettings.push_back(setting5);
  VisSetting setting6(VisSetting::SPIN, "Loudness Threshold For Hard Cuts");
  for (int i=0; i <= 100; i+=5)
  {
    char temp[10];
    sprintf(temp, "%i%%", i);
    setting6.AddEntry(temp);
  }
  setting6.current = (int)((g_plugin->m_fHardCutLoudnessThresh - 1.25f) * 10.0f);
  m_vecSettings.push_back(setting6);
  VisSetting setting7(VisSetting::SPIN, "Average Time Between Hard Cuts");
  for (int i=0; i <= 115; i+=5)
  {
    char temp[10];
    sprintf(temp, "%i secs", i+5);
    setting7.AddEntry(temp);
  }
  setting7.current = (int)((g_plugin->m_fHardCutHalflife - 5)/5);
  m_vecSettings.push_back(setting7);
  VisSetting setting8(VisSetting::SPIN, "Maximum Refresh Rate");
  for (int i=20; i <= 60; i+=5)
  {
    char temp[10];
    sprintf(temp, "%i fps", i);
    setting8.AddEntry(temp);
  }
  setting8.current = (g_plugin->m_max_fps_fs - 20) / 5;
  m_vecSettings.push_back(setting8);

  VisSetting setting9(VisSetting::CHECK, "Enable Stereo 3d");
  setting9.current = g_plugin->m_bAlways3D ? 1 : 0;
  m_vecSettings.push_back(setting9);

}

void SaveSettings()
{
  char szXMLFile[1024];
  strcpy(szXMLFile,"P:\\Visualisations\\");
  strcat(szXMLFile,g_visName);
  strcat(szXMLFile,".xml");

  WriteXML doc;
  if (!doc.Open(szXMLFile, "visualisation"))
    return;

  doc.WriteTag("PresetPackFolder", g_packFolder);
  doc.WriteTag("PresetPack", m_szPresetSave);
  doc.WriteTag("EnableRating", g_plugin->m_bEnableRating);
  doc.WriteTag("InstaScan", g_plugin->m_bInstaScan);
  doc.WriteTag("HardCutsDisabled", g_plugin->m_bHardCutsDisabled);
//  doc.WriteTag("AnisotropicFiltering", g_plugin->m_bAnisotropicFiltering);
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

extern "C" void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
{
  if (!pPresets || !numPresets || !currentPreset || !locked || !g_plugin) return;
  *pPresets = g_plugin->m_pPresetAddr;
  *numPresets = g_plugin->m_nPresets;
  *currentPreset = g_plugin->m_nCurrentPreset;
  *locked = g_plugin->m_bHoldPreset;
}

extern "C" void GetSettings(vector<VisSetting> **vecSettings)
{
  if (!vecSettings) return;
  // load in our settings
  *vecSettings = &m_vecSettings;
  return;
}

extern "C" void UpdateSetting(int num)
{
  VisSetting &setting = m_vecSettings[num];
  if (strcmpi(setting.name, "Use Preset") == 0)
    OnAction(34, (void *)&setting.current);
  else if (strcmpi(setting.name, "Automatic Blend Time") == 0)
    g_plugin->m_fBlendTimeAuto = (float)(setting.current + 1) / 5.0f;
  else if (strcmpi(setting.name, "Time Between Presets") == 0)
    g_plugin->m_fTimeBetweenPresets = (float)(setting.current + 5);
  else if (strcmpi(setting.name, "Additional Random Time") == 0)
    g_plugin->m_fTimeBetweenPresetsRand = (float)(setting.current + 5);
//  else if (strcmpi(setting.name, "Enable Anisotropic Filtering") == 0)
//    g_plugin->m_bAnisotropicFiltering = setting.current == 1;
  else if (strcmpi(setting.name, "Enable Hard Cuts") == 0)
    g_plugin->m_bHardCutsDisabled = setting.current == 0;
  else if (strcmpi(setting.name, "Loudness Threshold For Hard Cuts") == 0)
    g_plugin->m_fHardCutLoudnessThresh = (float)setting.current/10.0f + 1.25f;
  else if (strcmpi(setting.name, "Average Time Between Hard Cuts") == 0)
    g_plugin->m_fHardCutHalflife = (float)setting.current*5 + 5;
  else if (strcmpi(setting.name, "Maximum Refresh Rate") == 0)
    g_plugin->m_max_fps_fs = setting.current*5 + 20;
  else if (strcmpi(setting.name, "Enable Stereo 3d") == 0)
    g_plugin->m_bAlways3D = setting.current == 1;
  else if (strcmpi(setting.name, "Preset Pack") == 0)
  {
    
    // Check if its a zip or a folder
    int len = strlen(setting.entry[setting.current]);

    if (len >= 4 && strcmpi(setting.entry[setting.current] + len - 4, ".zip") == 0)
    {
      // Zip file
      strcpy(g_plugin->m_szPresetDir, "zip://Z:\\temp,1,,Q:\\visualisations\\"); 
      strcat(g_plugin->m_szPresetDir,  g_packFolder);
      strcat(g_plugin->m_szPresetDir,  "\\");
      strcat(g_plugin->m_szPresetDir,  setting.entry[setting.current]);
      strcat(g_plugin->m_szPresetDir, ",\\");
    }
    else if (len >= 4 && strcmpi(setting.entry[setting.current] + len - 4, ".rar") == 0)
    {
      // Rar file
      strcpy(g_plugin->m_szPresetDir, "rar://Z:\\temp,2,,Q:\\visualisations\\"); 
      strcat(g_plugin->m_szPresetDir,  g_packFolder);
      strcat(g_plugin->m_szPresetDir,  "\\");
      strcat(g_plugin->m_szPresetDir,  setting.entry[setting.current]);
      strcat(g_plugin->m_szPresetDir, ",\\");
    }
    else        
    {
      // Normal folder
      strcpy(g_plugin->m_szPresetDir,  "Q:\\visualisations\\");
      strcat(g_plugin->m_szPresetDir,  g_packFolder);
      strcat(g_plugin->m_szPresetDir,  "\\");
      strcat(g_plugin->m_szPresetDir,  setting.entry[setting.current]);
      strcat(g_plugin->m_szPresetDir, "\\");
    }

    // save dir so that we can resave the .xml file
    sprintf(m_szPresetSave, "%s", setting.entry[setting.current]);

    g_plugin->m_bHoldPreset = false; // Disable locked preset as its no longer there
    g_plugin->UpdatePresetList();	

    // set current preset index to -1 because current preset is no longer in the list
    g_plugin->m_nCurrentPreset = -1;
    g_plugin->LoadRandomPreset(g_plugin->m_fBlendTimeUser);
  }
  SaveSettings();
}
