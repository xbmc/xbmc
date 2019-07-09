/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformWin10.h"

#include "filesystem/SpecialProtocol.h"
#include "platform/Environment.h"

#include <stdlib.h>

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
