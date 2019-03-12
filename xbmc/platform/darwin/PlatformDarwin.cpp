/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformDarwin.h"
#include <stdlib.h>
#include "filesystem/SpecialProtocol.h"

CPlatformDarwin::CPlatformDarwin()
{

}

CPlatformDarwin::~CPlatformDarwin()
{

}

void CPlatformDarwin::Init()
{
  CPlatformPosix::Init();
  setenv("SSL_CERT_FILE", CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem").c_str(), 0);
}