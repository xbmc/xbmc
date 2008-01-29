#ifndef PLUGINSETTINGS_H_
#define PLUGINSETTINGS_H_

#include "../guilib/tinyXML/tinyxml.h"

class CBasicSettings
{
public:
  CBasicSettings();
  virtual ~CBasicSettings();

  bool SaveFromDefault(void);
  virtual bool Load(const CURL& url)  { return false; }
  virtual bool Save(void) { return false; }
  void Clear();

  void Set(const CStdString& key, const CStdString& value);
  CStdString Get(const CStdString& key);

  TiXmlElement* GetPluginRoot();
protected: 
  TiXmlDocument   m_userXmlDoc;
  TiXmlDocument   m_pluginXmlDoc;
};

class CPluginSettings : public CBasicSettings
{
public:
  CPluginSettings();
  virtual ~CPluginSettings();
  bool Load(const CURL& url);
  bool Save(void);
  static bool SettingsExist(const CStdString &strPath);
  
  CPluginSettings& operator =(const CBasicSettings&);
private: 
  CStdString      m_id;
  CURL            m_url;
  CStdString      m_userFileName;  
};

extern CPluginSettings g_currentPluginSettings;

#endif
