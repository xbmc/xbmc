/*
 *      Copyright (C) 2016-present Team Kodi
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

#include "PlatformWin10.h"
#include <stdlib.h>
#include "filesystem/SpecialProtocol.h"
#include "platform/Environment.h"

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformWin10();
}

CPlatformWin10::CPlatformWin10() = default;

CPlatformWin10::~CPlatformWin10() = default;

void CPlatformWin10::Init()
{
  CEnvironment::setenv("SSL_CERT_FILE", CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem").c_str(), 1);
}
