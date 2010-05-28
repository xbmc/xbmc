#pragma once

/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "tinyXML\tinyxml.h"
#include "log.h"
#include "FilterSelectionRule.h"
#include "DllLibCurl.h"
#include "RegExp.h"

class CGlobalFilterSelectionRule
{
public:
  CGlobalFilterSelectionRule(TiXmlElement* pRule)
  {
    Initialize(pRule);
  }

  ~CGlobalFilterSelectionRule()
  {
    delete m_pSource;
    delete m_pSplitter;
    delete m_pAudio;
    delete m_pVideo;
    delete m_pExtras;
    delete m_pAudioRenderer;

    CLog::Log(LOGDEBUG, "%s Ressources released", __FUNCTION__);
  }

  bool Match (const CFileItem& pFileItem)
  {
    CURL url(pFileItem.m_strPath);
    CRegExp regExp;

    if (m_fileTypes.empty())
      return false;

    if (CompileRegExp(m_fileTypes, regExp) && !MatchesRegExp(url.GetFileType(), regExp)) return false;
    if (CompileRegExp(m_fileName, regExp) && !MatchesRegExp(pFileItem.m_strPath, regExp)) return false;

    return true;
  }

  void GetSourceFilters(const CFileItem& item, std::vector<CStdString> &vecCores)
  {
    m_pSource->GetFilters(item, vecCores);
  }

  void GetSplitterFilters(const CFileItem& item, std::vector<CStdString> &vecCores)
  {
    m_pSplitter->GetFilters(item, vecCores);
  }

  void GetAudioRendererFilters(const CFileItem& item, std::vector<CStdString> &vecCores, SVideoStreamIndexes *pStreamIndexes = NULL)
  {
    m_pAudioRenderer->GetFilters(item, vecCores, false, pStreamIndexes);
  }

  void GetVideoFilters(const CFileItem& item, std::vector<CStdString> &vecCores, bool dxva = false)
  {
    m_pVideo->GetFilters(item, vecCores, dxva);
  }

  void GetAudioFilters(const CFileItem& item, std::vector<CStdString> &vecCores, bool dxva = false, SVideoStreamIndexes *pStreamIndexes = NULL)
  {
    m_pAudio->GetFilters(item, vecCores, dxva, pStreamIndexes);
  }

  void GetExtraFilters(const CFileItem& item, std::vector<CStdString> &vecCores, bool dxva = false)
  {
    m_pExtras->GetFilters(item, vecCores, dxva);
  }

private:
  CStdString m_name;
  CStdString m_fileName;
  CStdString m_fileTypes;
  CFilterSelectionRule * m_pSource;
  CFilterSelectionRule * m_pSplitter;
  CFilterSelectionRule * m_pVideo;
  CFilterSelectionRule * m_pAudio;
  CFilterSelectionRule * m_pExtras;
  CFilterSelectionRule * m_pAudioRenderer;

  int GetTristate(const char* szValue) const
  {
    if (szValue)
    {
      if (stricmp(szValue, "true") == 0) return 1;
      if (stricmp(szValue, "false") == 0) return 0;
    }
    return -1;
  }
  bool CompileRegExp(const CStdString& str, CRegExp& regExp) const
  {
    return str.length() > 0 && regExp.RegComp(str.c_str());
  }
  bool MatchesRegExp(const CStdString& str, CRegExp& regExp) const
  {
    return regExp.RegFind(str, 0) == 0;
  }

  void Initialize(TiXmlElement* pRule)
  {
    m_name = pRule->Attribute("name");
    if (!m_name || m_name.IsEmpty())
      m_name = "un-named";

    m_fileTypes = pRule->Attribute("filetypes");
    m_fileName = pRule->Attribute("filename");

    // Source rules
    m_pSource = new CFilterSelectionRule(pRule->FirstChildElement("source"), "source");

    // Splitter rules
    m_pSplitter = new CFilterSelectionRule(pRule->FirstChildElement("splitter"), "splitter");

    // Audio rules
    m_pAudio = new CFilterSelectionRule(pRule->FirstChildElement("audio"), "audio");

    // Video rules
    m_pVideo = new CFilterSelectionRule(pRule->FirstChildElement("video"), "video");

    // Extra rules
    m_pExtras = new CFilterSelectionRule(pRule->FirstChildElement("extra"), "extra");

    // Audio renderer rules
    m_pAudioRenderer = new CFilterSelectionRule(pRule->FirstChildElement("audiorenderer"), "audiorenderer");
  }
};