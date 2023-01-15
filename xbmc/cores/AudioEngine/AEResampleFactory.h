/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AEResample.h"

class IAEResample;

namespace ActiveAE
{

/**
 * Bit options to pass to CAEResampleFactory::Create
 */
enum AEResampleFactoryOptions
{
  /* This is a quick resample job (e.g. resample a single noise packet) and may not be worth using GPU acceleration */
  AERESAMPLEFACTORY_QUICK_RESAMPLE = 0x01
};

class CAEResampleFactory
{
public:
  static std::unique_ptr<IAEResample> Create(uint32_t flags = 0U);
};

}
