/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "HttpClientFactory.h"

#include "CurlHttpClient.h"

using namespace XFILE;

std::unique_ptr<IHttpClient> XFILE::CreateHttpClient()
{
  // CCurlHttpClient is a thin wrapper around CCurlFile on every platform. On
  // WASM, CCurlFile is implemented by xbmc/filesystem/wasm/CurlFileWasm.cpp
  // and uses XMLHttpRequest under the hood; no wasm-specific IHttpClient
  // implementation is needed.
  return std::make_unique<CCurlHttpClient>();
}
