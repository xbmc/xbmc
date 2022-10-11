/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/OverrideFile.h"

enum class VideoDbContentType;
class CVideoInfoTag;
class CURL;

namespace XFILE
{
class CVideoDatabaseFile : public COverrideFile
{
public:
  CVideoDatabaseFile(void);
  ~CVideoDatabaseFile(void) override;

  static CVideoInfoTag GetVideoTag(const CURL& url);

protected:
  std::string TranslatePath(const CURL& url) override;
  static VideoDbContentType GetType(const CURL& url);
};
}
