/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HttpClientFactory.h"

#include "CurlHttpClient.h"

#if defined(TARGET_WASM)
#include "wasm/XhrHttpClient.h"

#include "platform/wasm/network/TizenSockets.h"
#endif

using namespace XFILE;

std::unique_ptr<IHttpClient> XFILE::CreateHttpClient()
{
#if defined(TARGET_WASM)
  if (!kodi_wasm_has_sockets())
    return std::make_unique<CXhrHttpClient>();
#endif
  return std::make_unique<CCurlHttpClient>();
}
