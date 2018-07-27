/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Platform.h"

// Override for platform ports
#if !defined(PLATFORM_OVERRIDE)

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatform();
}

#endif

// base class definitions

CPlatform::CPlatform() = default;

CPlatform::~CPlatform() = default;

void CPlatform::Init()
{
  // nothing for now
}

