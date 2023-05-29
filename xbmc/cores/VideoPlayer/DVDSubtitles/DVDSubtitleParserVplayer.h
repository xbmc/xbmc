/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDSubtitleParser.h"
#include "SubtitlesAdapter.h"

#include <memory>

class CDVDSubtitleParserVplayer : public CDVDSubtitleParserText, private CSubtitlesAdapter
{
public:
  CDVDSubtitleParserVplayer(std::unique_ptr<CDVDSubtitleStream>&& pStream,
                            const std::string& strFile);
  ~CDVDSubtitleParserVplayer() = default;

  bool Open(CDVDStreamInfo& hints) override;

private:
  double m_framerate;
};
