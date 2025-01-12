/*
 *  Copyright (C) 2014 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileDirectory.h"
#include "music/tags/MusicInfoTag.h"

#include <memory>

extern "C" {
#include <libavformat/avformat.h>
}
namespace XFILE
{
  class CAudioBookFileDirectory : public IFileDirectory
  {
    public:
      ~CAudioBookFileDirectory(void) override;
      bool GetDirectory(const CURL& url, CFileItemList &items) override;
      bool Exists(const CURL& url) override;
      bool ContainsFiles(const CURL& url) override;
      bool IsAllowed(const CURL& url) const override { return true; }

    protected:
      void AddCommaDelimitedString(const std::vector<std::string>& data,
                                   const std::vector<std::string>& separators,
                                   MUSIC_INFO::CMusicInfoTag& musictag);
      AVIOContext* m_ioctx = nullptr;
      AVFormatContext* m_fctx = nullptr;
  };
}
