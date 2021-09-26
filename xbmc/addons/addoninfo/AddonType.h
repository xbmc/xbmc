/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/addoninfo/AddonExtensions.h"

#include <set>
#include <string>

class TiXmlElement;

namespace ADDON
{

typedef enum
{
  ADDON_UNKNOWN,
  ADDON_VIZ,
  ADDON_SKIN,
  ADDON_PVRDLL,
  ADDON_INPUTSTREAM,
  ADDON_GAMEDLL,
  ADDON_PERIPHERALDLL,
  ADDON_SCRIPT,
  ADDON_SCRIPT_WEATHER,
  ADDON_SUBTITLE_MODULE,
  ADDON_SCRIPT_LYRICS,
  ADDON_SCRAPER_ALBUMS,
  ADDON_SCRAPER_ARTISTS,
  ADDON_SCRAPER_MOVIES,
  ADDON_SCRAPER_MUSICVIDEOS,
  ADDON_SCRAPER_TVSHOWS,
  ADDON_SCREENSAVER,
  ADDON_PLUGIN,
  ADDON_REPOSITORY,
  ADDON_WEB_INTERFACE,
  ADDON_SERVICE,
  ADDON_AUDIOENCODER,
  ADDON_CONTEXT_ITEM,
  ADDON_AUDIODECODER,
  ADDON_RESOURCE_IMAGES,
  ADDON_RESOURCE_LANGUAGE,
  ADDON_RESOURCE_UISOUNDS,
  ADDON_RESOURCE_GAMES,
  ADDON_RESOURCE_FONT,
  ADDON_VFS,
  ADDON_IMAGEDECODER,
  ADDON_SCRAPER_LIBRARY,
  ADDON_SCRIPT_LIBRARY,
  ADDON_SCRIPT_MODULE,
  ADDON_GAME_CONTROLLER,
  ADDON_VIDEOCODEC,

  /**
    * @brief virtual addon types
    */
  //@{
  ADDON_VIDEO,
  ADDON_AUDIO,
  ADDON_IMAGE,
  ADDON_EXECUTABLE,
  ADDON_GAME,
  //@}

  ADDON_MAX
} TYPE;

class CAddonInfoBuilder;
class CAddonDatabaseSerializer;

class CAddonType : public CAddonExtensions
{
public:
  CAddonType(TYPE type = ADDON_UNKNOWN) : m_type(type) {}

  TYPE Type() const { return m_type; }
  std::string LibPath() const;
  const std::string& LibName() const { return m_libname; }

  bool ProvidesSubContent(const TYPE& content) const
  {
    return content == ADDON_UNKNOWN ? false : m_type == content || m_providedSubContent.count(content) > 0;
  }

  bool ProvidesSeveralSubContents() const
  {
    return m_providedSubContent.size() > 1;
  }

  size_t ProvidedSubContents() const
  {
    return m_providedSubContent.size();
  }

  /*!
   * @brief Indicates whether a given type is a dependency type (e.g. addons which the main type is
   * a script.module)
   *
   * @param[in] type the provided type
   * @return true if type is one of the dependency types
   */
  static bool IsDependencyType(TYPE type);

private:
  friend class CAddonInfoBuilder;
  friend class CAddonDatabaseSerializer;

  void SetProvides(const std::string& content);

  TYPE m_type;
  std::string m_path;
  std::string m_libname;
  std::set<TYPE> m_providedSubContent;
};

} /* namespace ADDON */
