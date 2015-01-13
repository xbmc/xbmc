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
#include "cores/dvdplayer/DVDPlayer.h"
#include "cores/paplayer/PAPlayer.h"
#include "cores/ExternalPlayer/ExternalPlayer.h"
#ifdef HAS_UPNP
#include "network/upnp/UPnPPlayer.h"
#endif
#include "utils/log.h"

class CPlayerCoreConfig
{
friend class CPlayerCoreFactory;

public:
  CPlayerCoreConfig(std::string name, const EPLAYERCORES eCore, const TiXmlElement* pConfig, const std::string& id = ""):
    m_name(name),
    m_id(id)
  {
    m_eCore = eCore;
    m_bPlaysAudio = false;
    m_bPlaysVideo = false;

    if (pConfig)
    {
      m_config = (TiXmlElement*)pConfig->Clone();
      const char *szAudio = pConfig->Attribute("audio");
      const char *szVideo = pConfig->Attribute("video");
      m_bPlaysAudio = szAudio && stricmp(szAudio, "true") == 0;
      m_bPlaysVideo = szVideo && stricmp(szVideo, "true") == 0;
    }
    else
    {
      m_config = NULL;
    }
    CLog::Log(LOGDEBUG, "CPlayerCoreConfig::<ctor>: created player %s for core %d", m_name.c_str(), m_eCore);
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

  const EPLAYERCORES& GetType() const
  {
    return m_eCore;
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
    switch(m_eCore)
    {
      case EPC_MPLAYER:
      case EPC_DVDPLAYER: pPlayer = new CDVDPlayer(callback); break;
      case EPC_PAPLAYER: pPlayer = new PAPlayer(callback); break;
      case EPC_EXTPLAYER: pPlayer = new CExternalPlayer(callback); break;
#if defined(HAS_UPNP)
      case EPC_UPNPPLAYER: pPlayer = new UPNP::CUPnPPlayer(callback, m_id.c_str()); break;
#endif
      default: return NULL;
    }

    if (pPlayer->Initialize(m_config))
    {
      return pPlayer;
    }
    else
    {
      SAFE_DELETE(pPlayer);
      return NULL;
    }
  }

private:
  std::string m_name;
  std::string m_id;
  bool m_bPlaysAudio;
  bool m_bPlaysVideo;
  EPLAYERCORES m_eCore;
  TiXmlElement* m_config;
};
