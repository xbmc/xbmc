#include "guifontmanager.h"
#include "tinyxml/tinyxml.h"

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
  OutputDebugString("Could'nt load font name:");
  OutputDebugString(strFontName.c_str());
  OutputDebugString("  filename:");
  OutputDebugString(strFilename.c_str());
  OutputDebugString("\n");
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

void GUIFontManager::LoadFonts(const CStdString& strFilename)
{
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile(strFilename.c_str()) )
  {
    OutputDebugString("Could'nt load font xml:");
    OutputDebugString(strFilename.c_str());
    OutputDebugString("\n");

    return ;
  }
  TiXmlElement* pRootElement =xmlDoc.RootElement();

  CStdString strValue=pRootElement->Value();
  if (strValue!=CStdString("fonts")) return ;
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