#include "Settings.h"
#include "Boxalizer.h"
#include <cstdio>
#include <io.h>
#include <vector>

class CSettings g_settings;
struct CSettings::stSettings g_stSettings;

extern CBoxalizer myVis;
extern LPDIRECT3DDEVICE8	m_pd3dDevice;
// the settings vector
std::vector<VisSetting> m_vecSettings;

void CSettings::SetDefaults()
{
	g_stSettings.m_iSyncDelay = 15;
	g_stSettings.m_iBars = 8;
	g_stSettings.m_bLogScale = true;
	g_stSettings.m_bAverageLevels = false;
	g_stSettings.m_fMinFreq = 80;
	g_stSettings.m_fMaxFreq = 16000;
	g_stSettings.m_bMixChannels = false;
	g_stSettings.m_iLingerTime = 1500;
	g_stSettings.m_fBarDepth = 3.0f;
	
	//Camera Settings
	g_stSettings.m_bCamStatic = false;
	g_stSettings.m_fCamX = 0.0f;
	g_stSettings.m_fCamY = 10.0f;
	g_stSettings.m_fCamLookX = 0.0f;
	g_stSettings.m_fCamLookY = 0.0f;

	strcpy(g_stSettings.m_szTextureFile, "");
  strcpy(m_szVisName,"boxalizer");
}

// Load settings from the Boxalizer.xml configuration file
void CSettings::LoadSettings()
{
  char szXMLFile[1024];
  strcpy(szXMLFile,"P:\\Visualisations\\");
  strcat(szXMLFile,"boxalizer.xml");
  FILE *f = fopen(szXMLFile,"r");
  if (!f)
  {
    strcpy(szXMLFile,"T:\\Visualisations\\");
    strcat(szXMLFile,"boxalizer.xml");
  }
  else
    fclose(f);

  CXmlDocument doc;
  if (doc.Load(szXMLFile) < 0)
    return;

  XmlNode childNode;
  XmlNode node = doc.GetNextNode(XML_ROOT_NODE);
  while (node>0)
  {
    if (strcmpi(doc.GetNodeTag(node),"visualisation"))
    {
      node = doc.GetNextNode(node);
      continue;
    }
    if (childNode = doc.GetChildNode(node,"presets"))
    {
      char* presets = doc.GetNodeText(childNode);
      // Check if its a zip or a folder
      int len = strlen(presets);
      if (len < 4 || strcmpi(presets + len - 4, ".zip") != 0)
      {
        // Normal folder
        strcpy(m_szPresetsPath,  "Q:\\visualisations\\");
        strcat(m_szPresetsPath,  presets);
        strcat(m_szPresetsPath, "\\");
      }
      else
      {
        strcpy(m_szPresetsPath, "zip://q%3A%5Cvisualisations%5C"); 
        strcat(m_szPresetsPath,  presets);
        strcat(m_szPresetsPath, "/");
      }
      // save directory for later
      strcpy(m_szPresetsDir, presets);
    }
    if (childNode = doc.GetChildNode(node,"currentpreset"))
    {
      m_nCurrentPreset = atoi(doc.GetNodeText(childNode));
    }
    node = doc.GetNextNode(node);
  }

  // ok, load up our presets
  m_nPresets = 0;
  if (strlen(m_szPresetsPath) > 0)
  { // run through and grab all presets in this folder...
    struct _finddata_t c_file;
    long hFile;
    char szMask[512];

    strcpy(szMask, m_szPresetsPath);
    int len = strlen(szMask);
    if (szMask[len-1] != '/') 
    {
      strcat(szMask, "/");
    }
    strcat(szMask, "*.*");

    if( (hFile = _findfirst(szMask, &c_file )) != -1L )		// note: returns filename -without- path
    {
      do
      {
        int len = strlen(c_file.name);
        if (len <= 4 || strcmpi(&c_file.name[len - 4], ".xml") != 0)
          continue;
        if (m_szPresets[m_nPresets]) delete[] m_szPresets[m_nPresets];
        m_szPresets[m_nPresets] = new char[len - 4 + 1];
        strncpy(m_szPresets[m_nPresets], c_file.name, len - 4);
        m_szPresets[m_nPresets][len - 4] = 0;
        m_nPresets++;
      }
      while(_findnext(hFile,&c_file) == 0 && m_nPresets < MAX_PRESETS);

      _findclose( hFile );

      // sort the presets (slow, but how many presets can one really have?)
      for (int i = 0; i < m_nPresets; i++)
      {
        for (int j = i+1; j < m_nPresets; j++)
        {
          if (strcmpi(m_szPresets[i], m_szPresets[j]) > 0)
          { // swap i, j
            char temp[256];
            strcpy(temp, m_szPresets[i]);
            delete[] m_szPresets[i];
            m_szPresets[i] = new char[strlen(m_szPresets[j]) + 1];
            strcpy(m_szPresets[i], m_szPresets[j]);
            delete[] m_szPresets[j];
            m_szPresets[j] = new char[strlen(temp) + 1];
            strcpy(m_szPresets[j], temp);
          }
        }
      }
    }
  }
  // setup our settings structure (passable to GUI)
  m_vecSettings.clear();
  // nothing currently
}

