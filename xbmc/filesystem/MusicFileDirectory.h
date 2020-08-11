/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileDirectory.h"
#include "music/tags/MusicInfoTag.h"

namespace XFILE
{
  class CMusicFileDirectory : public IFileDirectory
  {
    public:
      CMusicFileDirectory(void);
      ~CMusicFileDirectory(void) override;
      bool GetDirectory(const CURL& url, CFileItemList &items) override;
      bool Exists(const CURL& url) override;
      bool ContainsFiles(const CURL& url) override;
      bool AllowAll() const override { return true; }
    protected:
      virtual bool Load(const std::string& strFileName,
                        MUSIC_INFO::CMusicInfoTag& tag,
                        EmbeddedArt* art = nullptr) { return false; }
      virtual int GetTrackCount(const std::string& strPath) = 0;
      std::string m_strExt;
      MUSIC_INFO::CMusicInfoTag m_tag;
  };
}
