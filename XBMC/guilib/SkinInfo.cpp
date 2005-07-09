#include "stdafx.h"
#include "SkinInfo.h"
#include "../xbmc/Util.h"
#include "../xbmc/Settings.h"

#define SKIN_MIN_VERSION 1.3

CSkinInfo g_SkinInfo; // global

CSkinInfo::CSkinInfo()
{
  m_DefaultResolution = INVALID;
  m_DefaultResolutionWide = INVALID;
  m_strBaseDir = "";
  m_iNumCreditLines = 0;
}

CSkinInfo::~CSkinInfo()
{}

void CSkinInfo::Load(const CStdString& strSkinDir)
{
  m_strBaseDir = strSkinDir;
  m_DefaultResolution = INVALID;  // set to INVALID to denote that there is no default res here
  m_DefaultResolutionWide = INVALID;
  // Load from skin.xml
  TiXmlDocument xmlDoc;
  CStdString strFile = m_strBaseDir + "\\skin.xml";
  if (xmlDoc.LoadFile(strFile.c_str()))
  { // ok - get the default skin folder out of it...
    TiXmlElement* pRootElement = xmlDoc.RootElement();
    CStdString strValue = pRootElement->Value();
    if (strValue != "skin")
    {
      CLog::Log(LOGERROR, "file :%s doesnt contain <skin>", strFile.c_str());
    }
    else
    { // get the default resolution
      TiXmlNode *pChild = pRootElement->FirstChild("defaultresolution");
      if (pChild)
      { // found the defaultresolution tag
        CStdString strDefaultDir = pChild->FirstChild()->Value();
        strDefaultDir = strDefaultDir.ToLower();
        if (strDefaultDir == "pal") m_DefaultResolution = PAL_4x3;
        else if (strDefaultDir == "pal16x9") m_DefaultResolution = PAL_16x9;
        else if (strDefaultDir == "ntsc") m_DefaultResolution = NTSC_4x3;
        else if (strDefaultDir == "ntsc16x9") m_DefaultResolution = NTSC_16x9;
        else if (strDefaultDir == "720p") m_DefaultResolution = HDTV_720p;
        else if (strDefaultDir == "1080i") m_DefaultResolution = HDTV_1080i;
      }
      CLog::Log(LOGINFO, "Default 4:3 resolution directory is %s%s", m_strBaseDir.c_str(), GetDirFromRes(m_DefaultResolution).c_str());

      pChild = pRootElement->FirstChild("defaultresolutionwide");
      if (pChild)
      { // found the defaultresolution tag
        CStdString strDefaultDir = pChild->FirstChild()->Value();
        strDefaultDir = strDefaultDir.ToLower();
        if (strDefaultDir == "pal") m_DefaultResolutionWide = PAL_4x3;
        else if (strDefaultDir == "pal16x9") m_DefaultResolutionWide = PAL_16x9;
        else if (strDefaultDir == "ntsc") m_DefaultResolutionWide = NTSC_4x3;
        else if (strDefaultDir == "ntsc16x9") m_DefaultResolutionWide = NTSC_16x9;
        else if (strDefaultDir == "720p") m_DefaultResolutionWide = HDTV_720p;
        else if (strDefaultDir == "1080i") m_DefaultResolutionWide = HDTV_1080i;
      }
      else
        m_DefaultResolutionWide = m_DefaultResolution; // default to same as 4:3
      CLog::Log(LOGINFO, "Default 16:9 resolution directory is %s%s", m_strBaseDir.c_str(), GetDirFromRes(m_DefaultResolutionWide).c_str());

      // get the version
      pChild = pRootElement->FirstChild("version");
      if (pChild)
      {
        m_Version = atof(pChild->FirstChild()->Value());
        CLog::Log(LOGINFO, "Skin version is: %s", pChild->FirstChild()->Value());
      }

      // now load the credits information
      pChild = pRootElement->FirstChild("credits");
      if (pChild)
      { // ok, run through the credits
        TiXmlNode *pGrandChild = pChild->FirstChild("skinname");
        if (pGrandChild)
        {
          CStdString strName = pGrandChild->FirstChild()->Value();
          swprintf(credits[0], L"%S Skin", strName.Left(44).c_str());
        }
        pGrandChild = pChild->FirstChild("name");
        m_iNumCreditLines = 1;
        while (pGrandChild && m_iNumCreditLines < 6)
        {
          CStdString strName = pGrandChild->FirstChild()->Value();
          swprintf(credits[m_iNumCreditLines], L"%S", strName.Left(49).c_str());
          m_iNumCreditLines++;
          pGrandChild = pGrandChild->NextSibling("name");
        }
      }
    }
  }

}

