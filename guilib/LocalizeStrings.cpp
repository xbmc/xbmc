#include "localizestrings.h"
#include "tinyxml/tinyxml.h"

CLocalizeStrings g_localizeStrings;

CLocalizeStrings::CLocalizeStrings(void)
{
}

CLocalizeStrings::~CLocalizeStrings(void)
{
}


bool CLocalizeStrings::Load(const CStdString& strFileName)
{
  m_vecStrings.erase(m_vecStrings.begin(),m_vecStrings.end());
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile(strFileName.c_str()) )
  {
    return false;
  }
  TiXmlElement* pRootElement =xmlDoc.RootElement();
  CStdString strValue=pRootElement->Value();
  if (strValue!=CStdString("strings")) return false;
  const TiXmlNode *pChild = pRootElement->FirstChild();
  while (pChild)
  {
    CStdString strValue=pChild->Value();
    if (strValue=="string")
    {
      const TiXmlNode *pChildID = pChild->FirstChild("id");
      const TiXmlNode *pChildText = pChild->FirstChild("value");
      DWORD dwID=atoi(pChildID->FirstChild()->Value());
      WCHAR wszText[1024];
      swprintf(wszText,L"%S", pChildText->FirstChild()->Value() );
      wstring strText=wszText;
      m_vecStrings[dwID]=strText;
    }
    pChild=pChild->NextSibling();
  }
  return true;
}

const wstring&  CLocalizeStrings::Get(DWORD dwCode)
{
  ivecStrings i;
  i = m_vecStrings.find(dwCode);
  if (i==m_vecStrings.end())
  {
    return L"";
  }
  return i->second;
}