/*
 *      Copyright (C) 2005-2017 Team XBMC
 *      http://kodi.tv
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

#ifdef TARGET_LINUX
bool OPTIONALS::OSSRegister()
{
  return false;
}
#endif

//-----------------------------------------------------------------------------
// PulseAudio
//-----------------------------------------------------------------------------

#ifdef HAS_PULSEAUDIO
#include "cores/AudioEngine/Sinks/AESinkPULSE.h"
bool OPTIONALS::PulseAudioRegister()
{
  bool ret = CAESinkPULSE::Register();
  return ret;
}
#else
bool OPTIONALS::PulseAudioRegister()
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
