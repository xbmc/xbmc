#pragma once
/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "IVideoInfoTagLoader.h"
#include "video/VideoInfoTag.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

//! \brief Video tag loader from plugin source.
class CVideoTagLoaderPlugin : public VIDEO::IVideoInfoTagLoader
{
public:
  CVideoTagLoaderPlugin(const CFileItem& item, bool forceRefresh);

  virtual ~CVideoTagLoaderPlugin() = default;

  //! \brief Returns whether or not read has info.
  bool HasInfo() const override;

  //! \brief Load "tag" from plugin.
  //! \param tag Tag to load info into
  CInfoScanner::INFO_TYPE Load(CVideoInfoTag& tag, bool prioritise,
                               std::vector<EmbeddedArt>* = nullptr) override;

  inline std::unique_ptr<std::map<std::string, std::string>>& GetArt()
  {
    return m_art;
  }
protected:
  std::unique_ptr<CVideoInfoTag> m_tag;
  std::unique_ptr<std::map<std::string, std::string>> m_art;
  bool m_force_refresh;
};
