/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileDirectory.h"
#include "music/tags/MatroskaTagReader.h"

#include <memory>
#include <string>

extern "C"
{
#include <libavformat/avformat.h>
}

namespace XFILE
{
class CAudioBookFileDirectory : public IFileDirectory
{
public:
  ~CAudioBookFileDirectory(void) override;
  bool GetDirectory(const CURL& url, CFileItemList& items) override;
  bool Exists(const CURL& url) override;
  bool ContainsFiles(const CURL& url) override;
  bool IsAllowed(const CURL& url) const override { return true; }

protected:
  AVIOContext* m_ioctx = nullptr;
  AVFormatContext* m_fctx = nullptr;
  //! Read by ContainsFiles() to count the tracks, consumed by GetDirectory() to build them.
  //! Kept with the URL it was read from: every method takes one, and nothing promises they
  //! are the same URL twice.
  std::string m_matroskaUrl;
  std::unique_ptr<MUSIC_INFO::MatroskaAlbum> m_matroska;
};
} // namespace XFILE
