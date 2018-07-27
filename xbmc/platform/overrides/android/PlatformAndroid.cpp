/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformAndroid.h"
#include <stdlib.h>
#include "filesystem/SpecialProtocol.h"

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformAndroid();
}

CPlatformAndroid::CPlatformAndroid()
{

}

CPlatformAndroid::~CPlatformAndroid()
{

}

void CPlatformAndroid::Init()
{
    setenv("SSL_CERT_FILE", CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem").c_str(), 1);
}
