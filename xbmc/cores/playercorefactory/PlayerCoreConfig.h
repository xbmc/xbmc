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

#include "system.h"

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
      m_config = static_cast<TiXmlElement*>(pConfig->Clone());
      const char *sAudio = pConfig->Attribute("audio");
      const char *sVideo = pConfig->Attribute("video");
      m_bPlaysAudio = sAudio && StringUtils::CompareNoCase(sAudio, "true") == 0;
      m_bPlaysVideo = sVideo && StringUtils::CompareNoCase(sVideo, "true") == 0;
    }
    else
    {
      m_config = nullptr;
    }
    CLog::Log(LOGDEBUG, "CPlayerCoreConfig::<ctor>: created player %s", m_name.c_str());
  }

  virtual ~CPlayerCoreConfig()
  {
    SAFE_DELETE(m_config);
  }

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

  IPlayer* CreatePlayer(IPlayerCallback& callback) const
  {
    IPlayer* pPlayer;
    if (m_type.compare("video") == 0)
    {
      pPlayer = new CVideoPlayer(callback);
    }
    else if (m_type.compare("music") == 0)
    {
      pPlayer = new PAPlayer(callback);
    }
    else if (m_type.compare("game") == 0)
    {
      pPlayer = new KODI::RETRO::CRetroPlayer(callback);
    }
    else if (m_type.compare("external") == 0)
    {
      pPlayer = new CExternalPlayer(callback);
    }

#if defined(HAS_UPNP)
    else if (m_type.compare("remote") == 0)
    {
      pPlayer = new UPNP::CUPnPPlayer(callback, m_id.c_str());
    }
#endif
    else
      return nullptr;

    pPlayer->m_name = m_name;
    pPlayer->m_type = m_type;

    if (pPlayer->Initialize(m_config))
    {
      return pPlayer;
    }
    else
    {
      SAFE_DELETE(pPlayer);
      return nullptr;
    }
  }

  std::string m_name;
  std::string m_id; // uuid for upnp
  std::string m_type;
  bool m_bPlaysAudio;
  bool m_bPlaysVideo;
  TiXmlElement* m_config;
};
