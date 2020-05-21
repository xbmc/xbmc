/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

#ifdef TARGET_LINUX
namespace OPTIONALS
{
bool OSSRegister();
}
#endif

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
