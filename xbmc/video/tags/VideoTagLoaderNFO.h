/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IVideoInfoTagLoader.h"

#include <string>
#include <vector>

//! \brief Video tag loader using nfo files.
class CVideoTagLoaderNFO : public KODI::VIDEO::IVideoInfoTagLoader
{
public:
  CVideoTagLoaderNFO(const CFileItem& item,
                     ADDON::ScraperPtr info,
                     bool lookInFolder);

  ~CVideoTagLoaderNFO() override = default;

  //! \brief Returns whether or not read has info.
  bool HasInfo() const override;

  //! \brief Load "tag" from nfo file.
  //! \brief tag Tag to load info into
  CInfoScanner::INFO_TYPE Load(CVideoInfoTag& tag, bool prioritise,
                               std::vector<EmbeddedArt>* = nullptr) override;

protected:
  //! \brief Find nfo file for item
  //! \param item The item to find NFO file for
  //! \param movieFolder If true, look for movie.nfo
  std::string FindNFO(const CFileItem& item, bool movieFolder) const;

  std::string m_path; //!< Path to nfo file
};
