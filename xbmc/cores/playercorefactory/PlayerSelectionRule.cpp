/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerSelectionRule.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "music/MusicFileItemClassify.h"
#include "network/NetworkFileItemClassify.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/RegExp.h"
#include "utils/StreamDetails.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <algorithm>

using namespace KODI;

CPlayerSelectionRule::CPlayerSelectionRule(TiXmlElement* pRule)
{
  Initialize(pRule);
}

void CPlayerSelectionRule::Initialize(TiXmlElement* pRule)
{
  m_name = XMLUtils::GetAttribute(pRule, "name");
  if (m_name.empty())
    m_name = "un-named";

  CLog::Log(LOGDEBUG, "CPlayerSelectionRule::Initialize: creating rule: {}", m_name);

  m_tInternetStream = GetTristate(pRule->Attribute("internetstream"));
  m_tRemote = GetTristate(pRule->Attribute("remote"));
  m_tAudio = GetTristate(pRule->Attribute("audio"));
  m_tVideo = GetTristate(pRule->Attribute("video"));
  m_tGame = GetTristate(pRule->Attribute("game"));

  m_tBD = GetTristate(pRule->Attribute("bd"));
  m_tDVD = GetTristate(pRule->Attribute("dvd"));
  m_tDVDFile = GetTristate(pRule->Attribute("dvdfile"));
  m_tDiscImage = GetTristate(pRule->Attribute("discimage"));
  if (m_tDiscImage < 0)
  {
    m_tDiscImage = GetTristate(pRule->Attribute("dvdimage"));
    if (m_tDiscImage >= 0)
      CLog::Log(LOGWARNING, "\"dvdimage\" tag is deprecated. use \"discimage\"");
  }

  m_protocols = XMLUtils::GetAttribute(pRule, "protocols");
  m_fileTypes = XMLUtils::GetAttribute(pRule, "filetypes");
  m_mimeTypes = XMLUtils::GetAttribute(pRule, "mimetypes");
  m_fileName = XMLUtils::GetAttribute(pRule, "filename");

  m_audioCodec = XMLUtils::GetAttribute(pRule, "audiocodec");
  m_audioChannels = XMLUtils::GetAttribute(pRule, "audiochannels");
  m_videoCodec = XMLUtils::GetAttribute(pRule, "videocodec");
  m_videoResolution = XMLUtils::GetAttribute(pRule, "videoresolution");
  m_videoAspect = XMLUtils::GetAttribute(pRule, "videoaspect");
  m_hdrType = XMLUtils::GetAttribute(pRule, "hdrtype");

  m_bStreamDetails = m_audioCodec.length() > 0 || m_audioChannels.length() > 0 ||
                     m_videoCodec.length() > 0 || m_videoResolution.length() > 0 ||
                     m_videoAspect.length() > 0 || m_hdrType.length() > 0;

  if (m_bStreamDetails && !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS))
  {
    CLog::Log(LOGWARNING,
              "CPlayerSelectionRule::Initialize: rule: {} needs media flagging, which is disabled",
              m_name);
  }

  m_playerName = XMLUtils::GetAttribute(pRule, "player");

  TiXmlElement* pSubRule = pRule->FirstChildElement("rule");
  while (pSubRule)
  {
    vecSubRules.emplace_back(std::make_unique<CPlayerSelectionRule>(pSubRule));
    pSubRule = pSubRule->NextSiblingElement("rule");
  }
}

