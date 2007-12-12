#include "stdafx.h"
#include "PluginSettings.h"
#include "Util.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"

CPluginSettings::CPluginSettings()
{
}

bool CPluginSettings::Load(const CURL url)
{
  m_url = url;  

  // create the users filepath
  m_userFileName.Format("P:\\plugin_data\\%s\\%s", url.GetHostName().c_str(), url.GetFileName().c_str());
  CUtil::RemoveSlashAtEnd(m_userFileName);
  CUtil::AddFileToFolder(m_userFileName, "settings.xml", m_userFileName);
  
  // Create our final path
  CStdString pluginFileName = "Q:\\plugins\\";

  CUtil::AddFileToFolder(pluginFileName, url.GetHostName(), pluginFileName);
  CUtil::AddFileToFolder(pluginFileName, url.GetFileName(), pluginFileName);

  // Replace the / at end, GetFileName() leaves a / at the end
  pluginFileName.Replace("/", "\\");

  CUtil::AddFileToFolder(pluginFileName, "resources", pluginFileName);
  CUtil::AddFileToFolder(pluginFileName, "settings.xml", pluginFileName);

  if (!m_pluginXmlDoc.LoadFile(pluginFileName.c_str()))
  {
    CLog::Log(LOGERROR, "Unable to load: %s, Line %d\n%s", pluginFileName.c_str(), m_pluginXmlDoc.ErrorRow(), m_pluginXmlDoc.ErrorDesc());
    return false;
  }
  
  // Make sure that the plugin XML has the settings element
  TiXmlElement *setting = m_pluginXmlDoc.RootElement();
  if (!setting || strcmpi(setting->Value(), "settings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading Settings %s: cannot find root element 'settings'", pluginFileName.c_str());
    return false;
  }  
  
  // Load the user saved settings. If it does not exist, create it
  if (!m_userXmlDoc.LoadFile(m_userFileName.c_str()))
  {
    TiXmlDocument doc;
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");
    doc.InsertEndChild(decl);
    
    TiXmlElement xmlRootElement("settings");
    doc.InsertEndChild(xmlRootElement);
    
    m_userXmlDoc = doc;
    
    // Don't worry about the actual settings, they will be set when the user clicks "Ok"
    // in the settings dialog
  }

  return true;
}

CPluginSettings::~CPluginSettings()
{
}

bool CPluginSettings::Save(void)
{
  // break down the path into directories
  CStdString strRoot, strType, strPlugin;
  CUtil::GetDirectory(m_userFileName, strPlugin);
  CUtil::RemoveSlashAtEnd(strPlugin);
  CUtil::GetDirectory(strPlugin, strType);
  CUtil::RemoveSlashAtEnd(strType);
  CUtil::GetDirectory(strType, strRoot);
  CUtil::RemoveSlashAtEnd(strRoot);

  // create the individual folders
  if (!DIRECTORY::CDirectory::Exists(strRoot))
    DIRECTORY::CDirectory::Create(strRoot);
  if (!DIRECTORY::CDirectory::Exists(strType))
    DIRECTORY::CDirectory::Create(strType);
  if (!DIRECTORY::CDirectory::Exists(strPlugin))
    DIRECTORY::CDirectory::Create(strPlugin);

  return m_userXmlDoc.SaveFile(m_userFileName);
}

void CPluginSettings::Set(const CStdString key, const CStdString value)
{
  if (key == "") return;

  bool done = false;
  
  // Try to find the setting and change its value
  TiXmlElement *setting = m_userXmlDoc.RootElement()->FirstChildElement("setting");
  while (setting && !done)
  {
    const char *id = setting->Attribute("id");
    if (id && strcmpi(id, key) == 0)
    {
      setting->SetAttribute("value", value.c_str());
      done = true;
    }

    setting = setting->NextSiblingElement("setting");
  }
  
  // Setting not found, add it
  if (!done)
  {
    TiXmlElement nodeSetting("setting");
    nodeSetting.SetAttribute("id", key.c_str());
    nodeSetting.SetAttribute("value", value.c_str());
    m_userXmlDoc.RootElement()->InsertEndChild(nodeSetting);
  }
}

CStdString CPluginSettings::Get(const CStdString key)
{
  CStdString result;
  
  // Try to find the setting and return its value
  TiXmlElement *setting = m_userXmlDoc.RootElement()->FirstChildElement("setting");
  while (setting)
  {
    const char *id = setting->Attribute("id");
    if (id && strcmpi(id, key) == 0)
    {
      result = setting->Attribute("value");
      return result;
    }

    setting = setting->NextSiblingElement("setting");
  }
  
  // Try to find the setting in the plugin and return its default value
  setting = m_pluginXmlDoc.RootElement()->FirstChildElement("setting");
  while (setting)
  {
    const char *id = setting->Attribute("id");
    if (id && strcmpi(id, key) == 0 && setting->Attribute("default"))
    {
      result = setting->Attribute("default");
      return result;
    }

    setting = setting->NextSiblingElement("setting");
  }
  
  // Otherwise return empty string
  return result;
}

TiXmlElement* CPluginSettings::GetPluginRoot()
{
  return m_pluginXmlDoc.RootElement();
}

bool CPluginSettings::SettingsExist(const CStdString &strPath)
{
  CURL url(strPath);
  CStdString pluginFileName = "Q:\\plugins\\";

  // Create our final path
  CUtil::AddFileToFolder(pluginFileName, url.GetHostName(), pluginFileName);
  CUtil::AddFileToFolder(pluginFileName, url.GetFileName(), pluginFileName);

  // Replace the / at end, GetFileName() leaves a / at the end
  pluginFileName.Replace("/", "\\");

  CUtil::AddFileToFolder(pluginFileName, "resources", pluginFileName);
  CUtil::AddFileToFolder(pluginFileName, "settings.xml", pluginFileName);

  // Load the settings file to verify it's valid
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(pluginFileName.c_str()))
    return false;

  // Make sure that the plugin XML has the settings element
  TiXmlElement *setting = xmlDoc.RootElement();
  if (!setting || strcmpi(setting->Value(), "settings") != 0)
    return false;

  return true;
}

CPluginSettings g_currentPluginSettings;
