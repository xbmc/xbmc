#include "stdafx.h"
#include "guifontmanager.h"
#include "tinyxml/tinyxml.h"
#include "../xbmc/utils/log.h"
GUIFontManager  g_fontManager;

GUIFontManager::GUIFontManager(void)
{
}

GUIFontManager::~GUIFontManager(void)
{
}


CGUIFont* GUIFontManager::Load(const CStdString& strFontName,const CStdString& strFilename)
{
  //check if font already exists
  CGUIFont* pFont = GetFont(strFontName);
  if (pFont) return pFont;
 
  CGUIFont* pNewFont = new CGUIFont();
  if (pNewFont->Load(strFontName,strFilename))
  {
    // font is loaded
    m_vecFonts.push_back(pNewFont);
    return pNewFont;
  }
  // font could not b loaded
  CLog::Log("Could'nt load font name:%s file:%s", strFontName.c_str(),strFilename.c_str());
  delete pNewFont;
  return NULL;
}

CGUIFont*	GUIFontManager::GetFont(const CStdString& strFontName)
{
  for (int i=0; i < (int)m_vecFonts.size(); ++i)
  {
    CGUIFont* pFont=m_vecFonts[i];
    if (pFont->GetFontName() == strFontName) return pFont;
  }
  return NULL;
}

void GUIFontManager::Clear()
{
	for (int i=0; i < (int)m_vecFonts.size(); ++i)
  {
    CGUIFont* pFont=m_vecFonts[i];
		delete pFont;
	}
	m_vecFonts.erase(m_vecFonts.begin(),m_vecFonts.end());
}

void GUIFontManager::LoadFonts(const CStdString& strFilename)
{
  CLog::Log("Loading fonts from %s",strFilename.c_str());
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile(strFilename.c_str()) )
  {
    CLog::Log("Could'nt load %s",strFilename.c_str());
    return ;
  }
  TiXmlElement* pRootElement =xmlDoc.RootElement();

  CStdString strValue=pRootElement->Value();
  if (strValue!=CStdString("fonts")) 
  {
    CLog::Log("file %s doesnt start with <fonts>",strFilename.c_str());
    return ;
  }
  const TiXmlNode *pChild = pRootElement->FirstChild();
  while (pChild)
  {
    CStdString strValue=pChild->Value();
    if (strValue=="font")
    {  
      const TiXmlNode *pNode = pChild->FirstChild("name");
      if (pNode)
      {
          CStdString strFontName=pNode->FirstChild()->Value();
          const TiXmlNode *pNode = pChild->FirstChild("filename");
          if (pNode)
          {
            CStdString strFontFileName=pNode->FirstChild()->Value();
            Load(strFontName,strFontFileName);
          }
      }

    }
    pChild=pChild->NextSibling();  
  }

}