/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AESinkFactory.h"
#include "SystemInfo.h"
#include "utils/log.h"
#include "settings/AdvancedSettings.h"

#ifdef _WIN32
  #include "Sinks/AESinkWASAPI.h"
  #include "Sinks/AESinkDirectSound.h"
#elif defined _LINUX
  #include "Sinks/AESinkALSA.h"
  #include "Sinks/AESinkOSS.h"
#elif defined __APPLE__
  #include "Sinks/AESinkCoreAudio.h"
#else
  #pragma message("NOTICE: No audio sink for target platform.  Audio output will not be available.")
#endif

#define TRY_SINK(SINK) \
{ \
  tmpFormat = desiredFormat; \
  tmpDevice = device; \
  sink      = new CAESink ##SINK(); \
  if (sink->Initialize(tmpFormat, tmpDevice)) \
  { \
    if (rawPassthrough && tmpFormat.m_sampleRate != desiredFormat.m_sampleRate) \
      CLog::Log(LOGERROR, "CAESinkFactory::Create " #SINK " failed to open at the desired sample rate (%dHz)", tmpFormat.m_sampleRate); \
    else \
    { \
      desiredFormat = tmpFormat; \
      device        = tmpDevice; \
      return sink; \
    } \
  } \
  sink->Deinitialize(); \
  delete sink; \
}

IAESink *CAESinkFactory::Create(CStdString &driver, CStdString &device, AEAudioFormat &desiredFormat, bool rawPassthrough)
{
  AEAudioFormat  tmpFormat;
  CStdString     tmpDevice;
  IAESink        *sink;

#ifdef _WIN32
  if(driver == "WASAPI")
    TRY_SINK(WASAPI)
  else if(driver == "DIRECTSOUND")
    TRY_SINK(DirectSound)

  if(g_sysinfo.IsVistaOrHigher() && !g_advancedSettings.m_audioForceDirectSound)
    TRY_SINK(WASAPI)
  else
    TRY_SINK(DirectSound)
#elif defined _LINUX
  if (driver == "ALSA")
    TRY_SINK(ALSA)
  else if (driver == "OSS")
    TRY_SINK(OSS)

  if(driver != "ALSA")
      TRY_SINK(ALSA)
  if(driver != "OSS" )
    TRY_SINK(OSS)
#elif defined __APPLE__
  //TRY_SINK(CoreAudio);
#endif

  //Complete failure.
  return NULL;
}

void CAESinkFactory::Enumerate(AEDeviceList &devices, bool passthrough)
{
#ifdef _WIN32

  if(g_sysinfo.IsVistaOrHigher() && !g_advancedSettings.m_audioForceDirectSound)
    CAESinkWASAPI::EnumerateDevices(devices, passthrough);
  else
    CAESinkDirectSound::EnumerateDevices(devices, passthrough);
    
#elif defined _LINUX

  CAESinkALSA::EnumerateDevices(devices, passthrough);
  CAESinkOSS ::EnumerateDevices(devices, passthrough);

#elif defined __APPLE__

  CAESinkCoreAudio::EnumerateDevices(devices, false);

#endif
}

