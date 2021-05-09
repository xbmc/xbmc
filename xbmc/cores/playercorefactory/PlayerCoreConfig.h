/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/ExternalPlayer/ExternalPlayer.h"
#include "cores/IPlayer.h"
#include "cores/RetroPlayer/RetroPlayer.h"
#include "cores/VideoPlayer/VideoPlayer.h"
#include "cores/paplayer/PAPlayer.h"
#ifdef HAS_UPNP
#include "network/upnp/UPnPPlayer.h"
#endif
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <utility>

class CPlayerCoreConfig
{
public:
  CPlayerCoreConfig(std::string name,
                    std::string type,
                    const TiXmlElement* pConfig,
                    const std::string& id = "")
    : m_name(std::move(name)), m_id(id), m_type(std::move(type))
  {
    m_bPlaysAudio = false;
    m_bPlaysVideo = false;

    if (pConfig)
    {
      m_config.reset(static_cast<TiXmlElement*>(pConfig->Clone()));
      const char *sAudio = pConfig->Attribute("audio");
      const char *sVideo = pConfig->Attribute("video");
      m_bPlaysAudio = sAudio && StringUtils::CompareNoCase(sAudio, "true") == 0;
      m_bPlaysVideo = sVideo && StringUtils::CompareNoCase(sVideo, "true") == 0;
    }

    CLog::Log(LOGDEBUG, "CPlayerCoreConfig::<ctor>: created player {}", m_name);
  }

  ~CPlayerCoreConfig() = default;

  const std::string& GetName() const
  {
    return m_name;
  }

  const std::string& GetId() const
  {
    return m_id;
  }

  bool PlaysAudio() const
  {
    return m_bPlaysAudio;
  }

  bool PlaysVideo() const
  {
    return m_bPlaysVideo;
  }

  std::shared_ptr<IPlayer> CreatePlayer(IPlayerCallback& callback) const
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

    if (player->Initialize(m_config.get()))
      return player;

    return nullptr;
  }

  std::string m_name;
  std::string m_id; // uuid for upnp
  std::string m_type;
  bool m_bPlaysAudio;
  bool m_bPlaysVideo;
  std::unique_ptr<TiXmlElement> m_config;
};
