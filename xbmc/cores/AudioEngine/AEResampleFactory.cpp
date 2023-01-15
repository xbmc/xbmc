/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AEResampleFactory.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEResampleFFMPEG.h"

namespace ActiveAE
{

std::unique_ptr<IAEResample> CAEResampleFactory::Create(uint32_t flags /* = 0 */)
{
  return std::make_unique<CActiveAEResampleFFMPEG>();
}

}
