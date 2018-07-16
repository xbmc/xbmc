/*
 *  Copyright (C) 2005-2017 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/linux/OptionalsReg.h"

//-----------------------------------------------------------------------------
// OSS
//-----------------------------------------------------------------------------
#include "cores/AudioEngine/Sinks/AESinkOSS.h"
bool OPTIONALS::OSSRegister()
{
  CAESinkOSS::Register();
  return true;
}