void CSettings::LoadPreset(int nPreset)
{
  bool bReverseFreq[2] = {false, false};

  // Set up the defaults
  SetDefaults();

  if (m_nPresets > 0)
  {
    if (nPreset < 0) nPreset = m_nPresets - 1;
    if (nPreset >= m_nPresets) nPreset = 0;
    m_nCurrentPreset = nPreset;

    char szPresetFile[1024];
    strcpy(szPresetFile, m_szPresetsPath);
    int len = strlen(szPresetFile);
    if (len > 0 && szPresetFile[len-1] != '/')
      strcat(szPresetFile, "/");
    strcat(szPresetFile, m_szPresets[m_nCurrentPreset]);
    strcat(szPresetFile, ".xml");
    CXmlDocument doc;
    if (doc.Load(szPresetFile)>=0)
    {
      XmlNode childNode;
      XmlNode node = doc.GetNextNode(XML_ROOT_NODE);
      while(node > 0)
      {
        if(!strcmpi(doc.GetNodeTag(node), "visualisation"))
        {
          node = doc.GetNextNode(node);
          continue;
        }
        if (childNode = doc.GetChildNode(node, "syncdelay"))
        {
          g_stSettings.m_iSyncDelay = atoi(doc.GetNodeText(childNode));
          if (g_stSettings.m_iSyncDelay < 0)
            g_stSettings.m_iSyncDelay = 0;
        }
        else if(childNode = doc.GetChildNode(node, "bars"))
        {
          g_stSettings.m_iBars = atoi(doc.GetNodeText(childNode));
          if(g_stSettings.m_iBars < 1)
            g_stSettings.m_iBars = 1;
          if(g_stSettings.m_iBars > MAX_BARS)
            g_stSettings.m_iBars = MAX_BARS;
        }
        else if (childNode = doc.GetChildNode(node, "freqmin"))
        {
          g_stSettings.m_fMinFreq = (float)atof(doc.GetNodeText(childNode));
          if(g_stSettings.m_fMinFreq < MIN_FREQUENCY) 
            g_stSettings.m_fMinFreq = MIN_FREQUENCY;
          if(g_stSettings.m_fMinFreq > MAX_FREQUENCY-1) 
            g_stSettings.m_fMinFreq = MAX_FREQUENCY-1;
        }
        else if (childNode = doc.GetChildNode(node, "freqmax"))
        {
          g_stSettings.m_fMaxFreq = (float)atof(doc.GetNodeText(childNode));
          if(g_stSettings.m_fMaxFreq <= g_stSettings.m_fMinFreq) 
            g_stSettings.m_fMaxFreq = g_stSettings.m_fMinFreq+1;
          if(g_stSettings.m_fMaxFreq > MAX_FREQUENCY) 
            g_stSettings.m_fMaxFreq = MAX_FREQUENCY;
        }
        else if (childNode = doc.GetChildNode(node, "lingertime"))
        {
          g_stSettings.m_iLingerTime = atoi(doc.GetNodeText(childNode));
          if(g_stSettings.m_iLingerTime < 100)
            g_stSettings.m_iLingerTime = 100;
        }
        else if (childNode = doc.GetChildNode(node, "rowdepth"))
        {
          g_stSettings.m_fBarDepth = (float)atof(doc.GetNodeText(childNode));
          if(g_stSettings.m_fBarDepth < 0.1f)
            g_stSettings.m_fBarDepth = 0.1f;
        }
        else if (childNode = doc.GetChildNode(node, "camposx"))
        {
          g_stSettings.m_fCamX = (float)atof(doc.GetNodeText(childNode));
        }
        else if (childNode = doc.GetChildNode(node, "camposy"))
        {
          g_stSettings.m_fCamY = (float)atof(doc.GetNodeText(childNode));
        }
        else if (childNode = doc.GetChildNode(node, "camlookx"))
        {
          g_stSettings.m_fCamLookX = (float)atof(doc.GetNodeText(childNode));
        }
        else if (childNode = doc.GetChildNode(node, "camlooky"))
        {
          g_stSettings.m_fCamLookY = (float)atof(doc.GetNodeText(childNode));
        }
        else if (childNode = doc.GetChildNode(node, "staticcam"))
        {
          if(strcmp(doc.GetNodeText(childNode), "TRUE") == 0)
            g_stSettings.m_bCamStatic = true;
          else
            g_stSettings.m_bCamStatic = false;
        }
        else if (childNode = doc.GetChildNode(node, "texture"))
        {
          strcpy(g_stSettings.m_szTextureFile, doc.GetNodeText(childNode));
        }
        node = doc.GetNextNode(node);
      }
    }
    doc.Close();
  }

  myVis.Init(m_pd3dDevice);
}

