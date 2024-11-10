/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRPathUtils.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_providers.h" // PVR_PROVIDER_INVALID_UID
#include "pvr/PVRConstants.h" // PVR_CLIENT_INVALID_UID
#include "pvr/PVRManager.h"
#include "pvr/providers/PVRProvider.h"
#include "pvr/providers/PVRProviders.h"
#include "utils/StringUtils.h"

#include <cstdlib>

namespace PVR::UTILS
{

bool HasClientAndProvider(const std::string& path)
{
  const CURL url{path};
  const std::string clientIdStr{url.GetOption("clientid")};
  const std::string providerIdStr{url.GetOption("providerid")};
  return (!clientIdStr.empty() && !providerIdStr.empty() && StringUtils::IsInteger(clientIdStr) &&
          StringUtils::IsInteger(providerIdStr));
}

bool GetClientAndProviderFromPath(const CURL& url, int& clientId, int& providerId)
{
  const std::string clientIdStr{url.GetOption("clientid")};
  const std::string providerIdStr{url.GetOption("providerid")};
  const bool filterByClientAndProvider{!clientIdStr.empty() && !providerIdStr.empty() &&
                                       StringUtils::IsInteger(clientIdStr) &&
                                       StringUtils::IsInteger(providerIdStr)};
  if (filterByClientAndProvider)
  {
    clientId = std::atoi(clientIdStr.c_str());
    providerId = std::atoi(providerIdStr.c_str());
    return true;
  }
  else
  {
    clientId = PVR_CLIENT_INVALID_UID;
    providerId = PVR_PROVIDER_INVALID_UID;
    return false;
  }
}

std::string GetProviderNameFromPath(const std::string& path)
{
  const CURL url{path};
  int clientId{PVR_CLIENT_INVALID_UID};
  int providerId{PVR_PROVIDER_INVALID_UID};
  if (GetClientAndProviderFromPath(url, clientId, providerId))
  {
    const std::shared_ptr<const CPVRProvider> provider{
        CServiceBroker::GetPVRManager().Providers()->GetByClient(clientId, providerId)};
    if (provider)
      return provider->GetName();
  }
  return {};
}

} // namespace PVR::UTILS
