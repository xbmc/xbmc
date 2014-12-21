/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "URL.h"
#include "PlayerSelectionRule.h"
#include "video/VideoInfoTag.h"
#include "utils/StreamDetails.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/RegExp.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

CPlayerSelectionRule::CPlayerSelectionRule(TiXmlElement* pRule)
{
  Initialize(pRule);
}

CPlayerSelectionRule::~CPlayerSelectionRule()
{
  for (unsigned int i = 0; i < vecSubRules.size(); i++)
  {
    delete vecSubRules[i];
  }
  vecSubRules.clear();
}

void CPlayerSelectionRule::Initialize(TiXmlElement* pRule)
{
  m_name = XMLUtils::GetAttribute(pRule, "name");
  if (m_name.empty())
    m_name = "un-named";

  CLog::Log(LOGDEBUG, "CPlayerSelectionRule::Initialize: creating rule: %s", m_name.c_str());

  m_tInternetStream = GetTristate(pRule->Attribute("internetstream"));
  m_tRemote = GetTristate(pRule->Attribute("remote"));
  m_tAudio = GetTristate(pRule->Attribute("audio"));
  m_tVideo = GetTristate(pRule->Attribute("video"));

  m_tBD = GetTristate(pRule->Attribute("bd"));
  m_tDVD = GetTristate(pRule->Attribute("dvd"));
  m_tDVDFile = GetTristate(pRule->Attribute("dvdfile"));
  m_tDVDImage = GetTristate(pRule->Attribute("dvdimage"));

  m_protocols = XMLUtils::GetAttribute(pRule, "protocols");
  m_fileTypes = XMLUtils::GetAttribute(pRule, "filetypes");
  m_mimeTypes = XMLUtils::GetAttribute(pRule, "mimetypes");
  m_fileName = XMLUtils::GetAttribute(pRule, "filename");

  m_audioCodec = XMLUtils::GetAttribute(pRule, "audiocodec");
  m_audioChannels = XMLUtils::GetAttribute(pRule, "audiochannels");
  m_videoCodec = XMLUtils::GetAttribute(pRule, "videocodec");
  m_videoResolution = XMLUtils::GetAttribute(pRule, "videoresolution");
  m_videoAspect = XMLUtils::GetAttribute(pRule, "videoaspect");

  m_bStreamDetails = m_audioCodec.length() > 0 || m_audioChannels.length() > 0 ||
    m_videoCodec.length() > 0 || m_videoResolution.length() > 0 || m_videoAspect.length() > 0;

  if (m_bStreamDetails && !CSettings::Get().GetBool("myvideos.extractflags"))
  {
      CLog::Log(LOGWARNING, "CPlayerSelectionRule::Initialize: rule: %s needs media flagging, which is disabled", m_name.c_str());
  }

  m_playerName = XMLUtils::GetAttribute(pRule, "player");
  m_playerCoreId = 0;

  TiXmlElement* pSubRule = pRule->FirstChildElement("rule");
  while (pSubRule)
  {
    vecSubRules.push_back(new CPlayerSelectionRule(pSubRule));
    pSubRule = pSubRule->NextSiblingElement("rule");
  }
}

int CPlayerSelectionRule::GetTristate(const char* szValue)
{
  if (szValue)
  {
    if (stricmp(szValue, "true") == 0) return 1;
    if (stricmp(szValue, "false") == 0) return 0;
  }
  return -1;
}

bool CPlayerSelectionRule::CompileRegExp(const std::string& str, CRegExp& regExp)
{
  return !str.empty() && regExp.RegComp(str.c_str());
}

bool CPlayerSelectionRule::MatchesRegExp(const std::string& str, CRegExp& regExp)
{
  return regExp.RegFind(str, 0) == 0;
}