void CSettings::SaveSettings()
{
  char szXMLFile[1024];
  strcpy(szXMLFile,"P:\\Visualisations\\");
  strcat(szXMLFile,m_szVisName);
  strcat(szXMLFile,".xml");

  WriteXML doc;
  if (!doc.Open(szXMLFile, "visualisation"))
    return;

  doc.WriteTag("presets", m_szPresetsDir);
  doc.WriteTag("currentpreset", m_nCurrentPreset);
  doc.Close();
}
extern "C" bool OnAction(long action, void *param)
{
  printf("OnAction %i called with param=%p", action, param);
  if (!g_settings.m_nPresets)
    return false;
  if (action == VIS_ACTION_PREV_PRESET)
    g_settings.LoadPreset(g_settings.m_nCurrentPreset-1);
  if (action == VIS_ACTION_NEXT_PRESET)
    g_settings.LoadPreset(g_settings.m_nCurrentPreset+1);
  if (action == VIS_ACTION_LOAD_PRESET && param)
    g_settings.LoadPreset(*(int *)param);
  g_settings.SaveSettings();
  return true;
}

extern "C" void GetInfo(VIS_INFO* pInfo)
{
  pInfo->bWantsFreq =vInfo.bWantsFreq;
	pInfo->iSyncDelay=0;
}

extern "C" void GetSettings(vector<VisSetting> **settings)
{
  if (!settings) return;
  // load in our settings
  g_settings.LoadSettings();
  *settings = &m_vecSettings;
  return;
}

extern "C" void UpdateSetting(int num)
{
  VisSetting &setting = m_vecSettings[num];
//  SaveSettings();
}

extern "C" void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
{
  if (!pPresets || !numPresets || !currentPreset || !locked) return;
  *pPresets = g_settings.m_szPresets;
  *numPresets = g_settings.m_nPresets;
  *currentPreset = g_settings.m_nCurrentPreset;
  *locked = false;
}