bool CSkinInfo::Check(const CStdString& strSkinDir)
{
  bool bVersionOK = false;
  // Load from skin.xml
  TiXmlDocument xmlDoc;
  CStdString strFile = strSkinDir + "\\skin.xml";
  CStdString strGoodPath = strSkinDir;
  if (xmlDoc.LoadFile(strFile.c_str()))
  { // ok - get the default res folder out of it...
    TiXmlElement* pRootElement = xmlDoc.RootElement();
    CStdString strValue = pRootElement->Value();
    if (strValue == "skin")
    { // get the default resolution
      TiXmlNode *pChild = pRootElement->FirstChild("defaultresolution");
      if (pChild)
      { // found the defaultresolution tag
        strGoodPath += "\\";
        strGoodPath += pChild->FirstChild()->Value();
      }
      // get the version
      pChild = pRootElement->FirstChild("version");
      if (pChild)
      {
        bVersionOK = atof(pChild->FirstChild()->Value()) >= SKIN_MIN_VERSION;
        CLog::Log(LOGINFO, "Skin version is: %s", pChild->FirstChild()->Value());
      }
    }
  }
  // Check to see if we have a good path
  CStdString strFontXML = strGoodPath + "\\font.xml";
  CStdString strHomeXML = strGoodPath + "\\home.xml";
  CStdString strReferencesXML = strGoodPath + "\\references.xml";
  if ( CFile::Exists(strFontXML) &&
       CFile::Exists(strHomeXML) &&
       CFile::Exists(strReferencesXML) && bVersionOK )
  {
    return true;
  }
  return false;
}

CStdString CSkinInfo::GetSkinPath(const CStdString& strFile, RESOLUTION *res)
{
  // first try and load from the current resolution's directory
  *res = g_guiSettings.m_LookAndFeelResolution;
  CStdString strPath;
  strPath.Format("%s%s\\%s", m_strBaseDir.c_str(), GetDirFromRes(*res).c_str(), strFile.c_str());
  if (CFile::Exists(strPath))
    return strPath;
  // if we're in 1080i mode, try 720p next
  if (*res == HDTV_1080i)
  {
    *res = HDTV_720p;
    strPath.Format("%s%s\\%s", m_strBaseDir.c_str(), GetDirFromRes(*res).c_str(), strFile.c_str());
    if (CFile::Exists(strPath))
      return strPath;
  }
  // that failed - drop to the default widescreen resolution if where in a widemode
  if (*res == PAL_16x9 || *res == NTSC_16x9 || *res == HDTV_480p_16x9 || *res == HDTV_720p)
  {
    *res = m_DefaultResolutionWide;
    strPath.Format("%s%s\\%s", m_strBaseDir.c_str(), GetDirFromRes(*res).c_str(), strFile.c_str());
    if (CFile::Exists(strPath))
      return strPath;
  }
  // that failed - drop to the default resolution
  *res = m_DefaultResolution;
  strPath.Format("%s%s\\%s", m_strBaseDir.c_str(), GetDirFromRes(*res).c_str(), strFile.c_str());
  // check if we don't have any subdirectories
  if (*res == INVALID) *res = PAL_4x3;
  return strPath;
}

CStdString CSkinInfo::GetDirFromRes(RESOLUTION res)
{
  CStdString strRes;
  switch (res)
  {
  case INVALID:
    strRes = "";
    break;
  case PAL_4x3:
    strRes = "\\pal";
    break;
  case PAL_16x9:
    strRes = "\\pal16x9";
    break;
  case NTSC_4x3:
  case HDTV_480p_4x3:
    strRes = "\\ntsc";
    break;
  case NTSC_16x9:
  case HDTV_480p_16x9:
    strRes = "\\ntsc16x9";
    break;
  case HDTV_720p:
    strRes = "\\720p";
    break;
  case HDTV_1080i:
    strRes = "\\1080i";
    break;
  }
  return strRes;
}

wchar_t* CSkinInfo::GetCreditsLine(int i)
{
  if (i < m_iNumCreditLines)
    return credits[i];
  else
    return NULL;
}

double CSkinInfo::GetMinVersion()
{
  return SKIN_MIN_VERSION;
}
