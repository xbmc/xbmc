/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformLinux.h"

#include "ServiceBroker.h"
#include "application/AppParams.h"
#include "filesystem/SpecialProtocol.h"

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

#if defined(TARGET_WEBOS)
  // WebOS ipks run in a chroot like environment. $HOME is set to the ipk dir and $LD_LIBRARY_PATH is lib
  const auto HOME = std::string(getenv("HOME"));
  setenv("XDG_RUNTIME_DIR", "/tmp/xdg", 1);
  setenv("XKB_CONFIG_ROOT", "/usr/share/X11/xkb", 1);
  setenv("WAYLAND_DISPLAY", "wayland-0", 1);
  setenv("PYTHONHOME", (HOME + "/lib/python3").c_str(), 1);
  setenv("PYTHONPATH", (HOME + "/lib/python3").c_str(), 1);
  setenv("PYTHONIOENCODING", "UTF-8", 1);
  setenv("KODI_HOME", HOME.c_str(), 1);
  setenv("SSL_CERT_FILE",
         CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem").c_str(), 1);
#endif

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

  std::string_view sink = CServiceBroker::GetAppParams()->GetAudioBackend();

  if (sink == "alsa")
  {
    OPTIONALS::ALSARegister();
  }
  else if (sink == "pulseaudio")
  {
    OPTIONALS::PulseAudioRegister();
  }
  else if (sink == "pipewire")
  {
    OPTIONALS::PipewireRegister();
  }
  else if (sink == "sndio")
  {
    OPTIONALS::SndioRegister();
  }
  else if (sink == "alsa+pulseaudio")
  {
    OPTIONALS::ALSARegister();
    OPTIONALS::PulseAudioRegister();
  }
  else
  {
    if (!OPTIONALS::PipewireRegister())
    {
      if (!OPTIONALS::PulseAudioRegister())
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
  RegisterComponent(std::make_shared<CFDEventMonitor>());
#if defined(HAVE_LIBUDEV)
  RegisterComponent(std::make_shared<CALSADeviceMonitor>());
#endif
#if !defined(HAVE_X11)
  RegisterComponent(std::make_shared<CALSAHControlMonitor>());
#endif
#endif // HAS_ALSA
  return true;
}

void CPlatformLinux::DeinitStageOne()
{
#if defined(HAS_ALSA)
#if !defined(HAVE_X11)
  DeregisterComponent(typeid(CALSAHControlMonitor));
#endif
#if defined(HAVE_LIBUDEV)
  DeregisterComponent(typeid(CALSADeviceMonitor));
#endif
  DeregisterComponent(typeid(CFDEventMonitor));
#endif // HAS_ALSA
}

bool CPlatformLinux::IsConfigureAddonsAtStartupEnabled()
{
#if defined(ADDONS_CONFIGURE_AT_STARTUP)
  return true;
#else
  return false;
#endif
}
