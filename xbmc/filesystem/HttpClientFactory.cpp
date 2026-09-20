/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HttpClientFactory.h"

#include "CurlHttpClient.h"

using namespace XFILE;

std::unique_ptr<IHttpClient> XFILE::CreateHttpClient()
{
  // Curl is the only HTTP backend currently available. The WASM platform,
  // where libcurl is not usable, will hook its own IHttpClient implementation
  // in here so that scraper and metadata consumers stay backend-agnostic.
  return std::make_unique<CCurlHttpClient>();
}
