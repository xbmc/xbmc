/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

class IPlayer;
class IPlayerCallback;
class CXBMCTinyXML2;

namespace tinyxml2
{
class XMLElement;
}

class CPlayerCoreConfig
{
public:
  CPlayerCoreConfig(std::string name,
                    std::string type,
                    const tinyxml2::XMLElement* pConfig,
                    const std::string& id = "");

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

  std::shared_ptr<IPlayer> CreatePlayer(IPlayerCallback& callback) const;

  std::string m_name;
  std::string m_id; // uuid for upnp
  std::string m_type;
  bool m_bPlaysAudio{false};
  bool m_bPlaysVideo{false};
  std::unique_ptr<CXBMCTinyXML2> m_config;
};
