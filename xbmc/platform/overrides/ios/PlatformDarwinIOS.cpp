/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PlatformDarwinIOS.h"
#include <stdlib.h>
#include "filesystem/SpecialProtocol.h"
#if HAS_LIRC
#include "platform/linux/input/LIRC.h"
#endif

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformDarwinIOS();
}

CPlatformDarwinIOS::CPlatformDarwinIOS() = default;

CPlatformDarwinIOS::~CPlatformDarwinIOS() = default;

void CPlatformDarwinIOS::Init()
{
  setenv("SSL_CERT_FILE", CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem").c_str(), 0);
#if HAS_LIRC
  CRemoteControl::Register();
#endif
}