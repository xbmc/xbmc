/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"
#include "URL.h"

namespace XFILE
{

/*!
 \brief Abstracts a DVD virtual directory (dvd://) which in turn points to the actual physical drive
 */
class CDVDDirectory : public IDirectory
{
public:
  CDVDDirectory() = default;
  ~CDVDDirectory() override = default;
  bool GetDirectory(const CURL& url, CFileItemList& items) override;
  void GetEpisodeTitles(const CFileItem& episode,
                        CFileItemList& items,
                        std::vector<CVideoInfoTag> episodesOnDisc,
                        const std::vector<std::vector<unsigned int>>& clips,
                        const std::vector<std::vector<unsigned int>>& playlists,
                        std::map<unsigned int, std::string>& playlist_langs) const;
  bool GetMainItem(const CURL& url, CFileItem& main) override;

private:
  void GetRoot(CFileItemList& items) const;
  void GetRoot(CFileItemList& items,
               const CFileItem& episode,
               const std::vector<CVideoInfoTag>& episodesOnDisc) const;
  bool GetEpisodeDirectory(const CURL& url,
                           const CFileItem& episode,
                           CFileItemList& items,
                           const std::vector<CVideoInfoTag>& episodesOnDisc) override;
  void AddRootOptions(CFileItemList& items) const;
  void GetTitles(const int job, CFileItemList& items) const;
  static CURL GetUnderlyingCURL(const CURL& url);

  CURL m_url;
};
} // namespace XFILE
