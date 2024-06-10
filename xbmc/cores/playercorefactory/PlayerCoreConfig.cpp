/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerCoreConfig.h"

#include "cores/ExternalPlayer/ExternalPlayer.h"
#include "cores/IPlayer.h"
#include "cores/RetroPlayer/RetroPlayer.h"
#include "cores/VideoPlayer/VideoPlayer.h"
#include "cores/paplayer/PAPlayer.h"
#ifdef HAS_UPNP
#include "network/upnp/UPnPPlayer.h"
#endif
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <utility>

CPlayerCoreConfig::CPlayerCoreConfig(std::string name,
                                     std::string type,
                                     const tinyxml2::XMLElement* pConfig,
                                     const std::string& id /* = "" */)
  : m_name(std::move(name)), m_id(id), m_type(std::move(type))
{
  if (pConfig)
  {
    m_config.reset();
    m_config = std::make_unique<CXBMCTinyXML2>();

    pConfig->DeepClone(m_config->GetDocument());

    const char* sAudio = pConfig->Attribute("audio");
    const char* sVideo = pConfig->Attribute("video");
    m_bPlaysAudio = sAudio && StringUtils::CompareNoCase(sAudio, "true") == 0;
    m_bPlaysVideo = sVideo && StringUtils::CompareNoCase(sVideo, "true") == 0;
  }

  CLog::Log(LOGDEBUG, "CPlayerCoreConfig::<ctor>: created player {}", m_name);
}

std::shared_ptr<IPlayer> CPlayerCoreConfig::CreatePlayer(IPlayerCallback& callback) const
{
  std::shared_ptr<IPlayer> player;

  if (m_type.compare("video") == 0)
  {
    player = std::make_shared<CVideoPlayer>(callback);
  }
  else if (m_type.compare("music") == 0)
  {
    player = std::make_shared<PAPlayer>(callback);
  }
  else if (m_type.compare("game") == 0)
  {
    player = std::make_shared<KODI::RETRO::CRetroPlayer>(callback);
  }
  else if (m_type.compare("external") == 0)
  {
    player = std::make_shared<CExternalPlayer>(callback);
  }

#if defined(HAS_UPNP)
  else if (m_type.compare("remote") == 0)
  {
    player = std::make_shared<UPNP::CUPnPPlayer>(callback, m_id.c_str());
  }
#endif
  else
    return nullptr;

  if (!player)
    return nullptr;

  player->m_name = m_name;
  player->m_type = m_type;

  if (player->Initialize(m_config->RootElement()))
    return player;

  return nullptr;
}
