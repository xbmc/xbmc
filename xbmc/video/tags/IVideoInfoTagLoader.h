/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "InfoScanner.h"
#include "addons/Scraper.h"

#include <string>
#include <utility>

class CFileItem;
class CVideoInfoTag;
class EmbeddedArt;

namespace KODI::VIDEO
{

//! \brief Base class for video tag loaders.
class IVideoInfoTagLoader
{
public:
  //! \brief Constructor
  //! \param item The item to load info for
  //! \param info Scraper info
  //! \param llokInFolder True to look in folder holding file
  IVideoInfoTagLoader(const CFileItem& item, ADDON::ScraperPtr info, bool lookInFolder)
    : m_item(item), m_info(std::move(info))
  {
  }
  virtual ~IVideoInfoTagLoader() = default;

  //! \brief Returns true if we have info to provide.
  virtual bool HasInfo() const = 0;

  //! \brief Load tag from file.
  //! \brief tag Tag to load info into
  //! \brief prioritise True to prioritise data over existing data in tag
  //! \returns True if tag was read, false otherwise
  virtual CInfoScanner::InfoType Load(CVideoInfoTag& tag,
                                      bool prioritise,
                                      std::vector<EmbeddedArt>* art = nullptr) = 0;

  //! \brief Load an additional version of the item, if the source holds one.
  //! \param index 1-based index of the version to load (2 is the first additional version)
  //! \param tag Tag to load info into, only populated when the result is FULL
  //! \returns The load result, NONE if there is no such version
  //! Only nfo files can hold multiple versions currently.
  //! Must not be called before Load().
  virtual CInfoScanner::InfoType LoadVersion(int /*index*/, CVideoInfoTag& /*tag*/)
  {
    return CInfoScanner::InfoType::NONE;
  }

  //! \brief Returns url associated with obtained URL (NFO_URL et al).
  const CScraperUrl& ScraperUrl() const { return m_url; }

  //! \brief Returns the bluray playlist the loaded info refers to, -1 if none was given.
  //! Only carried by nfos (a <playlist> element); other loaders have no notion of a playlist.
  //! Valid once Load() has been called.
  virtual int GetBlurayPlaylist() const { return -1; }

  //! \brief Returns current scaper info.
  const ADDON::ScraperPtr GetAddonInfo() const { return m_info; }

protected:
  const CFileItem& m_item; //!< Reference to item to load for
  ADDON::ScraperPtr m_info; //!< Scraper info
  CScraperUrl m_url; //!< URL for scraper
};

}
