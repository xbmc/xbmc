
#include "..\stdafx.h"
#include "Settings.h"

#define ELEMENT_REGISTERED_PDB_FILES    "RegisteredSymbolFiles"

CSettings::CSettings()
{
}

CSettings::~CSettings()
{
}


bool CSettings::Load(const char* file)
{
  if (m_xmlDocument.LoadFile(file))
  {
    TiXmlElement* pRootElement = m_xmlDocument.RootElement();
    if (pRootElement)
    {
      TiXmlElement* pPdbFilesElement = pRootElement->FirstChildElement(ELEMENT_REGISTERED_PDB_FILES);
      if (pPdbFilesElement)
      {
        // get all the registered files
        TiXmlNode* pFileNode = NULL;
        pFileNode = pPdbFilesElement->IterateChildren("File", pFileNode);
        while (pFileNode)
        {
          TiXmlElement* pFileElement = pFileNode->ToElement();
          if (!pFileElement) return false;
          std::string pdbFile = pFileElement->GetText();
          m_pdbFiles.push_back(pdbFile);
          
          pFileNode = pPdbFilesElement->IterateChildren("File", pFileNode);
        }
      }
      return true;
    }
  }
  
  return false;
}

bool CSettings::Save(const char* file)
{
  return false;
}


std::vector<std::string>& CSettings::GetRegisteredPdbFiles()
{
  return m_pdbFiles;
}
