/*
 *      Copyright (C) 2005-2017 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include "IVideoInfoTagLoader.h"
#include "utils/ScraperUrl.h"

#include <string>
#include <vector>

//! \brief Video tag loader using nfo files.
class CVideoTagLoaderNFO : public VIDEO::IVideoInfoTagLoader
{
public:
  CVideoTagLoaderNFO(const CFileItem& item,
                     ADDON::ScraperPtr info,
                     bool lookInFolder);

  virtual ~CVideoTagLoaderNFO() = default;

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
