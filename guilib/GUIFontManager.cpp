#include "include.h"
#include "GUIFontManager.h"
#include "GraphicContext.h"
#include "SkinInfo.h"
#include "GUIFontTTF.h"
#include "GUIFont.h"
#include "XMLUtils.h"
#include "GuiControlFactory.h"
#include "../xbmc/Util.h"


GUIFontManager g_fontManager;

GUIFontManager::GUIFontManager(void)
{
  m_fontsetUnicode=false;
}

GUIFontManager::~GUIFontManager(void)
{}

CGUIFont* GUIFontManager::LoadTTF(const CStdString& strFontName, const CStdString& strFilename, DWORD textColor, DWORD shadowColor, const int iSize, const int iStyle, float aspect)
{
  //check if font already exists
  CGUIFont* pFont = GetFont(strFontName, false);
  if (pFont)
    return pFont;

  // adjust aspect ratio
  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  // #ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  if (g_SkinInfo.GetVersion() > 2.0 && m_skinResolution == PAL_16x9 || m_skinResolution == PAL60_16x9 || m_skinResolution == NTSC_16x9 || m_skinResolution == HDTV_480p_16x9)
    aspect *= 0.75f;

  aspect *= g_graphicsContext.GetGUIScaleY() / g_graphicsContext.GetGUIScaleX();
  float newSize = (float) iSize / g_graphicsContext.GetGUIScaleY();
    
  CStdString strPath;
  if (strFilename[1] != ':')
  {
    strPath = g_graphicsContext.GetMediaDir();
    strPath += "\\fonts\\";
    strPath += CUtil::GetFileName(strFilename);
  }
  else
    strPath = strFilename;

#ifdef _LINUX
  strPath = PTH_IC(strPath);
#endif

  // check if we already have this font file loaded (font object could differ only by color or style)
  CStdString TTFfontName;
  TTFfontName.Format("%s_%f_%f", strFilename, newSize, aspect);
  CGUIFontTTF* pFontFile = GetFontFile(TTFfontName);
  if (!pFontFile)
  {
    pFontFile = new CGUIFontTTF(TTFfontName);
    bool bFontLoaded = pFontFile->Load(strPath, newSize, aspect);
    if (!bFontLoaded)
    {
      // Now try to load it from media\fonts
      if (strFilename[1] != ':')
      {
        strPath = _P("Q:\\media\\Fonts\\");
        strPath += CUtil::GetFileName(strFilename);
#ifdef _LINUX
        strPath = PTH_IC(strPath);
#endif
      }

      bFontLoaded = pFontFile->Load(strPath, newSize, aspect);
    }

    if (!bFontLoaded)
    {
      delete pFontFile;

      // font could not b loaded
      CLog::Log(LOGERROR, "Couldn't load font name:%s file:%s", strFontName.c_str(), _P(strPath).c_str());

      return NULL;
    }
    m_vecFontFiles.push_back(pFontFile);
  }

  // font file is loaded, create our CGUIFont
  CGUIFont *pNewFont = new CGUIFont(strFontName, iStyle, textColor, shadowColor, pFontFile);
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

void GUIFontManager::FreeFontFile(CGUIFontTTF *pFont)
{
  for (vector<CGUIFontTTF*>::iterator it = m_vecFontFiles.begin(); it != m_vecFontFiles.end(); ++it)
  {
    if (pFont == *it)
    {
      m_vecFontFiles.erase(it);
      delete pFont;
      return;
    }
  }
}

CGUIFontTTF* GUIFontManager::GetFontFile(const CStdString& strFileName)
{
  for (int i = 0; i < (int)m_vecFontFiles.size(); ++i)
  {
    CGUIFontTTF* pFont = m_vecFontFiles[i];
    if (pFont->GetFileName() == strFileName)
      return pFont;
  }
  return NULL;
}

CGUIFont* GUIFontManager::GetFont(const CStdString& strFontName, bool fallback /*= true*/)
{
  for (int i = 0; i < (int)m_vecFonts.size(); ++i)
  {
    CGUIFont* pFont = m_vecFonts[i];
    if (pFont->GetFontName() == strFontName)
      return pFont;
  }
  // fall back to "font13" if we have none
  if (fallback && !strFontName.IsEmpty() && !strFontName.Equals("-") && !strFontName.Equals("font13"))
    return GetFont("font13");
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
  m_fontsetUnicode=false;
}

void GUIFontManager::LoadFonts(const CStdString& strFontSet)
{
  TiXmlDocument xmlDoc;
  if (!OpenFontFile(xmlDoc))
    return;

  // set scaling resolution so that we can scale our font sizes correctly
  // as fonts aren't scaled at render time (due to aliasing) we must scale
  // the size of the fonts before they are drawn to bitmaps
  g_graphicsContext.SetScalingResolution(m_skinResolution, 0, 0, true);
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  const TiXmlNode *pChild = pRootElement->FirstChild();

  // If there are no fontset's defined in the XML (old skin format) run in backward compatibility
  // and ignore the fontset request
  CStdString strValue = pChild->Value();
  if (strValue == "fontset")
  {
    CStdString foundTTF;
    while (pChild)
    {
      strValue = pChild->Value();
      if (strValue == "fontset")
      {
        const char* idAttr = ((TiXmlElement*) pChild)->Attribute("id");

        const char* unicodeAttr = ((TiXmlElement*) pChild)->Attribute("unicode");

        if (foundTTF.IsEmpty() && idAttr != NULL && unicodeAttr != NULL && stricmp(unicodeAttr, "true") == 0)
          foundTTF = idAttr;

        // Check if this is the fontset that we want
        if (idAttr != NULL && stricmp(strFontSet.c_str(), idAttr) == 0)
        {
          m_fontsetUnicode=false;
          // Check if this is the a ttf fontset
          if (unicodeAttr != NULL && stricmp(unicodeAttr, "true") == 0)
            m_fontsetUnicode=true;

          if (m_fontsetUnicode)
          {
            LoadFonts(pChild->FirstChild());
            break;
          }
        }

      }

      pChild = pChild->NextSibling();
    }

    // If no fontset was loaded
    if (pChild == NULL)
    {
      CLog::Log(LOGWARNING, "file doesnt have <fontset> with name '%s', defaulting to first fontset", strFontSet.c_str());
      if (!foundTTF.IsEmpty())
        LoadFonts(foundTTF);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "file doesnt have <fontset> in <fonts>, but rather %s", strValue.c_str());
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
        CGUIControlFactory::GetColor(fontNode, "shadow", shadowColor);
        CGUIControlFactory::GetColor(fontNode, "color", textColor);
        const TiXmlNode *pNode = fontNode->FirstChild("filename");
        if (pNode)
        {
          CStdString strFontFileName = pNode->FirstChild()->Value();
          if (strFontFileName.Find(".ttf") >= 0)
          {
            int iSize = 20;
            int iStyle = FONT_STYLE_NORMAL;
            float aspect = 1.0f;

            XMLUtils::GetInt(fontNode, "size", iSize);
            if (iSize <= 0) iSize = 20;

            pNode = fontNode->FirstChild("style");
            if (pNode)
            {
              CStdString style = pNode->FirstChild()->Value();
              iStyle = FONT_STYLE_NORMAL;
              if (style == "bold")
                iStyle = FONT_STYLE_BOLD;
              else if (style == "italics")
                iStyle = FONT_STYLE_ITALICS;
              else if (style == "bolditalics")
                iStyle = FONT_STYLE_BOLD_ITALICS;
            }

            XMLUtils::GetFloat(fontNode, "aspect", aspect);

            LoadTTF(strFontName, strFontFileName, textColor, shadowColor, iSize, iStyle, aspect);
          }
        }
      }
    }

    fontNode = fontNode->NextSibling();
  }
}

