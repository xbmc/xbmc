
#pragma once

#include "lib\tinyxml\tinyxml.h"

class CSettings
{
public:
  CSettings();
  ~CSettings();

  bool Load(const char* file);
  bool Save(const char* file);
  
  std::vector<std::string>& GetRegisteredPdbFiles();

private:
  TiXmlDocument m_xmlDocument;
  std::vector<std::string> m_pdbFiles;
};
