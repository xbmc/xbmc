/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OptionalsReg.h"


//-----------------------------------------------------------------------------
// ALSA
//-----------------------------------------------------------------------------

#ifdef HAS_ALSA
#include "cores/AudioEngine/Sinks/AESinkALSA.h"
bool OPTIONALS::ALSARegister()
{
  CAESinkALSA::Register();
  return true;
}
#else
bool OPTIONALS::ALSARegister()
{
  return false;
}
#endif

//-----------------------------------------------------------------------------
// PulseAudio
//-----------------------------------------------------------------------------

#ifdef HAS_PULSEAUDIO
#include "cores/AudioEngine/Sinks/AESinkPULSE.h"
bool OPTIONALS::PulseAudioRegister(bool allowPipeWireCompatServer)
{
  bool ret = CAESinkPULSE::Register(allowPipeWireCompatServer);
  return ret;
}
#else
bool OPTIONALS::PulseAudioRegister(bool)
{
  return false;
}
#endif

//-----------------------------------------------------------------------------
// Pipewire
//-----------------------------------------------------------------------------

#ifdef HAS_PIPEWIRE
#include "cores/AudioEngine/Sinks/pipewire/AESinkPipewire.h"
bool OPTIONALS::PipewireRegister()
{
  bool ret = AE::SINK::CAESinkPipewire::Register();
  return ret;
}
#else
bool OPTIONALS::PipewireRegister()
{
  return false;
}
#endif

//-----------------------------------------------------------------------------
// sndio
//-----------------------------------------------------------------------------

#ifdef HAS_SNDIO
#include "cores/AudioEngine/Sinks/AESinkSNDIO.h"
bool OPTIONALS::SndioRegister()
{
  CAESinkSNDIO::Register();
  return true;
}
#else
bool OPTIONALS::SndioRegister()
{
  return false;
}
#endif

//-----------------------------------------------------------------------------
// Lirc
//-----------------------------------------------------------------------------

#ifdef HAS_LIRC
#include "platform/linux/input/LIRC.h"
#include "ServiceBroker.h"
class OPTIONALS::CLircContainer
{
public:
  CLircContainer()
  {
    m_lirc.Start();
  }
protected:
  CLirc m_lirc;
};
#else
class OPTIONALS::CLircContainer
{
};
#endif

OPTIONALS::CLircContainer* OPTIONALS::LircRegister()
{
  return new CLircContainer();
}
void OPTIONALS::delete_CLircContainer::operator()(CLircContainer *p) const
{
  delete p;
}
