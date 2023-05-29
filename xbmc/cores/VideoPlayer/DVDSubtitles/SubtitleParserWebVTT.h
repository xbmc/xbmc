/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDSubtitleParser.h"
#include "SubtitlesAdapter.h"

#include <memory>

class CSubtitleParserWebVTT : public CDVDSubtitleParserText, public CSubtitlesAdapter
{
public:
  CSubtitleParserWebVTT(std::unique_ptr<CDVDSubtitleStream>&& pStream, const std::string& strFile);
  ~CSubtitleParserWebVTT() = default;

  bool Open(CDVDStreamInfo& hints) override;
};