void CPlayerSelectionRule::GetPlayers(const CFileItem& item, VECPLAYERCORES &vecCores)
{
  CLog::Log(LOGDEBUG, "CPlayerSelectionRule::GetPlayers: considering rule: %s", m_name.c_str());

  if (m_bStreamDetails && !item.HasVideoInfoTag()) return;
  if (m_tAudio >= 0 && (m_tAudio > 0) != item.IsAudio()) return;
  if (m_tVideo >= 0 && (m_tVideo > 0) != item.IsVideo()) return;
  if (m_tInternetStream >= 0 && (m_tInternetStream > 0) != item.IsInternetStream()) return;
  if (m_tRemote >= 0 && (m_tRemote > 0) != item.IsRemote()) return;

  if (m_tBD >= 0 && (m_tBD > 0) != (item.IsBDFile() && item.IsOnDVD())) return;
  if (m_tDVD >= 0 && (m_tDVD > 0) != item.IsDVD()) return;
  if (m_tDVDFile >= 0 && (m_tDVDFile > 0) != item.IsDVDFile()) return;
  if (m_tDVDImage >= 0 && (m_tDVDImage > 0) != item.IsDiscImage()) return;

  CRegExp regExp(false, CRegExp::autoUtf8);

  if (m_bStreamDetails)
  {
    if (!item.GetVideoInfoTag()->HasStreamDetails())
    {
      CLog::Log(LOGDEBUG, "CPlayerSelectionRule::GetPlayers: cannot check rule: %s, no StreamDetails", m_name.c_str());
      return;
    }

    CStreamDetails streamDetails = item.GetVideoInfoTag()->m_streamDetails;

    if (CompileRegExp(m_audioCodec, regExp) && !MatchesRegExp(streamDetails.GetAudioCodec(), regExp)) return;
    
    std::stringstream itoa;
    itoa << streamDetails.GetAudioChannels();
    std::string audioChannelsstr = itoa.str();

    if (CompileRegExp(m_audioChannels, regExp) && !MatchesRegExp(audioChannelsstr, regExp)) return;

    if (CompileRegExp(m_videoCodec, regExp) && !MatchesRegExp(streamDetails.GetVideoCodec(), regExp)) return;

    if (CompileRegExp(m_videoResolution, regExp) &&
        !MatchesRegExp(CStreamDetails::VideoDimsToResolutionDescription(streamDetails.GetVideoWidth(), streamDetails.GetVideoHeight()), regExp)) return;

    if (CompileRegExp(m_videoAspect, regExp) &&
        !MatchesRegExp(CStreamDetails::VideoAspectToAspectDescription(streamDetails.GetVideoAspect()),  regExp)) return;
  }

  CURL url(item.GetPath());

  if (CompileRegExp(m_fileTypes, regExp) && !MatchesRegExp(url.GetFileType(), regExp)) return;

  if (CompileRegExp(m_protocols, regExp) && !MatchesRegExp(url.GetProtocol(), regExp)) return;

  if (CompileRegExp(m_mimeTypes, regExp) && !MatchesRegExp(item.GetMimeType(), regExp)) return;

  if (CompileRegExp(m_fileName, regExp) && !MatchesRegExp(item.GetPath(), regExp)) return;

  CLog::Log(LOGDEBUG, "CPlayerSelectionRule::GetPlayers: matches rule: %s", m_name.c_str());

  for (unsigned int i = 0; i < vecSubRules.size(); i++)
    vecSubRules[i]->GetPlayers(item, vecCores);

  PLAYERCOREID playerCoreId = GetPlayerCore();
  if (playerCoreId != EPC_NONE)
  {
    CLog::Log(LOGDEBUG, "CPlayerSelectionRule::GetPlayers: adding player: %s (%d) for rule: %s", m_playerName.c_str(), playerCoreId, m_name.c_str());
    vecCores.push_back(GetPlayerCore());
  }
}

PLAYERCOREID CPlayerSelectionRule::GetPlayerCore()
{
  if (!m_playerCoreId)
  {
    m_playerCoreId = CPlayerCoreFactory::Get().GetPlayerCore(m_playerName);
  }
  return m_playerCoreId;
}

