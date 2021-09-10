/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformAndroid.h"

#include "filesystem/SpecialProtocol.h"
#include "platform/Environment.h"
#include "utils/log.h"
#include "windowing/android/WinSystemAndroidGLESContext.h"

#include "platform/android/activity/XBMCApp.h"
#include "platform/android/powermanagement/AndroidPowerSyscall.h"

#include <stdlib.h>

#include <androidjni/Build.h>

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformAndroid();
}

bool CPlatformAndroid::InitStageOne()
{
  if (!CPlatformPosix::InitStageOne())
  {
    CLog::Log(LOGINFO, "Error: CPlatformPosix::InitStageOne failed");
    return false;
  }

  CEnvironment::setenv("SSL_CERT_FILE",
                       CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem"),
                       1);
  CEnvironment::setenv("REQUESTS_CA_BUNDLE",
                       CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem"),
                       1);

  CLog::Log(LOGDEBUG, "CPlatformAndroid::InitStageOne - SSL_CERT_FILE: {}",
            CEnvironment::getenv("SSL_CERT_FILE"));
  CEnvironment::setenv("OS", "Linux", true); // for python scripts that check the OS

  CWinSystemAndroidGLESContext::Register();

  CAndroidPowerSyscall::Register();

  return true;
}

bool CPlatformAndroid::InitStageTwo()
{
  std::string envSslCertFile = CEnvironment::getenv("SSL_CERT_FILE");
  if (envSslCertFile.empty()
  {
    CLog::Log(LOGDEBUG, "CPlatformAndroid::InitStageTow - SSL_CERT_FILE not set, setting it again");
    CEnvironment::setenv("SSL_CERT_FILE",
                         CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem"),
                         1);
    CEnvironment::setenv("REQUESTS_CA_BUNDLE",
                         CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem"),
                         1);
  }
  return true;
}

void CPlatformAndroid::PlatformSyslog()
{
  CLog::Log(
      LOGINFO,
      "Product: {}, Device: {}, Board: {} - Manufacturer: {}, Brand: {}, Model: {}, Hardware: {}",
      CJNIBuild::PRODUCT, CJNIBuild::DEVICE, CJNIBuild::BOARD, CJNIBuild::MANUFACTURER,
      CJNIBuild::BRAND, CJNIBuild::MODEL, CJNIBuild::HARDWARE);
  std::string extstorage;
  bool extready = CXBMCApp::GetExternalStorage(extstorage);
  CLog::Log(LOGINFO, "External storage path = {}; status = {}", extstorage,
            extready ? "ok" : "nok");
}
