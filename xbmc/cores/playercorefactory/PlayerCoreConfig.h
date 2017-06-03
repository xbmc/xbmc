#pragma once
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

#include "utils/XBMCTinyXML.h"
#include "cores/IPlayer.h"
#include "PlayerCoreFactory.h"
#include "cores/VideoPlayer/VideoPlayer.h"
#include "cores/paplayer/PAPlayer.h"
#include "cores/RetroPlayer/RetroPlayer.h"
#include "cores/ExternalPlayer/ExternalPlayer.h"
#ifdef HAS_UPNP
#include "network/upnp/UPnPPlayer.h"
#endif
#include "utils/log.h"

class CPlayerCoreConfig
{
public:

  CPlayerCoreConfig(std::string name, std::string type, const TiXmlElement* pConfig, const std::string& id = ""):
    m_name(name),
    m_id(id),
    m_type(type)
  {
    m_bPlaysAudio = false;
    m_bPlaysVideo = false;

    if (pConfig)
    {
      m_config = (TiXmlElement*)pConfig->Clone();
      const char *sAudio = pConfig->Attribute("audio");
      const char *sVideo = pConfig->Attribute("video");
      m_bPlaysAudio = sAudio && stricmp(sAudio, "true") == 0;
      m_bPlaysVideo = sVideo && stricmp(sVideo, "true") == 0;
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
      pPlayer = new GAME::CRetroPlayer(callback);
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
