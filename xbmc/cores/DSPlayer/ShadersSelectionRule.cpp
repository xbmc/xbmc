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

#ifdef HAS_DS_PLAYER

#include "URL.h"
#include "ShadersSelectionRule.h"
#include "video/VideoInfoTag.h"
#include "utils/StreamDetails.h"
#include "settings/GUISettings.h"
#include "utils/log.h"
#include "utils/XMLUtils.h"

CShadersSelectionRule::CShadersSelectionRule(TiXmlElement* pRule)
  : m_shaderId(-1)
{
  Initialize(pRule);
}

CShadersSelectionRule::~CShadersSelectionRule()
{}

void CShadersSelectionRule::Initialize(TiXmlElement* pRule)
{
  if (! pRule)
    return;

  m_name = pRule->Attribute("name");
  if (!m_name || m_name.IsEmpty())
    m_name = "un-named";

  m_dxva = GetTristate(pRule->Attribute("dxva"));

  m_mimeTypes = pRule->Attribute("mimetypes");
  m_fileName = pRule->Attribute("filename");

  m_audioCodec = pRule->Attribute("audiocodec");
  m_audioChannels = pRule->Attribute("audiochannels");
  m_videoCodec = pRule->Attribute("videocodec");
  m_videoResolution = pRule->Attribute("videoresolution");
  m_videoAspect = pRule->Attribute("videoaspect");
  m_videoFourcc = pRule->Attribute("fourcc");

  m_bStreamDetails = m_audioCodec.length() > 0 || m_audioChannels.length() > 0 || m_videoFourcc.length() > 0 ||
    m_videoCodec.length() > 0 || m_videoResolution.length() > 0 || m_videoAspect.length() > 0;

  if (m_bStreamDetails && !g_guiSettings.GetBool("myvideos.extractflags"))
  {
      CLog::Log(LOGWARNING, "CFilterSelectionRule::Initialize: rule: %s needs media flagging, which is disabled", m_name.c_str());
  }

  if (pRule->QueryIntAttribute("id", (int *) &m_shaderId) != TIXML_SUCCESS)
    m_shaderId = -1;

  TiXmlElement* pSubRule = pRule->FirstChildElement("shader");
  while (pSubRule) 
  {
    vecSubRules.push_back(new CShadersSelectionRule(pSubRule));
    pSubRule = pSubRule->NextSiblingElement("shader");
  }
}

int CShadersSelectionRule::GetTristate(const char* szValue) const
{
  if (szValue)
  {
    if (stricmp(szValue, "true") == 0) return 1;
    if (stricmp(szValue, "false") == 0) return 0;
  }
  return -1;
}

bool CShadersSelectionRule::CompileRegExp(const CStdString& str, CRegExp& regExp) const
{
  return str.length() > 0 && regExp.RegComp(str.c_str());
}

bool CShadersSelectionRule::MatchesRegExp(const CStdString& str, CRegExp& regExp) const
{
  return regExp.RegFind(str, 0) != -1; // Need more testing
}

void CShadersSelectionRule::GetShaders(const CFileItem& item, std::vector<uint32_t> &shaders, bool dxva)
{
  //CLog::Log(LOGDEBUG, "CFilterSelectionRule::GetFilters: considering rule: %s", m_name.c_str());

  if (m_bStreamDetails && (!item.HasVideoInfoTag())) return;
  /*
  if (m_tAudio >= 0 && (m_tAudio > 0) != item.IsAudio()) return;
  if (m_tVideo >= 0 && (m_tVideo > 0) != item.IsVideo()) return;
  if (m_tInternetStream >= 0 && (m_tInternetStream > 0) != item.IsInternetStream()) return;

  if (m_tDVD >= 0 && (m_tDVD > 0) != item.IsDVD()) return;
  if (m_tDVDFile >= 0 && (m_tDVDFile > 0) != item.IsDVDFile()) return;
  if (m_tDVDImage >= 0 && (m_tDVDImage > 0) != item.IsDVDImage()) return;*/

  if (m_dxva >= 0 && (m_dxva > 0) != dxva) return;

  CRegExp regExp;

  if (m_bStreamDetails)
  {
    if (!item.GetVideoInfoTag()->HasStreamDetails())
    {
      CLog::Log(LOGDEBUG, "CFilterSelectionRule::GetFilters: cannot check rule: %s, no StreamDetails", m_name.c_str());
      return;
    }

    CStreamDetails streamDetails = item.GetVideoInfoTag()->m_streamDetails;

    if (CompileRegExp(m_audioCodec, regExp) && !MatchesRegExp(streamDetails.GetAudioCodec(), regExp)) return;

    if (CompileRegExp(m_videoCodec, regExp) && !MatchesRegExp(streamDetails.GetVideoCodec(), regExp)) return;

    if (CompileRegExp(m_videoFourcc, regExp) && !MatchesRegExp(streamDetails.GetVideoFourcc(), regExp)) return;

    if (CompileRegExp(m_videoResolution, regExp) &&
        !MatchesRegExp(CStreamDetails::VideoDimsToResolutionDescription(streamDetails.GetVideoWidth(),
        streamDetails.GetVideoHeight()), regExp)) return;

    if (CompileRegExp(m_videoAspect, regExp) && !MatchesRegExp(CStreamDetails::VideoAspectToAspectDescription(
      item.GetVideoInfoTag()->m_streamDetails.GetVideoAspect()), regExp)) return;
  }

  CURL url(item.GetPath());

  //if (CompileRegExp(m_fileTypes, regExp) && !MatchesRegExp(url.GetFileType(), regExp)) return;
  
  //if (CompileRegExp(m_protocols, regExp) && !MatchesRegExp(url.GetProtocol(), regExp)) return;
  
  if (CompileRegExp(m_mimeTypes, regExp) && !MatchesRegExp(item.GetMimeType(), regExp)) return;

  if (CompileRegExp(m_fileName, regExp) && !MatchesRegExp(item.GetPath(), regExp)) return;

  //CLog::Log(LOGDEBUG, "CFilterSelectionRule::GetFilters: matches rule: %s", m_name.c_str());

  if (m_shaderId > -1)
  {
    CLog::Log(LOGDEBUG, "CFilterSelectionRule::GetFilters: adding shader: %d for rule: %s", m_shaderId, m_name.c_str());
    shaders.push_back(m_shaderId);
  }

  for (unsigned int i = 0; i < vecSubRules.size(); i++)
    vecSubRules[i]->GetShaders(item, shaders, dxva);
}

#endif