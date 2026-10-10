/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#define _GLIBCXX_USE_CXX11_ABI 0

#include "MediaPipelineWebOS.h"

#include <cstring>
#include <memory>

#include <starfish-media-pipeline/StarfishMediaAPIs.h>

std::unique_ptr<char[]> CMediaPipelineWebOS::FeedLegacy(StarfishMediaAPIs* api, const char* payload)
{
  auto res = api->Feed(payload);
  auto p = std::make_unique<char[]>(res.size() + 1);
  std::memcpy(p.get(), res.c_str(), res.size() + 1); // includes '\0'

  return p;
}
