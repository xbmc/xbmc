#include "include.h"
#include "GUIFontManager.h"
#include "GraphicContext.h"
#include "SkinInfo.h"
#include "GUIFontXPR.h"
#include "GUIFontTTF.h"
#include "GUIFont.h"
#include "XMLUtils.h"

#include <xfont.h>


GUIFontManager g_fontManager;

GUIFontManager::GUIFontManager(void)
{}

GUIFontManager::~GUIFontManager(void)
{}

CGUIFont* GUIFontManager::LoadXPR(const CStdString& strFontName, const CStdString& strFilename, DWORD textColor, DWORD shadowColor)
{
  //check if font already exists
  CGUIFont* pFont = GetFont(strFontName);
  if (pFont)
    return pFont;

  CStdString strPath;
  if (strFilename[1] != ':')
  {
    strPath = g_graphicsContext.GetMediaDir();
    strPath += "\\fonts\\";
    strPath += strFilename;
  }
  else
    strPath = strFilename;

  // check if we already have this font file loaded...
  CGUIFontBase* pFontFile = GetFontFile(strFilename);
  if (!pFontFile)
  {
    pFontFile = new CGUIFontXPR(strFilename);
    // First try to load it from the skin directory
    boolean bFontLoaded = ((CGUIFontXPR *)pFontFile)->Load(strPath);
    if (!bFontLoaded)
    {
      // Now try to load it from media\fonts
      if (strFilename[1] != ':')
      {
        strPath = "Q:\\media\\Fonts\\";
        strPath += strFilename;
      }

      bFontLoaded = ((CGUIFontXPR *)pFontFile)->Load(strPath);
    }

    if (!bFontLoaded)
    {
      delete pFontFile;
      // font could not b loaded
      CLog::Log(LOGERROR, "Couldn't load font name:%s file:%s", strFontName.c_str(), strPath.c_str());
      return NULL;
    }
    m_vecFontFiles.push_back(pFontFile);
  }

  // font file is loaded, create our CGUIFont
  CGUIFont *pNewFont = new CGUIFont(strFontName, textColor, shadowColor, pFontFile);
  m_vecFonts.push_back(pNewFont);
  return pNewFont;
}

CGUIFont* GUIFontManager::LoadTTF(const CStdString& strFontName, const CStdString& strFilename, DWORD textColor, DWORD shadowColor, const int iSize, const int iStyle)
{
  //check if font already exists
  CGUIFont* pFont = GetFont(strFontName);
  if (pFont)
    return pFont;

  CStdString strPath;
  if (strFilename[1] != ':')
  {
    strPath = g_graphicsContext.GetMediaDir();
    strPath += "\\fonts\\";
    strPath += strFilename;
  }
  else
    strPath = strFilename;

  // check if we already have this font file loaded...
  CStdString TTFfontName;
  TTFfontName.Format("%s_%i_%i", strFilename, iSize, iStyle);
  CGUIFontBase* pFontFile = GetFontFile(TTFfontName);
  if (!pFontFile)
  {
    pFontFile = new CGUIFontTTF(TTFfontName);
    boolean bFontLoaded = ((CGUIFontTTF *)pFontFile)->Load(strPath, iSize, iStyle);
    if (!bFontLoaded)
    {
      // Now try to load it from media\fonts
      if (strFilename[1] != ':')
      {
        strPath = "Q:\\media\\Fonts\\";
        strPath += strFilename;
      }

      bFontLoaded = ((CGUIFontTTF *)pFontFile)->Load(strPath, iSize, iStyle);
    }

    if (!bFontLoaded)
    {
      delete pFontFile;

      // font could not b loaded
      CLog::Log(LOGERROR, "Couldn't load font name:%s file:%s", strFontName.c_str(), strPath.c_str());

      return NULL;
    }
    m_vecFontFiles.push_back(pFontFile);
  }

  // font file is loaded, create our CGUIFont
  CGUIFont *pNewFont = new CGUIFont(strFontName, textColor, shadowColor, pFontFile);
  m_vecFonts.push_back(pNewFont);
  return pNewFont;
}

void GUIFontManager::Unload(const CStdString& strFontName)
{
  for (vector<CGUIFont*>::iterator iFont = m_vecFonts.begin(); iFont != m_vecFonts.end(); ++iFont)
  {
    if ((*iFont)->GetFontName() == strFontName)
    {
      delete (*iFont);
      m_vecFonts.erase(iFont);
      return;
    }
  }
}

CGUIFontBase* GUIFontManager::GetFontFile(const CStdString& strFileName)
{
  for (int i = 0; i < (int)m_vecFontFiles.size(); ++i)
  {
    CGUIFontBase* pFont = m_vecFontFiles[i];
    if (pFont->GetFileName() == strFileName)
      return pFont;
  }
  return NULL;
}

