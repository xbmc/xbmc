/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CurlGlobalInit.h"

#include "utils/log.h"

// Needed for now because our CURL is not namespaced
// and is included in the pch on Windows
#define CURL CURL_HANDLE
#include <curl/curl.h>
#undef CURL

namespace KODI
{
namespace PLATFORM
{

CCurlGlobalInit::CCurlGlobalInit()
{
  /* we handle this ourself */
  if (curl_global_init(CURL_GLOBAL_ALL))
  {
    CLog::Log(LOGERROR, "Error initializing libcurl");
  }
}

CCurlGlobalInit::~CCurlGlobalInit()
{
  // close libcurl
  curl_global_cleanup();
}

} // namespace PLATFORM
} // namespace KODI