int CPlayerSelectionRule::GetTristate(const char* szValue)
{
  if (szValue)
  {
    if (StringUtils::CompareNoCase(szValue, "true") == 0)
      return 1;
    if (StringUtils::CompareNoCase(szValue, "false") == 0)
      return 0;
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

void CPlayerSelectionRule::GetPlayers(const CFileItem& item, std::vector<std::string>&validPlayers, std::vector<std::string>&players)
{
  CLog::Log(LOGDEBUG, "CPlayerSelectionRule::GetPlayers: considering rule: {}", m_name);

  if (m_bStreamDetails && !item.HasVideoInfoTag())
    return;
  if (m_tAudio >= 0 && (m_tAudio > 0) != MUSIC::IsAudio(item))
    return;
  if (m_tVideo >= 0 && (m_tVideo > 0) != VIDEO::IsVideo(item))
    return;
  if (m_tGame >= 0 && (m_tGame > 0) != item.IsGame())
    return;
  if (m_tInternetStream >= 0 && (m_tInternetStream > 0) != NETWORK::IsInternetStream(item))
    return;
  if (m_tRemote >= 0 && (m_tRemote > 0) != NETWORK::IsRemote(item))
    return;

  if (m_tBD >= 0 && (m_tBD > 0) != (VIDEO::IsBDFile(item) && item.IsOnDVD()))
    return;
  if (m_tDVD >= 0 && (m_tDVD > 0) != item.IsDVD())
    return;
  if (m_tDVDFile >= 0 && (m_tDVDFile > 0) != VIDEO::IsDVDFile(item))
    return;
  if (m_tDiscImage >= 0 && (m_tDiscImage > 0) != item.IsDiscImage())
    return;

  CRegExp regExp(false, CRegExp::autoUtf8);

  if (m_bStreamDetails)
  {
    if (!item.GetVideoInfoTag()->HasStreamDetails())
    {
      CLog::Log(LOGDEBUG,
                "CPlayerSelectionRule::GetPlayers: cannot check rule: {}, no StreamDetails",
                m_name);
      return;
    }

    CStreamDetails streamDetails = item.GetVideoInfoTag()->m_streamDetails;

    if (CompileRegExp(m_audioCodec, regExp) && !MatchesRegExp(streamDetails.GetAudioCodec(), regExp))
      return;

    std::stringstream itoa;
    itoa << streamDetails.GetAudioChannels();
    std::string audioChannelsstr = itoa.str();

    if (CompileRegExp(m_audioChannels, regExp) && !MatchesRegExp(audioChannelsstr, regExp))
      return;

    if (CompileRegExp(m_videoCodec, regExp) && !MatchesRegExp(streamDetails.GetVideoCodec(), regExp))
      return;

    if (CompileRegExp(m_videoResolution, regExp) &&
        !MatchesRegExp(CStreamDetails::VideoDimsToResolutionDescription(streamDetails.GetVideoWidth(), streamDetails.GetVideoHeight()), regExp))
      return;

    if (CompileRegExp(m_videoAspect, regExp) &&
        !MatchesRegExp(CStreamDetails::VideoAspectToAspectDescription(streamDetails.GetVideoAspect()),  regExp))
      return;

    std::string hdrType{streamDetails.GetVideoHdrType()};
    if (hdrType.length() == 0)
      hdrType = "none";

    if (CompileRegExp(m_hdrType, regExp) && !MatchesRegExp(hdrType, regExp))
      return;
  }

  CURL url(item.GetDynPath());

  if (CompileRegExp(m_fileTypes, regExp) && !MatchesRegExp(url.GetFileType(), regExp))
    return;

  if (CompileRegExp(m_protocols, regExp) && !MatchesRegExp(url.GetProtocol(), regExp))
    return;

  if (CompileRegExp(m_mimeTypes, regExp) && !MatchesRegExp(item.GetMimeType(), regExp))
    return;

  if (CompileRegExp(m_fileName, regExp) && !MatchesRegExp(item.GetDynPath(), regExp))
    return;

  CLog::Log(LOGDEBUG, "CPlayerSelectionRule::GetPlayers: matches rule: {}", m_name);

  for (const auto& rule : vecSubRules)
    rule->GetPlayers(item, validPlayers, players);

  if (std::find(validPlayers.begin(), validPlayers.end(), m_playerName) != validPlayers.end())
  {
    CLog::Log(LOGDEBUG, "CPlayerSelectionRule::GetPlayers: adding player: {} for rule: {}",
              m_playerName, m_name);
    players.push_back(m_playerName);
  }
}