CGUIFont* GUIFontManager::GetFont(const CStdString& strFontName)
{
  for (int i = 0; i < (int)m_vecFonts.size(); ++i)
  {
    CGUIFont* pFont = m_vecFonts[i];
    if (pFont->GetFontName() == strFontName)
      return pFont;
  }
  return NULL;
}

void GUIFontManager::Clear()
{
  for (int i = 0; i < (int)m_vecFonts.size(); ++i)
  {
    CGUIFont* pFont = m_vecFonts[i];
    delete pFont;
  }

  m_vecFonts.clear();
  m_vecFontFiles.clear();
}

void GUIFontManager::LoadFonts(const CStdString& strFontSet)
{
  // Get the file to load fonts from:
  RESOLUTION res;
  CStdString strPath = g_SkinInfo.GetSkinPath("font.xml", &res);
  CLog::Log(LOGINFO, "Loading fonts from %s", strPath.c_str());

  TiXmlDocument xmlDoc;
  // first try our preferred file
  if ( !xmlDoc.LoadFile(strPath.c_str()) )
  {
    CLog::Log(LOGERROR, "Couldn't load %s", strPath.c_str());
    return ;
  }
  TiXmlElement* pRootElement = xmlDoc.RootElement();

  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("fonts"))
  {
    CLog::Log(LOGERROR, "file %s doesnt start with <fonts>", strPath.c_str());
    return ;
  }
  const TiXmlNode *pChild = pRootElement->FirstChild();

  // If there are no fontset's defined in the XML (old skin format) run in backward compatibility
  // and ignore the fontset request
  strValue = pChild->Value();
  if (strValue == "font")
  {
    LoadFonts(pChild);
  }
  else if (strValue == "fontset")
  {
    while (pChild)
    {
      strValue = pChild->Value();
      if (strValue == "fontset")
      {
        const char* idAttr = ((TiXmlElement*) pChild)->Attribute("id");

        // Check if this is the fontset that we want
        if (idAttr != NULL && stricmp(strFontSet.c_str(), idAttr) == 0)
        {
          LoadFonts(pChild->FirstChild());
          break;
        }
      }

      pChild = pChild->NextSibling();
    }

    // If no fontset was loaded
    if (pChild == NULL)
    {
      CLog::Log(LOGWARNING, "file %s doesnt have <fontset> with name '%s', defaulting to first fontset", strPath.c_str(), strFontSet.c_str());
      LoadFonts(pRootElement->FirstChild()->FirstChild());
    }
  }
  else
  {
    CLog::Log(LOGERROR, "file %s doesnt have <font> or <fontset> in <fonts>, but rather %s", strPath.c_str(), strValue.c_str());
    return ;
  }
}

void GUIFontManager::LoadFonts(const TiXmlNode* fontNode)
{
  while (fontNode)
  {
    CStdString strValue = fontNode->Value();
    if (strValue == "font")
    {
      const TiXmlNode *pNode = fontNode->FirstChild("name");
      if (pNode)
      {
        CStdString strFontName = pNode->FirstChild()->Value();
        DWORD shadowColor = 0;
        DWORD textColor = 0;
        XMLUtils::GetHex(fontNode, "shadow", shadowColor);
        XMLUtils::GetHex(fontNode, "color", textColor);
        const TiXmlNode *pNode = fontNode->FirstChild("filename");
        if (pNode)
        {
          CStdString strFontFileName = pNode->FirstChild()->Value();

          if (strstr(strFontFileName, ".xpr") != NULL)
          {
            LoadXPR(strFontName, strFontFileName, textColor, shadowColor);
          }
          else if (strstr(strFontFileName, ".ttf") != NULL)
          {
            int iSize = 20;
            int iStyle = XFONT_NORMAL;

            XMLUtils::GetInt(fontNode, "size", iSize);
            if (iSize <= 0) iSize = 20;

            pNode = fontNode->FirstChild("style");
            if (pNode)
            {
              CStdString style = pNode->FirstChild()->Value();
              if (style == "normal")
                iStyle = XFONT_NORMAL;
              else if (style == "bold")
                iStyle = XFONT_BOLD;
              else if (style == "italics")
                iStyle = XFONT_ITALICS;
              else if (style == "bolditalics")
                iStyle = XFONT_BOLDITALICS;
            }

            LoadTTF(strFontName, strFontFileName, textColor, shadowColor, iSize, iStyle);
          }
        }
      }
    }

    fontNode = fontNode->NextSibling();
  }
}