bool GUIFontManager::OpenFontFile(TiXmlDocument& xmlDoc)
{
  // Get the file to load fonts from:
  CStdString strPath = _P(g_SkinInfo.GetSkinPath("Font.xml", &m_skinResolution));
  CLog::Log(LOGINFO, "Loading fonts from %s", strPath.c_str());

  // first try our preferred file
  if ( !xmlDoc.LoadFile(strPath.c_str()) )
  {
    CLog::Log(LOGERROR, "Couldn't load %s", strPath.c_str());
    return false;
  }
  TiXmlElement* pRootElement = xmlDoc.RootElement();

  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("fonts"))
  {
    CLog::Log(LOGERROR, "file %s doesnt start with <fonts>", strPath.c_str());
    return false;
  }

  return true;
}

bool GUIFontManager::GetFirstFontSetUnicode(CStdString& strFontSet)
{
  strFontSet.Empty();

  // Load our font file
  TiXmlDocument xmlDoc;
  if (!OpenFontFile(xmlDoc))
    return false;

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  const TiXmlNode *pChild = pRootElement->FirstChild();

  CStdString strValue = pChild->Value();
  if (strValue == "fontset")
  {
    while (pChild)
    {
      strValue = pChild->Value();
      if (strValue == "fontset")
      {
        const char* idAttr = ((TiXmlElement*) pChild)->Attribute("id");

        const char* unicodeAttr = ((TiXmlElement*) pChild)->Attribute("unicode");

        // Check if this is a fontset with a ttf attribute set to true
        if (unicodeAttr != NULL && stricmp(unicodeAttr, "true") == 0)
        {
          //  This is the first ttf fontset
          strFontSet=idAttr;
          break;
        }

      }

      pChild = pChild->NextSibling();
    }

    // If no fontset was loaded
    if (pChild == NULL)
      CLog::Log(LOGWARNING, "file doesnt have <fontset> with with attibute unicode=\"true\"");
  }
  else
  {
    CLog::Log(LOGERROR, "file doesnt have <fontset> in <fonts>, but rather %s", strValue.c_str());
  }

  return !strFontSet.IsEmpty();
}

bool GUIFontManager::IsFontSetUnicode(const CStdString& strFontSet)
{
  TiXmlDocument xmlDoc;
  if (!OpenFontFile(xmlDoc))
    return false;

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  const TiXmlNode *pChild = pRootElement->FirstChild();

  CStdString strValue = pChild->Value();
  if (strValue == "fontset")
  {
    while (pChild)
    {
      strValue = pChild->Value();
      if (strValue == "fontset")
      {
        const char* idAttr = ((TiXmlElement*) pChild)->Attribute("id");

        const char* unicodeAttr = ((TiXmlElement*) pChild)->Attribute("unicode");

        // Check if this is the fontset that we want
        if (idAttr != NULL && stricmp(strFontSet.c_str(), idAttr) == 0)
          return (unicodeAttr != NULL && stricmp(unicodeAttr, "true") == 0);

      }

      pChild = pChild->NextSibling();
    }
  }

  return false;
}
