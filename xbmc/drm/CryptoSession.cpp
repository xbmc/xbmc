/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
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
