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

   m_userFileName.Format("P:\\plugins\\%s\\%s", url.GetHostName().c_str(), url.GetFileName().c_str());
   CUtil::RemoveSlashAtEnd(m_userFileName);
   m_userFileName += "-settings.xml";

   // Load the settings file from the plugin directory 
   CStdString pluginFileName = "Q:\\plugins\\";
   CUtil::AddFileToFolder(pluginFileName, url.GetHostName(), pluginFileName);
   CUtil::AddFileToFolder(pluginFileName, url.GetFileName(), pluginFileName);
   CUtil::AddFileToFolder(pluginFileName, "resources", pluginFileName);
   CUtil::AddFileToFolder(pluginFileName, "settings.xml", pluginFileName);
   pluginFileName.Replace("/","\\");

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
   CStdString dir;
   dir.Format("P:\\plugins");
   if (!DIRECTORY::CDirectory::Exists(dir))
      DIRECTORY::CDirectory::Create(dir);

   dir.Format("P:\\plugins\\%s", m_url.GetHostName().c_str());
   if (!DIRECTORY::CDirectory::Exists(dir))
      DIRECTORY::CDirectory::Create(dir);
      
   return m_userXmlDoc.SaveFile(m_userFileName);
}

void CPluginSettings::Set(const CStdString key, const CStdString value)
{
   bool done = false;
   
   // Try to find the setting and change its value
   TiXmlElement *setting = m_userXmlDoc.RootElement()->FirstChildElement("setting");
   while (setting && !done)
   {
      const char *id = setting->Attribute("id");
      if (strcmpi(id, key) == 0)
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
      if (strcmpi(id, key) == 0)
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
      if (strcmpi(id, key) == 0 && setting->Attribute("default"))
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

CPluginSettings g_currentPluginSettings;
