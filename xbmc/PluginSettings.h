#ifndef PLUGINSETTINGS_H_
#define PLUGINSETTINGS_H_

#include "tinyXML/tinyxml.h" 

class CPluginSettings
{
public:
  CPluginSettings();
  ~CPluginSettings();
  bool Load(const CURL url);
  bool Save(void);
  void Set(const CStdString key, const CStdString value);
  CStdString Get(const CStdString key);
  TiXmlElement* GetPluginRoot();
  
private: 
  TiXmlDocument   m_userXmlDoc;
  TiXmlDocument   m_pluginXmlDoc;
  CStdString      m_id;
  CURL            m_url;
  CStdString      m_userFileName;  
};

extern CPluginSettings g_currentPluginSettings;

#endif
