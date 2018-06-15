/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CryptoSession.h"

using namespace DRM;

std::vector<GET_CRYPTO_SESSION_INTERFACE_FN> CCryptoSession::s_registeredInterfaces;

void CCryptoSession::RegisterInterface(GET_CRYPTO_SESSION_INTERFACE_FN fn)
{
  s_registeredInterfaces.push_back(fn);
}

CCryptoSession* CCryptoSession::GetCryptoSession(const std::string &UUID, const std::string &cipherAlgo, const std::string &macAlgo)
{
  CCryptoSession* retVal = nullptr;
  for (auto fn : s_registeredInterfaces)
    if ((retVal = fn(UUID, cipherAlgo, macAlgo)))
      break;
  return retVal;
}
