/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDInputStream.h"

#include <string>
#include <vector>

typedef std::shared_ptr<CDVDInputStream> InputStreamPtr;
class IDVDPlayer;

class InputStreamMultiStreams : public CDVDInputStream
{
  friend class CDemuxMultiSource;

public:
  InputStreamMultiStreams(DVDStreamType type, const CFileItem& fileitem)
    : CDVDInputStream(type, fileitem) {}

  ~InputStreamMultiStreams() override = default;

protected:
  std::vector<InputStreamPtr> m_InputStreams;    // input streams for current playing file
};
