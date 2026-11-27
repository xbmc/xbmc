/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"
#include "playlists/PlayListTypes.h"

#include <optional>
#include <string>

namespace XFILE
{
  class CPlaylistDirectory : public IDirectory
  {
  public:
    CPlaylistDirectory(void);
    ~CPlaylistDirectory(void) override;
    bool GetDirectory(const CURL& url, CFileItemList &items) override;
    bool AllowAll() const override { return true; }

    //! \brief The path listing the playlist of \p type.
    static std::string PathOf(KODI::PLAYLIST::Type type);

    //! \brief The playlist \p url lists, if it is a playlist path.
    static std::optional<KODI::PLAYLIST::Type> TypeOf(const CURL& url);
  };
}
