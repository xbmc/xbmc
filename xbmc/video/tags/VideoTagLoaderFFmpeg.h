/*
 *      Copyright (C) 2017 Team XBMC
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

#include <string>
#include <vector>

struct AVFormatContext;
struct AVIOContext;
namespace XFILE
{
  class CFile;
}

//! \brief Video tag loader using FFMPEG.
class CVideoTagLoaderFFmpeg : public VIDEO::IVideoInfoTagLoader
{
public:
  //! \brief Constructor.
  CVideoTagLoaderFFmpeg(const CFileItem& item,
                        const ADDON::ScraperPtr& info,
                        bool lookInFolder);

  //! \brief Destructor.
  virtual ~CVideoTagLoaderFFmpeg();

  //! \brief Returns whether or not reader has info.
  bool HasInfo() const override;

  //! \brief Load "tag" from nfo file.
  //! \brief tag Tag to load info into
  CInfoScanner::INFO_TYPE Load(CVideoInfoTag& tag, bool,
                               std::vector<EmbeddedArt>* art = nullptr) override;

  ADDON::ScraperPtr GetScraperInfo() const { return m_info; }

protected:
  ADDON::ScraperPtr m_info; //!< Passed scraper info
  AVIOContext* m_ioctx = nullptr; //!< IO context for file
  AVFormatContext* m_fctx = nullptr; //!< Format context for file
  XFILE::CFile* m_file = nullptr; //!< VFS file handle for file
  mutable int m_metadata_stream = -1; //!< Stream holding kodi metadata (mkv)
  mutable bool m_override_data = false; //!< Data is for overriding

  //! \brief Load tags from MKV file.
  CInfoScanner::INFO_TYPE LoadMKV(CVideoInfoTag& tag, std::vector<EmbeddedArt>* art);

  //! \brief Load tags from MP4 file.
  CInfoScanner::INFO_TYPE LoadMP4(CVideoInfoTag& tag, std::vector<EmbeddedArt>* art);

  //! \brief Load tags from AVI file.
  CInfoScanner::INFO_TYPE LoadAVI(CVideoInfoTag& tag, std::vector<EmbeddedArt>* art);
};

