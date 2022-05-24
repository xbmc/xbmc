/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformLinux.h"

#if defined(HAS_ALSA)
#include "cores/AudioEngine/Sinks/alsa/ALSADeviceMonitor.h"
#include "cores/AudioEngine/Sinks/alsa/ALSAHControlMonitor.h"
#endif

#include "utils/StringUtils.h"

#if defined(HAS_ALSA)
#include "platform/linux/FDEventMonitor.h"
#endif

#include "platform/linux/powermanagement/LinuxPowerSyscall.h"

// clang-format off
#if defined(HAS_GLES)
#if defined(HAVE_WAYLAND)
#include "windowing/wayland/WinSystemWaylandEGLContextGLES.h"
#endif
#if defined(HAVE_X11)
#include "windowing/X11/WinSystemX11GLESContext.h"
#endif
#if defined(HAVE_GBM)
#include "windowing/gbm/WinSystemGbmGLESContext.h"
#endif
#endif

#if defined(HAS_GL)
#if defined(HAVE_WAYLAND)
#include "windowing/wayland/WinSystemWaylandEGLContextGL.h"
#endif
#if defined(HAVE_X11)
#include "windowing/X11/WinSystemX11GLContext.h"
#endif
#if defined(HAVE_GBM)
#include "windowing/gbm/WinSystemGbmGLContext.h"
#endif
#endif
// clang-format on

#include <cstdlib>

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformLinux();
}

bool CPlatformLinux::InitStageOne()
{
  if (!CPlatformPosix::InitStageOne())
    return false;

  setenv("OS", "Linux", true); // for python scripts that check the OS

#if defined(HAS_GLES)
#if defined(HAVE_WAYLAND)
  KODI::WINDOWING::WAYLAND::CWinSystemWaylandEGLContextGLES::Register();
#endif
#if defined(HAVE_X11)
  KODI::WINDOWING::X11::CWinSystemX11GLESContext::Register();
#endif
#if defined(HAVE_GBM)
  KODI::WINDOWING::GBM::CWinSystemGbmGLESContext::Register();
#endif
#endif

#if defined(HAS_GL)
#if defined(HAVE_WAYLAND)
  KODI::WINDOWING::WAYLAND::CWinSystemWaylandEGLContextGL::Register();
#endif
#if defined(HAVE_X11)
  KODI::WINDOWING::X11::CWinSystemX11GLContext::Register();
#endif
#if defined(HAVE_GBM)
  KODI::WINDOWING::GBM::CWinSystemGbmGLContext::Register();
#endif
#endif

  CLinuxPowerSyscall::Register();

  std::string envSink;
  if (getenv("KODI_AE_SINK"))
    envSink = getenv("KODI_AE_SINK");

  if (StringUtils::EqualsNoCase(envSink, "ALSA"))
  {
    OPTIONALS::ALSARegister();
  }
  else if (StringUtils::EqualsNoCase(envSink, "PULSE"))
  {
    OPTIONALS::PulseAudioRegister();
  }
  else if (StringUtils::EqualsNoCase(envSink, "PIPEWIRE"))
  {
    OPTIONALS::PipewireRegister();
  }
  else if (StringUtils::EqualsNoCase(envSink, "SNDIO"))
  {
    OPTIONALS::SndioRegister();
  }
  else if (StringUtils::EqualsNoCase(envSink, "ALSA+PULSE"))
  {
    OPTIONALS::ALSARegister();
    OPTIONALS::PulseAudioRegister();
  }
  else
  {
    if (!OPTIONALS::PulseAudioRegister())
    {
      if (!OPTIONALS::PipewireRegister())
      {
        if (!OPTIONALS::ALSARegister())
        {
          OPTIONALS::SndioRegister();
        }
      }
    }
  }

  m_lirc.reset(OPTIONALS::LircRegister());

#if defined(HAS_ALSA)
  RegisterService(std::make_shared<CFDEventMonitor>());
#if defined(HAVE_LIBUDEV)
  RegisterService(std::make_shared<CALSADeviceMonitor>());
#endif
#if !defined(HAVE_X11)
  RegisterService(std::make_shared<CALSAHControlMonitor>());
#endif
#endif // HAS_ALSA
  return true;
}

void CPlatformLinux::DeinitStageOne()
{
#if defined(HAS_ALSA)
#if !defined(HAVE_X11)
  DeregisterService(typeid(CALSAHControlMonitor));
#endif
#if defined(HAVE_LIBUDEV)
  DeregisterService(typeid(CALSADeviceMonitor));
#endif
  DeregisterService(typeid(CFDEventMonitor));
#endif // HAS_ALSA
}
