/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileDirectory.h"

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
      /*!
       * Check if a file already has multiple chapter/song records in the music DB.
       * If the file is already scanned, the player can use the existing DB rows
       * directly and there is no need to wrap the file as a directory or run a
       * full FFmpeg probe of it.
       * return true if the DB contains > 1 song for this file (i.e. already scanned).
       */
      static bool HasChaptersInDatabase(const CURL& url);

    protected:
      AVIOContext* m_ioctx = nullptr;
      AVFormatContext* m_fctx = nullptr;

    private:
      static int GetSongCountFromDatabase(const CURL& url);
  };
}
