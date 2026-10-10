/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"
#include "media/MediaSection.h"

#include <optional>
#include <string>
#include <vector>

class CMediaSource;

namespace XFILE
{
  class CSourcesDirectory : public IDirectory
  {
  public:
    CSourcesDirectory(void);
    ~CSourcesDirectory(void) override;
    bool GetDirectory(const CURL& url, CFileItemList &items) override;
    bool GetDirectory(const std::vector<CMediaSource>& sources, CFileItemList& items);
    bool Exists(const CURL& url) override;
    bool AllowAll() const override { return true; }

    //! \brief The path listing the sources of \p section, e.g. sources://video/.
    static std::string PathOf(KODI::MEDIA::MediaSection section);

    //! \brief The section whose sources \p path lists. Nullopt for any other path.
    static std::optional<KODI::MEDIA::MediaSection> SectionOf(const std::string& path);
  };
}
