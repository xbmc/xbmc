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
#include "utils/log.h"
#include "FilterSelectionRule.h"
#include "ShadersSelectionRule.h"
#include "filesystem/DllLibCurl.h"
#include "utils/RegExp.h"
#include "URL.h"
#include "video/VideoInfoTag.h"
#include "utils/StreamDetails.h"

class CGlobalFilterSelectionRule
{
public:
  CGlobalFilterSelectionRule(TiXmlElement* pRule)
    : m_url(false)
  {
    Initialize(pRule);
  }

  ~CGlobalFilterSelectionRule()
  {
    delete m_pSource;
    delete m_pSplitter;
    delete m_pAudio;
    delete m_pVideo;
    delete m_pSubs;
    delete m_pExtras;
    delete m_pAudioRenderer;
    delete m_pShaders;

    //CLog::Log(LOGDEBUG, "%s Ressources released", __FUNCTION__); Log Spam
  }

  bool Match(const CFileItem& pFileItem, bool checkUrl = false)
  {
    CURL url(pFileItem.GetPath());
    CRegExp regExp;

    if (m_fileTypes.empty() && m_fileName.empty() && m_Protocols.empty())
      return false;

    if (m_bStreamDetails && (!pFileItem.HasVideoInfoTag()))
      return false;

    if (m_bStreamDetails)
    {
      if (!pFileItem.GetVideoInfoTag()->HasStreamDetails())
      {
        CLog::Log(LOGDEBUG, "%s: %s, no StreamDetails", __FUNCTION__, m_name.c_str());
        return false;
      }
      CStreamDetails streamDetails = pFileItem.GetVideoInfoTag()->m_streamDetails;

      if (CompileRegExp(m_videoCodec, regExp) && !MatchesRegExp(streamDetails.GetVideoCodec(), regExp)) return false;
    }

    if (!checkUrl && m_url > 0) return false;
    if (checkUrl && pFileItem.IsInternetStream() && m_url < 1) return false;
    if (CompileRegExp(m_fileTypes, regExp) && !MatchesRegExp(url.GetFileType(), regExp)) return false;
    if (CompileRegExp(m_fileName, regExp) && !MatchesRegExp(pFileItem.GetPath(), regExp)) return false;
    if (CompileRegExp(m_Protocols, regExp) && !MatchesRegExp(url.GetProtocol(), regExp)) return false;

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

  void GetAudioRendererFilters(const CFileItem& item, std::vector<CStdString> &vecCores)
  {
    m_pAudioRenderer->GetFilters(item, vecCores, false);
  }

  void GetVideoFilters(const CFileItem& item, std::vector<CStdString> &vecCores, bool dxva = false)
  {
    m_pVideo->GetFilters(item, vecCores, dxva);
  }

  void GetAudioFilters(const CFileItem& item, std::vector<CStdString> &vecCores, bool dxva = false)
  {
    m_pAudio->GetFilters(item, vecCores, dxva);
  }

  void GetSubsFilters(const CFileItem& item, std::vector<CStdString> &vecCores, bool dxva = false)
  {
    m_pSubs->GetFilters(item, vecCores, dxva);
  }

  void GetExtraFilters(const CFileItem& item, std::vector<CStdString> &vecCores, bool dxva = false)
  {
    m_pExtras->GetFilters(item, vecCores, dxva);
  }

  void GetShaders(const CFileItem& item, std::vector<uint32_t>& shaders, std::vector<uint32_t>& shadersStages, bool dxva = false)
  {
    m_pShaders->GetShaders(item, shaders, shadersStages, dxva);
  }

private:
  int        m_url;
  bool       m_bStreamDetails;
  CStdString m_name;
  CStdString m_fileName;
  CStdString m_fileTypes;
  CStdString m_Protocols;
  CStdString m_videoCodec;
  CFilterSelectionRule * m_pSource;
  CFilterSelectionRule * m_pSplitter;
  CFilterSelectionRule * m_pVideo;
  CFilterSelectionRule * m_pAudio;
  CFilterSelectionRule * m_pSubs;
  CFilterSelectionRule * m_pExtras;
  CFilterSelectionRule * m_pAudioRenderer;
  CShadersSelectionRule * m_pShaders;

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

    m_url = GetTristate(pRule->Attribute("url"));
    m_fileTypes = pRule->Attribute("filetypes");
    m_fileName = pRule->Attribute("filename");
    m_Protocols = pRule->Attribute("protocols");
    m_videoCodec = pRule->Attribute("videocodec");
    m_bStreamDetails = m_videoCodec.length() > 0;

    // Source rules
    m_pSource = new CFilterSelectionRule(pRule->FirstChildElement("source"), "source");

    // Splitter rules
    m_pSplitter = new CFilterSelectionRule(pRule->FirstChildElement("splitter"), "splitter");

    // Audio rules
    m_pAudio = new CFilterSelectionRule(pRule->FirstChildElement("audio"), "audio");

    // Video rules
    m_pVideo = new CFilterSelectionRule(pRule->FirstChildElement("video"), "video");

    // Subs rules
    m_pSubs = new CFilterSelectionRule(pRule->FirstChildElement("subs"), "subs");

    // Extra rules
    m_pExtras = new CFilterSelectionRule(pRule->FirstChildElement("extra"), "extra");

    // Audio renderer rules
    m_pAudioRenderer = new CFilterSelectionRule(pRule->FirstChildElement("audiorenderer"), "audiorenderer");

    // Shaders
    m_pShaders = new CShadersSelectionRule(pRule->FirstChildElement("shaders"));
  }
};