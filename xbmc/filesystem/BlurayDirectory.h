/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"
#include "URL.h"
#include "video/VideoInfoTag.h"

#include <memory>

class CFileItem;
class CFileItemList;

typedef struct bluray BLURAY;
typedef struct bd_title_info BLURAY_TITLE_INFO;

namespace XFILE
{

class CBlurayDirectory : public IDirectory
{
public:
  CBlurayDirectory() = default;
  ~CBlurayDirectory() override;
  bool GetDirectory(const CURL& url, CFileItemList &items) override;
  bool GetEpisodeDirectory(const CURL& url,
                           const CFileItem& episode,
                           CFileItemList& items,
                           const std::vector<CVideoInfoTag>& episodesOnDisc);

  bool InitializeBluray(const std::string &root);
  std::string GetBlurayTitle();
  std::string GetBlurayID();

private:
  enum class DiscInfo
  {
    TITLE,
    ID
  };

  void         Dispose();
  std::string  GetDiscInfoString(DiscInfo info);
  void GetRoot(CFileItemList& items);
  void GetRoot(CFileItemList& items,
               const CFileItem& episode,
               const std::vector<CVideoInfoTag>& episodesOnDisc);
  void AddRootOptions(CFileItemList& items) const;
  void         GetTitles(bool main, CFileItemList &items);
  void GetEpisodeTitles(const CFileItem& episode,
                        CFileItemList& items,
                        std::vector<CVideoInfoTag> episodesOnDisc);
  std::vector<BLURAY_TITLE_INFO*> GetUserPlaylists();
  std::shared_ptr<CFileItem> GetTitle(const BLURAY_TITLE_INFO* title, const std::string& label);
  CURL         GetUnderlyingCURL(const CURL& url);
  std::string  HexToString(const uint8_t * buf, int count);
  CURL          m_url;
  BLURAY*       m_bd = nullptr;
  bool          m_blurayInitialized = false;
};

}
