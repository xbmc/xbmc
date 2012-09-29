/*
 *      Copyright (C) 2010-2012 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AESinkFactory.h"
#include "Interfaces/AESink.h"
#if defined(TARGET_WINDOWS)
  #include "Sinks/AESinkWASAPI.h"
  #include "Sinks/AESinkDirectSound.h"
#elif defined(TARGET_ANDROID)
  #include "Sinks/AESinkAUDIOTRACK.h"
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
  #if defined(HAS_ALSA)
    #include "Sinks/AESinkALSA.h"
  #endif
  #include "Sinks/AESinkOSS.h"
#else
  #pragma message("NOTICE: No audio sink for target platform.  Audio output will not be available.")
#endif
#include "Sinks/AESinkProfiler.h"
#include "Sinks/AESinkNULL.h"

#include "settings/AdvancedSettings.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"

void CAESinkFactory::ParseDevice(std::string &device, std::string &driver)
{
  int pos = device.find_first_of(':');
  if (pos > 0)
  {
    driver = device.substr(0, pos);
    std::transform(driver.begin(), driver.end(), driver.begin(), ::toupper);

    // check that it is a valid driver name
    if (
#if defined(TARGET_WINDOWS)
        driver == "WASAPI"      ||
        driver == "DIRECTSOUND" ||
#elif defined(TARGET_ANDROID)
        driver == "AUDIOTRACK"  ||
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
  #if defined(HAS_ALSA)
        driver == "ALSA"        ||
  #endif
        driver == "OSS"         ||
#endif
        driver == "PROFILER")
      device = device.substr(pos + 1, device.length() - pos - 1);
    else
      driver.clear();
  }
  else
    driver.clear();
}

#define TRY_SINK(SINK) \
{ \
  tmpFormat = desiredFormat; \
  tmpDevice = device; \
  sink      = new CAESink ##SINK(); \
  if (sink->Initialize(tmpFormat, tmpDevice)) \
  { \
    desiredFormat = tmpFormat; \
    device        = tmpDevice; \
    return sink; \
  } \
  sink->Deinitialize(); \
  delete sink; \
  sink = NULL; \
}

IAESink *CAESinkFactory::Create(std::string &device, AEAudioFormat &desiredFormat, bool rawPassthrough)
{
  // extract the driver from the device string if it exists
  std::string driver;
  ParseDevice(device, driver);

  AEAudioFormat  tmpFormat;
  IAESink       *sink;
  std::string    tmpDevice;

  if (driver == "PROFILER")
    TRY_SINK(Profiler);


#if defined(TARGET_WINDOWS)
  if ((driver.empty() && g_sysinfo.IsVistaOrHigher() ||
    driver == "WASAPI") && !g_advancedSettings.m_audioForceDirectSound)
    TRY_SINK(WASAPI)
  else
    TRY_SINK(DirectSound) // always fall back to DirectSound

#elif defined(TARGET_ANDROID)
  if (driver.empty() || driver == "AUDIOTRACK")
    TRY_SINK(AUDIOTRACK)

#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
  #if defined(HAS_ALSA)
  if (driver.empty() || driver == "ALSA")
    TRY_SINK(ALSA)
  #endif

  if (driver.empty() || driver == "OSS")
    TRY_SINK(OSS)
#endif

  // complete failure.
  TRY_SINK(NULL);

  // should never get here
  ASSERT(false);
  return NULL;
}

#define ENUMERATE_SINK(SINK) { \
  AESinkInfo info; \
  info.m_sinkName = #SINK; \
  CAESink ##SINK::EnumerateDevicesEx(info.m_deviceInfoList); \
  if(!info.m_deviceInfoList.empty()) \
    list.push_back(info); \
}

void CAESinkFactory::EnumerateEx(AESinkInfoList &list)
{
#if defined(TARGET_WINDOWS)
  if (g_sysinfo.IsVistaOrHigher() && !g_advancedSettings.m_audioForceDirectSound)
    ENUMERATE_SINK(WASAPI);
  ENUMERATE_SINK(DirectSound);
#elif defined(TARGET_ANDROID)
    ENUMERATE_SINK(AUDIOTRACK);
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
  #if defined(HAS_ALSA)
    ENUMERATE_SINK(ALSA);
  #endif

    ENUMERATE_SINK(OSS);
#endif

}
