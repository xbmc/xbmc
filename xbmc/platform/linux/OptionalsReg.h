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

#pragma once

//-----------------------------------------------------------------------------
// ALSA
//-----------------------------------------------------------------------------

namespace OPTIONALS
{
bool ALSARegister();
}

//-----------------------------------------------------------------------------
// PulseAudio
//-----------------------------------------------------------------------------

namespace OPTIONALS
{
bool PulseAudioRegister();
}

//-----------------------------------------------------------------------------
// OSS
//-----------------------------------------------------------------------------

namespace OPTIONALS
{
bool OSSRegister();
}

//-----------------------------------------------------------------------------
// sndio
//-----------------------------------------------------------------------------

namespace OPTIONALS
{
bool SndioRegister();
}

//-----------------------------------------------------------------------------
// Lirc
//-----------------------------------------------------------------------------

namespace OPTIONALS
{
class CLircContainer;
CLircContainer* LircRegister();
struct delete_CLircContainer
{
  void operator()(CLircContainer *p) const;
};
}
