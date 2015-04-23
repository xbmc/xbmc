/*
 *      Copyright (C) 2010-2013 Team XBMC
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
#elif defined(TARGET_RASPBERRY_PI)
  #include "Sinks/AESinkPi.h"
  #include "Sinks/AESinkALSA.h"
#elif defined(TARGET_DARWIN_IOS)
  #include "Sinks/AESinkDARWINIOS.h"
#elif defined(TARGET_DARWIN_OSX)
  #include "Sinks/AESinkDARWINOSX.h"
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
  #if defined(HAS_ALSA)
    #include "Sinks/AESinkALSA.h"
  #endif
  #if defined(HAS_PULSEAUDIO)
    #include "Sinks/AESinkPULSE.h"
  #endif
  #include "Sinks/AESinkOSS.h"
#else
  #pragma message("NOTICE: No audio sink for target platform.  Audio output will not be available.")
#endif
#include "Sinks/AESinkNULL.h"

#include "utils/log.h"

#include <algorithm>

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
#elif defined(TARGET_RASPBERRY_PI)
        driver == "PI"          ||
        driver == "ALSA"        ||
#elif defined(TARGET_DARWIN_IOS)
        driver == "DARWINIOS"  ||
#elif defined(TARGET_DARWIN_OSX)
        driver == "DARWINOSX"  ||
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
  #if defined(HAS_ALSA)
        driver == "ALSA"        ||
  #endif
  #if defined(HAS_PULSEAUDIO)
        driver == "PULSE"       ||
  #endif
        driver == "OSS"         ||
#endif
        driver == "PROFILER"    ||
        driver == "NULL")
      device = device.substr(pos + 1, device.length() - pos - 1);
    else
      driver.clear();
  }
  else
    driver.clear();
}

IAESink *CAESinkFactory::TrySink(std::string &driver, std::string &device, AEAudioFormat &format)
{
  IAESink *sink = NULL;

  if (driver == "NULL")
    sink = new CAESinkNULL();
  else
  {
#if defined(TARGET_WINDOWS)
    if (driver == "WASAPI")
      sink = new CAESinkWASAPI();
    if (driver == "DIRECTSOUND")
      sink = new CAESinkDirectSound();
#elif defined(TARGET_ANDROID)
    sink = new CAESinkAUDIOTRACK();
#elif defined(TARGET_RASPBERRY_PI)
  if (driver == "PI")
    sink = new CAESinkPi();
  #if defined(HAS_ALSA)
  if (driver == "ALSA")
    sink = new CAESinkALSA();
  #endif
#elif defined(TARGET_DARWIN_IOS)
    sink = new CAESinkDARWINIOS();
#elif defined(TARGET_DARWIN_OSX)
    sink = new CAESinkDARWINOSX();
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
 #if defined(HAS_PULSEAUDIO)
    if (driver == "PULSE")
      sink = new CAESinkPULSE();
 #endif
 #if defined(HAS_ALSA)
    if (driver == "ALSA")
      sink = new CAESinkALSA();
 #endif
    if (driver == "OSS")
      sink = new CAESinkOSS();
#endif
  }

  if (!sink)
    return NULL;

  if (sink->Initialize(format, device))
  {
    // do some sanity checks
    if (format.m_sampleRate == 0)
      CLog::Log(LOGERROR, "Sink %s:%s returned invalid sample rate", driver.c_str(), device.c_str());
    else if (format.m_channelLayout.Count() == 0)
      CLog::Log(LOGERROR, "Sink %s:%s returned invalid channel layout", driver.c_str(), device.c_str());
    else if (format.m_frames < 256)
      CLog::Log(LOGERROR, "Sink %s:%s returned invalid buffer size: %d", driver.c_str(), device.c_str(), format.m_frames);
    else
      return sink;
  }
  sink->Deinitialize();
  delete sink;
  return NULL;
}

IAESink *CAESinkFactory::Create(std::string &device, AEAudioFormat &desiredFormat, bool rawPassthrough)
{
  // extract the driver from the device string if it exists
  std::string driver;
  ParseDevice(device, driver);

  AEAudioFormat  tmpFormat = desiredFormat;
  IAESink       *sink;
  std::string    tmpDevice = device;

  sink = TrySink(driver, tmpDevice, tmpFormat);
  if (sink)
  {
    desiredFormat = tmpFormat;
    return sink;
  }

  return NULL;
}

void CAESinkFactory::EnumerateEx(AESinkInfoList &list, bool force)
{
  AESinkInfo info;
#if defined(TARGET_WINDOWS)

  info.m_deviceInfoList.clear();
  info.m_sinkName = "DIRECTSOUND";
  CAESinkDirectSound::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
    list.push_back(info);

  info.m_deviceInfoList.clear();
  info.m_sinkName = "WASAPI";
  CAESinkWASAPI::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
    list.push_back(info);

#elif defined(TARGET_ANDROID)

  info.m_deviceInfoList.clear();
  info.m_sinkName = "AUDIOTRACK";
  CAESinkAUDIOTRACK::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
    list.push_back(info);

#elif defined(TARGET_RASPBERRY_PI)

  info.m_deviceInfoList.clear();
  info.m_sinkName = "PI";
  CAESinkPi::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
    list.push_back(info);
  #if defined(HAS_ALSA)
  info.m_deviceInfoList.clear();
  info.m_sinkName = "ALSA";
  CAESinkALSA::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
    list.push_back(info);
  #endif
#elif defined(TARGET_DARWIN_IOS)

  info.m_deviceInfoList.clear();
  info.m_sinkName = "DARWINIOS";
  CAESinkDARWINIOS::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
    list.push_back(info);

#elif defined(TARGET_DARWIN_OSX)

  info.m_deviceInfoList.clear();
  info.m_sinkName = "DARWINOSX";
  CAESinkDARWINOSX::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
    list.push_back(info);

#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
  // check if user wants us to do something specific
  if (getenv("AE_SINK"))
  {
    std::string envSink = (std::string)getenv("AE_SINK");
    std::transform(envSink.begin(), envSink.end(), envSink.begin(), ::toupper);
    info.m_deviceInfoList.clear();
    #if defined(HAS_PULSEAUDIO)
    if (envSink == "PULSE")
      CAESinkPULSE::EnumerateDevicesEx(info.m_deviceInfoList, force);
    #endif
    #if defined(HAS_ALSA)
    if (envSink == "ALSA")
      CAESinkALSA::EnumerateDevicesEx(info.m_deviceInfoList, force);
    #endif
    if (envSink == "OSS")
      CAESinkOSS::EnumerateDevicesEx(info.m_deviceInfoList, force);

    if(!info.m_deviceInfoList.empty())
    {
      info.m_sinkName = envSink;
      list.push_back(info);
      return;
    }
    else
      CLog::Log(LOGNOTICE, "User specified Sink %s could not be enumerated", envSink.c_str());
  }

  #if defined(HAS_PULSEAUDIO)
  info.m_deviceInfoList.clear();
  info.m_sinkName = "PULSE";
  CAESinkPULSE::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
  {
    list.push_back(info);
    return;
  }
  #endif

  #if defined(HAS_ALSA)
  info.m_deviceInfoList.clear();
  info.m_sinkName = "ALSA";
  CAESinkALSA::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
  {
    list.push_back(info);
    return;
  }
  #endif

  info.m_deviceInfoList.clear();
  info.m_sinkName = "OSS";
  CAESinkOSS::EnumerateDevicesEx(info.m_deviceInfoList, force);
  if(!info.m_deviceInfoList.empty())
    list.push_back(info);

#endif

}
